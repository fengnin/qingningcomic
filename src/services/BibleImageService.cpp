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

BibleImageService* BibleImageService::instance()
{
    if (!m_instance) {
        m_instance = new BibleImageService();
    }
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
    
    QString provider = AppConfig::instance()->imageService().provider;
    LOG_INFO("BibleImageService", QString("Using image provider: %1").arg(provider));
    
    if (provider == "volcengine") {
        VolcEngineImageClient* volcClient = ServiceContainer::instance()->volcEngineImageClient();
        if (!volcClient || !volcClient->isInitialized()) {
            LOG_ERROR("BibleImageService", "VolcEngine image client not initialized");
            emit portraitGenerated(character.id(), QString());
            return;
        }
        
        VolcEngineImageClient::GenerateOptions options;
        options.prompt = promptResult.text;
        options.width = 1328;
        options.height = 1328;
        options.seed = -1;
        options.returnUrl = true;
        options.requestId = character.id();
        
        volcClient->generateAsync(options);
    } else {
        QwenImageClient::GenerateOptions options;
        options.prompt = promptResult.text;
        options.negativePrompt = promptResult.negativePrompt;
        options.style = "manga";
        options.textRendering = false;
        options.requestId = character.id();
        
        QwenImageClient::instance()->generateAsync(options);
    }
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
    
    QString provider = AppConfig::instance()->imageService().provider;
    LOG_INFO("BibleImageService", QString("Using image provider: %1").arg(provider));
    
    if (provider == "volcengine") {
        VolcEngineImageClient* volcClient = ServiceContainer::instance()->volcEngineImageClient();
        if (!volcClient || !volcClient->isInitialized()) {
            LOG_ERROR("BibleImageService", "VolcEngine image client not initialized");
            emit sceneReferenceGenerated(scene.id(), QString());
            return;
        }
        
        VolcEngineImageClient::GenerateOptions options;
        options.prompt = promptResult.text;
        options.width = 1328;
        options.height = 1328;
        options.seed = -1;
        options.returnUrl = true;
        options.requestId = scene.id();
        
        volcClient->generateAsync(options);
    } else {
        QwenImageClient::GenerateOptions options;
        options.prompt = promptResult.text;
        options.negativePrompt = promptResult.negativePrompt;
        options.style = "manga";
        options.textRendering = false;
        options.requestId = scene.id();
        
        QwenImageClient::instance()->generateAsync(options);
    }
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
    if (!isGenerating()) {
        return;
    }
    
    QString imagePath;
    bool success = result.success && !result.imageData.isEmpty();
    
    if (success) {
        if (m_currentType == "character") {
            imagePath = FileStorage::instance()->saveCharacterReference(
                m_currentCharacter.id(), result.imageData);
            
            if (!imagePath.isEmpty()) {
                Character updatedCharacter = m_currentCharacter;
                updatedCharacter.setPortraitPath(imagePath);
                CharacterExtractor::instance()->updateCharacter(updatedCharacter);
                
                LOG_INFO("BibleImageService", QString("Portrait saved: %1").arg(imagePath));
                emit portraitGenerated(m_currentCharacter.id(), imagePath);
            }
        } else if (m_currentType == "scene") {
            imagePath = FileStorage::instance()->saveSceneReference(
                m_currentScene.id(), result.imageData);
            
            if (!imagePath.isEmpty()) {
                Scene updatedScene = m_currentScene;
                updatedScene.setReferenceImagePath(imagePath);
                SceneExtractor::instance()->updateScene(updatedScene);
                
                LOG_INFO("BibleImageService", QString("Scene reference saved: %1").arg(imagePath));
                emit sceneReferenceGenerated(m_currentScene.id(), imagePath);
            }
        }
        
        if (!imagePath.isEmpty()) {
            recordSuccess();
        } else {
            recordFailure();
            LOG_WARNING("BibleImageService", "Failed to save image");
        }
    } else {
        recordFailure();
        LOG_WARNING("BibleImageService", QString("Image generation failed: %1")
            .arg(result.errorMessage));
    }
    
    scheduleNext([this]() { processNextItem(); });
}

void BibleImageService::onVolcEngineImageGenerated(const VolcEngineImageClient::GenerateResult& result)
{
    if (!isGenerating()) {
        return;
    }
    
    QString imagePath;
    bool success = result.success && !result.imageData.isEmpty();
    
    if (success) {
        if (m_currentType == "character") {
            imagePath = FileStorage::instance()->saveCharacterReference(
                m_currentCharacter.id(), result.imageData);
            
            if (!imagePath.isEmpty()) {
                Character updatedCharacter = m_currentCharacter;
                updatedCharacter.setPortraitPath(imagePath);
                CharacterExtractor::instance()->updateCharacter(updatedCharacter);
                
                LOG_INFO("BibleImageService", QString("Portrait saved: %1").arg(imagePath));
                emit portraitGenerated(m_currentCharacter.id(), imagePath);
            }
        } else if (m_currentType == "scene") {
            imagePath = FileStorage::instance()->saveSceneReference(
                m_currentScene.id(), result.imageData);
            
            if (!imagePath.isEmpty()) {
                Scene updatedScene = m_currentScene;
                updatedScene.setReferenceImagePath(imagePath);
                SceneExtractor::instance()->updateScene(updatedScene);
                
                LOG_INFO("BibleImageService", QString("Scene reference saved: %1").arg(imagePath));
                emit sceneReferenceGenerated(m_currentScene.id(), imagePath);
            }
        }
        
        if (!imagePath.isEmpty()) {
            recordSuccess();
        } else {
            recordFailure();
            LOG_WARNING("BibleImageService", "Failed to save image");
        }
    } else {
        recordFailure();
        LOG_WARNING("BibleImageService", QString("Image generation failed: %1")
            .arg(result.errorMessage));
    }
    
    scheduleNext([this]() { processNextItem(); });
}

void BibleImageService::processNextItem()
{
    if (!shouldContinue()) {
        return;
    }
    
    bool hasMore = false;
    QString provider = AppConfig::instance()->imageService().provider;
    
    if (m_currentType == "character" && m_currentCharIndex < m_pendingCharacters.size()) {
        const Character& character = m_pendingCharacters[m_currentCharIndex];
        
        advanceProgress(character.name());
        
        if (!character.portraitPath().isEmpty()) {
            LOG_INFO("BibleImageService", QString("Skipping character %1, already has portrait")
                .arg(character.name()));
            recordSuccess();
            m_currentCharIndex++;
            scheduleNext([this]() { processNextItem(); });
            return;
        }
        
        m_currentCharacter = character;
        m_currentScene = Scene();
        hasMore = true;
        
        PromptBuilder::PromptResult promptResult = buildCharacterPortraitPrompt(character);
        LOG_INFO("BibleImageService", QString("Generating portrait for: %1 (provider: %2)")
            .arg(character.name()).arg(provider));
        
        if (provider == "volcengine") {
            VolcEngineImageClient* volcClient = ServiceContainer::instance()->volcEngineImageClient();
            if (!volcClient || !volcClient->isInitialized()) {
                LOG_ERROR("BibleImageService", "VolcEngine image client not initialized");
                recordFailure();
                m_currentCharIndex++;
                scheduleNext([this]() { processNextItem(); });
                return;
            }
            
            VolcEngineImageClient::GenerateOptions options;
            options.prompt = promptResult.text;
            options.width = 1328;
            options.height = 1328;
            options.seed = -1;
            options.returnUrl = true;
            options.requestId = character.id();
            
            volcClient->generateAsync(options);
        } else {
            QwenImageClient::GenerateOptions options;
            options.prompt = promptResult.text;
            options.negativePrompt = promptResult.negativePrompt;
            options.style = "manga";
            options.textRendering = false;
            options.requestId = character.id();
            
            QwenImageClient::instance()->generateAsync(options);
        }
        m_currentCharIndex++;
        
    } else if (m_currentType == "scene" && m_currentSceneIndex < m_pendingScenes.size()) {
        const Scene& scene = m_pendingScenes[m_currentSceneIndex];
        
        advanceProgress(scene.name());
        
        if (!scene.referenceImagePath().isEmpty()) {
            LOG_INFO("BibleImageService", QString("Skipping scene %1, already has reference")
                .arg(scene.name()));
            recordSuccess();
            m_currentSceneIndex++;
            scheduleNext([this]() { processNextItem(); });
            return;
        }
        
        m_currentCharacter = Character();
        m_currentScene = scene;
        hasMore = true;
        
        PromptBuilder::PromptResult promptResult = buildSceneReferencePrompt(scene);
        LOG_INFO("BibleImageService", QString("Generating reference for: %1 (provider: %2)")
            .arg(scene.name()).arg(provider));
        
        if (provider == "volcengine") {
            VolcEngineImageClient* volcClient = ServiceContainer::instance()->volcEngineImageClient();
            if (!volcClient || !volcClient->isInitialized()) {
                LOG_ERROR("BibleImageService", "VolcEngine image client not initialized");
                recordFailure();
                m_currentSceneIndex++;
                scheduleNext([this]() { processNextItem(); });
                return;
            }
            
            VolcEngineImageClient::GenerateOptions options;
            options.prompt = promptResult.text;
            options.width = 1328;
            options.height = 1328;
            options.seed = -1;
            options.returnUrl = true;
            options.requestId = scene.id();
            
            volcClient->generateAsync(options);
        } else {
            QwenImageClient::GenerateOptions options;
            options.prompt = promptResult.text;
            options.negativePrompt = promptResult.negativePrompt;
            options.style = "manga";
            options.textRendering = false;
            options.requestId = scene.id();
            
            QwenImageClient::instance()->generateAsync(options);
        }
        m_currentSceneIndex++;
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
            LOG_INFO("BibleImageService", QString("All Bible images completed: success=%1, failed=%2")
                .arg(batchState().successCount).arg(batchState().failedCount));
        } else {
            emit bibleBatchCompleted(m_currentType, batchState().successCount, batchState().failedCount);
            LOG_INFO("BibleImageService", QString("Batch generation completed: type=%1, success=%2, failed=%3")
                .arg(m_currentType).arg(batchState().successCount).arg(batchState().failedCount));
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
    sceneJson["description"] = details.description;
    
    QJsonObject visualCharacteristics;
    visualCharacteristics["architecture"] = details.building;
    visualCharacteristics["colorScheme"] = details.color;
    visualCharacteristics["atmosphere"] = details.atmosphere;
    
    QJsonArray landmarks;
    if (!details.landmark.isEmpty()) {
        landmarks.append(details.landmark);
    }
    visualCharacteristics["keyLandmarks"] = landmarks;
    
    QJsonObject lighting;
    lighting["naturalLight"] = details.timeOfDay;
    visualCharacteristics["lighting"] = lighting;
    
    sceneJson["visualCharacteristics"] = visualCharacteristics;
    
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
