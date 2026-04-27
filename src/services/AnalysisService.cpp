#include "services/AnalysisService.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "services/BibleGenerator.h"
#include "services/BibleImageService.h"
#include "services/ImageService.h"
#include "services/NovelService.h"
#include "services/BibleContextInjector.h"
#include "api/QwenPromptBuilder.h"
#include "utils/SchemaFileUtils.h"
#include "utils/AnalysisTextUtils.h"
#include "utils/JsonUtils.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include "utils/AnalysisJobUtils.h"
#include "data/DatabaseManager.h"
#include "utils/SceneKeyUtils.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QUuid>

namespace {

QMap<QString, QString> buildCharacterLookup(const QList<Character>& characters)
{
    QMap<QString, QString> lookup;
    for (const Character& character : characters) {
        const QString id = character.id().trimmed();
        if (id.isEmpty()) {
            continue;
        }

        for (const QString& key : CharacterExtractor::buildIdentityKeys(character)) {
            const QString trimmed = key.trimmed();
            if (!trimmed.isEmpty()) {
                lookup[trimmed] = id;
            }
        }
    }
    return lookup;
}

QMap<QString, QString> buildSceneLookup(const QList<Scene>& scenes)
{
    QJsonArray sceneArray;
    for (const Scene& scene : scenes) {
        sceneArray.append(scene.toJson());
    }
    return SceneKeyUtils::buildSceneLookupMap(sceneArray);
}

QString resolveCharacterId(const QMap<QString, QString>& lookup, const QJsonObject& charObj)
{
    Character probe;
    probe.setName(charObj["name"].toString());
    probe.setRole(charObj["role"].toString());
    probe.setAliases(JsonUtils::jsonArrayToStringList(charObj["aliases"].toArray()));

    for (const QString& key : CharacterExtractor::buildIdentityKeys(probe)) {
        const QString trimmed = key.trimmed();
        if (!trimmed.isEmpty() && lookup.contains(trimmed)) {
            return lookup.value(trimmed);
        }
    }
    return QString();
}

QString resolveSceneId(const QMap<QString, QString>& lookup, const QJsonObject& background, const QString& panelSceneText)
{
    const QStringList candidates = {
        background.value("sceneId").toString(),
        background.value("setting").toString(),
        panelSceneText,
        SceneKeyUtils::normalizeSceneLabel(background.value("setting").toString()),
        SceneKeyUtils::normalizeSceneLabel(panelSceneText)
    };
    return SceneKeyUtils::resolveSceneLookupId(lookup, candidates);
}

QwenClient::StoryboardResult buildStoryboardResultFromJson(const QJsonObject& result)
{
    QwenClient::StoryboardResult storyboardResult;
    storyboardResult.panels = result["panels"].toArray();
    storyboardResult.characters = result["characters"].toArray();
    storyboardResult.scenes = result["scenes"].toArray();
    storyboardResult.totalPages = result["totalPages"].toInt();
    storyboardResult.success = true;
    return storyboardResult;
}

QJsonArray toJsonArray(const QList<ExtractedCharacter>& characters)
{
    QJsonArray result;
    for (const ExtractedCharacter& character : characters) {
        result.append(character.toJson());
    }
    return result;
}

QJsonArray toJsonArray(const QList<ExtractedScene>& scenes)
{
    QJsonArray result;
    for (const ExtractedScene& scene : scenes) {
        result.append(scene.toJson());
    }
    return result;
}

void attachCharacterIds(QJsonArray& panels, const QMap<QString, QString>& charNameToId)
{
    for (int i = 0; i < panels.size(); ++i) {
        QJsonObject panel = panels[i].toObject();

        if (panel.contains("characters") && panel["characters"].isArray()) {
            QJsonArray characters = panel["characters"].toArray();
            QJsonArray updatedCharacters;

            for (int j = 0; j < characters.size(); ++j) {
                QJsonObject charObj = characters[j].toObject();
                const QString charId = resolveCharacterId(charNameToId, charObj);
                if (!charId.isEmpty()) {
                    charObj["charId"] = charId;
                }
                updatedCharacters.append(charObj);
            }
            panel["characters"] = updatedCharacters;
        }

        panels[i] = panel;
    }
}

void attachSceneIds(QJsonArray& panels, const QMap<QString, QString>& sceneNameToId)
{
    for (int i = 0; i < panels.size(); ++i) {
        QJsonObject panel = panels[i].toObject();
        if (panel.contains("background")) {
            QJsonObject background = panel["background"].toObject();
            const QString sceneId = resolveSceneId(sceneNameToId, background, panel["scene"].toString());
            if (!sceneId.isEmpty()) {
                panel["sceneId"] = sceneId;
                background["sceneId"] = sceneId;
                panel["background"] = background;
            }
        }
        panels[i] = panel;
    }
}

QJsonObject buildStoryboardData(const QJsonArray& panels, const QJsonArray& characters, const QJsonArray& scenes, int totalPages)
{
    QJsonObject storyboardData;
    storyboardData["panels"] = panels;
    storyboardData["characters"] = characters;
    storyboardData["scenes"] = scenes;
    storyboardData["totalPages"] = totalPages;
    return storyboardData;
}

} // namespace

AnalysisService* AnalysisService::m_instance = nullptr;
std::once_flag AnalysisService::m_instanceOnceFlag;

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
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new AnalysisService();
    });
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
    if (!m_cachedSchema.isEmpty()) {
        return m_cachedSchema;
    }

    const QStringList schemaPaths = {
        QStringLiteral("schemas/storyboard.json"),
        QStringLiteral("../schemas/storyboard.json")
    };

    for (const QString& schemaPath : schemaPaths) {
        const QString resolvedPath = SchemaFileUtils::resolveSchemaPath(schemaPath);
        QJsonObject schema = SchemaFileUtils::readJsonObjectFile(resolvedPath);
        if (!schema.isEmpty()) {
            LOG_INFO("AnalysisService", QString("Loaded schema from %1").arg(resolvedPath));
            m_cachedSchema = schema;
            return m_cachedSchema;
        }
    }
    
    LOG_ERROR("AnalysisService", QString("Failed to load JSON Schema. AppDir: %1")
        .arg(QCoreApplication::applicationDirPath()));
    return QJsonObject();
}

bool AnalysisService::isTaskMatch(const QString& taskId, const TaskData& task) const
{
    if (taskId.isEmpty()) {
        return false;
    }

    if (task.type != TaskType::GenerateStoryboard) {
        return false;
    }
    
    if (task.id.isEmpty()) {
        return taskId == m_currentTaskId;
    }
    
    return task.id == taskId;
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
    
    if (m_processing) {
        LOG_WARNING("AnalysisService", QString("Analysis already in progress for novel: %1, rejecting new request for: %2")
            .arg(m_currentNovelId).arg(novelId));
        emit analysisFailed(novelId, tr("已有分析任务正在进行中，请稍后再试"));
        return;
    }
    m_currentNovelId = novelId;
    m_currentChapterNumber = chapterNumber;
    m_currentSourceText = text;
    m_lastResult = AnalysisResult();
    m_processing = true;
    
    NovelService::instance()->updateStatus(novelId, NovelStatus::Analyzing);
    
    m_currentJobId = AnalysisJobUtils::createJob(novelId, chapterNumber);
    
    emit analysisStarted(novelId);
    
    QJsonObject schema = loadJsonSchema();
    if (schema.isEmpty()) {
        handleAnalysisFailure(novelId, TR("无法加载 JSON Schema"));
        return;
    }

    const QString userMessage = QwenPromptBuilder::buildUserMessageWithBible(
        text, existingCharacters, existingScenes, chapterNumber);
    const int estimatedBytes = userMessage.toUtf8().size();
    const bool shouldUseTaskQueue =
        text.length() > m_qwen->maxChunkLength() || estimatedBytes > 30000;
    
    if (shouldUseTaskQueue) {
        TaskData task;
        task.type = TaskType::GenerateStoryboard;
        task.novelId = novelId;
        task.chapterNumber = chapterNumber;
        task.text = text;
        task.params["existingCharacters"] = existingCharacters;
        task.params["existingScenes"] = existingScenes;
        task.total = 100;
        
        m_currentTaskId = TaskQueue::instance()->enqueue(task);
        LOG_INFO("AnalysisService", QString("Switched to queued storyboard analysis: taskId=%1, textLength=%2, estimatedPromptBytes=%3")
            .arg(m_currentTaskId)
            .arg(text.length())
            .arg(estimatedBytes));
        updateProgress(1, tr("章节较长，正在使用分块分析..."));
        return;
    }

    m_currentTaskId.clear();
    m_qwen->generateStoryboardStream(text, schema, true, existingCharacters, existingScenes, chapterNumber);
}

void AnalysisService::processTask(TaskData& task)
{
    LOG_INFO("AnalysisService", QString("Processing task for novel: %1, chapter: %2")
        .arg(task.novelId).arg(task.chapterNumber));
    
    QJsonObject schema = loadJsonSchema();
    if (schema.isEmpty()) {
        task.errorMessage = tr("JSON Schema 加载失败");
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
    
    saveResults(task.novelId, task.chapterNumber, result, task.text);
    
    task.result["panelCount"] = result.panels.size();
    task.result["characterCount"] = result.characters.size();
    task.result["sceneCount"] = result.scenes.size();
}

void AnalysisService::saveResults(const QString& novelId, int chapterNumber, const QwenClient::StoryboardResult& result, const QString& sourceText)
{
    LOG_INFO("AnalysisService", QString("saveResults called: novelId=%1, chapter=%2, panels=%3, characters=%4, scenes=%5")
        .arg(novelId).arg(chapterNumber).arg(result.panels.size()).arg(result.characters.size()).arg(result.scenes.size()));

    CharacterExtractor* characterExtractor = CharacterExtractor::instance();
    SceneExtractor* sceneExtractor = SceneExtractor::instance();

    QList<ExtractedCharacter> extractedChars = characterExtractor->extractFromCharacters(result.characters, sourceText);
    int savedCharCount = characterExtractor->saveCharacters(novelId, extractedChars);
    LOG_INFO("AnalysisService", QString("Saved %1 characters to database").arg(savedCharCount));

    QList<ExtractedScene> extractedScenes = sceneExtractor->extractFromScenes(result.scenes, sourceText);
    int savedSceneCount = sceneExtractor->saveScenes(novelId, extractedScenes);
    LOG_INFO("AnalysisService", QString("Saved %1 scenes to database").arg(savedSceneCount));

    const QJsonArray bibleCharacters = toJsonArray(extractedChars);
    const QJsonArray bibleScenes = toJsonArray(extractedScenes);

    QList<Character> savedCharacters = characterExtractor->getCharactersByNovel(novelId);
    QMap<QString, QString> charNameToId = buildCharacterLookup(savedCharacters);

    QList<Scene> savedScenes = sceneExtractor->getScenesByNovel(novelId);
    QMap<QString, QString> sceneNameToId = buildSceneLookup(savedScenes);

    QJsonArray panelsWithIds = result.panels;
    attachCharacterIds(panelsWithIds, charNameToId);
    attachSceneIds(panelsWithIds, sceneNameToId);

    QJsonObject storyboardData = buildStoryboardData(
        panelsWithIds, result.characters, result.scenes, result.totalPages);

    if (!m_storyboardService->saveStoryboard(novelId, storyboardData, chapterNumber, false)) {
        LOG_ERROR("AnalysisService", "Failed to save storyboard");
        throw std::runtime_error(tr("保存分镜数据失败").toStdString());
    }
    LOG_INFO("AnalysisService", "Storyboard saved successfully");
    
    BibleGenerator::instance()->updateExistingCharacters(novelId, bibleCharacters);
    BibleGenerator::instance()->updateExistingScenes(novelId, bibleScenes);
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
        AnalysisJobUtils::updateJobStatus(m_currentJobId, "failed", errorMessage);
    }
    
    if (!novelId.isEmpty()) {
        NovelService::instance()->updateStatus(novelId, NovelStatus::Error);
        emit analysisFailed(novelId, errorMessage);
    }

    m_currentSourceText.clear();
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
    TaskData task = TaskQueue::instance()->getTask(taskId);
    if (task.type != TaskType::GenerateStoryboard) {
        return;
    }

    if (taskId == m_currentTaskId) {
        LOG_INFO("AnalysisService", QString("Task started: %1").arg(taskId));
    }
}

void AnalysisService::onTaskProgress(const QString& taskId, int progress, const QString& message)
{
    TaskData task = TaskQueue::instance()->getTask(taskId);
    if (task.type != TaskType::GenerateStoryboard) {
        return;
    }

    if (taskId == m_currentTaskId) {
        updateProgress(progress, message);
    }
}

void AnalysisService::onTaskCompleted(const QString& taskId, const QJsonObject& result)
{
    LOG_INFO("AnalysisService", QString("onTaskCompleted: taskId=%1").arg(taskId));
    
    TaskData task = TaskQueue::instance()->getTask(taskId);
    if (task.type != TaskType::GenerateStoryboard) {
        LOG_DEBUG("AnalysisService", QString("Ignoring non-storyboard task completion: taskId=%1, type=%2")
            .arg(taskId, task.typeString()));
        return;
    }
    
    if (!isTaskMatch(taskId, task)) {
        LOG_WARNING("AnalysisService", QString("Task not matched: taskId=%1, currentTaskId=%2")
            .arg(taskId, m_currentTaskId));
        finishProcessing();
        return;
    }
    
    setResultSuccess(task.chapterNumber, 
                     result["panelCount"].toInt(),
                     result["characterCount"].toInt(),
                     result["sceneCount"].toInt());
    
    QString novelId = (taskId == m_currentTaskId) ? m_currentNovelId : task.novelId;

    updateProgress(50, tr("正在生成角色和场景图片..."));
    generateImagesAfterAnalysis(novelId, task.chapterNumber);

    LOG_INFO("AnalysisService", QString("Task completed: novelId=%1, chapter=%2, panels=%3")
        .arg(novelId).arg(task.chapterNumber).arg(m_lastResult.panelCount));
}

void AnalysisService::onTaskFailed(const QString& taskId, const QString& error)
{
    LOG_INFO("AnalysisService", QString("onTaskFailed: taskId=%1, error=%2").arg(taskId, error));
    
    TaskData task = TaskQueue::instance()->getTask(taskId);
    if (task.type != TaskType::GenerateStoryboard) {
        return;
    }
    
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
    
    updateProgress(streamProgress, tr("正在生成内容..."));
}

void AnalysisService::onQwenStreamCompleted(const QJsonObject& result)
{
    LOG_INFO("AnalysisService", QString("Stream completed, processing result..."));

    if (result.isEmpty()) {
        handleAnalysisFailure(m_currentNovelId, tr("流式输出结果为空"));
        return;
    }

    try {
        handleGeneratedStoryboard(
            m_currentNovelId,
            m_currentChapterNumber,
            buildStoryboardResultFromJson(result),
            result);
    } catch (const std::exception& e) {
        handleAnalysisFailure(m_currentNovelId, QString::fromUtf8(e.what()));
        return;
    }
}

void AnalysisService::updateProgress(int progress, const QString& message)
{
    emit analysisProgress(progress, 100, message);
    
    if (!m_currentJobId.isEmpty()) {
        AnalysisJobUtils::updateJobProgress(m_currentJobId, progress);
    }
}

void AnalysisService::handleGeneratedStoryboard(const QString& novelId, int chapterNumber,
                                                const QwenClient::StoryboardResult& storyboardResult,
                                                const QJsonObject& rawResult)
{
    updateProgress(45, tr("正在解析分镜数据..."));
    updateProgress(47, tr("正在保存分镜数据..."));
    saveResults(novelId, chapterNumber, storyboardResult, m_currentSourceText);
    updateProgress(50, tr("正在生成角色和场景图片..."));

    setResultSuccess(chapterNumber,
                     storyboardResult.panels.size(),
                     storyboardResult.characters.size(),
                     storyboardResult.scenes.size());

    generateImagesAfterAnalysis(novelId, chapterNumber);

    if (!rawResult.isEmpty()) {
        emit streamCompleted(novelId, rawResult);
    }

    LOG_INFO("AnalysisService", QString("Analysis completed: novelId=%1, panels=%2, chars=%3, scenes=%4")
        .arg(novelId)
        .arg(storyboardResult.panels.size())
        .arg(storyboardResult.characters.size())
        .arg(storyboardResult.scenes.size()));
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
    Q_UNUSED(successCount)
    Q_UNUSED(failedCount)
    m_bibleImagesCompleted = true;
    finalizeAnalysis();
}

void AnalysisService::finalizeAnalysis()
{
    disconnect(BibleImageService::instance(), &BibleImageService::batchProgress,
               this, &AnalysisService::onBibleImageProgress);
    disconnect(BibleImageService::instance(), &BibleImageService::allBibleImagesCompleted,
               this, &AnalysisService::onBibleImageCompleted);
    
    updateProgress(100, tr("分析完成"));
    AnalysisJobUtils::updateJobStatus(m_currentJobId, "completed");
    
    if (!m_currentNovelId.isEmpty()) {
        NovelService::instance()->updateStatus(m_currentNovelId, NovelStatus::Completed);
    }
    
    finishProcessing();
    m_currentSourceText.clear();
    
    emit analysisCompleted(m_currentNovelId, m_lastResult);
    
    LOG_INFO("AnalysisService", QString("Analysis completed: novelId=%1").arg(m_currentNovelId));
}
