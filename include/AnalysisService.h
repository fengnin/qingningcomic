#ifndef ANALYSISSERVICE_H
#define ANALYSISSERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaType>
#include "QwenClient.h"
#include "StoryboardService.h"
#include "TaskQueue.h"

struct AnalysisResult {
    bool success = false;
    QString errorMessage;
    QString storyboardId;
    int chapterNumber = 1;
    int panelCount = 0;
    int characterCount = 0;
    int sceneCount = 0;
};

Q_DECLARE_METATYPE(AnalysisResult)

class AnalysisService : public QObject
{
    Q_OBJECT

public:
    static AnalysisService* instance();
    
    void analyzeNovel(const QString& novelId, const QString& text, int chapterNumber = 0);
    void analyzeNovelWithBible(const QString& novelId, const QString& text, 
                               int chapterNumber,
                               const QJsonArray& existingCharacters,
                               const QJsonArray& existingScenes);
    
    AnalysisResult getLastResult() const;
    bool isProcessing() const;
    QString currentJobId() const;
    
    void processTask(TaskData& task);

signals:
    void analysisStarted(const QString& novelId);
    void analysisProgress(int current, int total, const QString& message);
    void analysisCompleted(const QString& novelId, const AnalysisResult& result);
    void analysisFailed(const QString& novelId, const QString& error);
    void streamProgress(const QString& partialContent);
    void streamCompleted(const QString& novelId, const QJsonObject& result);
    
    void imageGenerationProgress(int current, int total, const QString& message);

private slots:
    void onQwenProgress(int current, int total, const QString& message);
    void onQwenError(const QString& operation, const QString& message);
    void onQwenStreamProgress(const QString& partialContent);
    void onQwenStreamCompleted(const QJsonObject& result);
    void onTaskStarted(const QString& taskId);
    void onTaskProgress(const QString& taskId, int progress, const QString& message);
    void onTaskCompleted(const QString& taskId, const QJsonObject& result);
    void onTaskFailed(const QString& taskId, const QString& error);
    
    void onBibleImageProgress(int current, int total, const QString& info);
    void onBibleImageCompleted(int successCount, int failedCount);

private:
    AnalysisService(QObject* parent = nullptr);
    ~AnalysisService();
    AnalysisService(const AnalysisService&) = delete;
    AnalysisService& operator=(const AnalysisService&) = delete;
    
    QJsonObject loadJsonSchema();
    bool isTaskMatch(const QString& taskId, const TaskData& task) const;
    void saveResults(const QString& novelId, int chapterNumber, const QwenClient::StoryboardResult& result);
    
    QString createJob(const QString& novelId, int chapterNumber);
    void updateJobStatus(const QString& jobId, const QString& status, const QString& errorMessage = QString());
    void updateProgress(int progress, const QString& message);
    
    void setResultSuccess(int chapterNumber, int panelCount, int characterCount, int sceneCount);
    void setResultFailure(const QString& errorMessage);
    void finishProcessing();
    void handleAnalysisFailure(const QString& novelId, const QString& errorMessage);
    void generateImagesAfterAnalysis(const QString& novelId, int chapterNumber);
    void finalizeAnalysis();
    
    static AnalysisService* m_instance;
    QwenClient* m_qwen;
    StoryboardService* m_storyboardService;
    QString m_currentNovelId;
    QString m_currentTaskId;
    QString m_currentJobId;
    int m_currentChapterNumber = 1;
    AnalysisResult m_lastResult;
    bool m_processing = false;
    
    bool m_bibleImagesCompleted = false;
};

#endif // ANALYSISSERVICE_H
