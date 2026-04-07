#include "AnalysisService.h"
#include "CharacterExtractor.h"
#include "SceneExtractor.h"
#include "BibleGenerator.h"
#include "BibleImageService.h"
#include "ImageService.h"
#include "NovelService.h"
#include "Logger.h"
#include "EncodingUtils.h"
#include "DatabaseManager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QUuid>

AnalysisService* AnalysisService::m_instance = nullptr;

AnalysisService::AnalysisService(QObject* parent)
    : QObject(parent)
    , m_qwen(QwenClient::instance())
    , m_storyboardService(StoryboardService::instance())
{
    qRegisterMetaType<AnalysisResult>("AnalysisResult");
    
    connect(m_qwen, &QwenClient::progressChanged, this, &AnalysisService::onQwenProgress);
    connect(m_qwen, &QwenClient::errorOccurred, this, &AnalysisService::onQwenError);
    connect(m_qwen, &QwenClient::streamProgress, this, &AnalysisService::onQwenStreamProgress);
    connect(m_qwen, &QwenClient::streamCompleted, this, &AnalysisService::onQwenStreamCompleted);
    
    connect(TaskQueue::instance(), &TaskQueue::taskStarted, this, &AnalysisService::onTaskStarted, Qt::QueuedConnection);
    connect(TaskQueue::instance(), &TaskQueue::taskProgress, this, &AnalysisService::onTaskProgress, Qt::QueuedConnection);
    connect(TaskQueue::instance(), &TaskQueue::taskCompleted, this, &AnalysisService::onTaskCompleted, Qt::QueuedConnection);
    connect(TaskQueue::instance(), &TaskQueue::taskFailed, this, &AnalysisService::onTaskFailed, Qt::QueuedConnection);
    
    TaskQueue::instance()->start();
}

AnalysisService::~AnalysisService()
{
}

AnalysisService* AnalysisService::instance()
{
    if (!m_instance) {
        m_instance = new AnalysisService();
    }
    return m_instance;
}

AnalysisResult AnalysisService::getLastResult() const
{
    return m_lastResult;
}

bool AnalysisService::isProcessing() const
{
    return m_processing;
}

QString AnalysisService::currentJobId() const
{
    return m_currentJobId;
}

QJsonObject AnalysisService::loadJsonSchema()
{
    QString appDir = QCoreApplication::applicationDirPath();
    
    QStringList schemaPaths = {
        appDir + "/schemas/storyboard.json",
        appDir + "/../schemas/storyboard.json"
    };
    
    for (const QString& path : schemaPaths) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            
            if (!doc.isNull() && doc.isObject()) {
                LOG_INFO("AnalysisService", QString("Loaded schema from %1").arg(path));
                return doc.object();
            }
        }
    }
    
    LOG_ERROR("AnalysisService", QString("Failed to load JSON Schema. AppDir: %1").arg(appDir));
    return QJsonObject();
}

bool AnalysisService::isTaskMatch(const QString& taskId, const TaskData& task) const
{
    return (taskId == m_currentTaskId) || 
           (!task.novelId.isEmpty() && task.type == TaskType::GenerateStoryboard);
}

void AnalysisService::analyzeNovel(const QString& novelId, const QString& text, int chapterNumber)
{
    analyzeNovelWithBible(novelId, text, chapterNumber, QJsonArray(), QJsonArray());
}

void AnalysisService::analyzeNovelWithBible(const QString& novelId, const QString& text, 
                                            int chapterNumber,
                                            const QJsonArray& existingCharacters,
                                            const QJsonArray& existingScenes)
{
    LOG_INFO("AnalysisService", QString("Starting streaming analysis for novel: %1, chapter: %2")
        .arg(novelId).arg(chapterNumber));
    
    m_currentNovelId = novelId;
    m_currentChapterNumber = chapterNumber;
    m_lastResult = AnalysisResult();
    m_processing = true;
    
    NovelService::instance()->updateStatus(novelId, NovelStatus::Analyzing);
    
    m_currentJobId = createJob(novelId, chapterNumber);
    
    emit analysisStarted(novelId);
    
    QJsonObject schema = loadJsonSchema();
    if (schema.isEmpty()) {
        handleAnalysisFailure(novelId, TR("无法加载 JSON Schema"));
        return;
    }
    
    m_qwen->generateStoryboardStream(text, schema, true, existingCharacters, existingScenes, chapterNumber);
}

void AnalysisService::processTask(TaskData& task)
{
    LOG_INFO("AnalysisService", QString("Processing task for novel: %1, chapter: %2")
        .arg(task.novelId).arg(task.chapterNumber));
    
    QJsonObject schema = loadJsonSchema();
    if (schema.isEmpty()) {
        task.errorMessage = TR("无法加载 JSON Schema");
        throw std::runtime_error(task.errorMessage.toStdString());
    }
    
    QwenClient::StoryboardResult result = m_qwen->generateStoryboard(
        task.text, schema, true,
        task.params["existingCharacters"].toArray(),
        task.params["existingScenes"].toArray(),
        task.chapterNumber);
    
    if (!result.success) {
        task.errorMessage = result.errorMessage;
        throw std::runtime_error(result.errorMessage.toStdString());
    }
    
    saveResults(task.novelId, task.chapterNumber, result);
    
    task.result["panelCount"] = result.panels.size();
    task.result["characterCount"] = result.characters.size();
    task.result["sceneCount"] = result.scenes.size();
}

void AnalysisService::saveResults(const QString& novelId, int chapterNumber, const QwenClient::StoryboardResult& result)
{
    LOG_INFO("AnalysisService", QString("saveResults called: novelId=%1, chapter=%2, panels=%3, characters=%4, scenes=%5")
        .arg(novelId).arg(chapterNumber).arg(result.panels.size()).arg(result.characters.size()).arg(result.scenes.size()));
    
    QList<ExtractedCharacter> extractedChars = CharacterExtractor::instance()->extractFromCharacters(result.characters);
    int savedCharCount = CharacterExtractor::instance()->saveCharacters(novelId, extractedChars);
    LOG_INFO("AnalysisService", QString("Saved %1 characters to database").arg(savedCharCount));
    
    QList<ExtractedScene> extractedScenes = SceneExtractor::instance()->extractFromScenes(result.scenes);
    int savedSceneCount = SceneExtractor::instance()->saveScenes(novelId, extractedScenes);
    LOG_INFO("AnalysisService", QString("Saved %1 scenes to database").arg(savedSceneCount));
    
    QMap<QString, QString> charNameToId;
    QList<Character> savedCharacters = CharacterExtractor::instance()->getCharactersByNovel(novelId);
    for (const Character& c : savedCharacters) {
        charNameToId[c.name()] = c.id();
    }
    
    QMap<QString, QString> sceneNameToId;
    QList<Scene> savedScenes = SceneExtractor::instance()->getScenesByNovel(novelId);
    for (const Scene& s : savedScenes) {
        sceneNameToId[s.name()] = s.id();
    }
    
    QJsonArray panelsWithIds = result.panels;
    for (int i = 0; i < panelsWithIds.size(); ++i) {
        QJsonObject panel = panelsWithIds[i].toObject();
        
        if (panel.contains("characters") && panel["characters"].isArray()) {
            QJsonArray characters = panel["characters"].toArray();
            QJsonArray updatedCharacters;
            
            for (int j = 0; j < characters.size(); ++j) {
                QJsonObject charObj = characters[j].toObject();
                QString charName = charObj["name"].toString();
                
                if (charNameToId.contains(charName)) {
                    charObj["charId"] = charNameToId[charName];
                }
                updatedCharacters.append(charObj);
            }
            panel["characters"] = updatedCharacters;
        }
        
        if (panel.contains("scene") && !panel["scene"].toString().isEmpty()) {
            QString sceneName = panel["scene"].toString();
            if (sceneNameToId.contains(sceneName)) {
                panel["sceneId"] = sceneNameToId[sceneName];
            }
        }
        
        panelsWithIds[i] = panel;
    }
    
    QJsonObject storyboardData;
    storyboardData["panels"] = panelsWithIds;
    storyboardData["characters"] = result.characters;
    storyboardData["scenes"] = result.scenes;
    storyboardData["totalPages"] = result.totalPages;
    
    if (!m_storyboardService->saveStoryboard(novelId, storyboardData, chapterNumber)) {
        LOG_ERROR("AnalysisService", "Failed to save storyboard");
        return;
    }
    LOG_INFO("AnalysisService", "Storyboard saved successfully");
    
    BibleGenerator::instance()->updateExistingCharacters(novelId, result.characters);
    BibleGenerator::instance()->updateExistingScenes(novelId, result.scenes);
}

void AnalysisService::setResultSuccess(int chapterNumber, int panelCount, int characterCount, int sceneCount)
{
    m_lastResult.success = true;
    m_lastResult.chapterNumber = chapterNumber;
    m_lastResult.panelCount = panelCount;
    m_lastResult.characterCount = characterCount;
    m_lastResult.sceneCount = sceneCount;
}

void AnalysisService::setResultFailure(const QString& errorMessage)
{
    m_lastResult.success = false;
    m_lastResult.errorMessage = errorMessage;
}

void AnalysisService::finishProcessing()
{
    m_processing = false;
}

void AnalysisService::handleAnalysisFailure(const QString& novelId, const QString& errorMessage)
{
    LOG_ERROR("AnalysisService", QString("Analysis failed: %1").arg(errorMessage));
    
    finishProcessing();
    setResultFailure(errorMessage);
    
    if (!m_currentJobId.isEmpty()) {
        updateJobStatus(m_currentJobId, "failed", errorMessage);
    }
    
    if (!novelId.isEmpty()) {
        NovelService::instance()->updateStatus(novelId, NovelStatus::Error);
        emit analysisFailed(novelId, errorMessage);
    }
}

void AnalysisService::onQwenProgress(int current, int total, const QString& message)
{
    if (total > 0) {
        int progress = current * 100 / total;
        updateProgress(progress, message);
    }
}

void AnalysisService::onQwenError(const QString& operation, const QString& message)
{
    LOG_ERROR("AnalysisService", QString("%1: %2").arg(operation, message));
    handleAnalysisFailure(m_currentNovelId, message);
}

void AnalysisService::onTaskStarted(const QString& taskId)
{
    if (taskId == m_currentTaskId) {
        LOG_INFO("AnalysisService", QString("Task started: %1").arg(taskId));
    }
}

void AnalysisService::onTaskProgress(const QString& taskId, int progress, const QString& message)
{
    if (taskId == m_currentTaskId) {
        updateProgress(progress, message);
    }
}

void AnalysisService::onTaskCompleted(const QString& taskId, const QJsonObject& result)
{
    LOG_INFO("AnalysisService", QString("onTaskCompleted: taskId=%1").arg(taskId));
    
    TaskData task = TaskQueue::instance()->getTask(taskId);
    
    if (!isTaskMatch(taskId, task)) {
        LOG_WARNING("AnalysisService", QString("Task not matched: taskId=%1, currentTaskId=%2")
            .arg(taskId, m_currentTaskId));
        finishProcessing();
        return;
    }
    
    finishProcessing();
    setResultSuccess(task.chapterNumber, 
                     result["panelCount"].toInt(),
                     result["characterCount"].toInt(),
                     result["sceneCount"].toInt());
    
    QString novelId = (taskId == m_currentTaskId) ? m_currentNovelId : task.novelId;
    
    emit analysisCompleted(novelId, m_lastResult);
    LOG_INFO("AnalysisService", QString("Task completed: novelId=%1, chapter=%2, panels=%3")
        .arg(novelId).arg(task.chapterNumber).arg(m_lastResult.panelCount));
}

void AnalysisService::onTaskFailed(const QString& taskId, const QString& error)
{
    LOG_INFO("AnalysisService", QString("onTaskFailed: taskId=%1, error=%2").arg(taskId, error));
    
    TaskData task = TaskQueue::instance()->getTask(taskId);
    
    if (!isTaskMatch(taskId, task)) {
        return;
    }
    
    QString novelId = (taskId == m_currentTaskId) ? m_currentNovelId : task.novelId;
    
    handleAnalysisFailure(novelId, error);
}

void AnalysisService::onQwenStreamProgress(const QString& partialContent)
{
    emit streamProgress(partialContent);
    
    int charCount = partialContent.length();
    int streamProgress = qMin(40, charCount / 500);
    
    updateProgress(streamProgress, TR("正在生成内容..."));
}

void AnalysisService::onQwenStreamCompleted(const QJsonObject& result)
{
    LOG_INFO("AnalysisService", QString("Stream completed, processing result..."));
    
    if (result.isEmpty()) {
        handleAnalysisFailure(m_currentNovelId, TR("流式输出结果为空"));
        return;
    }
    
    updateProgress(45, TR("正在解析分镜数据..."));
    
    QwenClient::StoryboardResult storyboardResult;
    storyboardResult.panels = result["panels"].toArray();
    storyboardResult.characters = result["characters"].toArray();
    storyboardResult.scenes = result["scenes"].toArray();
    storyboardResult.totalPages = result["totalPages"].toInt();
    storyboardResult.success = true;
    
    updateProgress(47, TR("正在保存分镜数据..."));
    saveResults(m_currentNovelId, m_currentChapterNumber, storyboardResult);
    
    updateProgress(50, TR("正在生成角色和场景图片..."));
    
    setResultSuccess(m_currentChapterNumber,
                     storyboardResult.panels.size(),
                     storyboardResult.characters.size(),
                     storyboardResult.scenes.size());
    
    generateImagesAfterAnalysis(m_currentNovelId, m_currentChapterNumber);
    
    emit streamCompleted(m_currentNovelId, result);
    
    LOG_INFO("AnalysisService", QString("Stream analysis completed: novelId=%1, panels=%2, chars=%3, scenes=%4")
        .arg(m_currentNovelId)
        .arg(storyboardResult.panels.size())
        .arg(storyboardResult.characters.size())
        .arg(storyboardResult.scenes.size()));
}

QString AnalysisService::createJob(const QString& novelId, int chapterNumber)
{
    QString jobId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QJsonObject params;
    params["chapterNumber"] = chapterNumber;
    QString paramsJson = QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Compact));
    
    QVariantMap data;
    data["id"] = jobId;
    data["type"] = "generate_storyboard";
    data["status"] = "running";
    data["novel_id"] = novelId;
    data["params"] = paramsJson;
    data["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    data["updated_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (DatabaseManager::instance()->insert("jobs", data) < 0) {
        LOG_ERROR("AnalysisService", "Failed to create job");
        return QString();
    }
    
    LOG_INFO("AnalysisService", QString("Created job: %1 for novel %2, chapter %3")
        .arg(jobId, novelId).arg(chapterNumber));
    
    return jobId;
}

void AnalysisService::updateJobStatus(const QString& jobId, const QString& status, const QString& errorMessage)
{
    if (jobId.isEmpty()) {
        LOG_WARNING("AnalysisService", "updateJobStatus: jobId is empty");
        return;
    }
    
    QVariantMap data;
    data["status"] = status;
    data["updated_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (status == "completed") {
        data["progress"] = 100;
    } else if (status == "failed") {
        data["progress"] = 0;
    }
    
    if (!errorMessage.isEmpty()) {
        data["error_message"] = errorMessage;
    }
    
    QString where = "id = ?";
    QVariantList values;
    values << jobId;
    
    DatabaseManager::instance()->update("jobs", data, where, values);
    
    LOG_INFO("AnalysisService", QString("Updated job %1 status to: %2, progress: %3")
        .arg(jobId, status).arg(data.value("progress", -1).toInt()));
}

void AnalysisService::updateProgress(int progress, const QString& message)
{
    emit analysisProgress(progress, 100, message);
    
    if (!m_currentJobId.isEmpty()) {
        QVariantMap data;
        data["progress"] = progress;
        DatabaseManager::instance()->update("jobs", data, "id = ?", QVariantList{m_currentJobId});
    }
}

void AnalysisService::generateImagesAfterAnalysis(const QString& novelId, int chapterNumber)
{
    LOG_INFO("AnalysisService", QString("Generating images after analysis: novelId=%1, chapter=%2").arg(novelId).arg(chapterNumber));
    
    m_bibleImagesCompleted = false;
    
    QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(novelId);
    QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(novelId);
    
    int totalCharacters = characters.size();
    int totalScenes = scenes.size();
    int totalImages = totalCharacters + totalScenes;
    
    LOG_INFO("AnalysisService", QString("Generating bible images: %1 characters, %2 scenes")
        .arg(totalCharacters).arg(totalScenes));
    
    if (totalImages == 0) {
        LOG_INFO("AnalysisService", "No bible images to generate, finalizing analysis");
        finalizeAnalysis();
        return;
    }
    
    emit imageGenerationProgress(0, totalImages, QString::fromUtf8("正在生成角色和场景图片..."));
    
    connect(BibleImageService::instance(), &BibleImageService::batchProgress,
            this, &AnalysisService::onBibleImageProgress, Qt::UniqueConnection);
    connect(BibleImageService::instance(), &BibleImageService::allBibleImagesCompleted,
            this, &AnalysisService::onBibleImageCompleted, Qt::UniqueConnection);
    BibleImageService::instance()->generateAllBibleImages(characters, scenes);
}

void AnalysisService::onBibleImageProgress(int current, int total, const QString& info)
{
    emit imageGenerationProgress(current, total, info);
    
    if (total > 0) {
        int imageProgress = 50 + (current * 50 / total);
        updateProgress(imageProgress, info);
    }
}

void AnalysisService::onBibleImageCompleted(int successCount, int failedCount)
{
    LOG_INFO("AnalysisService", QString("=== onBibleImageCompleted called: success=%1, failed=%2 ===")
        .arg(successCount).arg(failedCount));
    
    m_bibleImagesCompleted = true;
    finalizeAnalysis();
}

void AnalysisService::finalizeAnalysis()
{
    LOG_INFO("AnalysisService", QString("=== finalizeAnalysis called: novelId=%1 ===").arg(m_currentNovelId));
    
    disconnect(BibleImageService::instance(), &BibleImageService::batchProgress,
               this, &AnalysisService::onBibleImageProgress);
    disconnect(BibleImageService::instance(), &BibleImageService::allBibleImagesCompleted,
               this, &AnalysisService::onBibleImageCompleted);
    
    updateProgress(100, TR("分析完成"));
    updateJobStatus(m_currentJobId, "completed");
    
    finishProcessing();
    
    LOG_INFO("AnalysisService", QString("=== Emitting analysisCompleted signal: novelId=%1 ===").arg(m_currentNovelId));
    emit analysisCompleted(m_currentNovelId, m_lastResult);
    
    LOG_INFO("AnalysisService", QString("Analysis fully completed (including bible images): novelId=%1")
        .arg(m_currentNovelId));
}
