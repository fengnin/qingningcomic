#include "ImageService.h"
#include "ServiceContainer.h"
#include "DatabaseManager.h"
#include "utils/SingletonUtils.h"
#include "PromptBuilder.h"
#include "QwenImageClient.h"
#include "StorageClient.h"
#include "FileStorage.h"
#include "StoryboardService.h"
#include "CharacterExtractor.h"
#include "TaskQueue.h"
#include "Task.h"
#include "Logger.h"
#include "EncodingUtils.h"
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
    
    QtConcurrent::run(this, &ImageService::runBatchGeneration);
}

void ImageService::runBatchGeneration()
{
    while (true) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_batchCancelled || m_currentProcessIndex >= m_pendingPanelIds.size()) {
                break;
            }
        }

        QString panelId;
        GenerateMode mode;
        int currentIndex;
        int total;
        
        {
            QMutexLocker locker(&m_mutex);
            panelId = m_pendingPanelIds[m_currentProcessIndex];
            mode = m_currentMode;
            currentIndex = m_currentProcessIndex;
            total = m_pendingPanelIds.size();
        }

        QMetaObject::invokeMethod(this, "batchProgressChanged", Qt::QueuedConnection,
            Q_ARG(int, currentIndex + 1), Q_ARG(int, total),
            Q_ARG(QString, TR("正在生成面板 %1/%2").arg(currentIndex + 1).arg(total)));

        GenerateResult result = generatePanelImage(panelId, mode);

        {
            QMutexLocker locker(&m_mutex);
            if (result.success) {
                m_batchResult.successCount++;
            } else {
                m_batchResult.failedCount++;
                LOG_WARNING("ImageService", QString("Panel generation failed: %1 - %2").arg(panelId).arg(result.errorMessage));
            }
            m_batchResult.results.append(result);
            m_currentProcessIndex++;
        }
        
        QThread::msleep(50);
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
        
        QMap<QString, QJsonObject> characterRefs;
        
        QString novelId = fetchNovelIdByStoryboardId(ctx.panel.storyboardId());
        if (!novelId.isEmpty()) {
            QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(novelId);
            for (const Character& ch : characters) {
                QJsonObject charJson;
                charJson["id"] = ch.id();
                charJson["name"] = ch.name();
                charJson["role"] = ch.role();
                
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
                
                characterRefs[ch.name()] = charJson;
            }
        }
        
        QJsonObject options;
        options["mode"] = (ctx.mode == GenerateMode::HD) ? "hd" : "preview";
        
        PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panelJson, characterRefs, options);

        ctx.prompt = result.text;
        if (!ctx.panel.visualPrompt().isEmpty()) {
            ctx.prompt += ", " + ctx.panel.visualPrompt();
        }

        if (ctx.prompt.isEmpty()) {
            setError(TR("无法构建提示词"));
            return false;
        }

        LOG_INFO("ImageService", QString("Generated prompt for panel %1: %2")
            .arg(ctx.panelId).arg(ctx.prompt.left(200)));

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

    bool success = m_apiRetryPolicy->executeWithRetry(
        [&]() -> bool {
            QwenImageClient::GenerateOptions options;
            options.prompt = ctx.prompt;
            options.size = (ctx.mode == GenerateMode::HD)
                ? QwenImageClient::ImageSize::Square
                : QwenImageClient::ImageSize::Portrait;
            options.style = "manga";

            QwenImageClient::GenerateResult imageResult = QwenImageClient::instance()->generate(options);

            if (!imageResult.success) {
                setError(imageResult.errorMessage);
                return false;
            }

            ctx.imageData = imageResult.imageData;
            return true;
        },
        [&](int attempt, const QString& error) {
            LOG_WARNING("ImageService", 
                QString("generateImage retry %1/%2: %3")
                    .arg(attempt)
                    .arg(m_apiRetryPolicy->config().maxAttempts)
                    .arg(error));
        },
        [&](const QList<RetryAttempt>& attempts) {
            if (m_apiRetryPolicy->isFinalFailure()) {
                setError(TR_FMT("图像生成失败 (重试%1次): %2", 
                    QString::number(attempts.size()),
                    m_apiRetryPolicy->lastError()));
                
                LOG_ERROR("ImageService", 
                    QString("generateImage 最终失败，共尝试 %1 次: %2")
                        .arg(attempts.size())
                        .arg(m_apiRetryPolicy->lastError()));
            }
        }
    );
    
    return success;
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
    QJsonObject json;
    json["id"] = panel.id();
    json["page"] = panel.page();
    json["index"] = panel.index();
    json["scene"] = panel.scene();
    json["shotType"] = panel.shotType();
    json["cameraAngle"] = panel.cameraAngle();
    json["visualPrompt"] = panel.visualPrompt();
    json["visualPromptEn"] = panel.visualPromptEn();
    json["status"] = panel.status();
    
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
