#ifndef NOVELDETAILPAGE_H
#define NOVELDETAILPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QLayout>
#include <QProgressBar>
#include <QJsonObject>
#include <QJsonArray>
#include <initializer_list>
#include "models/Novel.h"
#include "models/Character.h"
#include "models/Scene.h"
#include "services/StoryboardService.h"
#include "services/AnalysisService.h"
#include "services/ImageService.h"
#include "services/BibleImageService.h"
#include "components/ChapterSpinBox.h"
#include "components/ModeComboBox.h"
#include "components/ChapterCard.h"
#include "components/StoryboardItem.h"
#include "components/BibleItem.h"
#include "components/PanelCard.h"
#include "components/AnalysisProgressWidget.h"
#include "components/AnalysisResultWidget.h"
#include "components/BibleSectionWidget.h"
#include "components/PanelPreviewWidget.h"
#include "services/AnalysisStatusManager.h"

class ChangeRequestService;

struct StoryboardItemData {
    int panelNumber = 0;
    QString panelId;
    QString scene;
    QString shotType;
    QString cameraAngle;
    QString characters;
    QString dialogue;
    QString visualPrompt;
    QString visualPromptEn;
    QString visualPromptCn;
};

class NovelDetailPage : public QWidget
{
    Q_OBJECT

public:
    explicit NovelDetailPage(QWidget *parent = nullptr);
    ~NovelDetailPage();
    
    void setNovel(const Novel &novel);
    void setChapterNumber(int chapterNumber);
    Novel currentNovel() const { return m_currentNovel; }

signals:
    void backClicked();
    void navigateToExportRequested();

private slots:
    void onBackClicked();
    void onAnalyzeClicked();
    void onExportClicked();
    void onViewExportsClicked();
    void onRefreshChaptersClicked();
    void onAddChapterClicked();
    void onChapterNumberChanged(int value);
    void onGeneratePanelsClicked();
    void onSubmitChangeRequestClicked();
    void onRefreshBibleClicked();
    void onChapterClicked(int chapterNumber);
    void onChapterDeleteRequested(int chapterNumber);
    void onAnalysisStarted(const QString& novelId);
    void onAnalysisCompleted(const QString& novelId, int chapterNumber);
    void onAnalysisFailed(const QString& novelId, const QString& error);
    void onBibleDataChanged();
    void onCharacterCountChanged(int count);
    void onSceneCountChanged(int count);
    void onBibleItemEditClicked(const QString &id, BibleType type);
    void onBibleItemDataChanged(const QString &id, const QStringList &details);
    void onBibleItemUploadClicked(const QString &id, const QString &imagePath, BibleType type);
    void onBibleItemDeleteImageClicked(const QString &id, BibleType type);
    void onBibleItemDeleteRequested(const QString &id, BibleType type);
    void onCharacterDataChanged(const QString &id, const Character &character);
    void onSceneDataChanged(const QString &id, const Scene &scene);
    void onPanelCardClicked(int panelNumber, const QString& panelId);
    void onStoryboardsLoaded(const QString& novelId, const QList<Storyboard>& storyboards);
    void onStoryboardLoaded(const QString& novelId, int chapterNumber, const Storyboard& storyboard);
    void onPanelsLoaded(const QString& novelId, int chapterNumber, const QList<Panel>& panels);
    void onStoryboardDataChanged(const QString &panelId, int panelNumber, const QString &scene,
                                  const QString &shotType, const QString &cameraAngle,
                                  const QString &characters, const QString &dialogue,
                                  const QString &visualPrompt, const QString &visualPromptEn,
                                  const QString &visualPromptCn);
    void onBibleImageBatchProgress(int current, int total, const QString& type);
    void onAllBibleImagesCompleted(int successCount, int failedCount);
    void onAllImageGenerationCompleted();

private:
    void initColorConstants();
    void setupUI();
    void setupConnections();
    
    QLabel* createSectionLabel(const QString &text);
    
    QFrame* createFeatureCardFrame();
    QWidget* createCardHeader(const QString &title, const QString &subtitle);
    QWidget* createButtonStatusRow(QPushButton *btn, const QString &statusText);
    QWidget* createButtonRow(QPushButton *&btn, const QString &btnText, const QString &statusText);
    void updateButtonStatus(QPushButton *btn, const QString &text, const QString &color);
    void updateChangeRequestOverviewStatus(const QString& text, const QString& color = QString());
    void beginChangeRequestProgress(const QString& requestId, const QString& naturalLanguage);
    void updateChangeRequestProgress(int current, int total, const QString& message);
    void finishChangeRequestProgress(bool success, const QString& message);
    void updateChapterHints(const QList<Storyboard>& storyboards);
    void updateChapterUI(int targetChapter);
    void refreshAddChapterUI();
    int nextAvailableChapterNumber() const;
    QLabel* createCompactTag(const QString &text, int fontSize = 12);
    QWidget* createHBoxLayoutRow(const std::initializer_list<QWidget*> &widgets, int spacing = 8);
    void startAutoImageGeneration(int chapter);
    void updateBibleMetaLabel();
    
    QScrollArea* createScrollArea(Qt::ScrollBarPolicy vPolicy = Qt::ScrollBarAsNeeded, 
                                   Qt::ScrollBarPolicy hPolicy = Qt::ScrollBarAsNeeded);
    QScrollArea* createVerticalScrollArea();
    QScrollArea* createHorizontalScrollArea();
    QWidget* createBibleCard(const QString &title, QLabel *countLabel, 
                              QWidget *container, int minHeight = 200);
    
    QWidget* createHeaderSection();
    QWidget* createChapterSection();
    QWidget* createFeatureSection();
    QWidget* createBibleSection();
    QWidget* createStoryboardSection();
    
    QWidget* createAddChapterCard();
    QWidget* createGeneratePanelsCard();
    QWidget* createChangeRequestCard();
    QWidget* createExportCard();
    
    QWidget* createCharacterBibleCard();
    QWidget* createSceneBibleCard();
    QWidget* createBibleCardWithItems(const QString &title, BibleType type);
    QWidget* createPanelPreviewCard();
    QWidget* createStoryboardCard();
    
    QLabel* createStatusLabel(const QString &status);
    ChapterCard* createChapterCard(int chapterNumber, int panelCount, 
                                   const QString &status, bool isActive);
    void updateDisplay();
    void refreshChapterCards(const QList<Storyboard>& storyboards);
    void refreshChapterCardsOnly();
    void clearChapterCards();
    void resetStoryboardDisplay();
    void syncChapterSelectionFromStoryboards(const QList<Storyboard>& storyboards);
    void refreshStoryboardItems();
    void refreshStoryboardItems(const QList<Panel>& panels);
    QList<Panel> loadPanelsFromDatabase() const;
    void refreshPanelsAfterBatchGeneration();
    void preparePanelBatchGenerationUI();
    void startPanelBatchGeneration(const QList<Panel>& panels, ImageService::BatchPresetMode presetMode);
    void beginPanelBatchGenerationUI(int panelCount);
    void finishPanelBatchGenerationUI(bool success, const QString& message = QString(),
                                      const QJsonObject& result = QJsonObject());
    void resetPanelBatchGenerationUI();
    void resetPanelBatchGenerationControls();
    bool isPanelBatchTaskForNovel(const QString& taskId, const QString& novelId) const;
    void clearPanelBatchTaskState();
    void handlePanelBatchGenerationFailure(const QString& errorMessage);
    void onPanelBatchTaskProgress(const QString& taskId, int progress, const QString& message);
    void onPanelBatchTaskCompleted(const QString& taskId, const QJsonObject& result);
    void onPanelBatchTaskFailed(const QString& taskId, const QString& error);
    QJsonObject buildStoryboardUpdateContent(const Panel& panel,
                                             const QString& scene,
                                             const QString& shotType,
                                             const QString& cameraAngle,
                                             const QString& characters,
                                             const QString& dialogue,
                                             const QString& visualPrompt,
                                             const QString& visualPromptEn,
                                             const QString& visualPromptCn) const;
    StoryboardItemData parsePanelToItem(const Panel& panel) const;
    void clearLayout(QLayout *layout);
    void updateChapterSelection(int chapterNumber);
    
    void setAnalysisStatus(AnalysisStatusManager::Status status, const QString& extraInfo = QString());
    QString getChapterText(int chapterNumber) const;
    bool validateChapterInput(const QString& text) const;
    void showChangeRequestSubmissionResult(const QString& requestId);
    void showChangeRequestExecutionResult(const QString& statusText, bool success);
    void handleAnalysisSuccess(int chapter);
    void handleAnalysisFailure(const QString& errorMessage);
    bool isCurrentNovel(const QString& novelId) const;
    void createRunningJobRecord(int chapterNumber);
    
    bool isBibleGenerating() const;
    void refreshBibleSection();
    void beginBibleImageGeneration();
    void endBibleImageGeneration();
    void requestBibleRefresh();
    void applyDeferredBibleRefresh();
    
    QJsonArray parseCharactersToJson(const QString& characters) const;
    QJsonArray parseDialogueToJson(const QString& dialogue,
                                   const QJsonArray& originalDialogue = QJsonArray(),
                                   const QStringList& characterNames = QStringList()) const;
    QString collectChangeRequestText() const;
    QJsonObject buildChangeRequestContext() const;
    void bindChangeRequestResultHandlers(ChangeRequestService* service);
    void showChangeRequestSubmissionFailure(const QString& errorMessage);
    
    QString m_colorTitle;
    QString m_colorSubtitle;
    QString m_colorText;
    QString m_colorHint;
    QString m_colorLabel;
    
    QVBoxLayout *m_mainLayout;
    QScrollArea *m_mainScrollArea = nullptr;
    QLabel *m_titleLabel;
    QLabel *m_statusLabel;
    QLabel *m_metaLabel;
    QLabel *m_chapterCountLabel;
    
    QPushButton *m_analyzeBtn;
    QPushButton *m_viewExportsBtn;
    ModeComboBox *m_exportFormatCombo;
    QPushButton *m_exportBtn;
    QLabel *m_exportStatusLabel;
    
    QWidget *m_chapterContainer;
    QScrollArea *m_chapterScrollArea = nullptr;
    QHBoxLayout *m_chapterContainerLayout;
    QList<ChapterCard*> m_chapterCards;
    ChapterSpinBox *m_chapterNumberSpin;
    QLabel *m_chapterHintLabel;
    QTextEdit *m_chapterTextEdit;
    QPushButton *m_addChapterBtn;
    QLabel *m_addChapterStatusLabel;
    
    ModeComboBox *m_generateModeCombo;
    QLabel *m_generateModeHintLabel;
    QPushButton *m_generatePanelsBtn;
    QTextEdit *m_changeRequestEdit;
    QPushButton *m_submitChangeRequestBtn;
    QLabel *m_changeRequestOverviewLabel = nullptr;
    
    BibleSectionWidget *m_bibleSectionWidget = nullptr;
    PanelPreviewWidget *m_panelPreviewWidget = nullptr;
    
    QWidget *m_storyboardContainer;
    QLabel *m_storyboardCountLabel;
    
    AnalysisProgressWidget *m_analysisProgress;
    AnalysisProgressWidget *m_panelGenerateProgress;
    AnalysisProgressWidget *m_changeRequestOverviewProgress = nullptr;
    AnalysisProgressWidget *m_changeRequestProgress = nullptr;
    AnalysisResultWidget *m_analysisResult;
    AnalysisStatusManager::Status m_analysisStatus;
    
    Novel m_currentNovel;
    int m_currentChapter;
    int m_completedChapterCount;
    QString m_currentJobId;
    QString m_activeChangeRequestId;
    QString m_activeChangeRequestSummary;
    bool m_changeRequestRunning = false;
    int m_selectedPanelNumber = 0;
    QString m_selectedPanelId;
    QMap<QPushButton*, QLabel*> m_statusLabelMap;
    
    QList<Character> m_pendingCharacters;
    QList<Scene> m_pendingScenes;
    QList<Panel> m_currentPanels;
    bool m_pendingPanelBatchGeneration = false;
    ImageService::BatchPresetMode m_pendingPanelBatchPresetMode = ImageService::BatchPresetMode::Square_1x1;
    bool m_panelBatchRefreshPending = false;
    QString m_panelBatchTaskId;
    QString m_panelBatchNovelId;
    int m_totalImageTasks;
    int m_completedImageTasks;
    bool m_isBibleImageGenerationRunning = false;
    bool m_deferredBibleRefresh = false;
    
    int m_characterCount = 0;
    int m_sceneCount = 0;
};

#endif // NOVELDETAILPAGE_H
