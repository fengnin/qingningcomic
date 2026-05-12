#include "services/BibleImageService.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "data/FileStorage.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "utils/PromptBuilder.h"
#include "utils/AppConfig.h"
#include "utils/BackgroundWhitener.h"
#include "utils/BibleUtils.h"
#include "utils/ImageBorderTrimmer.h"
#include "utils/LogSummaryUtils.h"
#include "utils/Logger.h"
#include <QJsonArray>
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
    m_currentType = BibleImageConstants::TYPE_CHARACTER;
    
    int total = characters.size() + scenes.size();
    startBatch(total);
    
    LOG_INFO("BibleImageService", QString("Starting combined generation: %1 characters + %2 scenes = %3 total")
        .arg(characters.size()).arg(scenes.size()).arg(total));
    
    if (characters.isEmpty() && scenes.isEmpty()) {
        emit allBibleImagesCompleted(0, 0);
        setGenerating(false);
        return;
    }
    
    if (characters.isEmpty()) {
        m_currentType = BibleImageConstants::TYPE_SCENE;
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
    
    scheduleNextItem();
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
    bool backgroundWhitened = false;

    if (request.type == BibleImageConstants::TYPE_SCENE) {
        processedImageData = ImageBorderTrimmer::trimDarkBorderImageData(processedImageData, &borderTrimmed);
    } else if (request.type == BibleImageConstants::TYPE_CHARACTER) {
        // 先白化背景，再检测暗边 — 避免模型生成的暗色背景被误判为需要裁剪的边框
        processedImageData = BackgroundWhitener::fillWhiteBackgroundImageData(processedImageData, 240);
        backgroundWhitened = true;

        QImage originalImage;
        originalImage.loadFromData(imageData);
        const qint64 originalArea = (qint64)originalImage.width() * originalImage.height();

        bool didTrim = false;
        QByteArray trimmedData = ImageBorderTrimmer::trimDarkBorderImageData(processedImageData, &didTrim);

        if (didTrim) {
            QImage trimmedImage;
            trimmedImage.loadFromData(trimmedData);
            const qint64 trimmedArea = (qint64)trimmedImage.width() * trimmedImage.height();

            if (trimmedArea < originalArea * 2 / 5) {
                LOG_WARNING("BibleImageService", QString(
                    "角色图片裁剪过度 (%1x%2 -> %3x%4, 面积比=%5%), 跳过裁剪")
                    .arg(originalImage.width()).arg(originalImage.height())
                    .arg(trimmedImage.width()).arg(trimmedImage.height())
                    .arg(QString::number(trimmedArea * 100.0 / originalArea, 'f', 1) + "%"));
            } else {
                processedImageData = trimmedData;
                borderTrimmed = true;
            }
        }
    }

    if (borderTrimmed) {
        QImage originalImage;
        QImage processedImage;
        originalImage.loadFromData(imageData);
        processedImage.loadFromData(processedImageData);
        if (!originalImage.isNull() && !processedImage.isNull()) {
            LOG_INFO("BibleImageService", QString(
                "Post-processed bible image: type=%1, borderTrimmed=%2, from %4x%5 to %6x%7")
                .arg(request.type)
                .arg(borderTrimmed)
                .arg(originalImage.width())
                .arg(originalImage.height())
                .arg(processedImage.width())
                .arg(processedImage.height()));
        }
    }

    if (backgroundWhitened) {
        LOG_INFO("BibleImageService", QString(
            "Post-processed character image: background whitened, size=%1 bytes")
            .arg(processedImageData.size()));
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
    maybeFinishBatch();
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
        m_pendingRequests[requestId] = createPendingRequest(type, character, scene, prompt, negativePrompt);
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
    characterJson["appearance"] = buildAppearanceJson(app);
    
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

QJsonObject BibleImageService::buildAppearanceJson(const CharacterAppearance& app)
{
    QJsonObject json;
    json["gender"] = app.gender;
    json["age"] = app.age;
    json["hairColor"] = app.hairColor;
    json["hairStyle"] = app.hairStyle;
    json["eyeColor"] = app.eyeColor;
    json["build"] = app.build;
    json["clothing"] = QJsonArray::fromStringList(app.clothing);
    json["accessories"] = QJsonArray::fromStringList(app.accessories);
    json["distinctiveFeatures"] = QJsonArray::fromStringList(app.distinctiveFeatures);
    return json;
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
