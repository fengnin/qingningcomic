#include "BibleImageService.h"
#include "QwenImageClient.h"
#include "VolcEngineImageClient.h"
#include "FileStorage.h"
#include "CharacterExtractor.h"
#include "SceneExtractor.h"
#include "PromptBuilder.h"
#include "ServiceContainer.h"
#include "AppConfig.h"
#include "Logger.h"
#include <QTimer>

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
    
    VolcEngineImageClient* volcClient = ServiceContainer::instance()->volcEngineImageClient();
    if (volcClient) {
        connect(volcClient, &VolcEngineImageClient::generateCompleted,
                this, &BibleImageService::onVolcEngineImageGenerated);
    }
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
    m_currentType = "character";
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
    m_currentType = "scene";
    setGenerating(true);
    
    PromptBuilder::PromptResult promptResult = buildSceneReferencePrompt(scene);
    LOG_INFO("BibleImageService", QString("Generating reference for: %1").arg(scene.name()));
    
    startImageGeneration(scene.id(), promptResult.text, promptResult.negativePrompt);
}

void BibleImageService::generateCharacterPortraits(const QList<Character>& characters)
{
    if (characters.isEmpty()) {
        emit bibleBatchCompleted("character", 0, 0);
        return;
    }
    
    m_pendingCharacters = characters;
    m_pendingScenes.clear();
    m_currentCharIndex = 0;
    m_currentSceneIndex = 0;
    m_combinedMode = false;
    m_currentType = "character";
    
    startBatch(characters.size());
    LOG_INFO("BibleImageService", QString("Starting batch portrait generation for %1 characters").arg(characters.size()));
    
    processNextItem();
}

void BibleImageService::generateSceneReferences(const QList<Scene>& scenes)
{
    if (scenes.isEmpty()) {
        emit bibleBatchCompleted("scene", 0, 0);
        return;
    }
    
    m_pendingScenes = scenes;
    m_pendingCharacters.clear();
    m_currentCharIndex = 0;
    m_currentSceneIndex = 0;
    m_combinedMode = false;
    m_currentType = "scene";
    
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
    m_currentType = "character";
    
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
        m_currentType = "scene";
    }
    
    processNextItem();
}

void BibleImageService::onQwenImageGenerated(const QwenImageClient::GenerateResult& result)
{
    if (!validateAndHandleResult(result.requestId, result.imageData, result.success, result.errorMessage)) {
        return;
    }
}

void BibleImageService::onVolcEngineImageGenerated(const VolcEngineImageClient::GenerateResult& result)
{
    if (!validateAndHandleResult(result.requestId, result.imageData, result.success, result.errorMessage)) {
        return;
    }
}

bool BibleImageService::validateAndHandleResult(const QString& requestId, const QByteArray& imageData, bool success, const QString& errorMessage)
{
    if (!isGenerating()) {
        return false;
    }
    
    if (!m_currentRequestId.isEmpty() && requestId != m_currentRequestId) {
        return false;
    }
    
    handleImageGenerated(imageData, success, errorMessage);
    return true;
}

void BibleImageService::handleImageGenerated(const QByteArray& imageData, bool success, const QString& errorMessage)
{
    if (success && !imageData.isEmpty()) {
        saveAndEmitResult(imageData);
        m_currentRetryCount = 0;
    } else {
        bool shouldRetry = false;
        
        if (errorMessage.contains("timeout", Qt::CaseInsensitive) ||
            errorMessage.contains("connection", Qt::CaseInsensitive) ||
            errorMessage.contains("network", Qt::CaseInsensitive) ||
            errorMessage.contains("503", Qt::CaseInsensitive) ||
            errorMessage.contains("502", Qt::CaseInsensitive) ||
            errorMessage.contains("504", Qt::CaseInsensitive)) {
            shouldRetry = true;
        }
        
        if (shouldRetry && m_currentRetryCount < MAX_RETRY_COUNT) {
            m_currentRetryCount++;
            LOG_WARNING("BibleImageService", QString("Network error (attempt %1/%2): %3. Retrying...")
                .arg(m_currentRetryCount).arg(MAX_RETRY_COUNT).arg(errorMessage));
            scheduleNext([this]() { retryCurrentItem(); }, 2000);
            return;
        } else {
            recordFailure();
            if (shouldRetry) {
                LOG_ERROR("BibleImageService", QString("Network error after %1 retries: %2")
                    .arg(MAX_RETRY_COUNT).arg(errorMessage));
            } else {
                LOG_WARNING("BibleImageService", QString("API error (no retry to save quota): %1").arg(errorMessage));
            }
            m_currentRetryCount = 0;
        }
    }
    
    scheduleNext([this]() { processNextItem(); });
}

void BibleImageService::saveAndEmitResult(const QByteArray& imageData)
{
    QString imagePath;
    
    if (m_currentType == "character") {
        imagePath = FileStorage::instance()->saveCharacterReference(
            m_currentCharacter.id(), imageData);
        
        if (!imagePath.isEmpty()) {
            Character updatedCharacter = m_currentCharacter;
            updatedCharacter.setPortraitPath(imagePath);
            CharacterExtractor::instance()->updateCharacter(updatedCharacter);
            
            LOG_INFO("BibleImageService", QString("Portrait saved: %1").arg(imagePath));
            emit portraitGenerated(m_currentCharacter.id(), imagePath);
            recordSuccess();
        }
    } else if (m_currentType == "scene") {
        imagePath = FileStorage::instance()->saveSceneReference(
            m_currentScene.id(), imageData);
        
        if (!imagePath.isEmpty()) {
            Scene updatedScene = m_currentScene;
            updatedScene.setReferenceImagePath(imagePath);
            SceneExtractor::instance()->updateScene(updatedScene);
            
            LOG_INFO("BibleImageService", QString("Scene reference saved: %1").arg(imagePath));
            emit sceneReferenceGenerated(m_currentScene.id(), imagePath);
            recordSuccess();
        }
    }
    
    if (imagePath.isEmpty()) {
        recordFailure();
        LOG_WARNING("BibleImageService", "Failed to save image");
    }
}

void BibleImageService::startImageGeneration(const QString& requestId, const QString& prompt, const QString& negativePrompt)
{
    m_currentPrompt = prompt;
    m_currentNegativePrompt = negativePrompt;
    m_currentRequestId = requestId;
    
    QString provider = AppConfig::instance()->imageService().provider;
    
    if (provider == "volcengine") {
        VolcEngineImageClient* volcClient = ServiceContainer::instance()->volcEngineImageClient();
        if (!volcClient || !volcClient->isInitialized()) {
            LOG_ERROR("BibleImageService", "VolcEngine image client not initialized");
            recordFailure();
            scheduleNext([this]() { processNextItem(); });
            return;
        }
        
        VolcEngineImageClient::GenerateOptions options;
        options.prompt = prompt;
        options.negativePrompt = negativePrompt;
        options.width = 1328;
        options.height = 1328;
        options.seed = -1;
        options.returnUrl = true;
        options.requestId = requestId;
        
        volcClient->generateAsync(options);
    } else {
        QwenImageClient::GenerateOptions options;
        options.prompt = prompt;
        options.negativePrompt = negativePrompt;
        options.style = "manga";
        options.textRendering = false;
        options.requestId = requestId;
        
        QwenImageClient::instance()->generateAsync(options);
    }
}

void BibleImageService::retryCurrentItem()
{
    QString requestId;
    if (m_currentType == "character") {
        requestId = m_currentCharacter.id();
        LOG_INFO("BibleImageService", QString("Retrying portrait for: %1 (attempt %2)")
            .arg(m_currentCharacter.name()).arg(m_currentRetryCount));
    } else {
        requestId = m_currentScene.id();
        LOG_INFO("BibleImageService", QString("Retrying reference for: %1 (attempt %2)")
            .arg(m_currentScene.name()).arg(m_currentRetryCount));
    }
    
    startImageGeneration(requestId, m_currentPrompt, m_currentNegativePrompt);
}

void BibleImageService::processNextItem()
{
    if (!shouldContinue()) {
        return;
    }
    
    bool hasMore = false;
    
    if (m_currentType == "character" && m_currentCharIndex < m_pendingCharacters.size()) {
        const Character& character = m_pendingCharacters[m_currentCharIndex];
        
        advanceProgress(character.name());
        m_currentCharIndex++;
        
        if (!character.portraitPath().isEmpty()) {
            LOG_INFO("BibleImageService", QString("Skipping character %1, already has portrait")
                .arg(character.name()));
            recordSuccess();
            scheduleNext([this]() { processNextItem(); });
            return;
        }
        
        m_currentCharacter = character;
        m_currentScene = Scene();
        hasMore = true;
        
        PromptBuilder::PromptResult promptResult = buildCharacterPortraitPrompt(character);
        LOG_INFO("BibleImageService", QString("Generating portrait for: %1")
            .arg(character.name()));
        
        startImageGeneration(character.id(), promptResult.text, promptResult.negativePrompt);
        
    } else if (m_currentType == "scene" && m_currentSceneIndex < m_pendingScenes.size()) {
        const Scene& scene = m_pendingScenes[m_currentSceneIndex];
        
        advanceProgress(scene.name());
        m_currentSceneIndex++;
        
        if (!scene.referenceImagePath().isEmpty()) {
            LOG_INFO("BibleImageService", QString("Skipping scene %1, already has reference")
                .arg(scene.name()));
            recordSuccess();
            scheduleNext([this]() { processNextItem(); });
            return;
        }
        
        m_currentCharacter = Character();
        m_currentScene = scene;
        hasMore = true;
        
        PromptBuilder::PromptResult promptResult = buildSceneReferencePrompt(scene);
        LOG_INFO("BibleImageService", QString("Generating reference for: %1")
            .arg(scene.name()));
        
        startImageGeneration(scene.id(), promptResult.text, promptResult.negativePrompt);
    }
    
    if (!hasMore && m_combinedMode && m_currentType == "character" && !m_pendingScenes.isEmpty()) {
        m_currentType = "scene";
        hasMore = true;
        scheduleNext([this]() { processNextItem(); });
        return;
    }
    
    if (!hasMore) {
        setGenerating(false);
        if (m_combinedMode) {
            emit allBibleImagesCompleted(batchState().successCount, batchState().failedCount);
        } else {
            emit bibleBatchCompleted(m_currentType, batchState().successCount, batchState().failedCount);
        }
    }
}

PromptBuilder::PromptResult BibleImageService::buildCharacterPortraitPrompt(const Character& character)
{
    QJsonObject characterJson;
    characterJson["name"] = character.name();
    characterJson["role"] = character.role();
    
    QJsonObject appearanceJson;
    CharacterAppearance app = character.appearance();
    appearanceJson["gender"] = app.gender;
    appearanceJson["age"] = app.age;
    appearanceJson["hairColor"] = app.hairColor;
    appearanceJson["hairStyle"] = app.hairStyle;
    appearanceJson["eyeColor"] = app.eyeColor;
    appearanceJson["build"] = app.build;
    appearanceJson["clothing"] = QJsonArray::fromStringList(app.clothing);
    appearanceJson["accessories"] = QJsonArray::fromStringList(app.accessories);
    appearanceJson["distinctiveFeatures"] = QJsonArray::fromStringList(app.distinctiveFeatures);
    characterJson["appearance"] = appearanceJson;
    
    QJsonObject options;
    options["style"] = "manga";
    options["mode"] = "preview";
    options["view"] = "front";
    options["pose"] = "standing";
    
    return PromptBuilder::buildCharacterPrompt(characterJson, options);
}

PromptBuilder::PromptResult BibleImageService::buildSceneReferencePrompt(const Scene& scene)
{
    QJsonObject sceneJson;
    sceneJson["name"] = scene.name();
    
    SceneDetails details = scene.details();
    QJsonObject detailsJson = details.toJson();
    
    for (const QString& key : detailsJson.keys()) {
        sceneJson[key] = detailsJson[key];
    }
    
    return PromptBuilder::buildScenePrompt(sceneJson);
}

void BibleImageService::finishBatch()
{
    BatchImageProcessor::finishBatch();
    
    if (m_combinedMode) {
        emit allBibleImagesCompleted(batchState().successCount, batchState().failedCount);
        LOG_INFO("BibleImageService", QString("=== finishBatch: All Bible images completed: success=%1, failed=%2 ===")
            .arg(batchState().successCount).arg(batchState().failedCount));
    } else {
        emit bibleBatchCompleted(m_currentType, batchState().successCount, batchState().failedCount);
        LOG_INFO("BibleImageService", QString("Batch generation completed: type=%1, success=%2, failed=%3")
            .arg(m_currentType).arg(batchState().successCount).arg(batchState().failedCount));
    }
}
