#ifndef STORYBOARDVIEWMODEL_H
#define STORYBOARDVIEWMODEL_H

#include "viewmodels/BaseViewModel.h"
#include "models/Storyboard.h"
#include "models/Panel.h"
#include "services/AnalysisService.h"
#include "services/ImageService.h"
#include "utils/SingletonUtils.h"
#include <QList>
#include <QMap>

class StoryboardService;

class StoryboardViewModel : public BaseViewModel
{
    Q_OBJECT
    DECLARE_SINGLETON(StoryboardViewModel)

public:
    explicit StoryboardViewModel(QObject* parent = nullptr);
    ~StoryboardViewModel();
    
    void initialize() override;
    
    QList<Storyboard> storyboards() const { return m_storyboards; }
    Storyboard currentStoryboard() const { return m_currentStoryboard; }
    QList<Panel> currentPanels() const { return m_currentPanels; }
    bool isAnalyzing() const { return m_analyzing; }
    QString currentJobId() const { return m_currentJobId; }
    
    QString cachedNovelId() const { return m_cachedNovelId; }
    int cachedChapterNumber() const { return m_cachedChapterNumber; }
    bool hasCachedStoryboards(const QString& novelId) const;
    bool hasCachedPanels(const QString& novelId, int chapterNumber) const;
    
    void loadStoryboards(const QString& novelId, bool forceReload = false);
    void loadStoryboard(const QString& novelId, int chapterNumber, bool forceReload = false);
    void loadPanels(const QString& storyboardId);
    void invalidatePanelsCache(const QString& storyboardId);
    
    void startAnalysis(const QString& novelId, const QString& text, int chapterNumber);
    void startAnalysisWithBible(const QString& novelId, const QString& text, int chapterNumber,
                                const QJsonArray& characters, const QJsonArray& scenes);
    
    void generatePanelImages(const QStringList& panelIds, const QString& mode);
    void generateAllPanelImages(const QString& storyboardId, const QString& mode);
    QString enqueueBatchPanelImageGeneration(const QStringList& panelIds, const QString& mode);
    
    bool createEmptyStoryboard(const QString& novelId, int chapterNumber);
    
    bool deleteStoryboard(const QString& novelId, int chapterNumber);
    
    Panel getPanel(const QString& panelId);
    bool updatePanel(const QString& panelId, const QJsonObject& content);
    
    QString getAnalysisProgress() const { return m_analysisProgress; }
    QString getAnalysisStage() const { return m_analysisStage; }
    
    void clearCache();

signals:
    void storyboardsLoaded(const QString& novelId, const QList<Storyboard>& storyboards);
    void storyboardLoaded(const QString& novelId, int chapterNumber, const Storyboard& storyboard);
    void panelsLoaded(const QString& novelId, int chapterNumber, const QList<Panel>& panels);
    void panelUpdated(const QString& panelId, const QJsonObject& content);
    
    void analysisStarted(const QString& novelId);
    void analysisProgress(const QString& stage, int progress);
    void analysisCompleted(const QString& novelId, int chapterNumber);
    void analysisFailed(const QString& novelId, const QString& error);
    
    void imageGenerationStarted(int total);
    void imageGenerationProgress(int current, int total);
    void imageGenerationCompleted(int success, int failed);
    
    void storyboardDeleted(const QString& novelId, int chapterNumber);

private slots:
    void onAnalysisStarted(const QString& novelId);
    void onAnalysisCompleted(const QString& novelId, const AnalysisResult& result);
    void onAnalysisFailed(const QString& novelId, const QString& error);
    void onStoryboardSaved(const QString& novelId);
    void applyStoryboardsLoaded(int token, const QString& novelId, const QList<Storyboard>& storyboards,
                                bool success, const QString& error);
    void applyStoryboardLoaded(int token, const QString& novelId, int chapterNumber,
                               const Storyboard& storyboard, const QList<Panel>& panels,
                               bool success, const QString& error);

private:
    void connectServiceSignals();
    void resetAnalysisState();
    
    StoryboardService* m_storyboardService;
    AnalysisService* m_analysisService;
    ImageService* m_imageService;
    
    QList<Storyboard> m_storyboards;
    Storyboard m_currentStoryboard;
    QList<Panel> m_currentPanels;
    
    QString m_cachedNovelId;
    int m_cachedChapterNumber = 0;
    QMap<QString, QList<Panel>> m_panelsCache;
    int m_storyboardsRequestToken = 0;
    int m_storyboardRequestToken = 0;
    bool m_storyboardsLoading = false;
    bool m_storyboardLoading = false;
    
    bool m_analyzing = false;
    QString m_currentJobId;
    QString m_analysisProgress;
    QString m_analysisStage;
};

#endif // STORYBOARDVIEWMODEL_H
