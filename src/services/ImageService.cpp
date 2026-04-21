#include "services/ImageService.h"
#include "services/ServiceContainer.h"
#include "data/DatabaseManager.h"
#include "utils/SingletonUtils.h"
#include "utils/PromptBuilder.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "api/StorageClient.h"
#include "data/FileStorage.h"
#include "services/StoryboardService.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "services/TaskQueue.h"
#include "models/Task.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include "utils/AppConfig.h"
#include <QJsonDocument>
#include <QDateTime>
#include <QFileInfo>
#include <QImage>
#include <QSqlQuery>
#include <QtConcurrent>
#include <QThread>
#include <QTimer>
#include <cmath>

namespace {
QByteArray normalizePresetMode(const QString& presetMode)
{
    return presetMode.trimmed().toLower().toUtf8();
}

QString classifyPanelPromptTarget(const QJsonObject& panelJson)
{
    QString visualPrompt = panelJson.value("visualPrompt").toString().trimmed();
    QString visualPromptEn = panelJson.value("visualPromptEn").toString().trimmed();
    QString combined = (visualPrompt + " " + visualPromptEn).trimmed().toLower();
    QString sceneText = panelJson.value("scene").toString().trimmed().toLower();
    QString shotType = panelJson.value("shotType").toString().trimmed().toLower();

    auto containsAny = [](const QString& text, const QStringList& keywords) {
        for (const QString& keyword : keywords) {
            if (text.contains(keyword.toLower())) {
                return true;
            }
        }
        return false;
    };
    const QStringList sceneKeywords = {
        "scene", "background", "environment", "setting", "location",
        "room", "street", "building", "architecture", "landscape",
        QString::fromUtf8("场景"), QString::fromUtf8("背景"), QString::fromUtf8("环境"),
        QString::fromUtf8("街道"), QString::fromUtf8("建筑"), QString::fromUtf8("风景"),
        QString::fromUtf8("室内"), QString::fromUtf8("室外"), QString::fromUtf8("城市"),
        QString::fromUtf8("自然")
    };
    const QStringList characterKeywords = {
        "character", "person", "face", "expression", "outfit",
        "clothing", "hair", "pose", "body", "identity",
        QString::fromUtf8("角色"), QString::fromUtf8("人物"), QString::fromUtf8("表情"),
        QString::fromUtf8("服装"), QString::fromUtf8("发型"), QString::fromUtf8("姿势"),
        QString::fromUtf8("外貌"), QString::fromUtf8("特征"), QString::fromUtf8("身份")
    };


    bool sceneCue = containsAny(combined, sceneKeywords) || containsAny(sceneText, sceneKeywords);
    bool charCue = containsAny(combined, characterKeywords);

    if (sceneCue && !charCue) {
        return "scene";
    }
    if (charCue && !sceneCue) {
        return "character";
    }

    static const QStringList sceneShotTypes = {
        "wide", "extreme-wide", "establishing", "full", "long", "master"
    };
    static const QStringList characterShotTypes = {
        "close-up", "extreme-close-up", "medium-close-up", "portrait", "single"
    };

    if (containsAny(shotType, sceneShotTypes)) {
        return "scene";
    }
    if (containsAny(shotType, characterShotTypes)) {
        return "character";
    }

    int charCount = panelJson.value("characters").toArray().size();
    if (charCount <= 1) {
        return "character";
    }
    if (charCount >= 3) {
        return "scene";
    }

    return "balanced";
}
}

ImageService::ResolutionConfig ImageService::ResolutionConfig::fromMode(GenerateMode mode)
{
    ResolutionConfig config;
    config.isPreset = false;
    
    QString provider = AppConfig::instance()->imageService().provider;
    if (provider == "volcengine") {
        config.width = (mode == GenerateMode::HD) ? 2048 : 1328;
        config.height = (mode == GenerateMode::HD) ? 2048 : 1328;
    } else {
        config.width = (mode == GenerateMode::HD) ? 1024 : 768;
        config.height = (mode == GenerateMode::HD) ? 1024 : 1024;
    }
    
    return config;
}

ImageService::ResolutionConfig ImageService::ResolutionConfig::fromPreset(BatchPresetMode preset)
{
    ResolutionConfig config;
    config.isPreset = true;
    config.presetName = ImageService::instance()->presetModeToString(preset);
    
    ImageService::instance()->getPresetResolution(preset, config.width, config.height);
    
    return config;
}

DEFINE_SINGLETON_INSTANCE_SIMPLE(ImageService)

ImageService::ImageService(QObject* parent)
    : BatchImageProcessor(parent)
    , m_currentMode(GenerateMode::Preview)
    , m_currentProcessIndex(0)
    , m_batchCancelled(false)
{
    qRegisterMetaType<GenerateResult>("ImageService::GenerateResult");
    qRegisterMetaType<GenerateResult>("GenerateResult");
    qRegisterMetaType<BatchResult>("ImageService::BatchResult");
    qRegisterMetaType<BatchResult>("BatchResult");
    
    RetryConfig dbConfig;
    dbConfig.maxAttempts = 3;
    dbConfig.baseDelayMs = 1000;
    dbConfig.backoffMultiplier = 2.0;
    dbConfig.maxDelayMs = 30000;
    m_dbRetryPolicy = new RetryPolicy(dbConfig, this);
    
    RetryConfig apiConfig;
    apiConfig.maxAttempts = 3;
    apiConfig.baseDelayMs = 2000;
    apiConfig.backoffMultiplier = 2.0;
    apiConfig.maxDelayMs = 60000;
    m_apiRetryPolicy = new RetryPolicy(apiConfig, this);
}

ImageService::~ImageService()
{
    cancelCurrentBatch();
}

void ImageService::cancelCurrentBatch()
{
    QMutexLocker locker(&m_mutex);
    m_batchCancelled = true;
    m_pendingPanelIds.clear();
    m_currentProcessIndex = 0;
    m_currentPresetMode.clear();
    
    m_generating = false;
    m_batchResult = BatchResult();
    
    emit batchGenerationCancelled();
}

ImageService::GenerateResult ImageService::generatePanelImageCore(const QString& panelId, const ResolutionConfig& resolution, bool checkConcurrency)
{
    if (panelId.isEmpty()) {
        return createErrorResult(panelId, tr("图片服务不可用"));
    }

    LOG_INFO("ImageService", QString("generatePanelImageCore: panelId=%1, resolution=%2x%3")
        .arg(panelId).arg(resolution.width).arg(resolution.height));

    if (checkConcurrency && isGenerating()) {
        LOG_WARNING("ImageService", QString("Generation already in progress for panel %1").arg(panelId));
        return createErrorResult(panelId, tr("正在生成中，请等待完成"));
    }

    if (checkConcurrency) {
        setGenerating(true);
    }

    auto resetState = [this, checkConcurrency]() {
        if (checkConcurrency) {
            setGenerating(false);
        }
    };

    GenerationContext ctx;
    ctx.panelId = panelId;
    ctx.resolution = resolution;
    if (resolution.isPreset) {
        ctx.presetMode = resolution.presetName;
    } else {
        ctx.mode = (resolution.width >= 2048) ? GenerateMode::HD : GenerateMode::Preview;
    }

    try {
        if (!safeFetchPanel(ctx)) {
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("获取面板数据失败") : lastError());
        }

        setError(tr("加载参考图失败"));

        if (!buildPromptForPanel(ctx)) {
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("构建提示词失败") : lastError());
        }

        if (!generateImage(ctx)) {
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("图像生成失败") : lastError());
        }

        if (!storeImage(ctx)) {
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("保存图像失败") : lastError());
        }

        if (!safeUpdateDatabase(ctx)) {
            LOG_ERROR("ImageService", QString("Failed to update database for panel: %1").arg(panelId));
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("数据库更新失败") : lastError());
        }

        LOG_INFO("ImageService", QString("Panel generated successfully: %1").arg(panelId));
        
        resetState();
        return finalizeResult(ctx);
    } catch (const std::exception& e) {
        QString errorMsg = QString::fromUtf8(e.what());
        LOG_ERROR("ImageService", QString("Exception during generation of panel %1: %2").arg(panelId).arg(errorMsg));
        resetState();
        return createErrorResult(panelId, errorMsg);
    } catch (...) {
        LOG_ERROR("ImageService", QString("Unknown exception during generation of panel %1").arg(panelId));
        resetState();
        return createErrorResult(panelId, tr("生成过程中发生未知错误"));
    }
}

ImageService::GenerateResult ImageService::generatePanelImage(const QString& panelId, GenerateMode mode)
{
    return generatePanelImageCore(panelId, ResolutionConfig::fromMode(mode), true);
}

ImageService::GenerateResult ImageService::generatePanelImage(const QString& panelId, BatchPresetMode presetMode)
{
    return generatePanelImageCore(panelId, ResolutionConfig::fromPreset(presetMode), true);
}

ImageService::GenerateResult ImageService::generatePanelImageInternal(const QString& panelId, const ResolutionConfig& resolution)
{
    return generatePanelImageCore(panelId, resolution, false);
}

void ImageService::startBatchGeneration(const QStringList& panelIds, const ResolutionConfig& resolution)
{
    if (isGenerating()) {
        emit errorOccurred("startBatchGeneration", tr("正在生成中，请等待完成"));
        return;
    }

    if (panelIds.isEmpty()) {
        BatchResult result;
        result.errorMessage = tr("面板列表为空");
        emit batchProgressChanged(0, 0, result.errorMessage);
        emit imageBatchCompleted(result);
        emit errorOccurred("startBatchGeneration", result.errorMessage);
        return;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_pendingPanelIds = panelIds;
        m_currentResolution = resolution;
        m_currentProcessIndex = 0;
        m_batchCancelled = false;

        m_batchResult = BatchResult();
        m_batchResult.totalCount = panelIds.size();
    }

    startBatch(panelIds.size());
    LOG_INFO("ImageService", QString("Starting batch generation for %1 panels").arg(panelIds.size()));

    QTimer::singleShot(0, this, &ImageService::processNextPanel);
}

void ImageService::generatePanelImages(const QStringList& panelIds, GenerateMode mode)
{
    startBatchGeneration(panelIds, ResolutionConfig::fromMode(mode));
}

void ImageService::generatePanelImages(const QStringList& panelIds, BatchPresetMode presetMode)
{
    startBatchGeneration(panelIds, ResolutionConfig::fromPreset(presetMode));
}

void ImageService::generateStoryboardImages(const QString& storyboardId, GenerateMode mode)
{
    QList<Panel> panels;
    
    try {
        panels = StoryboardService::instance()->getPanels(storyboardId);
    } catch (const std::exception& e) {
        LOG_ERROR("ImageService", QString("getPanels exception: %1").arg(e.what()));
        emit errorOccurred("generateStoryboardImages", tr("获取面板数据异常"));
        return;
    } catch (...) {
        LOG_ERROR("ImageService", "getPanels unknown exception");
        emit errorOccurred("generateStoryboardImages", tr("获取面板数据未知错误"));
        return;
    }

    if (panels.isEmpty()) {
        emit errorOccurred("generateStoryboardImages", TR_FMT("Storyboard has no panels: %1", storyboardId));
        return;
    }

    QStringList panelIds;
    for (const Panel& panel : panels) {
        panelIds.append(panel.id());
    }

    generatePanelImages(panelIds, mode);
}

void ImageService::generateStoryboardImages(const QString& storyboardId, BatchPresetMode presetMode)
{
    QList<Panel> panels;

    try {
        panels = StoryboardService::instance()->getPanels(storyboardId);
    } catch (const std::exception& e) {
        LOG_ERROR("ImageService", QString("getPanels exception: %1").arg(e.what()));
        emit errorOccurred("generateStoryboardImages", tr("获取分镜面板异常"));
        return;
    } catch (...) {
        LOG_ERROR("ImageService", "getPanels unknown exception");
        emit errorOccurred("generateStoryboardImages", tr("获取分镜面板未知错误"));
        return;
    }

    if (panels.isEmpty()) {
        emit errorOccurred("generateStoryboardImages", TR_FMT("No panels found for storyboard %1", storyboardId));
        return;
    }

    QStringList panelIds;
    for (const Panel& panel : panels) {
        panelIds.append(panel.id());
    }

    generatePanelImages(panelIds, presetMode);
}

ImageService::GenerateResult ImageService::regeneratePanelImage(const QString& panelId, GenerateMode mode)
{
    LOG_INFO("ImageService", QString("Regenerating panel: %1").arg(panelId));
    return generatePanelImage(panelId, mode);
}

ImageService::GenerateResult ImageService::regeneratePanelImage(const QString& panelId, BatchPresetMode presetMode)
{
    LOG_INFO("ImageService", QString("Regenerating panel preset: %1").arg(panelId));
    return generatePanelImage(panelId, presetMode);
}

ImageService::ProviderConfig ImageService::getProviderConfig() const
{
    ProviderConfig config;
    QString provider = AppConfig::instance()->imageService().provider;
    
    if (provider == "volcengine") {
        config.name = "volcengine";
        config.maxRefImages = 1;
        config.supportsImg2Img = true;
        config.previewWidth = 1056;
        config.previewHeight = 1584;
        config.hdWidth = 1328;
        config.hdHeight = 1328;
    } else {
        config.name = "qwen";
        config.maxRefImages = 3;
        config.supportsImg2Img = true;
        config.previewWidth = 512;
        config.previewHeight = 768;
        config.hdWidth = 1024;
        config.hdHeight = 1024;
    }
    
    return config;
}

bool ImageService::loadReferenceImage(GenerationContext& ctx)
{
    if (ctx.referenceImages.isEmpty()) {
        LOG_DEBUG("ImageService", "No reference images to load, using text2img");
        return true;
    }
    
    ProviderConfig config = getProviderConfig();
    
    if (ctx.referenceImages.size() > config.maxRefImages) {
        LOG_INFO("ImageService", QString("Limiting reference images from %1 to %2 for provider: %3")
            .arg(ctx.referenceImages.size()).arg(config.maxRefImages).arg(config.name));
        ctx.referenceImages = ctx.referenceImages.mid(0, config.maxRefImages);
    }
    
    QString refPath = ctx.referenceImages.first();
    QString fullPath = FileStorage::instance()->getFullPath(refPath);
    
    LOG_INFO("ImageService", QString("Loading reference image: %1 (full path: %2)")
        .arg(refPath).arg(fullPath));
    
    QFile refFile(fullPath);
    
    if (!refFile.exists()) {
        LOG_WARNING("ImageService", QString("Reference image not found: %1, falling back to text2image")
            .arg(refPath));
        ctx.referenceImages.clear();
        return true;
    }
    
    if (!refFile.open(QIODevice::ReadOnly)) {
        LOG_WARNING("ImageService", QString("Cannot open reference image: %1, falling back to text2image")
            .arg(refPath));
        ctx.referenceImages.clear();
        return true;
    }
    
    ctx.refImageData = refFile.readAll();
    refFile.close();
    
    LOG_INFO("ImageService", QString("Loaded reference image: %1 bytes").arg(ctx.refImageData.size()));
    
    if (ctx.referenceImages.size() > 1) {
        LOG_INFO("ImageService", QString("Multiple reference images detected (%1), using first as primary")
            .arg(ctx.referenceImages.size()));
    }
    
    return true;
}

bool ImageService::generateImage(GenerationContext& ctx)
{
    emit progressChanged(tr("正在生成图像..."), 30);
    
    if (!loadReferenceImage(ctx)) {
        return false;
    }
    
    return executeImageGeneration(ctx);
}

bool ImageService::executeWithVolcEngine(GenerationContext& ctx)
{
    VolcEngineImageClient* client = ServiceContainer::instance()->volcEngineImageClient();
    if (!client || !client->isInitialized()) {
        setError(tr("VolcEngine客户端不可用"));
        return false;
    }
    
    ProviderConfig config = getProviderConfig();
    
    VolcEngineImageClient::GenerateOptions options;
    options.prompt = ctx.prompt;
    options.negativePrompt = ctx.negativePrompt;
    if (!ctx.presetMode.isEmpty()) {
        getPresetResolution(ctx.presetMode, options.width, options.height);
    } else {
        options.width = (ctx.mode == GenerateMode::HD) ? config.hdWidth : config.previewWidth;
        options.height = (ctx.mode == GenerateMode::HD) ? config.hdHeight : config.previewHeight;
    }
    options.seed = -1;
    options.returnUrl = true;
    
    if (!ctx.refImageData.isEmpty()) {
        options.referenceImage = ctx.refImageData;
        LOG_INFO("ImageService", "Using img2img API with reference image");
    }
    
    VolcEngineImageClient::GenerateResult result = client->generate(options);
    
    if (!result.success) {
        setError(result.errorMessage);
        return false;
    }
    
    if (!result.imageUrl.isEmpty() && result.imageData.isEmpty()) {
        LOG_INFO("ImageService", QString("Downloading image from URL: %1").arg(result.imageUrl.left(100)));
        ctx.imageData = client->downloadImage(result.imageUrl);
        if (ctx.imageData.isEmpty()) {
            setError("Failed to download image from URL");
            return false;
        }
    } else {
        ctx.imageData = result.imageData;
    }
    
    return !ctx.imageData.isEmpty();
}

bool ImageService::executeWithQwen(GenerationContext& ctx)
{
    QwenImageClient::GenerateResult result;
    
    if (!ctx.refImageData.isEmpty()) {
        QwenImageClient::EditOptions options;
        options.prompt = ctx.prompt;
        options.sourceImage = ctx.refImageData;
        options.negativePrompt = ctx.negativePrompt;
        if (!ctx.presetMode.isEmpty()) {
            int width = 0;
            int height = 0;
            getPresetResolution(ctx.presetMode, width, height);
            options.size = QwenImageClient::ImageSize::Custom;
            options.width = width;
            options.height = height;
        } else {
            options.size = (ctx.mode == GenerateMode::HD)
                ? QwenImageClient::ImageSize::Square
                : QwenImageClient::ImageSize::Portrait;
        }
        result = QwenImageClient::instance()->edit(options);
    } else {
        QwenImageClient::GenerateOptions options;
        options.prompt = ctx.prompt;
        options.negativePrompt = ctx.negativePrompt;
        if (!ctx.presetMode.isEmpty()) {
            int width = 0;
            int height = 0;
            getPresetResolution(ctx.presetMode, width, height);
            options.size = QwenImageClient::ImageSize::Custom;
            options.width = width;
            options.height = height;
        } else {
            options.size = (ctx.mode == GenerateMode::HD)
                ? QwenImageClient::ImageSize::Square
                : QwenImageClient::ImageSize::Portrait;
        }
        options.style = "manga";
        result = QwenImageClient::instance()->generate(options);
    }

    if (!result.success) {
        setError(result.errorMessage);
        return false;
    }

    ctx.imageData = result.imageData;
    return true;
}

bool ImageService::executeImageGeneration(GenerationContext& ctx)
{
    ProviderConfig config = getProviderConfig();
    QString operationName = ctx.refImageData.isEmpty() ? "generateImage" : "generateWithReference";
    
    LOG_INFO("ImageService", QString("Using image provider: %1, hasRefImage: %2")
        .arg(config.name).arg(!ctx.refImageData.isEmpty()));
    
    bool success = m_apiRetryPolicy->executeWithRetry(
        [&]() -> bool {
            if (config.name == "volcengine") {
                return executeWithVolcEngine(ctx);
            } else {
                return executeWithQwen(ctx);
            }
        },
        [&](int attempt, const QString& error) {
            LOG_WARNING("ImageService", 
                QString("%1 retry %2/%3: %4")
                    .arg(operationName)
                    .arg(attempt)
                    .arg(m_apiRetryPolicy->config().maxAttempts)
                    .arg(error));
        },
        [&](const QList<RetryAttempt>& attempts) {
            if (m_apiRetryPolicy->isFinalFailure()) {
                QString errorType = ctx.refImageData.isEmpty()
                    ? tr("图像生成失败")
                    : tr("参考图生成失败");
                setError(TR_FMT("%1 failed after %2 attempts: %3",
                    errorType,
                    QString::number(attempts.size()),
                    m_apiRetryPolicy->lastError()));
                
                LOG_ERROR("ImageService", 
                    QString("%1 failed after %2 attempts: %3")
                        .arg(operationName)
                        .arg(attempts.size())
                        .arg(m_apiRetryPolicy->lastError()));
            }
        }
    );

    return success;
}

bool ImageService::storeImage(GenerationContext& ctx)
{
    emit progressChanged(tr("正在保存图像..."), 70);

    try {
        ctx.s3Key = !ctx.presetMode.isEmpty()
            ? QString("panels/%1/%2_%3.png").arg(ctx.panelId.left(8)).arg(ctx.panelId).arg(ctx.presetMode)
            : generateS3Key(ctx.panelId, ctx.mode);

        if (StorageClient::instance()->isInitialized()) {
            StorageClient::UploadResult result = StorageClient::instance()->upload(
                ctx.s3Key, ctx.imageData, "image/png");

            if (!result.success) {
                setError(TR_FMT("Cloud upload failed: %1", result.errorMessage));
                return false;
            }

            LOG_INFO("ImageService", QString("Image stored to cloud: %1").arg(ctx.s3Key));
        } else {
            QString modeStr = (ctx.mode == GenerateMode::Preview) ? "preview" : "hd";
            QString localPath = FileStorage::instance()->savePanelImage(ctx.panelId, ctx.imageData, modeStr);
            
            if (localPath.isEmpty()) {
                setError(TR_FMT("Local save failed: %1", FileStorage::instance()->lastError()));
                return false;
            }
            
            ctx.s3Key = localPath;
            LOG_INFO("ImageService", QString("Image stored locally: %1").arg(localPath));
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("ImageService", QString("storeImage exception: %1").arg(e.what()));
        setError(TR_FMT("Store image failed: %1", e.what()));
        return false;
    } catch (...) {
        LOG_ERROR("ImageService", "storeImage unknown exception");
        setError(tr("保存图像时发生异常"));
        return false;
    }
}

bool ImageService::safeFetchPanel(GenerationContext& ctx)
{
    emit progressChanged(tr("正在获取面板数据..."), 10);
    bool success = m_dbRetryPolicy->executeWithRetry(
        [&]() -> bool {
            Panel panel = StoryboardService::instance()->getPanel(ctx.panelId);
            if (!panel.id().isEmpty()) {
                ctx.panel = panel;
                return true;
            }
            return false;
        },
        [&](int attempt, const QString& error) {
            LOG_WARNING("ImageService", 
                QString("safeFetchPanel retry %1/%2: %3")
                    .arg(attempt)
                    .arg(m_dbRetryPolicy->config().maxAttempts)
                    .arg(error));
            
            DatabaseManager* db = DatabaseManager::instance();
            if (db) {
                db->reconnectIfNeeded();
            }
        },
        [&](const QList<RetryAttempt>& attempts) {
            if (m_dbRetryPolicy->isFinalFailure()) {
                setError(TR_FMT("Database fetch failed after %1 attempts: %2",
                    QString::number(attempts.size()),
                    m_dbRetryPolicy->lastError()));
                
                LOG_ERROR("ImageService", 
                    QString("safeFetchPanel final failure, attempts=%1, error=%2")
                        .arg(attempts.size())
                        .arg(m_dbRetryPolicy->lastError()));
            }
        }
    );
    
    if (!success && !m_dbRetryPolicy->isFinalFailure()) {
        setError(TR_FMT("Failed to fetch panel %1", ctx.panelId));
    }
    
    return success;
}

bool ImageService::buildPromptForPanel(GenerationContext& ctx)
{
    emit progressChanged(tr("正在构建提示词..."), 20);

    try {
        QJsonObject panelJson = buildPanelJson(ctx.panel);
        
        QString novelId = fetchNovelIdByStoryboardId(ctx.panel.storyboardId());
        QMap<QString, QJsonObject> characterRefs = fetchCharacterRefs(novelId, ctx.referenceImages);
        QMap<QString, QJsonObject> sceneRefs = fetchSceneRefs(novelId, ctx.referenceImages);
        
        LOG_INFO("ImageService", QString("buildPromptForPanel: panelId=%1, novelId=%2, characters=%3, scenes=%4")
            .arg(ctx.panelId).arg(novelId).arg(characterRefs.size()).arg(sceneRefs.size()));
        
        QJsonObject options;
        options["mode"] = !ctx.presetMode.isEmpty()
            ? ctx.presetMode
            : ((ctx.mode == GenerateMode::HD) ? "hd" : "preview");
        options["promptTarget"] = classifyPanelPromptTarget(panelJson);
        
        PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panelJson, characterRefs, sceneRefs, options);

        ctx.prompt = result.text;
        ctx.negativePrompt = result.negativePrompt;
        
        for (const QString& ref : result.referenceImages) {
            if (!ref.isEmpty() && !ctx.referenceImages.contains(ref)) {
                ctx.referenceImages.append(ref);
            }
        }
        
        if (ctx.prompt.isEmpty()) {
            setError(tr("提示词构建为空"));
            return false;
        }

        LOG_INFO("ImageService", QString("Prompt built: len=%1, refImages=%2, prompt=%3...")
            .arg(ctx.prompt.length()).arg(ctx.referenceImages.size()).arg(ctx.prompt.left(100)));

        return true;
    } catch (const std::exception& e) {
        setError(QString("Prompt build failed: %1").arg(e.what()));
        return false;

    } catch (...) {
        LOG_ERROR("ImageService", "buildPromptForPanel unknown exception");
        setError(tr("构建提示词时发生异常"));
        return false;
    }
}

QJsonObject ImageService::buildCharacterRef(const Character& ch, QStringList& outPortraitPaths)
{
    QJsonObject charJson;
    charJson["id"] = ch.id();
    charJson["name"] = ch.name();
    charJson["role"] = ch.role();
    
    QStringList portraitPaths;
    if (!ch.portraitPath().isEmpty()) {
        portraitPaths.append(ch.portraitPath());
    }
    charJson["portraitPath"] = ch.portraitPath();
    charJson["portraitPaths"] = QJsonArray::fromStringList(portraitPaths);
    
    CharacterAppearance app = ch.appearance();
    QJsonObject appearanceJson;
    appearanceJson["gender"] = app.gender;
    appearanceJson["age"] = QString::number(app.age);
    appearanceJson["build"] = app.build;
    appearanceJson["hairStyle"] = app.hairStyle;
    appearanceJson["hairColor"] = app.hairColor;
    appearanceJson["eyeColor"] = app.eyeColor;
    appearanceJson["clothing"] = QJsonArray::fromStringList(app.clothing);
    appearanceJson["accessories"] = QJsonArray::fromStringList(app.accessories);
    appearanceJson["distinctiveFeatures"] = QJsonArray::fromStringList(app.distinctiveFeatures);
    charJson["appearance"] = appearanceJson;
    
    for (const QString& path : portraitPaths) {
        if (!path.isEmpty() && !outPortraitPaths.contains(path)) {
            outPortraitPaths.append(path);
        }
    }
    
    return charJson;
}

QMap<QString, QJsonObject> ImageService::fetchCharacterRefs(const QString& novelId, QStringList& outReferenceImages)
{
    QMap<QString, QJsonObject> characterRefs;
    
    if (novelId.isEmpty()) {
        LOG_WARNING("ImageService", "fetchCharacterRefs: novelId is empty");
        return characterRefs;
    }
    
    QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(novelId);
    
    for (const Character& ch : characters) {
        QJsonObject charJson = buildCharacterRef(ch, outReferenceImages);
        characterRefs[ch.name()] = charJson;
    }
    
    QStringList charNames = characterRefs.keys();
    LOG_INFO("ImageService", QString("fetchCharacterRefs: %1 chars [%2], refImages=%3")
        .arg(characters.size()).arg(charNames.join(", ")).arg(outReferenceImages.size()));
    
    return characterRefs;
}

QJsonObject ImageService::buildSceneRef(const Scene& scene, QStringList& outReferenceImages)
{
    QJsonObject sceneJson;
    sceneJson["id"] = scene.id();
    sceneJson["name"] = scene.name();
    sceneJson["sceneId"] = scene.sceneId();
    
    SceneDetails details = scene.details();
    QJsonObject detailsJson;
    detailsJson["description"] = details.description;
    detailsJson["building"] = details.building;
    detailsJson["color"] = details.color;
    detailsJson["landmark"] = details.landmark;
    detailsJson["layout"] = details.layout;
    detailsJson["atmosphere"] = details.atmosphere;
    detailsJson["type"] = details.type;
    detailsJson["setting"] = details.setting;
    detailsJson["timeOfDay"] = details.timeOfDay;
    detailsJson["weather"] = details.weather;
    detailsJson["details"] = QJsonArray::fromStringList(details.details);
    sceneJson["details"] = detailsJson;
    
    QString referenceImagePath = scene.referenceImagePath();
    sceneJson["referenceImagePath"] = referenceImagePath;
    if (!referenceImagePath.isEmpty() && !outReferenceImages.contains(referenceImagePath)) {
        outReferenceImages.append(referenceImagePath);
    }
    
    return sceneJson;
}

QMap<QString, QJsonObject> ImageService::fetchSceneRefs(const QString& novelId, QStringList& outReferenceImages)
{
    QMap<QString, QJsonObject> sceneRefs;
    
    if (novelId.isEmpty()) {
        return sceneRefs;
    }
    
    QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(novelId);
    for (const Scene& scene : scenes) {
        QJsonObject sceneJson = buildSceneRef(scene, outReferenceImages);
        sceneRefs[scene.name()] = sceneJson;
        sceneRefs[scene.sceneId()] = sceneJson;
    }
    
    QStringList sceneNames = sceneRefs.keys();
    LOG_INFO("ImageService", QString("fetchSceneRefs: %1 scenes [%2], refImages=%3")
        .arg(scenes.size()).arg(sceneNames.join(", ")).arg(outReferenceImages.size()));
    
    return sceneRefs;
}

QString ImageService::fetchNovelIdByStoryboardId(const QString& storyboardId)
{
    if (storyboardId.isEmpty()) {
        return QString();
    }
    
    DatabaseManager *db = DatabaseManager::instance();
    if (!db) {
        return QString();
    }
    
    QString result;
    
    m_dbRetryPolicy->executeWithRetry(
        [&]() -> bool {
            if (!db->isConnected()) {
                if (!db->reconnectIfNeeded()) {
                    return false;
                }
            }
            
            QSqlQuery query(db->database());
            query.prepare("SELECT novel_id FROM storyboards WHERE id = ?");
            query.addBindValue(storyboardId);
            
            if (query.exec() && query.next()) {
                result = query.value(0).toString();
                return true;
            }
            
            return false;
        },
        [&](int attempt, const QString& error) {
            LOG_WARNING("ImageService", 
                QString("fetchNovelIdByStoryboardId retry %1/%2: %3")
                    .arg(attempt)
                    .arg(m_dbRetryPolicy->config().maxAttempts)
                    .arg(error));
            
            db->reconnectIfNeeded();
        },
        [&](const QList<RetryAttempt>& attempts) {
            if (m_dbRetryPolicy->isFinalFailure()) {
                LOG_ERROR("ImageService", 
                    QString("fetchNovelIdByStoryboardId failed after %1 attempts: %2")
                        .arg(attempts.size())
                        .arg(m_dbRetryPolicy->lastError()));
            }
        }
    );
    
    return result;
}

bool ImageService::safeUpdateDatabase(GenerationContext& ctx)
{
    emit progressChanged(tr("正在完成..."), 90);

    QJsonObject content = ctx.panel.rawContent();

    if (!ctx.presetMode.isEmpty()) {
        content["previewS3Key"] = ctx.s3Key;
        content["hdS3Key"] = ctx.s3Key;
    } else if (ctx.mode == GenerateMode::Preview) {
        content["previewS3Key"] = ctx.s3Key;
    } else {
        content["hdS3Key"] = ctx.s3Key;
    }
    content["status"] = "completed";
    content["generatedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    bool success = m_dbRetryPolicy->executeWithRetry(
        [&]() -> bool {
            return StoryboardService::instance()->updatePanel(ctx.panelId, content);
        },
        [&](int attempt, const QString& error) {
            LOG_WARNING("ImageService", 
                QString("safeUpdateDatabase retry %1/%2: %3")
                    .arg(attempt)
                    .arg(m_dbRetryPolicy->config().maxAttempts)
                    .arg(error));
            
            DatabaseManager* db = DatabaseManager::instance();
            if (db) {
                db->reconnectIfNeeded();
            }
        },
        [&](const QList<RetryAttempt>& attempts) {
            if (m_dbRetryPolicy->isFinalFailure()) {
                setError(TR_FMT("Database update failed (retry %1 times): %2", 
                    QString::number(attempts.size()),
                    m_dbRetryPolicy->lastError()));
                
                LOG_ERROR("ImageService", 
                    QString("safeUpdateDatabase final failure, attempts=%1, error=%2")
                        .arg(attempts.size())
                        .arg(m_dbRetryPolicy->lastError()));
            }
        }
    );
    
    if (!success && !m_dbRetryPolicy->isFinalFailure()) {
        setError(tr("数据库更新失败，正在重试..."));
    }
    
    return success;
}

ImageService::GenerateResult ImageService::finalizeResult(GenerationContext& ctx)
{
    emit progressChanged(tr("\u751f\u6210\u5b8c\u6210"), 100);
    
    GenerateResult result;
    result.success = true;
    result.panelId = ctx.panelId;
    result.s3Key = ctx.s3Key;
    
    try {
        if (StorageClient::instance()->isInitialized()) {
            result.imageUrl = StorageClient::instance()->getPresignedUrl(ctx.s3Key);
        } else {
            result.imageUrl = FileStorage::instance()->getFullPath(ctx.s3Key);
        }
    } catch (...) {
        result.imageUrl = FileStorage::instance()->getFullPath(ctx.s3Key);
    }
    
    if (!ctx.imageData.isEmpty()) {
        QImage image;
        if (image.loadFromData(ctx.imageData)) {
            result.width = image.width();
            result.height = image.height();
        }
    }
    
    result.timestamp = QDateTime::currentMSecsSinceEpoch();

    return result;
}

ImageService::GenerateResult ImageService::createErrorResult(const QString& panelId, const QString& message)
{
    GenerateResult result;
    result.success = false;
    result.panelId = panelId;
    result.errorMessage = message;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    return result;
}

QString ImageService::presetModeToString(BatchPresetMode presetMode) const
{
    switch (presetMode) {
        case BatchPresetMode::Square_1x1: return "1x1";
        case BatchPresetMode::Standard_3x2: return "3x2";
        case BatchPresetMode::Widescreen_16x9: return "16x9";
    }
    return "1x1";
}

ImageService::BatchPresetMode ImageService::presetModeFromString(const QString& presetMode) const
{
    const QByteArray mode = normalizePresetMode(presetMode);
    if (mode == "3x2") {
        return BatchPresetMode::Standard_3x2;
    }
    if (mode == "16x9") {
        return BatchPresetMode::Widescreen_16x9;
    }
    return BatchPresetMode::Square_1x1;
}

void ImageService::getPresetResolution(BatchPresetMode presetMode, int& width, int& height) const
{
    QString provider = AppConfig::instance()->imageService().provider;

    if (provider == "volcengine") {
        switch (presetMode) {
            case BatchPresetMode::Square_1x1:
                width = 1328;
                height = 1328;
                break;
            case BatchPresetMode::Standard_3x2:
                width = 1584;
                height = 1056;
                break;
            case BatchPresetMode::Widescreen_16x9:
                width = 1664;
                height = 936;
                break;
        }
        return;
    }

    switch (presetMode) {
        case BatchPresetMode::Square_1x1:
            width = 1024;
            height = 1024;
            break;
        case BatchPresetMode::Standard_3x2:
            width = 1536;
            height = 1024;
            break;
        case BatchPresetMode::Widescreen_16x9:
            width = 1280;
            height = 720;
            break;
    }
}

void ImageService::getPresetResolution(const QString& presetMode, int& width, int& height) const
{
    getPresetResolution(presetModeFromString(presetMode), width, height);
}

QString ImageService::enqueuePanelImageGeneration(const QString& panelId, GenerateMode mode)
{
    TaskData task;
    task.type = TaskType::GeneratePanelImage;
    task.panelId = panelId;
    task.total = 1;

    QJsonObject params;
    params["mode"] = (mode == GenerateMode::HD) ? "hd" : "preview";
    task.params = params;

    QString taskId = TaskQueue::instance()->enqueue(task);

    return taskId;
}

QString ImageService::enqueuePanelImageGeneration(const QString& panelId, BatchPresetMode presetMode)
{
    TaskData task;
    task.type = TaskType::GeneratePanelImage;
    task.panelId = panelId;
    task.total = 1;

    QJsonObject params;
    params["mode"] = presetModeToString(presetMode);
    task.params = params;

    QString taskId = TaskQueue::instance()->enqueue(task);

    return taskId;
}

QString ImageService::enqueueBatchPanelImageGeneration(const QStringList& panelIds, GenerateMode mode)
{
    if (panelIds.isEmpty()) {
        return QString();
    }
    
    TaskData task;
    task.type = TaskType::GeneratePanels;
    task.total = panelIds.size();
    task.completed = 0;

    QJsonObject params;
    params["mode"] = (mode == GenerateMode::HD) ? "hd" : "preview";
    QJsonArray panelIdArray;
    for (const QString& id : panelIds) {
        panelIdArray.append(id);
    }
    params["panelIds"] = panelIdArray;
    task.params = params;

    QString taskId = TaskQueue::instance()->enqueue(task);

    return taskId;
}

QString ImageService::enqueueBatchPanelImageGeneration(const QStringList& panelIds, BatchPresetMode presetMode)
{
    if (panelIds.isEmpty()) {
        return QString();
    }

    TaskData task;
    task.type = TaskType::GeneratePanels;
    task.total = panelIds.size();
    task.completed = 0;

    QJsonObject params;
    params["mode"] = presetModeToString(presetMode);
    QJsonArray panelIdArray;
    for (const QString& id : panelIds) {
        panelIdArray.append(id);
    }
    params["panelIds"] = panelIdArray;
    task.params = params;

    QString taskId = TaskQueue::instance()->enqueue(task);

    return taskId;
}

QStringList ImageService::enqueuePanelImageGenerations(const QStringList& panelIds, GenerateMode mode)
{
    QStringList taskIds;
    taskIds.reserve(panelIds.size());

    for (const QString& panelId : panelIds) {
        taskIds.append(enqueuePanelImageGeneration(panelId, mode));
    }

    return taskIds;
}

QStringList ImageService::enqueuePanelImageGenerations(const QStringList& panelIds, BatchPresetMode presetMode)
{
    QStringList taskIds;
    taskIds.reserve(panelIds.size());

    for (const QString& panelId : panelIds) {
        taskIds.append(enqueuePanelImageGeneration(panelId, presetMode));
    }

    return taskIds;
}

QJsonObject ImageService::handleGeneratePanelImageTask(const QJsonObject& taskJson)
{
    TaskData task = TaskData::fromJson(taskJson);

    GenerateMode mode = GenerateMode::Preview;
    if (task.params.contains("mode") && task.params["mode"].toString() == "hd") {
        mode = GenerateMode::HD;
    }
    QString modeStr = task.params.contains("mode") ? task.params["mode"].toString() : QString();
    GenerateResult result = (!modeStr.isEmpty() && modeStr != "preview" && modeStr != "hd")
        ? generatePanelImage(task.panelId, presetModeFromString(modeStr))
        : generatePanelImage(task.panelId, mode);

    QJsonObject resultJson;
    resultJson["success"] = result.success;
    resultJson["panelId"] = result.panelId;
    resultJson["imageUrl"] = result.imageUrl;
    resultJson["s3Key"] = result.s3Key;
    resultJson["errorMessage"] = result.errorMessage;
    resultJson["width"] = result.width;
    resultJson["height"] = result.height;
    resultJson["timestamp"] = result.timestamp;

    return resultJson;
}

QJsonObject ImageService::buildPanelJson(const Panel& panel)
{
    QJsonObject json = panel.rawContent();
    if (json.isEmpty()) {
        json["id"] = panel.id();
        json["page"] = panel.page();
        json["index"] = panel.index();
        json["scene"] = panel.scene();
        json["shotType"] = panel.shotType();
        json["cameraAngle"] = panel.cameraAngle();
        json["visualPrompt"] = panel.visualPrompt();
        json["visualPromptEn"] = panel.visualPromptEn();
        json["status"] = panel.status();
    } else {
        if (!json.contains("id")) {
            json["id"] = panel.id();
        }
        if (!json.contains("status")) {
            json["status"] = panel.status();
        }
    }
    
    QJsonArray charactersArray;
    for (const auto& c : panel.characters()) {
        QJsonObject charObj;
        charObj["charId"] = c.charId;
        charObj["name"] = c.name;
        charObj["pose"] = c.pose;
        charObj["expression"] = c.expression;
        charactersArray.append(charObj);
    }
    json["characters"] = charactersArray;
    
    QJsonArray dialogueArray;
    for (const auto& d : panel.dialogue()) {
        QJsonObject dialogueObj;
        dialogueObj["speaker"] = d.speaker;
        dialogueObj["text"] = d.text;
        dialogueObj["bubbleType"] = d.bubbleType;
        dialogueArray.append(dialogueObj);
    }
    json["dialogue"] = dialogueArray;
    
    return json;
}

QString ImageService::generateS3Key(const QString& panelId, GenerateMode mode)
{
    QString modeStr = (mode == GenerateMode::Preview) ? "preview" : "hd";
    return QString("panels/%1/%2_%3.png")
        .arg(panelId.left(8))
        .arg(panelId)
        .arg(modeStr);
}

ImageService::GenerateResult ImageService::generateWithRetryInternal(const QString& panelId, const ResolutionConfig& resolution, int maxRetries)
{
    GenerateResult result;
    int retryCount = 0;
    
    while (retryCount < maxRetries) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_batchCancelled) {
                return createErrorResult(panelId, tr("\u7528\u6237\u53d6\u6d88\u4e86\u64cd\u4f5c"));
            }
        }
        
        result = generatePanelImageInternal(panelId, resolution);
        
        if (result.success) {
            return result;
        }
        
        retryCount++;
        if (retryCount < maxRetries) {
            int delaySeconds = static_cast<int>(std::pow(2, retryCount)) * 5;
            LOG_WARNING("ImageService", QString("Panel %1 generation failed (attempt %2/%3): %4, retrying in %5s...")
                .arg(panelId).arg(retryCount).arg(maxRetries).arg(result.errorMessage).arg(delaySeconds));
            QThread::msleep(delaySeconds * 1000);
        }
    }
    
    return result;
}

ImageService::GenerateResult ImageService::generateWithRetry(const QString& panelId, GenerateMode mode, int maxRetries)
{
    return generateWithRetryInternal(panelId, ResolutionConfig::fromMode(mode), maxRetries);
}

ImageService::GenerateResult ImageService::generateWithRetry(const QString& panelId, const QString& presetMode, int maxRetries)
{
    return generateWithRetryInternal(panelId, ResolutionConfig::fromPreset(presetModeFromString(presetMode)), maxRetries);
}

void ImageService::processNextPanel()
{
    QString panelId;
    int currentIndex;
    int total;
    ResolutionConfig resolution;
    
    {
        QMutexLocker locker(&m_mutex);
        if (m_batchCancelled || m_currentProcessIndex >= m_pendingPanelIds.size()) {
            BatchResult finalResult = m_batchResult;
            finishBatch();
            
            LOG_INFO("ImageService", QString("Batch completed: %1 success, %2 failed")
                .arg(finalResult.successCount).arg(finalResult.failedCount));
            
            emit imageBatchCompleted(finalResult);
            return;
        }
        panelId = m_pendingPanelIds[m_currentProcessIndex];
        currentIndex = m_currentProcessIndex;
        total = m_pendingPanelIds.size();
        resolution = m_currentResolution;
    }

    emit batchProgressChanged(currentIndex + 1, total, tr("\u6b63\u5728\u5904\u7406..."));
    
    QtConcurrent::run(this, &ImageService::processPanelAsync, panelId, resolution, currentIndex, total);
}

void ImageService::processPanelAsync(const QString& panelId, const ResolutionConfig& resolution, int currentIndex, int total)
{
    Q_UNUSED(currentIndex)
    Q_UNUSED(total)
    
    const int MAX_RETRIES = 3;
    GenerateResult result = generateWithRetryInternal(panelId, resolution, MAX_RETRIES);
    
    bool cancelled = false;
    {
        QMutexLocker locker(&m_mutex);
        cancelled = m_batchCancelled;
        if (!cancelled) {
            if (result.success) {
                m_batchResult.successCount++;
            } else {
                m_batchResult.failedCount++;
                LOG_ERROR("ImageService", QString("Panel generation failed after %1 attempts: %2 - %3")
                    .arg(MAX_RETRIES).arg(panelId).arg(result.errorMessage));
            }
            m_batchResult.results.append(result);
        }
        m_currentProcessIndex++;
    }
    
    if (!cancelled && result.success) {
        emit panelGenerated(result);
    }
    
    QTimer::singleShot(100, this, &ImageService::processNextPanel);
}
