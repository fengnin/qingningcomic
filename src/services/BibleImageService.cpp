#include "services/BibleImageService.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "data/FileStorage.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "models/CharacterPortraitVersion.h"
#include "utils/PromptBuilder.h"
#include "utils/AppConfig.h"
#include "utils/BackgroundWhitener.h"
#include "utils/BibleUtils.h"
#include "utils/ImageBorderTrimmer.h"
#include "utils/LogSummaryUtils.h"
#include "utils/Logger.h"
#include <QFile>
#include <QDateTime>
#include <QUuid>
#include <QJsonArray>
#include <QJsonDocument>
#include <QImage>
#include <QTimer>

using namespace BibleUtils::SceneTextFilter;

namespace {
PendingImageRequest loadPendingRequest(std::mutex& stateMutex,
                                       const QMap<QString, PendingImageRequest>& requests,
                                       const QString& requestId,
                                       bool* found = nullptr)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    const auto it = requests.constFind(requestId);
    if (it == requests.constEnd()) {
        if (found) {
            *found = false;
        }
        return PendingImageRequest();
    }

    if (found) {
        *found = true;
    }

    return it.value();
}

bool hasPendingRequest(std::mutex& stateMutex,
                       const QMap<QString, PendingImageRequest>& requests,
                       const QString& requestId)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return requests.contains(requestId);
}

QString pendingRequestLabel(const PendingImageRequest& request)
{
    return request.type == BibleImageConstants::TYPE_CHARACTER
        ? request.character.name()
        : request.scene.name();
}

bool isRateLimitError(const QString& errorMessage)
{
    const QString lower = errorMessage.trimmed().toLower();
    return lower.contains(QStringLiteral("429"))
        || lower.contains(QStringLiteral("too many requests"))
        || lower.contains(QStringLiteral("flowlimitexceeded"));
}

int retryDelayMsForError(const QString& errorMessage, int retryCount)
{
    if (isRateLimitError(errorMessage)) {
        const int exponent = qMax(0, retryCount);
        const int delayMs = 5000 * (1 << qMin(exponent, 3));
        return qMin(delayMs, 30000);
    }
    return 1000;
}

PendingImageRequest createPendingRequest(const QString& type,
                                        const Character& character,
                                        const Scene& scene,
                                        const QString& prompt,
                                        const QString& negativePrompt)
{
    PendingImageRequest request;
    request.type = type;
    request.character = character;
    request.scene = scene;
    request.prompt = prompt;
    request.negativePrompt = negativePrompt;
    request.retryCount = 0;
    return request;
}

QwenImageClient::GenerateOptions buildBibleQwenOptions(const QString& prompt,
                                                       const QString& negativePrompt,
                                                       const QString& requestId,
                                                       const QString& type)
{
    QwenImageClient::GenerateOptions options;
    options.prompt = prompt;
    options.negativePrompt = negativePrompt;
    options.textRendering = false;
    options.requestId = requestId;

    if (type == BibleImageConstants::TYPE_CHARACTER) {
        options.size = QwenImageClient::ImageSize::Custom;
        options.width = 1024;
        options.height = 1440;
    } else {
        options.size = QwenImageClient::ImageSize::Square;
    }

    return options;
}

void dispatchBibleImageRequest(const QString& provider,
                               const QString& requestId,
                               const QString& prompt,
                               const QString& negativePrompt,
                               const QString& type)
{
    if (provider == QLatin1String("volcengine")) {
        VolcEngineImageClient::GenerateOptions options;
        options.prompt = prompt;
        options.negativePrompt = negativePrompt;
        options.requestId = requestId;

        // 根据图像类型优化参数（基于官方文档建议）
        if (type == BibleImageConstants::TYPE_CHARACTER) {
            // 角色立绘：中等CFG平衡约束与自然感
            // ⚠️ 不要设置太高（如7.5+），否则AI会过度解读"reference sheet"为真实照片并添加阴影
            options.scale = 4.5f;              // 平衡值：既保证背景纯净，又避免过度死板
            options.seed = -1;                 // 保持随机性
            options.usePreLlm = false;         // 关闭文本扩写
            options.width = 1328;              // 官方推荐1.3K分辨率
            options.height = 1328;
        } else if (type == BibleImageConstants::TYPE_SCENE) {
            // 场景参考图：低CFG允许更多创意
            options.scale = 3.0f;              // 场景图需要一定自由度
            options.seed = -1;
            options.usePreLlm = false;
            options.width = 1328;
            options.height = 1328;
        } else {
            // 其他类型：使用默认值
            options.scale = 2.5f;              // 官方默认值
        }

        VolcEngineImageClient::instance()->generateAsync(options);
        return;
    }

    QwenImageClient::instance()->generateAsync(
        buildBibleQwenOptions(prompt, negativePrompt, requestId, type));
}

}

BibleImageService* BibleImageService::m_instance = nullptr;
std::once_flag BibleImageService::m_instanceOnceFlag;

BibleImageService* BibleImageService::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new BibleImageService();
    });
    return m_instance;
}

BibleImageService::BibleImageService(QObject* parent)
    : BatchImageProcessor(parent)
    , m_currentCharIndex(0)
    , m_currentSceneIndex(0)
    , m_combinedMode(false)
{
    connect(QwenImageClient::instance(), &QwenImageClient::generateCompleted,
            this, &BibleImageService::onQwenImageGenerated);
    connect(QwenImageClient::instance(), &QwenImageClient::editCompleted,
            this, &BibleImageService::onQwenImageEdited);
    connect(VolcEngineImageClient::instance(), &VolcEngineImageClient::generateCompleted,
            this, &BibleImageService::onVolcEngineImageGenerated);
}

BibleImageService::~BibleImageService()
{
    m_pendingRequests.clear();
    m_pendingCharacters.clear();
    m_pendingScenes.clear();
    LOG_INFO("BibleImageService", "BibleImageService destroyed");
}

void BibleImageService::generateCharacterPortraitAsync(const Character& character)
{
    if (character.id().isEmpty()) {
        setError("Character ID is empty");
        emit portraitGenerated(character.id(), QString());
        return;
    }
    
    setGenerating(true);
    
    PromptBuilder::PromptResult promptResult = buildCharacterPortraitPrompt(character);
    LOG_INFO("BibleImageService", QString("Generating portrait for: %1").arg(character.name()));
    
    startImageGeneration(character.id(), promptResult.text, promptResult.negativePrompt, 
                         BibleImageConstants::TYPE_CHARACTER, character, Scene());
}

void BibleImageService::generateSceneReferenceAsync(const Scene& scene)
{
    if (scene.id().isEmpty()) {
        setError("Scene ID is empty");
        emit sceneReferenceGenerated(scene.id(), QString());
        return;
    }

    setGenerating(true);

    PromptBuilder::PromptResult promptResult = buildSceneReferencePrompt(scene);
    LOG_INFO("BibleImageService", QString("Generating reference for: %1").arg(scene.name()));

    startImageGeneration(scene.id(), promptResult.text, promptResult.negativePrompt,
                         BibleImageConstants::TYPE_SCENE, Character(), scene);
}

void BibleImageService::failEdit(const QString& characterId, const QString& reason, const QString& requestId)
{
    LOG_WARNING("BibleImageService", QString("editCharacterPortraitAsync: %1 (char=%2)").arg(reason, characterId));
    emit portraitEditFailed(characterId, reason);
    if (!requestId.isEmpty()) {
        clearPendingRequest(requestId);
    }
    if (isGenerating()) {
        if (!maybeFinishBatch()) {
            scheduleNextItem();
        }
    } else {
        setGenerating(false);
    }
}

bool BibleImageService::resolveEditBaseVersion(const Character& character,
                                               const QString& requestedBaseVersionId,
                                               QString& outVersionId,
                                               QString& outRelativePath)
{
    outVersionId = requestedBaseVersionId;
    outRelativePath.clear();

    if (!outVersionId.isEmpty()) {
        CharacterPortraitVersion v = CharacterExtractor::instance()->getPortraitVersion(outVersionId);
        if (v.id().isEmpty()) {
            return false;
        }
        outRelativePath = v.portraitPath();
        return !outRelativePath.isEmpty();
    }

    Character latest = CharacterExtractor::instance()->getCharacterById(character.id());
    if (latest.id().isEmpty()) {
        latest = character;
    }
    if (!latest.currentPortraitVersionId().isEmpty()) {
        CharacterPortraitVersion v = CharacterExtractor::instance()->getPortraitVersion(latest.currentPortraitVersionId());
        if (!v.id().isEmpty()) {
            outVersionId = v.id();
            outRelativePath = v.portraitPath();
        }
    }
    if (outRelativePath.isEmpty() && !latest.portraitPath().isEmpty()) {
        outRelativePath = latest.portraitPath();
        CharacterPortraitVersion v1 = CharacterExtractor::instance()->ensureFirstPortraitVersion(latest);
        if (!v1.id().isEmpty()) {
            outVersionId = v1.id();
        }
    }
    return !outRelativePath.isEmpty();
}

QByteArray BibleImageService::readSourceImageBytes(const QString& relativePath) const
{
    QFile sourceFile(FileStorage::instance()->getFullPath(relativePath));
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    const QByteArray bytes = sourceFile.readAll();
    sourceFile.close();
    return bytes;
}

void BibleImageService::editCharacterPortraitAsync(const Character& character,
                                                   const QString& editPrompt,
                                                   const QString& baseVersionId,
                                                   const QList<CharacterFieldDiff>& diff,
                                                   int sourceChapter)
{
    if (character.id().isEmpty()) {
        failEdit(character.id(), QStringLiteral("character id empty"));
        return;
    }
    if (editPrompt.trimmed().isEmpty()) {
        failEdit(character.id(), QStringLiteral("empty edit prompt"));
        return;
    }

    QString resolvedBaseVersionId;
    QString sourceRelativePath;
    if (!resolveEditBaseVersion(character, baseVersionId, resolvedBaseVersionId, sourceRelativePath)) {
        failEdit(character.id(), QStringLiteral("no source image"));
        return;
    }

    const QByteArray sourceBytes = readSourceImageBytes(sourceRelativePath);
    if (sourceBytes.isEmpty()) {
        failEdit(character.id(), QStringLiteral("cannot read source image"));
        return;
    }

    setGenerating(true);

    const QString requestId = QStringLiteral("edit-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        PendingImageRequest req;
        req.type = BibleImageConstants::TYPE_CHARACTER;
        req.character = character;
        req.prompt = editPrompt;
        req.sourceImage = sourceBytes;
        req.baseVersionId = resolvedBaseVersionId;
        req.diff = diff;
        req.sourceChapter = sourceChapter;
        m_pendingRequests[requestId] = req;
    }

    QwenImageClient::EditOptions opts;
    opts.prompt = editPrompt;
    opts.sourceImage = sourceBytes;
    opts.size = QwenImageClient::ImageSize::Custom;
    opts.width = 1024;
    opts.height = 1440;
    opts.requestId = requestId;

    LOG_INFO("BibleImageService", QString("Dispatching portrait edit: char=%1, baseVer=%2, promptLen=%3")
        .arg(character.name())
        .arg(resolvedBaseVersionId)
        .arg(editPrompt.length()));

    QwenImageClient::instance()->editAsync(opts);
}

void BibleImageService::generateCharacterPortraits(const QList<Character>& characters)
{
    if (characters.isEmpty()) {
        emit bibleBatchCompleted(BibleImageConstants::TYPE_CHARACTER, 0, 0);
        return;
    }

    m_pendingCharacters = characters;
    m_pendingScenes.clear();
    m_currentCharIndex = 0;
    m_currentSceneIndex = 0;
    m_combinedMode = false;
    m_currentType = BibleImageConstants::TYPE_CHARACTER;

    startBatch(characters.size());
    LOG_INFO("BibleImageService", QString("Starting batch portrait generation for %1 characters").arg(characters.size()));

    processNextItem();
}

void BibleImageService::generateSceneReferences(const QList<Scene>& scenes)
{
    if (scenes.isEmpty()) {
        emit bibleBatchCompleted(BibleImageConstants::TYPE_SCENE, 0, 0);
        return;
    }

    m_pendingScenes = scenes;
    m_pendingCharacters.clear();
    m_currentCharIndex = 0;
    m_currentSceneIndex = 0;
    m_combinedMode = false;
    m_currentType = BibleImageConstants::TYPE_SCENE;

    startBatch(scenes.size());
    LOG_INFO("BibleImageService", QString("Starting batch scene reference generation for %1 scenes").arg(scenes.size()));

    processNextItem();
}

void BibleImageService::generateAllBibleImages(const QList<Character>& characters, const QList<Scene>& scenes)
{
    m_pendingCharacters = characters;
    m_pendingScenes = scenes;
    m_currentCharIndex = 0;
    m_currentSceneIndex = 0;
    m_combinedMode = true;
    m_currentType = characters.isEmpty() ? BibleImageConstants::TYPE_SCENE : BibleImageConstants::TYPE_CHARACTER;

    const int total = characters.size() + scenes.size();
    startBatch(total);

    LOG_INFO("BibleImageService", QString("Starting combined generation: %1 characters + %2 scenes = %3 total")
        .arg(characters.size()).arg(scenes.size()).arg(total));

    if (total == 0) {
        emit allBibleImagesCompleted(0, 0);
        setGenerating(false);
        return;
    }

    processNextItem();
}

void BibleImageService::onQwenImageGenerated(const QwenImageClient::GenerateResult& result)
{
    if (!hasPendingRequest(m_stateMutex, m_pendingRequests, result.requestId)) {
        LOG_WARNING("BibleImageService", QString("Ignored unknown Qwen result: requestId=%1")
            .arg(result.requestId));
        return;
    }
    handleImageGenerated(result.requestId, result.imageData, result.success, result.errorMessage);
}

void BibleImageService::onQwenImageEdited(const QwenImageClient::GenerateResult& result)
{
    bool found = false;
    const PendingImageRequest request = loadPendingRequest(m_stateMutex, m_pendingRequests, result.requestId, &found);
    if (!found) {
        return;
    }

    if (!result.success || result.imageData.isEmpty()) {
        failEdit(request.character.id(),
                 result.errorMessage.isEmpty() ? QStringLiteral("edit returned empty image") : result.errorMessage,
                 result.requestId);
        return;
    }

    saveAndEmitEditResult(result.requestId, result.imageData);
}

void BibleImageService::onVolcEngineImageGenerated(const VolcEngineImageClient::GenerateResult& result)
{
    if (!hasPendingRequest(m_stateMutex, m_pendingRequests, result.requestId)) {
        LOG_WARNING("BibleImageService", QString("Ignored unknown VolcEngine result: requestId=%1")
            .arg(result.requestId));
        return;
    }
    handleImageGenerated(result.requestId, result.imageData, result.success, result.errorMessage);
}

void BibleImageService::handleImageGenerated(const QString& requestId, const QByteArray& imageData, bool success, const QString& errorMessage)
{
    bool found = false;
    const PendingImageRequest request = loadPendingRequest(m_stateMutex, m_pendingRequests, requestId, &found);
    if (!found) {
        LOG_WARNING("BibleImageService", QString("Request not found: requestId=%1").arg(requestId));
        return;
    }
    
    if (success && !imageData.isEmpty()) {
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (m_pendingRequests.contains(requestId)) {
                m_pendingRequests[requestId].retryCount = 0;
            }
        }
        saveAndEmitResult(requestId, imageData);
    } else {
        int retryCount = request.retryCount;

        if (retryCount < MAX_RETRY_COUNT) {
            LOG_WARNING("BibleImageService", QString("Image generation failed (attempt %1/%2): %3, retrying...")
                .arg(retryCount + 1).arg(MAX_RETRY_COUNT).arg(errorMessage));
            retryRequest(requestId, errorMessage);
            return;
        }
        handleRetryExhausted(requestId, errorMessage);
    }
}

void BibleImageService::retryRequest(const QString& requestId, const QString& errorMessage)
{
    bool found = false;
    PendingImageRequest request = loadPendingRequest(m_stateMutex, m_pendingRequests, requestId, &found);
    if (!found) {
        LOG_ERROR("BibleImageService", "Cannot retry: request not found");
        handleRetryExhausted(requestId, "request not found");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_pendingRequests.contains(requestId)) {
            m_pendingRequests[requestId].retryCount++;
            request = m_pendingRequests[requestId];
        }
    }
    
    LOG_INFO("BibleImageService", QString("Retrying image generation (attempt %1/%2)")
        .arg(request.retryCount).arg(MAX_RETRY_COUNT));
    
    if (!request.prompt.isEmpty()) {
        const int delayMs = retryDelayMsForError(errorMessage, request.retryCount);
        LOG_WARNING("BibleImageService", QString("Retry delay for %1: %2 ms")
            .arg(pendingRequestLabel(request))
            .arg(delayMs));
        QTimer::singleShot(delayMs, this, [this, requestId, request]() {
            startImageGeneration(requestId, request.prompt, request.negativePrompt,
                                 request.type, request.character, request.scene);
        });
    } else {
        handleRetryExhausted(requestId, "no prompt saved");
    }
}

void BibleImageService::saveAndEmitResult(const QString& requestId, const QByteArray& imageData)
{
    const PendingImageRequest request = loadPendingRequest(m_stateMutex, m_pendingRequests, requestId);
    QByteArray processedImageData = imageData;
    bool borderTrimmed = false;

    if (request.type == BibleImageConstants::TYPE_SCENE) {
        processedImageData = ImageBorderTrimmer::trimDarkBorderImageData(processedImageData, &borderTrimmed);
        if (borderTrimmed) {
            QImage orig, proc;
            orig.loadFromData(imageData);
            proc.loadFromData(processedImageData);
            if (!orig.isNull() && !proc.isNull()) {
                LOG_INFO("BibleImageService", QString(
                    "Post-processed bible image: type=%1, borderTrimmed=true, from %2x%3 to %4x%5")
                    .arg(request.type)
                    .arg(orig.width()).arg(orig.height())
                    .arg(proc.width()).arg(proc.height()));
            }
        }
    } else if (request.type == BibleImageConstants::TYPE_CHARACTER) {
        processedImageData = processCharacterImageData(imageData);
    }
    QString imagePath;
    bool savedOk = false;

    if (request.type == BibleImageConstants::TYPE_CHARACTER) {
        imagePath = saveCharacterImage(request, processedImageData);
        savedOk = !imagePath.isEmpty();
    } else if (request.type == BibleImageConstants::TYPE_SCENE) {
        imagePath = saveSceneImage(request, processedImageData);
        savedOk = !imagePath.isEmpty();
    }
    
    if (!savedOk) {
        recordFailure();
        LOG_WARNING("BibleImageService", QString("Failed to save image for %1: %2")
            .arg(request.type)
            .arg(pendingRequestLabel(request)));
    }
    
    clearPendingRequest(requestId);
    if (!maybeFinishBatch()) {
        scheduleNextItem();
    }
}

QString BibleImageService::saveCharacterImage(const PendingImageRequest& request, const QByteArray& imageData)
{
    Character character = request.character;
    if (character.id().isEmpty()) {
        LOG_WARNING("BibleImageService", "Cannot save character image: character ID is empty");
        return QString();
    }

    QString imagePath = FileStorage::instance()->saveCharacterReference(
        character.id(), imageData);

    if (!imagePath.isEmpty()) {
        if (!updateCharacterPortraitRecord(character, imagePath)) {
            return QString();
        }
        // 首版生成路径：同步写一条 v1 行 + current 指针
        Character latest = CharacterExtractor::instance()->getCharacterById(character.id());
        if (latest.id().isEmpty()) {
            latest = character;
            latest.setPortraitPath(imagePath);
        }
        ensureFirstPortraitVersionRecorded(latest, imagePath);
    }

    return imagePath;
}

QString BibleImageService::saveSceneImage(const PendingImageRequest& request, const QByteArray& imageData)
{
    Scene scene = request.scene;
    if (scene.id().isEmpty()) {
        LOG_WARNING("BibleImageService", "Cannot save scene image: scene ID is empty");
        return QString();
    }
    
    QString imagePath = FileStorage::instance()->saveSceneReference(
        scene.id(), imageData);
    
    if (!imagePath.isEmpty()) {
        if (!updateSceneReferenceRecord(scene, imagePath)) {
            return QString();
        }
    }
    
    return imagePath;
}

bool BibleImageService::updateCharacterPortraitRecord(Character character, const QString& imagePath)
{
    Character updatedCharacter = CharacterExtractor::instance()->getCharacterById(character.id());
    if (updatedCharacter.id().isEmpty()) {
        updatedCharacter = character;
    }
    updatedCharacter.setPortraitPath(imagePath);
    if (!CharacterExtractor::instance()->updateCharacter(updatedCharacter)) {
        LOG_WARNING("BibleImageService", QString("Saved portrait but failed to update character row: %1")
            .arg(character.name()));
        return false;
    }

    LOG_INFO("BibleImageService", QString("Portrait saved: %1").arg(imagePath));
    emit portraitGenerated(character.id(), imagePath);
    recordSuccess();
    return true;
}

bool BibleImageService::updateSceneReferenceRecord(Scene scene, const QString& imagePath)
{
    Scene updatedScene = SceneExtractor::instance()->getSceneById(scene.id());
    if (updatedScene.id().isEmpty()) {
        updatedScene = scene;
    }
    updatedScene.setReferenceImagePath(imagePath);
    if (!SceneExtractor::instance()->updateScene(updatedScene)) {
        LOG_WARNING("BibleImageService", QString("Saved scene reference but failed to update scene row: %1")
            .arg(scene.name()));
        return false;
    }

    LOG_INFO("BibleImageService", QString("Scene reference saved: %1").arg(imagePath));
    emit sceneReferenceGenerated(scene.id(), imagePath);
    recordSuccess();
    return true;
}

void BibleImageService::clearPendingRequest(const QString& requestId)
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_pendingRequests.remove(requestId);
}

bool BibleImageService::maybeFinishBatch()
{
    bool shouldFinish = false;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        shouldFinish = m_pendingRequests.isEmpty() &&
                       m_currentCharIndex >= m_pendingCharacters.size() &&
                       m_currentSceneIndex >= m_pendingScenes.size();
    }

    if (shouldFinish && isGenerating()) {
        finishBatch();
        return true;
    }

    return false;
}

void BibleImageService::scheduleNextItem()
{
    QTimer::singleShot(0, this, [this]() { processNextItem(); });
}

void BibleImageService::handleRetryExhausted(const QString& requestId, const QString& errorMessage)
{
    if (errorMessage == "request not found") {
        LOG_ERROR("BibleImageService", "Cannot retry: request not found");
    } else if (errorMessage == "no prompt saved") {
        LOG_ERROR("BibleImageService", "Cannot retry: no prompt saved");
    } else {
        LOG_ERROR("BibleImageService", QString("Image generation failed after %1 retries: %2")
            .arg(MAX_RETRY_COUNT).arg(errorMessage));
    }

    recordFailure();
    clearPendingRequest(requestId);
    maybeFinishBatch();
    scheduleNextItem();
}

void BibleImageService::startImageGeneration(const QString& requestId, const QString& prompt, const QString& negativePrompt, const QString& type, const Character& character, const Scene& scene)
{
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        const int existingRetryCount = m_pendingRequests.contains(requestId)
            ? m_pendingRequests[requestId].retryCount : 0;
        m_pendingRequests[requestId] = createPendingRequest(type, character, scene, prompt, negativePrompt);
        m_pendingRequests[requestId].retryCount = existingRetryCount;
    }

    LOG_INFO("BibleImageService", QString(
        "Dispatching image request: requestId=%1, type=%2, promptLen=%3, negativeLen=%4, promptHead=%5")
        .arg(requestId)
        .arg(type)
        .arg(prompt.length())
        .arg(negativePrompt.length())
        .arg(LogSummaryUtils::summarizeText(prompt)));
    
    dispatchBibleImageRequest(AppConfig::instance()->imageService().provider,
                              requestId,
                              prompt,
                              negativePrompt,
                              type);
}

void BibleImageService::processNextItem()
{
    if (!shouldContinue()) {
        return;
    }
    
    bool hasMore = false;
    QString currentType;
    int currentCharIndex, currentSceneIndex;
    int pendingCharactersSize, pendingScenesSize;
    bool combinedMode;
    
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        currentType = m_currentType;
        currentCharIndex = m_currentCharIndex;
        currentSceneIndex = m_currentSceneIndex;
        pendingCharactersSize = m_pendingCharacters.size();
        pendingScenesSize = m_pendingScenes.size();
        combinedMode = m_combinedMode;
    }
    
    if (currentType == BibleImageConstants::TYPE_CHARACTER && currentCharIndex < pendingCharactersSize) {
        hasMore = processNextCharacter();
    } else if (currentType == BibleImageConstants::TYPE_SCENE && currentSceneIndex < pendingScenesSize) {
        hasMore = processNextScene();
    }

    int inFlightRequestCount = 0;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        inFlightRequestCount = m_pendingRequests.size();
    }

    if (inFlightRequestCount > 0) {
        LOG_DEBUG("BibleImageService", QString("Waiting for %1 in-flight image request(s) before queueing next item")
            .arg(inFlightRequestCount));
        return;
    }

    bool shouldQueueNext = false;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (!hasMore && combinedMode && m_currentType == BibleImageConstants::TYPE_CHARACTER
            && m_currentCharIndex >= m_pendingCharacters.size() && !m_pendingScenes.isEmpty()) {
            m_currentType = BibleImageConstants::TYPE_SCENE;
            shouldQueueNext = true;
        } else if (m_currentType == BibleImageConstants::TYPE_CHARACTER) {
            shouldQueueNext = m_currentCharIndex < m_pendingCharacters.size()
                || (!m_pendingCharacters.isEmpty() && m_currentCharIndex >= m_pendingCharacters.size()
                    && !m_pendingScenes.isEmpty() && combinedMode);
        } else {
            shouldQueueNext = m_currentSceneIndex < m_pendingScenes.size();
        }
    }

    if (shouldQueueNext) {
        scheduleNext([this]() { processNextItem(); });
        return;
    }
    
    if (!maybeFinishBatch()) {
        LOG_DEBUG("BibleImageService", QString("Batch waiting for %1 pending image requests")
            .arg(m_pendingRequests.size()));
    }
}

bool BibleImageService::processNextCharacter()
{
    Character character;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_currentCharIndex >= m_pendingCharacters.size()) {
            return false;
        }
        character = m_pendingCharacters[m_currentCharIndex];
    }
    
    advanceProgress(character.name());
    
    if (!character.portraitPath().isEmpty()) {
        LOG_INFO("BibleImageService", QString("Skipping character %1, already has portrait")
            .arg(character.name()));
        recordSuccess();
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_currentCharIndex++;
        }
        return true;
    }
    
    PromptBuilder::PromptResult promptResult = buildCharacterPortraitPrompt(character);
    LOG_INFO("BibleImageService", QString("Generating portrait for: %1")
        .arg(character.name()));
    
    startImageGeneration(character.id(), promptResult.text, promptResult.negativePrompt, 
                         BibleImageConstants::TYPE_CHARACTER, character, Scene());
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_currentCharIndex++;
    }
    
    return true;
}

bool BibleImageService::processNextScene()
{
    Scene scene;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_currentSceneIndex >= m_pendingScenes.size()) {
            return false;
        }
        scene = m_pendingScenes[m_currentSceneIndex];
    }
    
    advanceProgress(scene.name());
    
    if (!scene.referenceImagePath().isEmpty()) {
        LOG_INFO("BibleImageService", QString("Skipping scene %1, already has reference")
            .arg(scene.name()));
        recordSuccess();
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_currentSceneIndex++;
        }
        return true;
    }
    
    PromptBuilder::PromptResult promptResult = buildSceneReferencePrompt(scene);
    LOG_INFO("BibleImageService", QString("Generating reference for: %1")
        .arg(scene.name()));
    
    startImageGeneration(scene.id(), promptResult.text, promptResult.negativePrompt,
                         BibleImageConstants::TYPE_SCENE, Character(), scene);
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_currentSceneIndex++;
    }
    
    return true;
}

PromptBuilder::PromptResult BibleImageService::buildCharacterPortraitPrompt(const Character& character)
{
    QJsonObject characterJson;
    characterJson["name"] = character.name();
    characterJson["role"] = character.role();
    
    CharacterAppearance app = character.appearance();
    characterJson["appearance"] = app.toPromptJson();
    
    QJsonObject options;
    options["mode"] = "preview";
    options["view"] = "front";
    options["pose"] = "standing";

    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(characterJson, options);
    LOG_INFO("BibleImageService", QString(
        "Character portrait prompt built: name=%1, promptLen=%2, negativeLen=%3, refs=%4, promptHead=%5, negativeHead=%6")
        .arg(character.name())
        .arg(result.text.length())
        .arg(result.negativePrompt.length())
        .arg(result.referenceImages.size())
        .arg(LogSummaryUtils::summarizeText(result.text))
        .arg(LogSummaryUtils::summarizeText(result.negativePrompt)));
    return result;
}

PromptBuilder::PromptResult BibleImageService::buildSceneReferencePrompt(const Scene& scene)
{
    QJsonObject sceneJson = buildSceneJson(scene);
    return PromptBuilder::buildScenePrompt(sceneJson);
}

QJsonObject BibleImageService::buildSceneJson(const Scene& scene)
{
    QJsonObject sceneJson;
    sceneJson["name"] = scene.name();
    
    SceneDetails details = scene.details();
    sceneJson["description"] = filterStaticSceneText(details.description);
    sceneJson["building"] = filterStaticSceneText(details.building);
    sceneJson["color"] = filterStaticSceneText(details.color);
    sceneJson["landmark"] = filterStaticSceneText(details.landmark);
    sceneJson["layout"] = filterStaticSceneText(details.layout);
    sceneJson["atmosphere"] = filterStaticSceneText(details.atmosphere);
    sceneJson["type"] = details.type;
    sceneJson["typeZh"] = details.typeZh;
    sceneJson["setting"] = details.setting;
    sceneJson["timeOfDay"] = details.timeOfDay;
    sceneJson["weather"] = details.weather;
    sceneJson["spaceSize"] = details.spaceSize;
    sceneJson["anchorPoints"] = toJsonArray(filterStaticSceneList(details.anchorPoints));
    sceneJson["signatureObjects"] = toJsonArray(filterStaticSceneList(details.signatureObjects));
    sceneJson["fixedColorBlocks"] = toJsonArray(filterStaticSceneList(details.fixedColorBlocks));
    sceneJson["consistencyRules"] = toJsonArray(filterStaticSceneList(details.consistencyRules));
    sceneJson["timeVariations"] = details.timeVariations;
    sceneJson["weatherVariations"] = details.weatherVariations;

    if (!details.visualCharacteristics.isEmpty()) {
        sceneJson["visualCharacteristics"] = details.visualCharacteristics;
    }
    if (!details.spatialLayout.isEmpty()) {
        sceneJson["spatialLayout"] = details.spatialLayout;
    }
    
    return sceneJson;
}

void BibleImageService::finishBatch()
{
    BatchImageProcessor::finishBatch();

    if (m_combinedMode) {
        emit allBibleImagesCompleted(batchState().successCount, batchState().failedCount);
        LOG_INFO("BibleImageService", QString("All Bible images completed: success=%1, failed=%2")
            .arg(batchState().successCount).arg(batchState().failedCount));
    } else {
        emit bibleBatchCompleted(m_currentType, batchState().successCount, batchState().failedCount);
        LOG_INFO("BibleImageService", QString("Batch generation completed: type=%1, success=%2, failed=%3")
            .arg(m_currentType).arg(batchState().successCount).arg(batchState().failedCount));
    }
}

bool BibleImageService::ensureFirstPortraitVersionRecorded(const Character& character, const QString& imagePath)
{
    if (character.id().isEmpty() || imagePath.isEmpty()) {
        return false;
    }

    QList<CharacterPortraitVersion> existing = CharacterExtractor::instance()->loadPortraitVersions(character.id());
    if (!existing.isEmpty()) {
        return true;
    }

    const QString versionId = insertPortraitVersionAndSetCurrent(
        character, imagePath, 1, QString(), QString(), QJsonObject(), character.appearance().toJson(), 0);
    if (versionId.isEmpty()) {
        return false;
    }
    LOG_INFO("BibleImageService", QString("First portrait version (v1) recorded for %1").arg(character.name()));
    return true;
}

QString BibleImageService::insertPortraitVersionAndSetCurrent(const Character& character,
                                                              const QString& portraitPath,
                                                              int versionNo,
                                                              const QString& baseVersionId,
                                                              const QString& editPrompt,
                                                              const QJsonObject& fieldDiff,
                                                              const QJsonObject& appearanceSnapshot,
                                                              int sourceChapter)
{
    CharacterPortraitVersion v;
    v.setId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    v.setCharacterId(character.id());
    v.setVersionNo(versionNo);
    v.setPortraitPath(portraitPath);
    v.setBaseVersionId(baseVersionId);
    v.setEditPrompt(editPrompt);
    v.setFieldDiff(fieldDiff);
    v.setAppearanceSnapshot(appearanceSnapshot);
    v.setSourceChapter(sourceChapter);
    v.setCreatedAt(QDateTime::currentDateTime());

    if (!CharacterExtractor::instance()->addPortraitVersion(v)) {
        LOG_WARNING("BibleImageService", QString("Failed to insert portrait version v%1 for %2")
            .arg(versionNo).arg(character.name()));
        return QString();
    }
    if (!CharacterExtractor::instance()->setCurrentPortraitVersion(character.id(), v.id())) {
        LOG_WARNING("BibleImageService", QString("Inserted v%1 but failed to set current for %2")
            .arg(versionNo).arg(character.name()));
        return QString();
    }
    return v.id();
}

QByteArray BibleImageService::processCharacterImageData(const QByteArray& raw) const
{
    // 先白化背景，再检测暗边 — 避免模型生成的暗色背景被误判为需要裁剪的边框
    QByteArray processed = BackgroundWhitener::fillWhiteBackgroundImageData(raw, 240);
    LOG_INFO("BibleImageService", QString("Post-processed character image: background whitened, size=%1 bytes")
        .arg(processed.size()));

    QImage originalImage;
    originalImage.loadFromData(raw);
    const qint64 originalArea = (qint64)originalImage.width() * originalImage.height();

    bool didTrim = false;
    QByteArray trimmed = ImageBorderTrimmer::trimDarkBorderImageData(processed, &didTrim);
    if (!didTrim) {
        return processed;
    }

    QImage trimmedImage;
    trimmedImage.loadFromData(trimmed);
    const qint64 trimmedArea = (qint64)trimmedImage.width() * trimmedImage.height();

    if (trimmedArea < originalArea * 2 / 5) {
        LOG_WARNING("BibleImageService", QString(
            "角色图片裁剪过度 (%1x%2 -> %3x%4, 面积比=%5%), 跳过裁剪")
            .arg(originalImage.width()).arg(originalImage.height())
            .arg(trimmedImage.width()).arg(trimmedImage.height())
            .arg(QString::number(trimmedArea * 100.0 / originalArea, 'f', 1) + "%"));
        return processed;
    }

    LOG_INFO("BibleImageService", QString(
        "Post-processed character image: borderTrimmed=true, from %1x%2 to %3x%4")
        .arg(originalImage.width()).arg(originalImage.height())
        .arg(trimmedImage.width()).arg(trimmedImage.height()));
    return trimmed;
}

void BibleImageService::saveAndEmitEditResult(const QString& requestId, const QByteArray& imageData)
{
    bool found = false;
    PendingImageRequest request = loadPendingRequest(m_stateMutex, m_pendingRequests, requestId, &found);
    if (!found) {
        return;
    }

    QByteArray processed = processCharacterImageData(imageData);

    Character character = CharacterExtractor::instance()->getCharacterById(request.character.id());
    if (character.id().isEmpty()) {
        character = request.character;
    }

    const int versionNo = CharacterExtractor::instance()->nextVersionNo(character.id());
    const QString relativePath = FileStorage::instance()->saveCharacterReferenceVersion(
        character.id(), versionNo, processed);
    if (relativePath.isEmpty()) {
        failEdit(character.id(), QStringLiteral("save file failed"), requestId);
        return;
    }

    QJsonArray diffArray;
    for (const CharacterFieldDiff& d : request.diff) {
        QJsonObject obj;
        obj["field"] = d.field;
        obj["oldValue"] = d.oldValue;
        obj["newValue"] = d.newValue;
        obj["sourceFrom"] = d.sourceFrom;
        obj["sourceTo"] = d.sourceTo;
        diffArray.append(obj);
    }
    QJsonObject diffJson;
    diffJson["fields"] = diffArray;

    // Apply diff to produce the appearance snapshot that matches this new portrait.
    CharacterAppearance snapshotApp = character.appearance();
    for (const CharacterFieldDiff& d : request.diff) {
        const QString& f = d.field;
        if      (f == "hairColor")          snapshotApp.hairColor = d.newValue;
        else if (f == "hairStyle")          snapshotApp.hairStyle = d.newValue;
        else if (f == "eyeColor")           snapshotApp.eyeColor = d.newValue;
        else if (f == "height")             snapshotApp.height = d.newValue;
        else if (f == "build")              snapshotApp.build = d.newValue;
        else if (f == "clothing")           snapshotApp.clothing = d.newValue.split(QStringLiteral("、"), Qt::SkipEmptyParts);
        else if (f == "accessories")        snapshotApp.accessories = d.newValue.split(QStringLiteral("、"), Qt::SkipEmptyParts);
        else if (f == "distinctiveFeatures") snapshotApp.distinctiveFeatures = d.newValue.split(QStringLiteral("、"), Qt::SkipEmptyParts);
    }

    const QString versionId = insertPortraitVersionAndSetCurrent(
        character, relativePath, versionNo,
        request.baseVersionId, request.prompt, diffJson, snapshotApp.toJson(), request.sourceChapter);
    if (versionId.isEmpty()) {
        failEdit(character.id(), QStringLiteral("insert version row failed"), requestId);
        return;
    }

    LOG_INFO("BibleImageService", QString("Portrait edit applied: char=%1, v%2, path=%3")
        .arg(character.name()).arg(versionNo).arg(relativePath));

    clearPendingRequest(requestId);

    emit portraitVersionGenerated(character.id(), versionId, relativePath);
    emit portraitGenerated(character.id(), relativePath);

    if (isGenerating()) {
        if (!maybeFinishBatch()) {
            scheduleNextItem();
        }
    } else {
        setGenerating(false);
    }
}
