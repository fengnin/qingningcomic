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
#include <initializer_list>
#include "Novel.h"
#include "Character.h"
#include "Scene.h"
#include "StoryboardService.h"
#include "AnalysisService.h"
#include "ImageService.h"
#include "BibleImageService.h"
#include "components/ChapterSpinBox.h"
#include "components/ModeComboBox.h"
#include "components/ChapterCard.h"
#include "components/StoryboardItem.h"
#include "components/BibleItem.h"
#include "components/PanelCard.h"
#include "components/AnalysisProgressWidget.h"
#include "components/AnalysisResultWidget.h"
#include "AnalysisStatusManager.h"

struct BibleEntryData {
    QString name;
    QStringList details;
    BibleType type = BibleType::Character;
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
    void onBibleItemEditClicked(const QString &name);
    void onBibleItemDataChanged(const QString &name, const QStringList &details);
    void onBibleItemUploadClicked(const QString &name, BibleType type);
    void onBibleItemDeleteImageClicked(const QString &name, BibleType type);
    QString extractBibleValue(const QString &detail, const QString &key) const;
    void onPanelCardClicked(int panelNumber);
    void onStoryboardDataChanged(const QString &panelId, int panelNumber, const QString &scene,
                                  const QString &shotType, const QString &cameraAngle,
                                  const QString &characters, const QString &dialogue,
                                  const QString &visualPrompt, const QString &visualPromptEn);
    void onBibleImageBatchProgress(int current, int total, const QString& type);
    void onAllBibleImagesCompleted(int successCount, int failedCount);
    void onAllImageGenerationCompleted();

private:
    void initColorConstants();
    void setupUI();
    
    QWidget* createTransparentWidget();
    QLabel* createLabel(const QString &text, const QString &color, int fontSize, bool bold = false);
    QLabel* createSectionLabel(const QString &text);
    QPushButton* createButton(const QString &text, const QString &style, int width, int height);
    QPushButton* createFeatureButton(const QString &text, int width = -1);
    void setupLayout(QLayout *layout, int left, int top, int right, int bottom, int spacing = 0);
    void applyCardShadow(QWidget *card);
    
    QFrame* createFeatureCardFrame();
    QWidget* createCardHeader(const QString &title, const QString &subtitle);
    QWidget* createButtonStatusRow(QPushButton *btn, const QString &statusText);
    QWidget* createButtonRow(QPushButton *&btn, const QString &btnText, const QString &statusText);
    void updateButtonStatus(QPushButton *btn, const QString &text, const QString &color);
    void updateChapterHints(const QList<Storyboard>& storyboards);
    void updateChapterUI(int targetChapter);
    QLabel* createCompactTag(const QString &text, int fontSize = 12);
    QWidget* createHBoxLayoutRow(const std::initializer_list<QWidget*> &widgets, int spacing = 8);
    void refreshBibleUI();
    void startAutoImageGeneration(int chapter);
    
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
    void refreshStoryboardItems();
    void refreshPanelPreview();
    PanelCard* createPanelCard(int panelNum, const QString& description, const QString& panelId = QString(), const QString& previewUrl = QString());
    void populatePanelPreviewWithSample();
    QList<QPair<int, QStringList>> loadStoryboardFromDatabase() const;
    QList<QPair<int, QStringList>> getSampleStoryboardItems() const;
    QPair<int, QStringList> parsePanelToItem(const Panel& panel) const;
    void clearLayout(QLayout *layout);
    void updateChapterSelection(int chapterNumber);
    
    void setAnalysisStatus(AnalysisStatusManager::Status status, const QString& extraInfo = QString());
    QString getChapterText(int chapterNumber) const;
    bool validateChapterInput(const QString& text) const;
    void handleAnalysisSuccess(int chapter);
    void handleAnalysisFailure(const QString& errorMessage);
    bool isCurrentNovel(const QString& novelId) const;
    void createRunningJobRecord(int chapterNumber);
    
    void clearBibleContainer(QWidget *container, const QString& logPrefix);
    void populateBibleContainer(QVBoxLayout *layout, const QList<BibleEntry>& entries, 
                                 BibleType type, const QString& emptyText);
    
    QStringList buildCharacterDetails(const Character& character) const;
    void addBibleItemToLayout(QVBoxLayout* layout, const QString& name, 
                              const QStringList& details, BibleType type);
    void populateCharacterBible(QVBoxLayout* layout, const QList<Character>& characters);
    void populateSceneBible(QVBoxLayout* layout, const QList<Scene>& scenes);
    
    QJsonArray parseCharactersToJson(const QString& characters);
    QJsonArray parseDialogueToJson(const QString& dialogue);
    
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
    QPushButton *m_generatePanelsBtn;
    QTextEdit *m_changeRequestEdit;
    QPushButton *m_submitChangeRequestBtn;
    
    QPushButton *m_refreshBibleBtn;
    QLabel *m_characterCountLabel;
    QLabel *m_sceneCountLabel;
    QWidget *m_characterBibleContainer;
    QWidget *m_sceneBibleContainer;
    
    QWidget *m_panelPreviewContainer;
    QLabel *m_panelPreviewTitleLabel;
    QLabel *m_panelPreviewHintLabel;
    
    QWidget *m_storyboardContainer;
    QLabel *m_storyboardCountLabel;
    
    AnalysisProgressWidget *m_analysisProgress;
    AnalysisProgressWidget *m_panelGenerateProgress;
    AnalysisResultWidget *m_analysisResult;
    AnalysisStatusManager::Status m_analysisStatus;
    
    Novel m_currentNovel;
    int m_currentChapter;
    int m_completedChapterCount;
    QString m_currentJobId;
    QMap<QPushButton*, QLabel*> m_statusLabelMap;
    
    // 图片生成状态
    QList<Character> m_pendingCharacters;
    QList<Scene> m_pendingScenes;
    int m_totalImageTasks;
    int m_completedImageTasks;
};

#endif // NOVELDETAILPAGE_H
