#include "services/BibleImageService.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "data/FileStorage.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "utils/PromptBuilder.h"
#include "utils/AppConfig.h"
#include "utils/BibleUtils.h"
#include "utils/Logger.h"
#include <QJsonArray>
#include <QTimer>

using namespace BibleUtils::SceneTextFilter;

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
    clearCurrentState();
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
    
    m_currentCharacter = character;
    m_currentScene = Scene();
    m_currentType = BibleImageConstants::TYPE_CHARACTER;
    setGenerating(true);
    
    PromptBuilder::PromptResult promptResult = buildCharacterPortraitPrompt(character);
    LOG_INFO("BibleImageService", QString("Generating portrait for: %1").arg(character.name()));
    
    startImageGeneration(character.id(), promptResult.text, promptResult.negativePrompt);
}

void BibleImageService::generateSceneReferenceAsync(const Scene& scene)
{
    if (scene.id().isEmpty()) {
        setError("Scene ID is empty");
        emit sceneReferenceGenerated(scene.id(), QString());
        return;
    }
    
    m_currentCharacter = Character();
    m_currentScene = scene;
    m_currentType = BibleImageConstants::TYPE_SCENE;
    setGenerating(true);
    
    PromptBuilder::PromptResult promptResult = buildSceneReferencePrompt(scene);
    LOG_INFO("BibleImageService", QString("Generating reference for: %1").arg(scene.name()));
    
    startImageGeneration(scene.id(), promptResult.text, promptResult.negativePrompt);
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
    if (!m_currentRequestId.isEmpty() && !result.requestId.isEmpty() && result.requestId != m_currentRequestId) {
        LOG_WARNING("BibleImageService", QString("Ignored stale Qwen result: requestId=%1, current=%2")
            .arg(result.requestId).arg(m_currentRequestId));
        return;
    }
    handleImageGenerated(result.imageData, result.success, result.errorMessage);
}

void BibleImageService::onVolcEngineImageGenerated(const VolcEngineImageClient::GenerateResult& result)
{
    if (!m_currentRequestId.isEmpty() && !result.requestId.isEmpty() && result.requestId != m_currentRequestId) {
        LOG_WARNING("BibleImageService", QString("Ignored stale VolcEngine result: requestId=%1, current=%2")
            .arg(result.requestId).arg(m_currentRequestId));
        return;
    }
    handleImageGenerated(result.imageData, result.success, result.errorMessage);
}

bool BibleImageService::validateAndHandleResult(const QString& requestId, const QByteArray& imageData, bool success, const QString& errorMessage)
{
    Q_UNUSED(requestId);
    handleImageGenerated(imageData, success, errorMessage);
    return true;
}

void BibleImageService::handleImageGenerated(const QByteArray& imageData, bool success, const QString& errorMessage)
{
    if (success && !imageData.isEmpty()) {
        m_currentRetryCount = 0;
        saveAndEmitResult(imageData);
    } else {
        if (m_currentRetryCount < MAX_RETRY_COUNT) {
            LOG_WARNING("BibleImageService", QString("Image generation failed (attempt %1/%2): %3, retrying...")
                .arg(m_currentRetryCount + 1).arg(MAX_RETRY_COUNT).arg(errorMessage));
            retryCurrentItem();
            return;
        } else {
            recordFailure();
            LOG_ERROR("BibleImageService", QString("Image generation failed after %1 retries: %2")
                .arg(MAX_RETRY_COUNT).arg(errorMessage));
            m_currentRetryCount = 0;
        }
    }
    
    scheduleNext([this]() { processNextItem(); });
}

void BibleImageService::retryCurrentItem()
{
    m_currentRetryCount++;
    
    LOG_INFO("BibleImageService", QString("Retrying image generation (attempt %1/%2)")
        .arg(m_currentRetryCount).arg(MAX_RETRY_COUNT));
    
    if (!m_currentPrompt.isEmpty()) {
        QTimer::singleShot(1000, this, [this]() {
            startImageGeneration(m_currentRequestId, m_currentPrompt, m_currentNegativePrompt);
        });
    } else {
        LOG_ERROR("BibleImageService", "Cannot retry: no prompt saved");
        recordFailure();
        scheduleNext([this]() { processNextItem(); });
    }
}

void BibleImageService::saveAndEmitResult(const QByteArray& imageData)
{
    QString imagePath;
    bool savedOk = false;
    
    if (m_currentType == BibleImageConstants::TYPE_CHARACTER) {
        imagePath = saveCharacterImage(imageData);
        savedOk = !imagePath.isEmpty();
    } else if (m_currentType == BibleImageConstants::TYPE_SCENE) {
        imagePath = saveSceneImage(imageData);
        savedOk = !imagePath.isEmpty();
    }
    
    if (!savedOk) {
        recordFailure();
        LOG_WARNING("BibleImageService", QString("Failed to save image for %1: %2")
            .arg(m_currentType)
            .arg(m_currentType == BibleImageConstants::TYPE_CHARACTER ? m_currentCharacter.name() : m_currentScene.name()));
    }
    
    clearCurrentState();
}

QString BibleImageService::saveCharacterImage(const QByteArray& imageData)
{
    QString imagePath = FileStorage::instance()->saveCharacterReference(
        m_currentCharacter.id(), imageData);
    
    if (!imagePath.isEmpty()) {
        Character updatedCharacter = CharacterExtractor::instance()->getCharacterById(m_currentCharacter.id());
        if (updatedCharacter.id().isEmpty()) {
            updatedCharacter = m_currentCharacter;
        }
        updatedCharacter.setPortraitPath(imagePath);
        if (!CharacterExtractor::instance()->updateCharacter(updatedCharacter)) {
            LOG_WARNING("BibleImageService", QString("Saved portrait but failed to update character row: %1")
                .arg(m_currentCharacter.name()));
            return QString();
        } else {
            LOG_INFO("BibleImageService", QString("Portrait saved: %1").arg(imagePath));
            emit portraitGenerated(m_currentCharacter.id(), imagePath);
            recordSuccess();
        }
    }
    
    return imagePath;
}

QString BibleImageService::saveSceneImage(const QByteArray& imageData)
{
    QString imagePath = FileStorage::instance()->saveSceneReference(
        m_currentScene.id(), imageData);
    
    if (!imagePath.isEmpty()) {
        Scene updatedScene = SceneExtractor::instance()->getSceneById(m_currentScene.id());
        if (updatedScene.id().isEmpty()) {
            updatedScene = m_currentScene;
        }
        updatedScene.setReferenceImagePath(imagePath);
        if (!SceneExtractor::instance()->updateScene(updatedScene)) {
            LOG_WARNING("BibleImageService", QString("Saved scene reference but failed to update scene row: %1")
                .arg(m_currentScene.name()));
            return QString();
        } else {
            LOG_INFO("BibleImageService", QString("Scene reference saved: %1").arg(imagePath));
            emit sceneReferenceGenerated(m_currentScene.id(), imagePath);
            recordSuccess();
        }
    }
    
    return imagePath;
}

void BibleImageService::clearCurrentState()
{
    m_currentCharacter = Character();
    m_currentScene = Scene();
    m_currentPrompt.clear();
    m_currentNegativePrompt.clear();
    m_currentRequestId.clear();
    m_currentRetryCount = 0;
}

void BibleImageService::startImageGeneration(const QString& requestId, const QString& prompt, const QString& negativePrompt)
{
    m_currentRequestId = requestId;
    m_currentPrompt = prompt;
    m_currentNegativePrompt = negativePrompt;
    
    QString provider = AppConfig::instance()->imageService().provider;
    
    if (provider == "volcengine") {
        VolcEngineImageClient::GenerateOptions options;
        options.prompt = prompt;
        options.negativePrompt = negativePrompt;
        options.requestId = requestId;
        VolcEngineImageClient::instance()->generateAsync(options);
    } else {
        QwenImageClient::GenerateOptions options;
        options.prompt = prompt;
        options.negativePrompt = negativePrompt;
        options.style = "manga";
        options.textRendering = false;
        options.requestId = requestId;
        
        if (m_currentType == BibleImageConstants::TYPE_CHARACTER) {
            // 角色立绘：1024×1440（比例 1:1.40625，匹配 UI 128×180 的 1:1.4）
            // 总像素 1474560 > 1048576(1024²)，分辨率不降反升
            options.size = QwenImageClient::ImageSize::Custom;
            options.width = 1024;
            options.height = 1440;
        } else {
            // 场景参考图：保持正方形 1024×1024
            options.size = QwenImageClient::ImageSize::Square;
        }
        
        QwenImageClient::instance()->generateAsync(options);
    }
}

void BibleImageService::processNextItem()
{
    if (!shouldContinue()) {
        return;
    }
    
    bool hasMore = false;
    
    if (m_currentType == BibleImageConstants::TYPE_CHARACTER && m_currentCharIndex < m_pendingCharacters.size()) {
        hasMore = processNextCharacter();
    } else if (m_currentType == BibleImageConstants::TYPE_SCENE && m_currentSceneIndex < m_pendingScenes.size()) {
        hasMore = processNextScene();
    }
    
    // 只有当所有角色都处理完毕后，才切换到场景处理
    // 修复：增加 m_currentCharIndex >= m_pendingCharacters.size() 检查
    if (!hasMore && m_combinedMode && m_currentType == BibleImageConstants::TYPE_CHARACTER 
        && m_currentCharIndex >= m_pendingCharacters.size() && !m_pendingScenes.isEmpty()) {
        m_currentType = BibleImageConstants::TYPE_SCENE;
        hasMore = true;
        scheduleNext([this]() { processNextItem(); });
        return;
    }
    
    if (!hasMore) {
        finishBatch();
    }
}

bool BibleImageService::processNextCharacter()
{
    const Character& character = m_pendingCharacters[m_currentCharIndex];
    
    advanceProgress(character.name());
    
    if (!character.portraitPath().isEmpty()) {
        LOG_INFO("BibleImageService", QString("Skipping character %1, already has portrait")
            .arg(character.name()));
        recordSuccess();
        m_currentCharIndex++;
        scheduleNext([this]() { processNextItem(); });
        return true;
    }
    
    m_currentCharacter = character;
    m_currentScene = Scene();
    
    PromptBuilder::PromptResult promptResult = buildCharacterPortraitPrompt(character);
    LOG_INFO("BibleImageService", QString("Generating portrait for: %1")
        .arg(character.name()));
    
    startImageGeneration(character.id(), promptResult.text, promptResult.negativePrompt);
    m_currentCharIndex++;
    
    return true;
}

bool BibleImageService::processNextScene()
{
    const Scene& scene = m_pendingScenes[m_currentSceneIndex];
    
    advanceProgress(scene.name());
    
    if (!scene.referenceImagePath().isEmpty()) {
        LOG_INFO("BibleImageService", QString("Skipping scene %1, already has reference")
            .arg(scene.name()));
        recordSuccess();
        m_currentSceneIndex++;
        scheduleNext([this]() { processNextItem(); });
        return true;
    }
    
    m_currentCharacter = Character();
    m_currentScene = scene;
    
    PromptBuilder::PromptResult promptResult = buildSceneReferencePrompt(scene);
    LOG_INFO("BibleImageService", QString("Generating reference for: %1")
        .arg(scene.name()));
    
    startImageGeneration(scene.id(), promptResult.text, promptResult.negativePrompt);
    m_currentSceneIndex++;
    
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
    options["style"] = "manga";
    options["mode"] = "preview";
    options["view"] = "front";
    options["pose"] = "standing";
    
    return PromptBuilder::buildCharacterPrompt(characterJson, options);
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
