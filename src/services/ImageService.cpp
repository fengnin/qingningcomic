#include "ImageService.h"
#include "ServiceContainer.h"
#include "DatabaseManager.h"
#include "utils/SingletonUtils.h"
#include "PromptBuilder.h"
#include "QwenImageClient.h"
#include "VolcEngineImageClient.h"
#include "StorageClient.h"
#include "FileStorage.h"
#include "StoryboardService.h"
#include "CharacterExtractor.h"
#include "SceneExtractor.h"
#include "TaskQueue.h"
#include "Task.h"
#include "Logger.h"
#include "EncodingUtils.h"
#include "AppConfig.h"
#include <QJsonDocument>
#include <QDateTime>
#include <QFileInfo>
#include <QSqlQuery>
#include <QtConcurrent>

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
    
    if (m_batchFuture.isRunning()) {
        m_batchFuture.cancel();
    }
    
    m_generating = false;
    m_batchResult = BatchResult();
    
    emit batchGenerationCancelled();
}

ImageService::GenerateResult ImageService::generatePanelImage(const QString& panelId, GenerateMode mode)
{
    if (panelId.isEmpty()) {
        return createErrorResult(panelId, TR("面板ID为空"));
    }
    
    if (!shouldContinue()) {
        LOG_WARNING("ImageService", QString("Generation cancelled for panel: %1").arg(panelId));
        return createErrorResult(panelId, TR("生成已取消"));
    }
    
    GenerationContext ctx;
    ctx.panelId = panelId;
    ctx.mode = mode;

    try {
        if (!safeFetchPanel(ctx)) {
            return createErrorResult(panelId, lastError().isEmpty() ? TR("获取面板数据失败") : lastError());
        }

        if (!buildPromptForPanel(ctx)) {
            return createErrorResult(panelId, lastError().isEmpty() ? TR("构建提示词失败") : lastError());
        }

        if (!generateImage(ctx)) {
            return createErrorResult(panelId, lastError().isEmpty() ? TR("图像生成失败") : lastError());
        }

        if (!storeImage(ctx)) {
            return createErrorResult(panelId, lastError().isEmpty() ? TR("图像存储失败") : lastError());
        }

        if (!safeUpdateDatabase(ctx)) {
            LOG_ERROR("ImageService", QString("Failed to update database for panel: %1").arg(panelId));
            return createErrorResult(panelId, lastError().isEmpty() ? TR("数据库更新失败") : lastError());
        }

        GenerateResult result = finalizeResult(ctx);
        LOG_INFO("ImageService", QString("Panel %1 image generated successfully").arg(panelId));

        return result;
    } catch (const std::exception& e) {
        QString errorMsg = QString::fromUtf8(e.what());
        LOG_ERROR("ImageService", QString("Exception during generation of panel %1: %2").arg(panelId).arg(errorMsg));
        return createErrorResult(panelId, errorMsg);
    } catch (...) {
        LOG_ERROR("ImageService", QString("Unknown exception during generation of panel %1").arg(panelId));
        return createErrorResult(panelId, TR("生成过程中发生未知错误"));
    }
}

void ImageService::generatePanelImages(const QStringList& panelIds, GenerateMode mode)
{
    if (isGenerating()) {
        emit errorOccurred("generatePanelImages", TR("已有生成任务在进行中"));
        return;
    }

    if (panelIds.isEmpty()) {
        emit errorOccurred("generatePanelImages", TR("面板列表为空"));
        return;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_pendingPanelIds = panelIds;
        m_currentMode = mode;
        m_currentProcessIndex = 0;
        m_batchCancelled = false;

        m_batchResult = BatchResult();
        m_batchResult.totalCount = panelIds.size();
    }

    startBatch(panelIds.size());
    LOG_INFO("ImageService", QString("Starting batch generation for %1 panels").arg(panelIds.size()));
    
    m_batchFuture = QtConcurrent::run(this, &ImageService::runBatchGeneration);
}

void ImageService::runBatchGeneration()
{
    const int MAX_RETRIES = 3;  // 最大重试次数（与原仓库一致）
    
    while (true) {
        QString panelId;
        int currentIndex;
        int total;
        GenerateMode mode;
        
        {
            QMutexLocker locker(&m_mutex);
            if (m_batchCancelled || m_currentProcessIndex >= m_pendingPanelIds.size()) {
                break;
            }
            panelId = m_pendingPanelIds[m_currentProcessIndex];
            currentIndex = m_currentProcessIndex;
            total = m_pendingPanelIds.size();
            mode = m_currentMode;
        }
        
        QMetaObject::invokeMethod(this, "batchProgressChanged", Qt::QueuedConnection,
            Q_ARG(int, currentIndex + 1), Q_ARG(int, total),
            Q_ARG(QString, TR("正在生成面板 %1/%2").arg(currentIndex + 1).arg(total)));

        // 带指数退避重试的生成（与原仓库一致）
        GenerateResult result;
        int retryCount = 0;
        bool success = false;
        
        while (!success && retryCount < MAX_RETRIES) {
            result = generatePanelImage(panelId, mode);
            success = result.success;
            
            if (!success) {
                retryCount++;
                if (retryCount < MAX_RETRIES) {
                    // 指数退避: 10s, 20s,// 指数退避重试的生成（与原仓库一致）
                    int delaySeconds = static_cast<int>(std::pow(2, retryCount)) * 10;
                    LOG_WARNING("ImageService", QString("Panel %1 generation failed (attempt %2/%3): %4, retrying in %5s...")
                        .arg(panelId).arg(retryCount).arg(MAX_RETRIES).arg(result.errorMessage).arg(delaySeconds));
                    QThread::msleep(delaySeconds * 1000);
                }
            }
        }
        
        {
            QMutexLocker locker(&m_mutex);
            if (success) {
                m_batchResult.successCount++;
            } else {
                m_batchResult.failedCount++;
                LOG_ERROR("ImageService", QString("Panel generation failed after %1 attempts: %2 - %3")
                    .arg(MAX_RETRIES).arg(panelId).arg(result.errorMessage));
            }
            m_batchResult.results.append(result);
            m_currentProcessIndex++;
        }
        
        // 请求间隔，避免 API 限流（火山引擎免费版并发限制为1，需要串行请求）
        // 每个请求完成后等待2秒再发送下一个
        QThread::msleep(2000);
    }

    BatchResult finalResult;
    {
        QMutexLocker locker(&m_mutex);
        finalResult = m_batchResult;
        finishBatch();
    }
    
    LOG_INFO("ImageService", QString("Batch completed: %1 success, %2 failed")
        .arg(finalResult.successCount).arg(finalResult.failedCount));
    
    QMetaObject::invokeMethod(this, "imageBatchCompleted", Qt::QueuedConnection,
        Q_ARG(ImageService::BatchResult, finalResult));
}

void ImageService::generateStoryboardImages(const QString& storyboardId, GenerateMode mode)
{
    QList<Panel> panels;
    
    try {
        panels = StoryboardService::instance()->getPanels(storyboardId);
    } catch (const std::exception& e) {
        LOG_ERROR("ImageService", QString("getPanels exception: %1").arg(e.what()));
        emit errorOccurred("generateStoryboardImages", TR("获取面板列表失败"));
        return;
    } catch (...) {
        LOG_ERROR("ImageService", "getPanels unknown exception");
        emit errorOccurred("generateStoryboardImages", TR("获取面板列表失败"));
        return;
    }

    if (panels.isEmpty()) {
        emit errorOccurred("generateStoryboardImages", TR_FMT("故事板没有面板: %1", storyboardId));
        return;
    }

    QStringList panelIds;
    for (const Panel& panel : panels) {
        panelIds.append(panel.id());
    }

    generatePanelImages(panelIds, mode);
}

ImageService::GenerateResult ImageService::regeneratePanelImage(const QString& panelId, GenerateMode mode)
{
    LOG_INFO("ImageService", QString("Regenerating panel: %1").arg(panelId));
    return generatePanelImage(panelId, mode);
}

bool ImageService::safeFetchPanel(GenerationContext& ctx)
{
    emit progressChanged(TR("获取面板数据"), 10);
    
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
                setError(TR_FMT("获取面板数据失败 (重试%1次): %2", 
                    QString::number(attempts.size()),
                    m_dbRetryPolicy->lastError()));
                
                LOG_ERROR("ImageService", 
                    QString("safeFetchPanel 最终失败，共尝试 %1 次: %2")
                        .arg(attempts.size())
                        .arg(m_dbRetryPolicy->lastError()));
            }
        }
    );
    
    if (!success && !m_dbRetryPolicy->isFinalFailure()) {
        setError(TR_FMT("面板不存在: %1", ctx.panelId));
    }
    
    return success;
}

bool ImageService::buildPromptForPanel(GenerationContext& ctx)
{
    emit progressChanged(TR("构建提示词"), 20);

    try {
        QJsonObject panelJson = buildPanelJson(ctx.panel);
        
        QString novelId = fetchNovelIdByStoryboardId(ctx.panel.storyboardId());
        QMap<QString, QJsonObject> characterRefs = fetchCharacterRefs(novelId, ctx.referenceImages);
        QMap<QString, QJsonObject> sceneRefs = fetchSceneRefs(novelId, ctx.referenceImages);
        
        QJsonObject options;
        options["mode"] = (ctx.mode == GenerateMode::HD) ? "hd" : "preview";
        
        PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panelJson, characterRefs, sceneRefs, options);

        ctx.prompt = result.text;
        ctx.negativePrompt = result.negativePrompt;
        
        for (const QString& ref : result.referenceImages) {
            if (!ref.isEmpty() && !ctx.referenceImages.contains(ref)) {
                ctx.referenceImages.append(ref);
            }
        }
        
        if (!ctx.panel.visualPrompt().isEmpty()) {
            ctx.prompt += ", " + ctx.panel.visualPrompt();
        }

        if (ctx.prompt.isEmpty()) {
            setError(TR("无法构建提示词"));
            return false;
        }
        
        // 根据提供商限制参考图数量
        QString provider = AppConfig::instance()->imageService().provider;
        int maxRefImages = (provider == "volcengine") ? 1 : 3;  // 火山引擎只支持单张参考图
        if (ctx.referenceImages.size() > maxRefImages) {
            LOG_INFO("ImageService", QString("Limiting reference images from %1 to %2 for provider: %3")
                .arg(ctx.referenceImages.size()).arg(maxRefImages).arg(provider));
            ctx.referenceImages = ctx.referenceImages.mid(0, maxRefImages);
        }

        LOG_INFO("ImageService", QString("Generated prompt for panel %1: %2")
            .arg(ctx.panelId).arg(ctx.prompt.left(200)));
        
        if (!ctx.referenceImages.isEmpty()) {
            LOG_INFO("ImageService", QString("Using %1 reference images for panel %2")
                .arg(ctx.referenceImages.size()).arg(ctx.panelId));
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("ImageService", QString("buildPromptForPanel exception: %1").arg(e.what()));
        setError(TR("构建提示词失败: %1").arg(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR("ImageService", "buildPromptForPanel unknown exception");
        setError(TR("构建提示词时发生未知错误"));
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
    appearanceJson["accessories"] = QJsonArray::fromStringList(QStringList());
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
        return characterRefs;
    }
    
    QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(novelId);
    for (const Character& ch : characters) {
        QJsonObject charJson = buildCharacterRef(ch, outReferenceImages);
        characterRefs[ch.name()] = charJson;
    }
    
    return characterRefs;
}

QJsonObject ImageService::buildSceneRef(const Scene& scene, QStringList& outReferenceImages)
{
    QJsonObject sceneJson;
    sceneJson["id"] = scene.id();
    sceneJson["name"] = scene.name();
    sceneJson["sceneId"] = scene.sceneId();  // 同时存储 sceneId 作为 key
    
    // 场景详情
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
    
    // 场景参考图
    QString refPath = scene.referenceImagePath();
    if (!refPath.isEmpty()) {
        sceneJson["referenceImagePath"] = refPath;
        if (!outReferenceImages.contains(refPath)) {
            outReferenceImages.append(refPath);
        }
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
        sceneRefs[scene.name()] = sceneJson;  // 使用 name 作为 key
        sceneRefs[scene.sceneId()] = sceneJson;  // 同时使用 sceneId 作为 key
    }
    
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
                    QString("fetchNovelIdByStoryboardId 最终失败，共尝试 %1 次: %2")
                        .arg(attempts.size())
                        .arg(m_dbRetryPolicy->lastError()));
            }
        }
    );
    
    return result;
}

bool ImageService::generateImage(GenerationContext& ctx)
{
    emit progressChanged(TR("调用图像生成 API"), 30);

    QByteArray refImageData;
    if (!ctx.referenceImages.isEmpty()) {
        // 多角色参考图处理：使用第一张作为主参考图
        QString refPath = ctx.referenceImages.first();
        // 将相对路径转换为完整路径
        QString fullPath = FileStorage::instance()->getFullPath(refPath);
        QFile refFile(fullPath);
        
        if (refFile.exists() && refFile.open(QIODevice::ReadOnly)) {
            refImageData = refFile.readAll();
            refFile.close();
            LOG_INFO("ImageService", QString("Loaded reference image: %1 bytes").arg(refImageData.size()));
            
            // 多张参考图时的处理策略
            if (ctx.referenceImages.size() > 1) {
                LOG_INFO("ImageService", QString("Multiple reference images detected (%1), using first as primary")
                    .arg(ctx.referenceImages.size()));
                // 后续可扩展：将其他参考图特征融入 prompt
            }
        } else {
            LOG_WARNING("ImageService", QString("Reference image not found: %1 (full path: %2), falling back to text2image")
                .arg(refPath, fullPath));
            ctx.referenceImages.clear();
        }
    }

    return executeImageGeneration(ctx, refImageData);
}

bool ImageService::executeImageGeneration(GenerationContext& ctx, const QByteArray& refImageData)
{
    bool useRefImage = !refImageData.isEmpty();
    QString operationName = useRefImage ? "generateWithReference" : "generateImage";
    
    // 根据配置选择图像服务提供商
    QString provider = AppConfig::instance()->imageService().provider;
    bool useVolcEngine = (provider == "volcengine");
    
    LOG_INFO("ImageService", QString("Using image provider: %1, hasRefImage: %2").arg(provider).arg(useRefImage));
    
    bool success = m_apiRetryPolicy->executeWithRetry(
        [&]() -> bool {
            if (useVolcEngine) {
                // 使用火山引擎 Seedream 3.0
                VolcEngineImageClient* volcClient = ServiceContainer::instance()->volcEngineImageClient();
                if (!volcClient || !volcClient->isInitialized()) {
                    setError(TR("火山引擎图像客户端未初始化"));
                    return false;
                }
                
                VolcEngineImageClient::GenerateOptions options;
                options.prompt = ctx.prompt;
                options.width = (ctx.mode == GenerateMode::HD) ? 1328 : 1056;
                options.height = (ctx.mode == GenerateMode::HD) ? 1328 : 1584;
                options.seed = -1;
                options.returnUrl = true;
                
                // 如果有参考图，使用图生图 API
                if (useRefImage) {
                    options.referenceImage = refImageData;
                    LOG_INFO("ImageService", "Using img2img API with reference image");
                }
                
                VolcEngineImageClient::GenerateResult imageResult = volcClient->generate(options);
                
                if (!imageResult.success) {
                    setError(imageResult.errorMessage);
                    return false;
                }
                
                // 如果返回的是URL，需要下载图片
                if (!imageResult.imageUrl.isEmpty() && imageResult.imageData.isEmpty()) {
                    LOG_INFO("ImageService", QString("Downloading image from URL: %1").arg(imageResult.imageUrl.left(100)));
                    ctx.imageData = volcClient->downloadImage(imageResult.imageUrl);
                    if (ctx.imageData.isEmpty()) {
                        setError("Failed to download image from URL");
                        return false;
                    }
                } else {
                    ctx.imageData = imageResult.imageData;
                }
                
                return !ctx.imageData.isEmpty();
            } else {
                // 使用阿里云通义万相
                QwenImageClient::GenerateResult imageResult;
                
                if (useRefImage) {
                    QwenImageClient::EditOptions options;
                    options.prompt = ctx.prompt;
                    options.sourceImage = refImageData;
                    options.negativePrompt = ctx.negativePrompt;
                    options.size = (ctx.mode == GenerateMode::HD)
                        ? QwenImageClient::ImageSize::Square
                        : QwenImageClient::ImageSize::Portrait;
                    imageResult = QwenImageClient::instance()->edit(options);
                } else {
                    QwenImageClient::GenerateOptions options;
                    options.prompt = ctx.prompt;
                    options.negativePrompt = ctx.negativePrompt;
                    options.size = (ctx.mode == GenerateMode::HD)
                        ? QwenImageClient::ImageSize::Square
                        : QwenImageClient::ImageSize::Portrait;
                    options.style = "manga";
                    imageResult = QwenImageClient::instance()->generate(options);
                }

                if (!imageResult.success) {
                    setError(imageResult.errorMessage);
                    return false;
                }

                ctx.imageData = imageResult.imageData;
                return true;
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
                if (useRefImage) {
                    setError(TR_FMT("参考图生成失败 (重试%1次): %2", 
                        QString::number(attempts.size()),
                        m_apiRetryPolicy->lastError()));
                } else {
                    setError(TR_FMT("图像生成失败 (重试%1次): %2", 
                        QString::number(attempts.size()),
                        m_apiRetryPolicy->lastError()));
                }
                
                LOG_ERROR("ImageService", 
                    QString("%1 最终失败，共尝试 %2 次: %3")
                        .arg(operationName)
                        .arg(attempts.size())
                        .arg(m_apiRetryPolicy->lastError()));
            }
        }
    );

    return success;
}

bool ImageService::generateWithReference(GenerationContext& ctx)
{
    LOG_INFO("ImageService", QString("Generating with %1 reference images").arg(ctx.referenceImages.size()));
    
    if (ctx.referenceImages.isEmpty()) {
        return generateImage(ctx);
    }
    
    // 多角色参考图处理策略：
    // 1. 如果只有一张参考图，直接使用
    // 2. 如果有多张参考图，选择第一张作为主参考图，并在 prompt 中补充其他角色描述
    QString refPath = ctx.referenceImages.first();
    // 将相对路径转换为完整路径
    QString fullPath = FileStorage::instance()->getFullPath(refPath);
    QFile refFile(fullPath);
    
    if (!refFile.exists()) {
        LOG_WARNING("ImageService", QString("Reference image not found: %1 (full path: %2), falling back to text2image")
            .arg(refPath, fullPath));
        ctx.referenceImages.clear();
        return generateImage(ctx);
    }
    
    if (!refFile.open(QIODevice::ReadOnly)) {
        LOG_WARNING("ImageService", QString("Cannot open reference image: %1 (full path: %2), falling back to text2image")
            .arg(refPath, fullPath));
        ctx.referenceImages.clear();
        return generateImage(ctx);
    }
    
    QByteArray refImageData = refFile.readAll();
    refFile.close();
    
    // 如果有多张参考图，在 prompt 中添加其他角色的参考信息
    if (ctx.referenceImages.size() > 1) {
        LOG_INFO("ImageService", QString("Multiple reference images detected (%1), using first as primary, others as prompt context")
            .arg(ctx.referenceImages.size()));
        
        // 这里可以扩展：将其他参考图的特征描述添加到 prompt 中
        // 目前先记录日志，后续可以接入图像理解 API 提取特征
    }
    
    return executeImageGeneration(ctx, refImageData);
}

bool ImageService::storeImage(GenerationContext& ctx)
{
    emit progressChanged(TR("存储图像"), 70);

    try {
        ctx.s3Key = generateS3Key(ctx.panelId, ctx.mode);

        if (StorageClient::instance()->isInitialized()) {
            StorageClient::UploadResult result = StorageClient::instance()->upload(
                ctx.s3Key, ctx.imageData, "image/png");

            if (!result.success) {
                setError(TR_FMT("图像存储失败: %1", result.errorMessage));
                return false;
            }

            LOG_INFO("ImageService", QString("Image stored to cloud: %1").arg(ctx.s3Key));
        } else {
            QString modeStr = (ctx.mode == GenerateMode::Preview) ? "preview" : "hd";
            QString localPath = FileStorage::instance()->savePanelImage(ctx.panelId, ctx.imageData, modeStr);
            
            if (localPath.isEmpty()) {
                setError(TR_FMT("本地存储失败: %1", FileStorage::instance()->lastError()));
                return false;
            }
            
            ctx.s3Key = localPath;
            LOG_INFO("ImageService", QString("Image stored locally: %1").arg(localPath));
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("ImageService", QString("storeImage exception: %1").arg(e.what()));
        setError(TR("图像存储失败: %1").arg(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR("ImageService", "storeImage unknown exception");
        setError(TR("图像存储时发生未知错误"));
        return false;
    }
}

bool ImageService::safeUpdateDatabase(GenerationContext& ctx)
{
    emit progressChanged(TR("更新数据库"), 90);

    QJsonObject content = ctx.panel.rawContent();

    if (ctx.mode == GenerateMode::Preview) {
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
                setError(TR_FMT("数据库更新失败 (重试%1次): %2", 
                    QString::number(attempts.size()),
                    m_dbRetryPolicy->lastError()));
                
                LOG_ERROR("ImageService", 
                    QString("safeUpdateDatabase 最终失败，共尝试 %1 次: %2")
                        .arg(attempts.size())
                        .arg(m_dbRetryPolicy->lastError()));
            }
        }
    );
    
    if (!success && !m_dbRetryPolicy->isFinalFailure()) {
        setError(TR("数据库更新失败"));
    }
    
    return success;
}

ImageService::GenerateResult ImageService::finalizeResult(GenerationContext& ctx)
{
    emit progressChanged(TR("完成"), 100);

    GenerateResult result;
    result.success = true;
    result.panelId = ctx.panelId;
    result.s3Key = ctx.s3Key;
    
    try {
        if (StorageClient::instance()->isInitialized()) {
            result.imageUrl = StorageClient::instance()->getPresignedUrl(ctx.s3Key);
        } else {
            result.imageUrl = ctx.s3Key;
        }
    } catch (...) {
        result.imageUrl = ctx.s3Key;
    }
    
    result.timestamp = QDateTime::currentMSecsSinceEpoch();

    return result;
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

    LOG_INFO("ImageService", QString("Enqueued panel image generation task: %1 for panel: %2")
        .arg(taskId, panelId));

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

    LOG_INFO("ImageService", QString("Enqueued batch panel image generation task: %1 for %2 panels")
        .arg(taskId).arg(panelIds.size()));

    return taskId;
}

QStringList ImageService::enqueuePanelImageGenerations(const QStringList& panelIds, GenerateMode mode)
{
    QStringList taskIds;
    taskIds.reserve(panelIds.size());

    for (const QString& panelId : panelIds) {
        taskIds.append(enqueuePanelImageGeneration(panelId, mode));
    }

    LOG_INFO("ImageService", QString("Enqueued %1 panel image generation tasks").arg(panelIds.size()));
    return taskIds;
}

QJsonObject ImageService::handleGeneratePanelImageTask(const QJsonObject& taskJson)
{
    TaskData task = TaskData::fromJson(taskJson);

    GenerateMode mode = GenerateMode::Preview;
    if (task.params.contains("mode") && task.params["mode"].toString() == "hd") {
        mode = GenerateMode::HD;
    }

    GenerateResult result = generatePanelImage(task.panelId, mode);

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
    // 优先使用 rawContent 获取完整面板数据（包含 background、composition、artStyle、atmosphere 等）
    QJsonObject json = panel.rawContent();
    if (json.isEmpty()) {
        // 如果 rawContent 为空，则手动构建基本字段
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
        // 确保 id 和 status 字段存在
        if (!json.contains("id")) {
            json["id"] = panel.id();
        }
        if (!json.contains("status")) {
            json["status"] = panel.status();
        }
    }
    
    // 始终更新角色和对白数据（确保最新）
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

ImageService::GenerateResult ImageService::createErrorResult(const QString& panelId, const QString& message)
{
    GenerateResult result;
    result.success = false;
    result.panelId = panelId;
    result.errorMessage = message;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    return result;
}
