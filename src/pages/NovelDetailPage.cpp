/**
 * @file NovelDetailPage.cpp
 * - ChapterSpinBox: components/ChapterSpinBox.cpp
 * - ModeComboBox: components/ModeComboBox.cpp
 * - ChapterCard: components/ChapterCard.cpp
 * - StoryboardItem: components/StoryboardItem.cpp
 * - BibleItem: components/BibleItem.cpp
 * - PanelCard: components/PanelCard.cpp
 */

#include "pages/NovelDetailPage.h"
#include "utils/StyleManager.h"
#include "viewmodels/StoryboardViewModel.h"
#include "viewmodels/NovelViewModel.h"
#include "services/NovelService.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "services/BibleGenerator.h"
#include "services/BibleImageService.h"
#include "services/ImageService.h"
#include "data/FileStorage.h"
#include "models/Bible.h"
#include "models/Panel.h"
#include "utils/EncodingUtils.h"
#include "utils/Logger.h"
#include "components/SuccessDialog.h"
#include "components/ConfirmDialog.h"
#include "data/DatabaseManager.h"
#include "components/EditorStyles.h"
#include "components/AnalysisProgressWidget.h"
#include "components/AnalysisResultWidget.h"
#include "components/ImageViewerDialog.h"
#include "services/AnalysisStatusManager.h"
#include "services/ServiceContainer.h"
#include "utils/StatusHelper.h"
#include "services/ChangeRequestService.h"
#include "utils/BibleUtils.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QElapsedTimer>
#include <QApplication>
#include <QScreen>
#include <QFont>
#include <QPalette>
#include <QRegularExpression>
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QFileInfo>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QSet>


namespace {
    using namespace EditorStyles;
    using EditorStyles::UI::setupLayout;
    
    // ========== 按钮样式 ==========
    const QString ARROW_BTN_STYLE = R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #9333EA;
            font-size: 10px;
            border-radius: 4px;
        }
        QPushButton:hover { 
            background: #F3E8FF; 
        }
        QPushButton:pressed { 
            background: #E9D5FF; 
        }
    )";
    
    // ========== 输入框样式 ==========
    const QString INPUT_LEFT_STYLE = R"(
        QLineEdit {
            border-radius: 10px 0 0 10px;
            border: 1px solid #E5E7EB;
            border-right: none;
            background: white;
            padding: 0 14px;
            font-size: 14px;
            color: #374151;
        }
        QLineEdit:focus { 
            border-color: #9333EA; 
        }
    )";
    
    const QString BTN_CONTAINER_RIGHT_STYLE = R"(
        QWidget {
            background: white;
            border: 1px solid #E5E7EB;
            border-left: none;
            border-radius: 0 10px 10px 0;
        }
    )";
    
    // ========== 派生样式 ==========
    const QString SPIN_BTN_STYLE = ARROW_BTN_STYLE;
    const QString DROP_BTN_STYLE = ARROW_BTN_STYLE;
    const QString SPIN_EDIT_STYLE = INPUT_LEFT_STYLE + "QLineEdit:read-only { color: #374151; }";
    const QString COMBO_EDIT_STYLE = INPUT_LEFT_STYLE + "QLineEdit:read-only { color: #374151; }";
    const QString SPIN_CONTAINER_STYLE = BTN_CONTAINER_RIGHT_STYLE;
    const QString COMBO_CONTAINER_STYLE = BTN_CONTAINER_RIGHT_STYLE;
    
    // ========== 菜单样式 ==========
    const QString MENU_STYLE = R"(
        QMenu {
            background: white;
            border: 1px solid #E5E7EB;
            border-radius: 10px;
            padding: 6px;
        }
        QMenu::item {
            padding: 10px 18px;
            border-radius: 6px;
            font-size: 14px;
            color: #374151;
        }
        QMenu::item:selected {
            background: #F3E8FF;
            color: #9333EA;
        }
    )";
    
    // ========== 标签样式 ==========
    const QString LABEL_STYLE = "QLabel#chapterTitle {"
        "font-size: 16px; font-weight: bold; color: #333333; background: transparent; border: none; }"
        "QLabel#chapterMeta {"
        "font-size: 14px; color: #666666; background: transparent; border: none; }";
    
    // ========== 编辑器按钮样式 ==========
    const QString EDITOR_CLOSE_BTN_STYLE = R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #9CA3AF;
            font-size: 16px;
            border-radius: 14px;
        }
        QPushButton:hover {
            background: #F3F4F6;
            color: #6B7280;
        }
        QPushButton:pressed {
            background: #E5E7EB;
        }
    )";
    
    const QString EDITOR_CANCEL_BTN_STYLE = R"(
        QPushButton {
            padding: 0 28px;
            font-size: 14px;
            font-weight: 500;
            border: 1px solid #E5E7EB;
            border-radius: 10px;
            background: white;
            color: #6B7280;
            height: 44px;
        }
        QPushButton:hover {
            background: #F9FAFB;
            border-color: #D1D5DB;
            color: #374151;
        }
    )";
    
    const QString EDITOR_SAVE_BTN_STYLE = R"(
        QPushButton {
            padding: 0 28px;
            font-size: 14px;
            font-weight: 600;
            border: none;
            border-radius: 10px;
            background: #9333EA;
            color: white;
            height: 44px;
        }
        QPushButton:hover {
            background: #7E22CE;
        }
        QPushButton:pressed {
            background: #6B21A8;
        }
    )";
    
    // ========== 编辑器输入样式 ==========
    const QString EDITOR_INPUT_STYLE = R"(
        QLineEdit {
            padding: 0 14px;
            font-size: 13px;
            border: 1px solid #E5E7EB;
            border-radius: 10px;
            background: white;
            height: 40px;
            color: #374151;
        }
        QLineEdit:focus {
            border-color: #9333EA;
            background: white;
        }
        QLineEdit::placeholder {
            color: #9CA3AF;
        }
    )";
    
    const QString EDITOR_TEXT_EDIT_STYLE = R"(
        QTextEdit {
            padding: 12px 14px;
            font-size: 13px;
            border: 1px solid #E5E7EB;
            border-radius: 10px;
            background: white;
            color: #374151;
            line-height: 1.5;
        }
        QTextEdit:focus {
            border-color: #9333EA;
            background: white;
        }
        QTextEdit::placeholder {
            color: #9CA3AF;
        }
    )";
    
    const QString EDITOR_COMBO_STYLE = R"(
        QComboBox {
            padding: 0 12px;
            font-size: 13px;
            border: none;
            border-radius: 8px;
            background: #F9FAFB;
            height: 40px;
        }
        QComboBox:hover {
            background: #F3F4F6;
        }
        QComboBox:focus {
            background: white;
        }
        QComboBox::drop-down {
            border: none;
            width: 32px;
            subcontrol-position: center right;
        }
        QComboBox::down-arrow {
            image: none;
        }
        QComboBox QAbstractItemView {
            background: white;
            border: 1px solid #E5E7EB;
            border-radius: 8px;
            selection-background-color: #F3E8FF;
            selection-color: #333333;
            padding: 4px;
        }
        QComboBox QAbstractItemView::item {
            height: 36px;
            padding: 0 12px;
            border-radius: 6px;
        }
        QComboBox QAbstractItemView::item:hover {
            background: #F3E8FF;
        }
    )";
    
    const QString EDITOR_COMBO_ARROW_STYLE = R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #9333EA;
            font-size: 10px;
            border-radius: 4px;
            min-width: 24px;
            max-width: 24px;
        }
        QPushButton:hover {
            background: #F3E8FF;
        }
        QPushButton:pressed {
            background: #E9D5FF;
        }
    )";
    
    // ========== 初始化数据 ==========
    const QStringList SAMPLE_PANEL_DESCRIPTIONS = {
        QString::fromUtf8("主角站在窗前，凝视远方"),
        QString::fromUtf8("两人在咖啡厅对话，气氛紧张"),
        QString::fromUtf8("雨夜街道，霓虹灯闪烁"),
        QString::fromUtf8("教室场景，阳光透过窗户洒入"),
        QString::fromUtf8("战斗场景，能量爆发"),
    };
    
    // ========== 圣经图片辅助函数 ==========
    void saveCharacterBibleImage(const QString& characterId, const QString& imagePath)
    {
        Character character = CharacterExtractor::instance()->getCharacterById(characterId);
        if (character.id().isEmpty()) {
            return;
        }

        character.setPortraitPath(imagePath);
        CharacterExtractor::instance()->updateCharacter(character);
    }

    void saveSceneBibleImage(const QString& novelId, const QString& sceneId, const QString& imagePath)
    {
        Scene scene = SceneExtractor::instance()->getSceneBySceneId(novelId, sceneId);
        if (scene.id().isEmpty()) {
            return;
        }

        scene.setReferenceImagePath(imagePath);
        SceneExtractor::instance()->updateScene(scene);
    }

    void deleteCharacterBibleImage(const QString& characterId)
    {
        Character character = CharacterExtractor::instance()->getCharacterById(characterId);
        if (character.id().isEmpty()) {
            return;
        }

        const QString targetPath = character.portraitPath();
        if (targetPath.isEmpty()) {
            return;
        }

        FileStorage::instance()->deleteReferenceImage(targetPath);
        character.setPortraitPath(QString());
        CharacterExtractor::instance()->updateCharacter(character);
    }

    void deleteSceneBibleImage(const QString& novelId, const QString& sceneId)
    {
        Scene scene = SceneExtractor::instance()->getSceneBySceneId(novelId, sceneId);
        if (scene.id().isEmpty()) {
            return;
        }

        const QString targetPath = scene.referenceImagePath();
        if (targetPath.isEmpty()) {
            return;
        }

        FileStorage::instance()->deleteReferenceImage(targetPath);
        scene.setReferenceImagePath(QString());
        SceneExtractor::instance()->updateScene(scene);
    }

    // ========== 辅助函数 ==========
    QWidget* createTransparentWidget()
    {
        QWidget *widget = new QWidget();
        widget->setStyleSheet(TRANSPARENT_BG);
        return widget;
    }
    
    QLabel* createLabel(const QString &text, const QString &color, int fontSize, bool bold = false)
    {
        QLabel *label = new QLabel(text);
        label->setStyleSheet(QString("font-size: %1px; color: %2; background: transparent;").arg(fontSize).arg(color));
        label->setFont(QFont("Microsoft YaHei", fontSize, bold ? QFont::Bold : QFont::Normal));
        return label;
    }
    
    QPushButton* createButton(const QString &text, const QString &style, int width, int height)
    {
        QPushButton *btn = new QPushButton(text);
        btn->setStyleSheet(style);
        if (width > 0) btn->setFixedWidth(width);
        btn->setFixedHeight(height);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    }
    
    QPushButton* createFeatureButton(const QString &text, int width = -1)
    {
        QPushButton *btn = new QPushButton(text);
        if (width > 0) {
            btn->setFixedSize(width, EditorStyles::Constants::BTN_HEIGHT);
        } else {
            btn->setFixedHeight(EditorStyles::Constants::BTN_HEIGHT);
        }
        btn->setStyleSheet(featureButtonStyle());
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    }
    
    void applyCardShadow(QWidget *card)
    {
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(12);
        shadow->setColor(QColor(0, 0, 0, 20));
        shadow->setOffset(0, 2);
        card->setGraphicsEffect(shadow);
    }
}

NovelDetailPage::NovelDetailPage(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_metaLabel(nullptr)
    , m_analyzeBtn(nullptr)
    , m_viewExportsBtn(nullptr)
    , m_exportFormatCombo(nullptr)
    , m_exportBtn(nullptr)
    , m_exportStatusLabel(nullptr)
    , m_chapterContainer(nullptr)
    , m_chapterNumberSpin(nullptr)
    , m_chapterHintLabel(nullptr)
    , m_chapterTextEdit(nullptr)
    , m_addChapterBtn(nullptr)
    , m_addChapterStatusLabel(nullptr)
    , m_generateModeCombo(nullptr)
    , m_generatePanelsBtn(nullptr)
    , m_changeRequestEdit(nullptr)
    , m_submitChangeRequestBtn(nullptr)
    , m_storyboardContainer(nullptr)
    , m_storyboardCountLabel(nullptr)
    , m_analysisProgress(nullptr)
    , m_panelGenerateProgress(nullptr)
    , m_analysisResult(nullptr)
    , m_analysisStatus(AnalysisStatusManager::Status::Ready)
    , m_currentChapter(1)
    , m_completedChapterCount(2)
{
    initColorConstants();
    setupUI();
    setupConnections();
}

void NovelDetailPage::setupConnections()
{
    StoryboardViewModel* vm = StoryboardViewModel::instance();

    connect(vm, &StoryboardViewModel::storyboardsLoaded,
            this, &NovelDetailPage::onStoryboardsLoaded, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::storyboardLoaded,
            this, &NovelDetailPage::onStoryboardLoaded, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::panelsLoaded,
            this, &NovelDetailPage::onPanelsLoaded, Qt::QueuedConnection);
    
    connect(vm, &StoryboardViewModel::analysisCompleted,
            this, &NovelDetailPage::onAnalysisCompleted, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::analysisFailed,
            this, &NovelDetailPage::onAnalysisFailed, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::analysisStarted,
            this, &NovelDetailPage::onAnalysisStarted, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::analysisProgress,
            this, [this](const QString& stage, int progress) {
                if (m_analysisProgress) {
                    m_analysisProgress->setState(AnalysisProgressWidget::State::Processing);
                    m_analysisProgress->setProgress(progress);
                    m_analysisProgress->setProgressText(stage);
                }
            }, Qt::QueuedConnection);
    
    connect(TaskQueue::instance(), &TaskQueue::taskProgress,
            this, [this](const QString& taskId, int progress, const QString& message) {
                Q_UNUSED(taskId);
                if (m_panelGenerateProgress) {
                    m_panelGenerateProgress->setProgress(progress);
                    m_panelGenerateProgress->setProgressText(message);
                }
            }, Qt::QueuedConnection);
    
    connect(TaskQueue::instance(), &TaskQueue::taskCompleted,
            this, [this](const QString& taskId, const QJsonObject& result) {
                Q_UNUSED(taskId);
                if (m_panelGenerateProgress) {
                    m_panelGenerateProgress->setState(AnalysisProgressWidget::State::Completed);
                    m_panelGenerateProgress->setResult(result);
                }
                
                m_generatePanelsBtn->setEnabled(true);
                m_generatePanelsBtn->setText(tr("开始生成"));
                
                QTimer::singleShot(100, this, [this]() {
                    if (m_panelPreviewWidget) {
                        m_panelPreviewWidget->refresh();
                    }
                });
            }, Qt::QueuedConnection);
    
    connect(TaskQueue::instance(), &TaskQueue::taskFailed,
            this, [this](const QString& taskId, const QString& error) {
                Q_UNUSED(taskId);
                if (m_panelGenerateProgress) {
                    m_panelGenerateProgress->setState(AnalysisProgressWidget::State::Failed);
                    m_panelGenerateProgress->setProgressText(error);
                }
                
                m_generatePanelsBtn->setEnabled(true);
                m_generatePanelsBtn->setText(tr("开始生成"));
            }, Qt::QueuedConnection);
    
    connect(CharacterExtractor::instance(), &CharacterExtractor::characterUpdated,
            this, [this](const QString&, const QString&) {
                if (m_currentNovel.id().isEmpty()) return;
                onBibleDataChanged();
            }, Qt::QueuedConnection);
    
    connect(SceneExtractor::instance(), &SceneExtractor::sceneUpdated,
            this, [this](const QString&, const QString&) {
                if (m_currentNovel.id().isEmpty()) return;
                onBibleDataChanged();
            }, Qt::QueuedConnection);

    connect(BibleImageService::instance(), &BibleImageService::allBibleImagesCompleted,
            this, &NovelDetailPage::onAllBibleImagesCompleted, Qt::QueuedConnection);
}

NovelDetailPage::~NovelDetailPage()
{
    if (TaskQueue::instance()) {
        disconnect(TaskQueue::instance(), &TaskQueue::taskProgress, this, nullptr);
        disconnect(TaskQueue::instance(), &TaskQueue::taskCompleted, this, nullptr);
        disconnect(TaskQueue::instance(), &TaskQueue::taskFailed, this, nullptr);
    }
    
    if (ImageService::instance()) {
        disconnect(ImageService::instance(), nullptr, this, nullptr);
    }
    
    if (BibleImageService::instance()) {
        disconnect(BibleImageService::instance(), nullptr, this, nullptr);
    }
}

void NovelDetailPage::initColorConstants()
{
    m_colorTitle = "#333333";
    m_colorSubtitle = "#999999";
    m_colorText = "#333333";
    m_colorHint = "#666666";
    m_colorLabel = "#666666";
}

void NovelDetailPage::setNovel(const Novel &novel)
{
    LOG_DEBUG("NovelDetailPage", QString("setNovel: %1, id=%2").arg(novel.title()).arg(novel.id()));
    m_currentNovel = novel;
    m_deferredBibleRefresh = false;
    
    updateDisplay();
    
    if (m_bibleSectionWidget) {
        m_bibleSectionWidget->setNovelId(novel.id());
    }
}

void NovelDetailPage::setChapterNumber(int chapterNumber)
{
    if (m_currentChapter == chapterNumber) {
        return;
    }
    m_currentChapter = chapterNumber;
    if (m_chapterNumberSpin) {
        m_chapterNumberSpin->setValue(chapterNumber);
    }
    if (m_panelPreviewWidget) {
        m_panelPreviewWidget->setChapter(chapterNumber);
    }
    updateDisplay();
}


QLabel* NovelDetailPage::createSectionLabel(const QString &text)
{
    return createLabel(text, m_colorText, 14);
}


QFrame* NovelDetailPage::createFeatureCardFrame()
{
    QFrame *card = new QFrame();
    card->setObjectName("featureCard");
    card->setStyleSheet(cardStyle());
    applyCardShadow(card);
    return card;
}

QWidget* NovelDetailPage::createCardHeader(const QString &title, const QString &subtitle)
{
    QWidget *headerRow = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    setupLayout(headerLayout, 0, 0, 0, 0, 8);
    
    headerLayout->addWidget(createLabel(title, m_colorTitle, 18, true));
    headerLayout->addStretch();
    
    QLabel *subtitleLabel = createLabel(subtitle, m_colorSubtitle, 14);
    subtitleLabel->setWordWrap(true);
    subtitleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    headerLayout->addWidget(subtitleLabel);
    
    return headerRow;
}

QWidget* NovelDetailPage::createButtonStatusRow(QPushButton *btn, const QString &statusText)
{
    QWidget *row = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(row);
    setupLayout(layout, 0, 0, 0, 0, 12);
    
    if (btn) {
        btn->setMinimumWidth(EditorStyles::Constants::BTN_CREATE_WIDTH);
    }
    
    layout->addWidget(btn);
    layout->addStretch();
    layout->addWidget(createLabel(statusText, m_colorText, 14));
    
    return row;
}

QWidget* NovelDetailPage::createButtonRow(QPushButton *&btn, const QString &btnText, const QString &statusText)
{
    QWidget *btnRow = createTransparentWidget();
    QHBoxLayout *btnLayout = new QHBoxLayout(btnRow);
    setupLayout(btnLayout, 0, 0, 0, 0, 12);
    
    btn = createFeatureButton(btnText);
    btn->setMinimumWidth(EditorStyles::Constants::BTN_CREATE_WIDTH);
    btnLayout->addWidget(btn);
    btnLayout->addStretch();
    
    QLabel *statusLabel = createLabel(statusText, m_colorText, 14);
    btnLayout->addWidget(statusLabel);
    
    m_statusLabelMap[btn] = statusLabel;
    
    return btnRow;
}

void NovelDetailPage::updateButtonStatus(QPushButton *btn, const QString &text, const QString &color)
{
    if (m_statusLabelMap.contains(btn)) {
        m_statusLabelMap[btn]->setText(text);
        m_statusLabelMap[btn]->setStyleSheet(QString("color: %1; font-size: 14px;").arg(color));
    }
}

void NovelDetailPage::updateChapterHints(const QList<Storyboard>& storyboards)
{
    QSet<int> existingChapters;
    for (const Storyboard& sb : storyboards) {
        existingChapters.insert(sb.chapterNumber());
    }
    
    if (m_chapterNumberSpin) {
        m_chapterNumberSpin->setExistingChapters(existingChapters);
    }
    
    int nextChapter = 1;
    while (existingChapters.contains(nextChapter)) {
        nextChapter++;
    }
    
    updateChapterUI(nextChapter);
}

void NovelDetailPage::updateChapterUI(int targetChapter)
{
    if (m_chapterHintLabel) {
        m_chapterHintLabel->setText(
            QStringLiteral("已完成 %1 章，下一章是 %2").arg(m_completedChapterCount).arg(targetChapter));
    }
    if (m_addChapterBtn) {
        m_addChapterBtn->setText(QStringLiteral("添加章节 %1").arg(targetChapter));
    }
}

QLabel* NovelDetailPage::createCompactTag(const QString &text, int fontSize)
{
    QLabel *tag = new QLabel(text);
    tag->setStyleSheet(QString("background: #F5F5F5; border-radius: 8px; padding: 4px 8px; font-size: %1px; color: #666666;").arg(fontSize));
    tag->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    return tag;
}

QWidget* NovelDetailPage::createHBoxLayoutRow(const std::initializer_list<QWidget*> &widgets, int spacing)
{
    QWidget *row = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(spacing);
    for (QWidget *widget : widgets) {
        layout->addWidget(widget);
    }
    return row;
}

QScrollArea* NovelDetailPage::createScrollArea(Qt::ScrollBarPolicy vPolicy, Qt::ScrollBarPolicy hPolicy)
{
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(scrollAreaStyle());
    scrollArea->setVerticalScrollBarPolicy(vPolicy);
    scrollArea->setHorizontalScrollBarPolicy(hPolicy);
    return scrollArea;
}

QScrollArea* NovelDetailPage::createVerticalScrollArea()
{
    return createScrollArea(Qt::ScrollBarAsNeeded, Qt::ScrollBarAlwaysOff);
}

QScrollArea* NovelDetailPage::createHorizontalScrollArea()
{
    return createScrollArea(Qt::ScrollBarAlwaysOff, Qt::ScrollBarAsNeeded);
}

QWidget* NovelDetailPage::createBibleCard(const QString &title, QLabel *countLabel, 
                                            QWidget *container, int minHeight)
{
    QFrame *card = new QFrame();
    card->setObjectName("bibleCard");
    card->setStyleSheet(bibleCardStyle());
    
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    setupLayout(cardLayout, 16, 16, 16, 16, 12);
    
    QWidget *headerRow = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    setupLayout(headerLayout, 0, 0, 0, 0, 0);
    
    headerLayout->addWidget(createLabel(title, m_colorTitle, 16, true));
    headerLayout->addStretch();
    headerLayout->addWidget(countLabel);
    cardLayout->addWidget(headerRow);
    
    QScrollArea *scrollArea = createVerticalScrollArea();
    scrollArea->setMinimumHeight(minHeight);
    scrollArea->setWidget(container);
    cardLayout->addWidget(scrollArea);
    
    return card;
}


void NovelDetailPage::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    setupLayout(m_mainLayout, 24, 24, 24, 24, 20);

    m_mainScrollArea = new QScrollArea(this);
    m_mainScrollArea->setWidgetResizable(true);
    m_mainScrollArea->setFrameShape(QFrame::NoFrame);
    m_mainScrollArea->setStyleSheet(scrollAreaStyle());
    m_mainScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mainScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_mainScrollArea->verticalScrollBar()->setStyleSheet(scrollBarStyle());
    m_mainScrollArea->horizontalScrollBar()->setStyleSheet(scrollBarStyle());

    QWidget *contentWidget = createTransparentWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    setupLayout(contentLayout, 0, 0, 0, 0, 20);

    contentLayout->addWidget(createHeaderSection());
    contentLayout->addWidget(createChapterSection());
    contentLayout->addWidget(createFeatureSection());
    contentLayout->addWidget(createBibleSection());
    contentLayout->addWidget(createStoryboardSection());
    contentLayout->addStretch();

    m_mainScrollArea->setWidget(contentWidget);
    m_mainLayout->addWidget(m_mainScrollArea);
}


QWidget* NovelDetailPage::createHeaderSection()
{
    QFrame *header = new QFrame();
    header->setObjectName("headerSection");
    header->setStyleSheet(headerSectionStyle());
    
    QHBoxLayout *layout = new QHBoxLayout(header);
    setupLayout(layout, 24, 20, 24, 20, 12);
    
    m_titleLabel = createLabel(tr("作品详情"), m_colorTitle, 28, true);
    m_statusLabel = createStatusLabel("completed");
    m_metaLabel = createLabel(tr("创作信息"), m_colorHint, 14);
    
    m_analyzeBtn = createButton(tr("分析分镜"), secondaryButtonStyle(), 100, EditorStyles::Constants::BTN_HEIGHT);
    connect(m_analyzeBtn, &QPushButton::clicked, this, &NovelDetailPage::onAnalyzeClicked);
    
    m_viewExportsBtn = createButton(tr("查看导出"), primaryButtonStyle(), 130, EditorStyles::Constants::BTN_HEIGHT);
    connect(m_viewExportsBtn, &QPushButton::clicked, this, &NovelDetailPage::onViewExportsClicked);
    
    layout->addWidget(m_titleLabel);
    layout->addSpacing(8);
    layout->addWidget(m_statusLabel);
    layout->addSpacing(16);
    layout->addWidget(m_metaLabel);
    layout->addWidget(createTransparentWidget(), 1);
    layout->addWidget(m_analyzeBtn);
    layout->addWidget(m_viewExportsBtn);
    
    return header;
}

QWidget* NovelDetailPage::createChapterSection()
{
    QFrame *section = new QFrame();
    section->setObjectName("chapterSection");
    section->setStyleSheet(chapterSectionStyle());
    
    QVBoxLayout *layout = new QVBoxLayout(section);
    setupLayout(layout, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_SPACING);
    
    QWidget *headerRow = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    setupLayout(headerLayout, 0, 0, 0, 0, 0);
    
    QWidget *titleContainer = createTransparentWidget();
    QVBoxLayout *titleLayout = new QVBoxLayout(titleContainer);
    setupLayout(titleLayout, 0, 0, 0, 0, 4);
    titleLayout->addWidget(createLabel(tr("章节管理"), "#212121", 16, true));
    m_chapterCountLabel = createLabel(tr("共 0 章"), m_colorHint, 14);
    titleLayout->addWidget(m_chapterCountLabel);
    
    QPushButton *refreshBtn = createButton(tr("刷新"), refreshButtonStyle(), -1, EditorStyles::Constants::BTN_HEIGHT);
    refreshBtn->setObjectName("refreshListBtn");
    connect(refreshBtn, &QPushButton::clicked, this, &NovelDetailPage::onRefreshChaptersClicked);
    
    headerLayout->addWidget(titleContainer);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshBtn);
    layout->addWidget(headerRow);
    
    m_chapterScrollArea = new QScrollArea();
    m_chapterScrollArea->setWidgetResizable(true);
    m_chapterScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_chapterScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chapterScrollArea->setStyleSheet(horizontalScrollAreaStyle());
    m_chapterScrollArea->setFixedHeight(120);
    
    m_chapterContainer = createTransparentWidget();
    m_chapterContainerLayout = new QHBoxLayout(m_chapterContainer);
    setupLayout(m_chapterContainerLayout, 0, 0, 0, 0, 16);
    
    m_chapterScrollArea->setWidget(m_chapterContainer);
    layout->addWidget(m_chapterScrollArea);
    
    return section;
}

QWidget* NovelDetailPage::createFeatureSection()
{
    QWidget *section = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(section);
    setupLayout(layout, 0, 0, 0, 0, EditorStyles::Constants::FEATURE_CARD_SPACING);
    
    QWidget *cards[] = {
        createAddChapterCard(), 
        createGeneratePanelsCard(), 
        createChangeRequestCard(),
        createExportCard()
    };
    for (QWidget *card : cards) {
        card->setMinimumWidth(280);
        layout->addWidget(card, 1);
    }
    
    return section;
}


QWidget* NovelDetailPage::createAddChapterCard()
{
    QFrame *card = createFeatureCardFrame();
    QVBoxLayout *layout = new QVBoxLayout(card);
    setupLayout(layout, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, 16);
    
    layout->addWidget(createCardHeader(tr("追加新章节"), tr("复用当前圣经，保持角色/场景连续性")));
    
    layout->addWidget(createSectionLabel(tr("章节编号")));
    
    m_chapterNumberSpin = new ChapterSpinBox();
    m_chapterNumberSpin->setMinimum(1);
    m_chapterNumberSpin->setMaximum(9999);
    m_chapterNumberSpin->setValue(m_completedChapterCount + 1);
    m_chapterNumberSpin->setFixedHeight(EditorStyles::Constants::BTN_HEIGHT);
    connect(m_chapterNumberSpin, &ChapterSpinBox::valueChanged, this, &NovelDetailPage::onChapterNumberChanged);
    layout->addWidget(m_chapterNumberSpin);
    
    m_chapterHintLabel = createLabel(
        QString("当前已完成 %1 章，默认生成第 %2 章")
            .arg(m_completedChapterCount)
            .arg(m_completedChapterCount + 1),
        m_colorHint, 12);
    layout->addWidget(m_chapterHintLabel);
    
    layout->addWidget(createSectionLabel(tr("章节内容")));
    
    m_chapterTextEdit = new QTextEdit();
    m_chapterTextEdit->setPlaceholderText(tr("输入章节正文内容..."));
    m_chapterTextEdit->setMinimumHeight(100);
    m_chapterTextEdit->setStyleSheet(inputStyle());
    layout->addWidget(m_chapterTextEdit);
    
    QWidget *btnRow = createButtonRow(m_addChapterBtn, QString("添加章节 %1").arg(m_completedChapterCount + 1), tr("就绪"));
    connect(m_addChapterBtn, &QPushButton::clicked, this, &NovelDetailPage::onAddChapterClicked);
    layout->addWidget(btnRow);
    
    m_analysisProgress = new AnalysisProgressWidget();
    layout->addWidget(m_analysisProgress);
    
    m_analysisResult = new AnalysisResultWidget();
    layout->addWidget(m_analysisResult);
    
    QLabel *hintLabel = createLabel(tr("逐个修订场景描述与 Prompt，保存后即可复用最新文本生成面板"), m_colorHint, 12);
    hintLabel->setWordWrap(true);
    layout->addWidget(hintLabel);
    
    return card;
}

QWidget* NovelDetailPage::createGeneratePanelsCard()
{
    QFrame *card = createFeatureCardFrame();
    QVBoxLayout *layout = new QVBoxLayout(card);
    setupLayout(layout, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING,
                EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, 16);
    
    layout->addWidget(createCardHeader(tr("面板批量生成"), tr("AI一键生成漫画面板")));
    
    layout->addWidget(createSectionLabel(tr("生成模式")));
    
    m_generateModeCombo = new ModeComboBox();
    m_generateModeCombo->addItem(tr("1:1（1024×1024）"));
    m_generateModeCombo->addItem(tr("3:2（1536×1024）"));
    m_generateModeCombo->addItem(tr("16:9（1280×720）"));
    m_generateModeCombo->setFixedHeight(EditorStyles::Constants::BTN_HEIGHT);
    layout->addWidget(m_generateModeCombo);

    layout->addSpacing(8);

    m_generatePanelsBtn = nullptr;
    QWidget *btnRow = createButtonRow(m_generatePanelsBtn, tr("开始生成"), tr("就绪"));
    connect(m_generatePanelsBtn, &QPushButton::clicked, this, &NovelDetailPage::onGeneratePanelsClicked);
    layout->addWidget(btnRow);

    QLabel *ratioHintLabel = createLabel(
        tr("提示: 生成过程可能需要较长时间，请耐心等待。"),
        m_colorHint, 12);
    ratioHintLabel->setWordWrap(true);
    layout->addWidget(ratioHintLabel);

    m_panelGenerateProgress = new AnalysisProgressWidget();
    layout->addWidget(m_panelGenerateProgress);
    
    layout->addStretch();
    return card;
}

QWidget* NovelDetailPage::createChangeRequestCard()
{
    QFrame *card = createFeatureCardFrame();
    QVBoxLayout *layout = new QVBoxLayout(card);
    setupLayout(layout, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, 16);
    
    layout->addWidget(createCardHeader(tr("自然语言修改请求"), tr("自动解析 CR-DSL 并执行修改闭环")));
    
    layout->addWidget(createSectionLabel(tr("修改指令")));
    
    m_changeRequestEdit = new QTextEdit();
    m_changeRequestEdit->setPlaceholderText(tr("输入自然语言修改需求，说明要改的页码、面板和内容。"));
    m_changeRequestEdit->setFixedHeight(66);
    m_changeRequestEdit->setStyleSheet(inputStyle());
    layout->addWidget(m_changeRequestEdit);
    
    layout->addSpacing(14);
    
    QWidget *btnRow = createButtonRow(m_submitChangeRequestBtn, tr("提交修改请求"), tr("任务状态 就绪"));
    connect(m_submitChangeRequestBtn, &QPushButton::clicked, this, &NovelDetailPage::onSubmitChangeRequestClicked);
    layout->addWidget(btnRow);
    
    layout->addWidget(createLabel(tr("系统会解析修改请求并生成对应的 CR-DSL 变更。"), m_colorHint, 12));
    layout->addStretch();
    
    return card;
}

QWidget* NovelDetailPage::createExportCard()
{
    QFrame *card = createFeatureCardFrame();
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    setupLayout(cardLayout, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, 16);
    
    cardLayout->addWidget(createCardHeader(tr("导出高清成品"), tr("PDF / Webtoon 长图 / 资源包")));
    
    cardLayout->addWidget(createSectionLabel(tr("导出格式")));
    
    m_exportFormatCombo = new ModeComboBox();
    m_exportFormatCombo->addItem("PDF");
    m_exportFormatCombo->addItem(tr("图片包"));
    m_exportFormatCombo->addItem(tr("ZIP压缩包"));
    m_exportFormatCombo->setFixedHeight(EditorStyles::Constants::BTN_HEIGHT);
    cardLayout->addWidget(m_exportFormatCombo);
    
    m_exportBtn = createFeatureButton(tr("开始导出"));
    connect(m_exportBtn, &QPushButton::clicked, this, &NovelDetailPage::onExportClicked);
    
    cardLayout->addWidget(createButtonStatusRow(m_exportBtn, tr("就绪")));
    cardLayout->addStretch();
    
    return card;
}


QWidget* NovelDetailPage::createBibleSection()
{
    QWidget *section = createTransparentWidget();
    
    QVBoxLayout *sectionLayout = new QVBoxLayout(section);
    setupLayout(sectionLayout, 0, 0, 0, 0, EditorStyles::Constants::CARD_SPACING);
    
    m_bibleSectionWidget = new BibleSectionWidget();
    
    connect(m_bibleSectionWidget, &BibleSectionWidget::bibleItemEditRequested,
            this, &NovelDetailPage::onBibleItemEditClicked);
    connect(m_bibleSectionWidget, &BibleSectionWidget::bibleItemDataChanged,
            this, &NovelDetailPage::onBibleItemDataChanged);
    connect(m_bibleSectionWidget, &BibleSectionWidget::bibleItemImageUpdated,
            this, &NovelDetailPage::onBibleItemUploadClicked);
    connect(m_bibleSectionWidget, &BibleSectionWidget::bibleItemDeleteRequested,
            this, &NovelDetailPage::onBibleItemDeleteRequested);
    connect(m_bibleSectionWidget, &BibleSectionWidget::characterCountChanged,
            this, &NovelDetailPage::onCharacterCountChanged);
    connect(m_bibleSectionWidget, &BibleSectionWidget::sceneCountChanged,
            this, &NovelDetailPage::onSceneCountChanged);
    connect(m_bibleSectionWidget, &BibleSectionWidget::characterDataChanged,
            this, &NovelDetailPage::onCharacterDataChanged);
    connect(m_bibleSectionWidget, &BibleSectionWidget::sceneDataChanged,
            this, &NovelDetailPage::onSceneDataChanged);
    
    m_bibleSectionWidget->setNovelId(m_currentNovel.id());
    
    sectionLayout->addWidget(m_bibleSectionWidget);
    
    m_panelPreviewWidget = new PanelPreviewWidget();
    m_panelPreviewWidget->setChapter(m_currentChapter);
    
    connect(m_panelPreviewWidget, &PanelPreviewWidget::panelClicked,
            this, &NovelDetailPage::onPanelCardClicked);
    
    sectionLayout->addWidget(m_panelPreviewWidget);
    
    return section;
}

/**
 */
QWidget* NovelDetailPage::createStoryboardSection()
{
    QWidget *section = createTransparentWidget();
    
    QVBoxLayout *sectionLayout = new QVBoxLayout(section);
    setupLayout(sectionLayout, 0, 0, 0, 0, EditorStyles::Constants::CARD_SPACING);
    
    QWidget *headerRow = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    setupLayout(headerLayout, 0, 0, 0, 0, 0);
    
    QWidget *titleGroup = createTransparentWidget();
    QVBoxLayout *titleLayout = new QVBoxLayout(titleGroup);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(4);
    
    QLabel *sectionTitle = createLabel(tr("分镜文本编辑"), m_colorTitle, 18, true);
    titleLayout->addWidget(sectionTitle);
    
    m_storyboardCountLabel = createLabel(tr("共 0 个分镜"), m_colorHint, 12);
    titleLayout->addWidget(m_storyboardCountLabel);
    
    headerLayout->addWidget(titleGroup);
    headerLayout->addStretch();
    
    QLabel *hintLabel = createLabel(tr("提示: 完成章节分析后自动生成分镜脚本。"), m_colorHint, 14);
    hintLabel->setWordWrap(true);
    hintLabel->setMaximumWidth(300);
    headerLayout->addWidget(hintLabel);
    
    sectionLayout->addWidget(headerRow);
    
    sectionLayout->addWidget(createStoryboardCard());
    
    return section;
}

/**
 */
QWidget* NovelDetailPage::createStoryboardCard()
{
    QFrame *card = new QFrame();
    card->setObjectName("storyboardCard");
    card->setStyleSheet(storyboardCardStyle());
    
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    setupLayout(cardLayout, 20, 20, 20, 16);
    
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(scrollAreaStyle());
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setMinimumHeight(400);
    
    m_storyboardContainer = new QWidget();
    m_storyboardContainer->setStyleSheet(TRANSPARENT_BG);
    m_storyboardContainer->setMinimumWidth(400);
    
    QVBoxLayout *containerLayout = new QVBoxLayout(m_storyboardContainer);
    containerLayout->setContentsMargins(0, 0, 8, 0);
    containerLayout->setSpacing(16);
    
    QList<QPair<int, QStringList>> storyboardItems = {
        {1, {"第1章", "分镜A", "分镜B", "分镜C", "分镜D"}},
        {2, {"第2章", "分镜A", "分镜B", "分镜C", "分镜D"}},
        {3, {"第3章", "分镜A", "分镜B", "分镜C", "分镜D"}},
        {4, {"第4章", "分镜A", "分镜B", "分镜C", "分镜D"}}
    };
    
    for (const auto &item : storyboardItems) {
        const QStringList parts = item.second;
        StoryboardItem *storyboardItem = new StoryboardItem(
            item.first,
            parts.value(0),
            parts.value(1),
            parts.value(2),
            parts.value(3),
            parts.value(4),
            parts.value(5),
            parts.value(6),
            parts.value(7)
        );
        connect(storyboardItem, &StoryboardItem::dataChanged, this, &NovelDetailPage::onStoryboardDataChanged);
        containerLayout->addWidget(storyboardItem);
    }
    
    containerLayout->addStretch();
    
    scrollArea->setWidget(m_storyboardContainer);
    cardLayout->addWidget(scrollArea);
    
    return card;
}


QLabel* NovelDetailPage::createStatusLabel(const QString &status)
{
    QLabel *label = new QLabel();
    QString baseStyle = "padding: 6px 12px; border-radius: 16px; font-size: 12px; background: transparent;";
    
    StatusHelper::StatusStyle style = StatusHelper::Novel::style(status);
    label->setText(style.text);
    label->setStyleSheet(baseStyle + QString("background-color: %1; color: %2;").arg(style.bgColor, style.textColor));
    
    return label;
}

ChapterCard* NovelDetailPage::createChapterCard(int chapterNumber, int panelCount, 
                                                  const QString &status, bool isActive)
{
    ChapterCard *card = new ChapterCard(chapterNumber, panelCount, status, isActive);
    connect(card, &ChapterCard::clicked, this, &NovelDetailPage::onChapterClicked);
    connect(card, &ChapterCard::deleteRequested, this, &NovelDetailPage::onChapterDeleteRequested);
    m_chapterCards.append(card);
    return card;
}

void NovelDetailPage::updateDisplay()
{
    if (!m_titleLabel || !m_metaLabel) {
        LOG_WARNING("NovelDetailPage", "updateDisplay: UI components not initialized");
        return;
    }
    
    if (m_currentNovel.id().isEmpty()) {
        m_titleLabel->setText(tr("作品详情"));
        m_metaLabel->setText(tr("请先选择一个作品"));
        return;
    }
    
    m_titleLabel->setText(m_currentNovel.title());
    
    StoryboardViewModel* vm = StoryboardViewModel::instance();
    vm->loadStoryboards(m_currentNovel.id());
    vm->loadStoryboard(m_currentNovel.id(), m_currentChapter);

    QList<Storyboard> storyboards;
    if (vm->hasCachedStoryboards(m_currentNovel.id())) {
        storyboards = vm->storyboards();
    }
    
    m_completedChapterCount = storyboards.size();
    
    int maxChapterNumber = 0;
    for (const Storyboard& storyboard : storyboards) {
        if (storyboard.chapterNumber() > maxChapterNumber) {
            maxChapterNumber = storyboard.chapterNumber();
        }
    }
    int nextChapterNumber = maxChapterNumber > 0 ? maxChapterNumber + 1 : 1;
    
    m_metaLabel->setText(QString("作品 ID: %1 | 已完成章节: %2 | 当前章节: %3")
        .arg(m_currentNovel.id())
        .arg(m_completedChapterCount)
        .arg(m_currentChapter));
    
    if (m_chapterCountLabel) {
        m_chapterCountLabel->setText(QString("章节数: %1").arg(m_completedChapterCount));
    }
    
    if (m_chapterNumberSpin) {
        if (m_chapterNumberSpin->value() != nextChapterNumber) {
            m_chapterNumberSpin->setValue(nextChapterNumber);
        }
    }
    
    updateChapterHints(storyboards);
    
    if (vm->isAnalyzing()) {
        setAnalysisStatus(AnalysisStatusManager::Status::Processing);
    } else {
        setAnalysisStatus(AnalysisStatusManager::Status::Ready);
        if (m_analysisProgress) {
            m_analysisProgress->reset();
        }
        if (m_analysisResult) {
            m_analysisResult->hide();
        }
    }
    
    refreshChapterCards(storyboards);
    refreshStoryboardItems();
    
    QTimer::singleShot(0, this, [this]() {
        updateBibleMetaLabel();
    });
}

void NovelDetailPage::updateBibleMetaLabel()
{
    if (m_currentNovel.id().isEmpty()) {
        return;
    }
    
    LOG_DEBUG("NovelDetailPage", QString("updateBibleMetaLabel: chapters=%1, characters=%2, scenes=%3")
        .arg(m_completedChapterCount).arg(m_characterCount).arg(m_sceneCount));
    
    m_metaLabel->setText(QString("作品 ID: %1 | 已完成章节: %2 | 当前章节: %3 | 角色: %4 | 场景: %5")
        .arg(m_currentNovel.id())
        .arg(m_completedChapterCount)
        .arg(m_currentChapter)
        .arg(m_characterCount)
        .arg(m_sceneCount));

}

void NovelDetailPage::refreshChapterCards(const QList<Storyboard>& storyboards)
{
    if (!m_chapterContainerLayout) return;
    
    clearChapterCards();
    
    for (const Storyboard& storyboard : storyboards) {
        ChapterCard *card = createChapterCard(
            storyboard.chapterNumber(), 
            storyboard.panelCount(), 
            "generated", 
            storyboard.chapterNumber() == m_currentChapter
        );
        card->setMinimumWidth(200);
        card->setMaximumWidth(250);
        m_chapterContainerLayout->addWidget(card);
    }
    
    m_chapterContainerLayout->addStretch();
}

void NovelDetailPage::clearChapterCards()
{
    for (ChapterCard *card : m_chapterCards) {
        card->disconnect();
        card->setParent(nullptr);
        delete card;
    }
    m_chapterCards.clear();
    
    if (m_chapterContainerLayout) {
        clearLayout(m_chapterContainerLayout);
    }
}

void NovelDetailPage::refreshChapterCardsOnly()
{
    if (!m_chapterContainerLayout) return;
    
    StoryboardViewModel* vm = StoryboardViewModel::instance();
    vm->loadStoryboards(m_currentNovel.id());
    QList<Storyboard> storyboards = vm->storyboards();
    refreshChapterCards(storyboards);
    
    if (m_chapterCountLabel) {
        m_chapterCountLabel->setText(QString("章节数: %1").arg(storyboards.size()));
    }
    
    int maxChapterNumber = 0;
    for (const Storyboard& storyboard : storyboards) {
        if (storyboard.chapterNumber() > maxChapterNumber) {
            maxChapterNumber = storyboard.chapterNumber();
        }
    }
    
    int nextChapterNumber = maxChapterNumber > 0 ? maxChapterNumber + 1 : 1;
    if (m_chapterNumberSpin) {
        m_chapterNumberSpin->setValue(nextChapterNumber);
    }
    
    updateChapterHints(storyboards);
}

void NovelDetailPage::onChapterClicked(int chapterNumber)
{
    m_currentChapter = chapterNumber;
    updateDisplay();
}

void NovelDetailPage::onChapterDeleteRequested(int chapterNumber)
{
    if (!ConfirmDialog::showConfirm(this, tr("确认删除"),
            QString("确定删除第 %1 章吗？此操作不可恢复。").arg(chapterNumber))) {
        return;
    }
    
    int savedMainVScrollPos = 0;
    int savedChapterHScrollPos = 0;
    
    if (m_mainScrollArea) {
        savedMainVScrollPos = m_mainScrollArea->verticalScrollBar()->value();
    }
    if (m_chapterScrollArea) {
        savedChapterHScrollPos = m_chapterScrollArea->horizontalScrollBar()->value();
    }
    
    if (!StoryboardViewModel::instance()->deleteStoryboard(m_currentNovel.id(), chapterNumber)) {
        SuccessDialog::showError(this, tr("删除失败"), tr("无法删除章节 %1").arg(chapterNumber));
        return;
    }
    
    if (m_currentChapter == chapterNumber) {
        m_currentChapter = 1;
    }
    m_completedChapterCount--;
    
    setUpdatesEnabled(false);
    
    refreshChapterCardsOnly();
    
    setUpdatesEnabled(true);
    
    QTimer::singleShot(0, this, [this, savedMainVScrollPos, savedChapterHScrollPos]() {
        if (m_mainScrollArea) {
            m_mainScrollArea->verticalScrollBar()->setValue(savedMainVScrollPos);
        }
        if (m_chapterScrollArea) {
            QScrollBar *hScrollBar = m_chapterScrollArea->horizontalScrollBar();
            int newPos = qMin(savedChapterHScrollPos, hScrollBar->maximum());
            hScrollBar->setValue(newPos);
        }
    });
}

QList<QPair<int, QStringList>> NovelDetailPage::getSampleStoryboardItems() const
{
    return {
        {1, {"第1章", "分镜A", "分镜B", "分镜C", "分镜D"}},
        {2, {"第2章", "分镜A", "分镜B", "分镜C", "分镜D"}},
        {3, {"第3章", "分镜A", "分镜B", "分镜C", "分镜D"}},
        {4, {"第4章", "分镜A", "分镜B", "分镜C", "分镜D"}}
    };
}

QPair<int, QStringList> NovelDetailPage::parsePanelToItem(const Panel& panel) const
{
    QJsonObject content = panel.rawContent();
    
    QString scene = content["scene"].toString();
    if (scene.isEmpty()) {
        scene = content["description"].toString();
    }
    
    QString shotType = content["shotType"].toString();
    if (shotType.isEmpty()) {
        shotType = content["shot_type"].toString();
    }
    if (shotType.isEmpty()) {
        shotType = content["shotTypeEn"].toString();
    }
    
    QString cameraAngle = content["cameraAngle"].toString();
    if (cameraAngle.isEmpty()) {
        cameraAngle = content["camera_angle"].toString();
    }
    
    QStringList charInfos;
    QJsonArray charArray = content["characters"].toArray();
    
    for (const QJsonValue& c : charArray) {
        QJsonObject charObj = c.toObject();
        QString name = charObj["name"].toString();
        QString pose = charObj["pose"].toString();
        QString expression = charObj["expression"].toString();
        
        QString info = name;
        QStringList notes;
        if (!pose.isEmpty()) notes << pose;
        if (!expression.isEmpty()) notes << expression;
        if (!notes.isEmpty()) {
            info += QString(" (%1)").arg(notes.join(", "));
        }
        charInfos << info;
    }
    
    QStringList dialogueInfos;
    QJsonArray dialogueArray = content["dialogue"].toArray();
    
    for (const QJsonValue& d : dialogueArray) {
        QJsonObject dialogueObj = d.toObject();
        QString speaker = dialogueObj["speaker"].toString();
        QString text = dialogueObj["text"].toString();
        
        if (!speaker.isEmpty() && speaker != "narration") {
            dialogueInfos << QString("%1: %2").arg(speaker, text);
        } else if (!text.isEmpty()) {
            dialogueInfos << text;
        }
    }
    
    QString visualPrompt = panel.visualPrompt();
    QString visualPromptEn = panel.visualPromptEn();
    
    int panelNum = (panel.page() - 1) * 6 + panel.index() + 1;
    
    return {panelNum, {panel.id(), scene, shotType, cameraAngle, charInfos.join(" | "), dialogueInfos.join(" | "), visualPrompt, visualPromptEn}};
}

void NovelDetailPage::clearLayout(QLayout *layout)
{
    if (!layout) return;
    
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            PanelCard *panelCard = qobject_cast<PanelCard*>(widget);
            if (panelCard) {
                panelCard->disconnect();
            } else {
                widget->disconnect();
            }
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
}

void NovelDetailPage::refreshStoryboardItems()
{
    if (!m_storyboardContainer) {
        LOG_WARNING("NovelDetailPage", "refreshStoryboardItems: m_storyboardContainer is null");
        return;
    }
    
    QVBoxLayout *containerLayout = qobject_cast<QVBoxLayout*>(m_storyboardContainer->layout());
    if (!containerLayout) {
        LOG_WARNING("NovelDetailPage", "refreshStoryboardItems: containerLayout is null");
        return;
    }
    
    clearLayout(containerLayout);
    
    QList<QPair<int, QStringList>> storyboardItems = loadStoryboardFromDatabase();

    int createdCount = 0;
    for (const auto &item : storyboardItems) {
        const QStringList parts = item.second;
        StoryboardItem *storyboardItem = new StoryboardItem(
            item.first,
            parts.value(0),
            parts.value(1),
            parts.value(2),
            parts.value(3),
            parts.value(4),
            parts.value(5),
            parts.value(6),
            parts.value(7)
        );
        connect(storyboardItem, &StoryboardItem::dataChanged, this, &NovelDetailPage::onStoryboardDataChanged);
        containerLayout->addWidget(storyboardItem);
        createdCount++;
    }
    
    if (m_storyboardCountLabel) {
        m_storyboardCountLabel->setText(QString(tr("共 %1 个分镜")).arg(createdCount));
    }
    
    containerLayout->addStretch();
    
    if (m_panelPreviewWidget) {
        QList<Panel> panels = StoryboardViewModel::instance()->currentPanels();
        m_panelPreviewWidget->setPanels(panels);
    }
}

QList<QPair<int, QStringList>> NovelDetailPage::loadStoryboardFromDatabase() const
{
    QList<QPair<int, QStringList>> items;
    QString novelId = m_currentNovel.id();
    
    if (novelId.isEmpty()) {
        return items;
    }

    StoryboardViewModel* vm = StoryboardViewModel::instance();
    if (!vm->hasCachedPanels(novelId, m_currentChapter)) {
        return items;
    }
    
    QList<Panel> panels = vm->currentPanels();
    
    for (const Panel& panel : panels) {
        items.append(parsePanelToItem(panel));
    }
    
    return items;
}

void NovelDetailPage::updateChapterSelection(int chapterNumber)
{
    m_currentChapter = chapterNumber;
    for (ChapterCard *card : m_chapterCards) {
        card->setActive(card->chapterNumber() == chapterNumber);
    }
    refreshStoryboardItems();
    
    if (m_panelPreviewWidget) {
        m_panelPreviewWidget->setChapter(chapterNumber);
    }
}

void NovelDetailPage::onStoryboardsLoaded(const QString& novelId, const QList<Storyboard>& storyboards)
{
    if (novelId != m_currentNovel.id()) {
        return;
    }

    m_completedChapterCount = storyboards.size();

    int maxChapterNumber = 0;
    for (const Storyboard& storyboard : storyboards) {
        if (storyboard.chapterNumber() > maxChapterNumber) {
            maxChapterNumber = storyboard.chapterNumber();
        }
    }

    const int nextChapterNumber = maxChapterNumber > 0 ? maxChapterNumber + 1 : 1;
    refreshChapterCards(storyboards);
    updateChapterHints(storyboards);

    if (m_chapterCountLabel) {
        m_chapterCountLabel->setText(QString("章节数: %1").arg(m_completedChapterCount));
    }
    if (m_chapterNumberSpin && m_chapterNumberSpin->value() != nextChapterNumber) {
        m_chapterNumberSpin->setValue(nextChapterNumber);
    }

    updateBibleMetaLabel();
}

void NovelDetailPage::onStoryboardLoaded(const QString& novelId, int chapterNumber, const Storyboard& storyboard)
{
    if (novelId != m_currentNovel.id() || chapterNumber != m_currentChapter) {
        return;
    }

    Q_UNUSED(storyboard)
    if (m_panelPreviewWidget) {
        m_panelPreviewWidget->setChapter(chapterNumber);
    }
}

void NovelDetailPage::onPanelsLoaded(const QString& novelId, int chapterNumber, const QList<Panel>& panels)
{
    if (novelId != m_currentNovel.id() || chapterNumber != m_currentChapter) {
        return;
    }

    Q_UNUSED(panels)
    refreshStoryboardItems();
}


void NovelDetailPage::onAnalysisStarted(const QString& novelId)
{
    if (novelId != m_currentNovel.id()) {
        return;
    }
    
    setAnalysisStatus(AnalysisStatusManager::Status::Processing);
    
    if (m_analysisProgress && m_analysisProgress->currentState() == AnalysisProgressWidget::State::Idle) {
        m_analysisProgress->reset();
        m_analysisProgress->setState(AnalysisProgressWidget::State::Connecting);
    }
    
    if (m_analysisResult) {
        m_analysisResult->hide();
    }
}

void NovelDetailPage::onBackClicked()
{
    emit backClicked();
}

void NovelDetailPage::onAnalyzeClicked()
{
    if (!AnalysisStatusManager::instance()->canStartAnalysis(m_analysisStatus)) {
        SuccessDialog::showWarning(this, tr("分析进行中"), tr("请等待当前分析完成后再开始新的分析"));
        return;
    }
    
    QString originalText = m_currentNovel.originalText();
    if (originalText.isEmpty()) {
        originalText = NovelService::instance()->loadText(m_currentNovel.id());
    }
    
    if (originalText.isEmpty()) {
        SuccessDialog::showWarning(this, tr("内容为空"), tr("小说原文为空，无法进行分析"));
        return;
    }
    
    int targetChapter = m_currentChapter > 0 ? m_currentChapter : 1;
    
    ConfirmDialog dialog(this);
    dialog.setTitle(tr("确认分析"));
    dialog.setMessage(QString("为第 %1 章生成分镜？").arg(targetChapter));
    
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    
    setAnalysisStatus(AnalysisStatusManager::Status::Processing);
    
    if (m_analysisProgress) {
        m_analysisProgress->reset();
        m_analysisProgress->setState(AnalysisProgressWidget::State::Connecting);
    }
    
    if (m_analysisResult) {
        m_analysisResult->hide();
    }
    
    QJsonArray existingCharacters = BibleGenerator::instance()->collectExistingCharacters(m_currentNovel.id());
    QJsonArray existingScenes = BibleGenerator::instance()->collectExistingScenes(m_currentNovel.id());
    
    StoryboardViewModel::instance()->startAnalysisWithBible(
        m_currentNovel.id(), 
        originalText, 
        targetChapter, 
        existingCharacters, 
        existingScenes
    );
}

void NovelDetailPage::onViewExportsClicked()
{
    emit navigateToExportRequested();
}

void NovelDetailPage::onRefreshChaptersClicked()
{
    QPushButton *refreshBtn = findChild<QPushButton*>("refreshListBtn");
    if (!refreshBtn) {
        refreshBtn = qobject_cast<QPushButton*>(sender());
    }
    
    QString originalText;
    if (refreshBtn) {
        originalText = refreshBtn->text();
        refreshBtn->setText(tr("刷新中..."));
    }
    
    QApplication::processEvents();
    
    updateDisplay();
    
    if (refreshBtn && !originalText.isEmpty()) {
        refreshBtn->setText(originalText);
    }
}

void NovelDetailPage::onAddChapterClicked()
{
    if (!AnalysisStatusManager::instance()->canStartAnalysis(m_analysisStatus)) {
        SuccessDialog::showWarning(this, tr("操作失败"), tr("当前有分析任务正在进行，请稍后再试"));
        return;
    }
    
    int chapterNumber = m_chapterNumberSpin->value();
    QString text = getChapterText(chapterNumber);
    
    if (!validateChapterInput(text)) {
        return;
    }
    
    setAnalysisStatus(AnalysisStatusManager::Status::Processing);
    
    if (m_analysisProgress) {
        m_analysisProgress->reset();
        m_analysisProgress->setState(AnalysisProgressWidget::State::Connecting);
    }
    
    if (m_analysisResult) {
        m_analysisResult->hide();
    }
    
    QString novelId = m_currentNovel.id();
    
    QJsonArray existingCharacters = BibleGenerator::instance()->collectExistingCharacters(novelId);
    QJsonArray existingScenes = BibleGenerator::instance()->collectExistingScenes(novelId);
    
    StoryboardViewModel::instance()->startAnalysisWithBible(novelId, text, chapterNumber, existingCharacters, existingScenes);
}

void NovelDetailPage::createRunningJobRecord(int chapterNumber)
{
    QString jobId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QJsonObject paramsJson;
    paramsJson["chapterNumber"] = chapterNumber;
    QString paramsStr = QString::fromUtf8(QJsonDocument(paramsJson).toJson(QJsonDocument::Compact));
    
    QString sql = QString(
        "INSERT INTO jobs (id, novel_id, type, status, progress, total_tasks, completed_tasks, failed_tasks, params, created_at, updated_at) "
        "VALUES ('%1', '%2', '%3', '%4', %5, %6, %7, %8, CAST('%9' AS JSON), '%10', '%11')")
        .arg(jobId)
        .arg(m_currentNovel.id())
        .arg("generate_storyboard")
        .arg("running")
        .arg(0)
        .arg(1)
        .arg(0)
        .arg(0)
        .arg(paramsStr.replace("'", "''"))
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    
    if (DatabaseManager::instance()->executeSql(sql)) {
        m_currentJobId = jobId;
        LOG_INFO("NovelDetailPage", QString("Running job record created: %1").arg(jobId));
    } else {
        LOG_ERROR("NovelDetailPage", "Failed to create running job record");
    }
}

QString NovelDetailPage::getChapterText(int chapterNumber) const
{
    if (chapterNumber == 1 && !m_currentNovel.originalText().isEmpty()) {
        return m_currentNovel.originalText();
    }
    return m_chapterTextEdit->toPlainText().trimmed();
}

bool NovelDetailPage::validateChapterInput(const QString& text) const
{
    if (text.isEmpty()) {
        SuccessDialog::showWarning(const_cast<NovelDetailPage*>(this), tr("输入为空"), tr("请输入章节内容"));
        return false;
    }
    return true;
}

void NovelDetailPage::setAnalysisStatus(AnalysisStatusManager::Status status, const QString& extraInfo)
{
    m_analysisStatus = status;
    
    QString buttonText = extraInfo;
    if (status == AnalysisStatusManager::Status::Processing) {
        buttonText = tr("分析中...");
    } else if (status == AnalysisStatusManager::Status::Ready) {
        buttonText = QString("添加章节 %1").arg(m_completedChapterCount + 1);
    }
    
    if (m_addChapterBtn && m_statusLabelMap.contains(m_addChapterBtn)) {
        AnalysisStatusManager::instance()->applyButtonStyle(
            m_addChapterBtn, 
            m_statusLabelMap[m_addChapterBtn], 
            status, 
            buttonText
        );
    }
}

bool NovelDetailPage::isCurrentNovel(const QString& novelId) const
{
    return novelId == m_currentNovel.id();
}

void NovelDetailPage::onAnalysisCompleted(const QString& novelId, int chapterNumber)
{
    if (!isCurrentNovel(novelId)) return;
    
    handleAnalysisSuccess(chapterNumber);
}

void NovelDetailPage::handleAnalysisSuccess(int chapter)
{
    setAnalysisStatus(AnalysisStatusManager::Status::Processing, tr("图片生成中..."));
    
    if (m_analysisProgress) {
        m_analysisProgress->setState(AnalysisProgressWidget::State::Processing);
    }
    
    if (m_analysisResult) {
        m_analysisResult->setResult(chapter, 0, 0, 0);
        m_analysisResult->showAnimated();
    }
    
    m_completedChapterCount++;
    m_currentChapter = chapter;
    m_chapterTextEdit->clear();
    
    StoryboardViewModel::instance()->clearCache();
    if (m_bibleSectionWidget) {
        m_bibleSectionWidget->refreshBible();
    }
    
    updateDisplay();
    
}

void NovelDetailPage::handleAnalysisFailure(const QString& errorMessage)
{
    setAnalysisStatus(AnalysisStatusManager::Status::Failed);
    
    if (m_analysisProgress) {
        m_analysisProgress->setState(AnalysisProgressWidget::State::Failed);
        m_analysisProgress->setProgressText(errorMessage);
    }
    
    SuccessDialog::showError(this, tr("分析失败"), tr("分析出错: %1").arg(errorMessage));
}

void NovelDetailPage::onAnalysisFailed(const QString& novelId, const QString& error)
{
    if (!isCurrentNovel(novelId)) return;
    handleAnalysisFailure(error);
}

void NovelDetailPage::onGeneratePanelsClicked()
{
    if (m_currentNovel.id().isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请先选择一个作品"));
        return;
    }
    
    if (ImageService::instance()->isGenerating()) {
        QMessageBox::warning(this, tr("生成中"), tr("正在生成面板，请等待完成"));
        return;
    }
    
    StoryboardViewModel* vm = StoryboardViewModel::instance();
    if (!vm->hasCachedPanels(m_currentNovel.id(), m_currentChapter)) {
        vm->loadStoryboard(m_currentNovel.id(), m_currentChapter);
        QMessageBox::information(this, tr("数据加载中"), tr("分镜数据还在加载，请稍后再试"));
        return;
    }

    Storyboard currentStoryboard = vm->currentStoryboard();
    if (currentStoryboard.id().isEmpty()) {
        QMessageBox::warning(this, tr("无分镜"), tr("当前章节没有分镜数据，请先分析"));
        return;
    }
    
    QList<Panel> panels = vm->currentPanels();
    
    QStringList panelIds;
    for (const Panel& p : panels) {
        panelIds.append(p.id());
    }
    
    if (panelIds.isEmpty()) {
        QMessageBox::warning(this, tr("无面板"), tr("当前分镜没有面板数据"));
        return;
    }
    
    ImageService::BatchPresetMode presetMode = ImageService::BatchPresetMode::Square_1x1;
    if (m_generateModeCombo) {
        switch (m_generateModeCombo->currentIndex()) {
            case 1:
                presetMode = ImageService::BatchPresetMode::Standard_3x2;
                break;
            case 2:
                presetMode = ImageService::BatchPresetMode::Widescreen_16x9;
                break;
            case 0:
            default:
                presetMode = ImageService::BatchPresetMode::Square_1x1;
                break;
        }
    }
    
    m_generatePanelsBtn->setEnabled(false);
    m_generatePanelsBtn->setText(tr("生成中..."));
    
    if (m_panelGenerateProgress) {
        m_panelGenerateProgress->reset();
        m_panelGenerateProgress->setState(AnalysisProgressWidget::State::Processing);
        m_panelGenerateProgress->setProgress(0);
        m_panelGenerateProgress->setProgressText(TR("正在生成面板图像 0/%1").arg(panelIds.size()));
    }
    
    LOG_INFO("NovelDetailPage", QString("Starting batch generation for %1 panels").arg(panelIds.size()));
    
    QString taskId = ImageService::instance()->enqueueBatchPanelImageGeneration(panelIds, presetMode);
    
}
void NovelDetailPage::startAutoImageGeneration(int chapter)
{
    Q_UNUSED(chapter);
    
    QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(m_currentNovel.id());
    QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(m_currentNovel.id());
    
    int totalTasks = characters.size() + scenes.size();
    
    if (totalTasks == 0) {
        onAllImageGenerationCompleted();
        return;
    }

    m_isBibleImageGenerationRunning = true;
    m_deferredBibleRefresh = false;
    
    if (m_analysisProgress) {
        m_analysisProgress->reset();
        m_analysisProgress->setState(AnalysisProgressWidget::State::Processing);
        m_analysisProgress->setProgress(0);
        m_analysisProgress->setProgressText(QString::fromUtf8("正在生成图像 0/%1").arg(totalTasks));
    }
    
    disconnect(BibleImageService::instance(), &BibleImageService::batchProgress, this, nullptr);
    
    connect(BibleImageService::instance(), &BibleImageService::batchProgress,
            this, &NovelDetailPage::onBibleImageBatchProgress, Qt::QueuedConnection);
    
    m_pendingCharacters = characters;
    m_pendingScenes = scenes;
    m_totalImageTasks = totalTasks;
    m_completedImageTasks = 0;
    
    BibleImageService::instance()->generateAllBibleImages(characters, scenes);
}

void NovelDetailPage::onBibleImageBatchProgress(int current, int total, const QString& type)
{
    if (m_analysisProgress) {
        int completed = m_completedImageTasks + current;
        int progress = m_totalImageTasks > 0 ? (completed * 100 / m_totalImageTasks) : 0;
        m_analysisProgress->setProgress(progress);
        m_analysisProgress->setProgressText(
            QString::fromUtf8("正在生成%1 %2/%3")
                .arg(type == "character" ? "角色肖像" : "场景参考图")
                .arg(current)
                .arg(total));
    }
}

void NovelDetailPage::onAllBibleImagesCompleted(int successCount, int failedCount)
{
    m_isBibleImageGenerationRunning = false;
    m_completedImageTasks = successCount + failedCount;
    onAllImageGenerationCompleted();
}

void NovelDetailPage::onAllImageGenerationCompleted()
{
    m_isBibleImageGenerationRunning = false;
    
    if (m_analysisProgress) {
        m_analysisProgress->setState(AnalysisProgressWidget::State::Completed);
        QJsonObject progressResult;
        progressResult["imageCount"] = m_completedImageTasks;
        m_analysisProgress->setResult(progressResult);
    }
    if (m_panelPreviewWidget) {
        m_panelPreviewWidget->refresh();
    }
    applyDeferredBibleRefresh();

    setAnalysisStatus(AnalysisStatusManager::Status::Success);
    NovelViewModel::instance()->updateNovelStatus(m_currentNovel.id(), NovelStatus::Completed);

    m_pendingCharacters.clear();
    m_pendingScenes.clear();
    m_totalImageTasks = 0;
    m_completedImageTasks = 0;
}


void NovelDetailPage::onSubmitChangeRequestClicked()
{
    if (!m_changeRequestEdit) {
        return;
    }
    
    QString naturalLanguage = m_changeRequestEdit->toPlainText().trimmed();
    if (naturalLanguage.isEmpty()) {
        QMessageBox::warning(this, tr("输入为空"), tr("请输入变更内容"));
        return;
    }
    
    if (m_currentNovel.id().isEmpty()) {
        QMessageBox::warning(this, tr("无作品"), tr("未选择任何作品"));
        return;
    }
    
    ChangeRequestService* service = ServiceContainer::instance()->changeRequestService();
    if (!service) {
        QMessageBox::warning(this, tr("服务不可用"), tr("变更请求服务不可用"));
        return;
    }
    
    QJsonObject context;
    context["storyboardId"] = m_currentNovel.storyboardId();
    context["chapterNumber"] = m_chapterNumberSpin->value();
    
    ChangeRequest cr = service->createChangeRequest(
        m_currentNovel.id(),
        naturalLanguage,
        context
    );
    
    if (cr.isValid()) {
        m_changeRequestEdit->clear();
        
        QMessageBox::information(this, tr("提交成功"),
            QString("变更请求已提交: %1").arg(cr.id()));
        
        disconnect(service, &ChangeRequestService::changeRequestCompleted, this, nullptr);
        disconnect(service, &ChangeRequestService::changeRequestFailed, this, nullptr);
        
        connect(service, &ChangeRequestService::changeRequestCompleted,
                this, [this](const QString& crId, const QJsonObject& result) {
            Q_UNUSED(crId);
            QMessageBox::information(this, tr("执行完成"),
                QString("变更请求状态: %1").arg(result["status"].toString()));
            refreshStoryboardItems();
        });
        
        connect(service, &ChangeRequestService::changeRequestFailed,
                this, [this](const QString& crId, const QString& error) {
            Q_UNUSED(crId);
            QMessageBox::warning(this, tr("执行失败"), QString("失败: %1").arg(error));
        });
        
        service->executeChangeRequestAsync(cr.id());
    } else {
        QMessageBox::warning(this, tr("服务错误"), QString("服务错误: %1").arg(service->lastError()));
    }
}

void NovelDetailPage::onChapterNumberChanged(int value)
{
    if (m_currentChapter == value) {
        return;
    }
    updateChapterUI(value);
}

void NovelDetailPage::onStoryboardDataChanged(const QString &panelId, int panelNumber, 
                                               const QString &scene, const QString &shotType, 
                                               const QString &cameraAngle,
                                               const QString &characters, const QString &dialogue,
                                               const QString &visualPrompt, const QString &visualPromptEn)
{
    Q_UNUSED(panelNumber);
    
    if (panelId.isEmpty()) {
        return;
    }
    
    Panel panel = StoryboardViewModel::instance()->getPanel(panelId);
    if (panel.id().isEmpty()) {
        return;
    }
    
    QJsonObject content = panel.rawContent();
    content["scene"] = scene;
    content["shotType"] = shotType;
    content["cameraAngle"] = cameraAngle;
    content["characters"] = parseCharactersToJson(characters);
    content["dialogue"] = parseDialogueToJson(dialogue);
    
    if (!visualPrompt.isEmpty()) {
        content["visualPrompt"] = visualPrompt;
    }
    if (!visualPromptEn.isEmpty()) {
        content["visualPromptEn"] = visualPromptEn;
    }
    
    StoryboardViewModel::instance()->updatePanel(panelId, content);
}

QJsonArray NovelDetailPage::parseCharactersToJson(const QString& characters)
{
    QJsonArray charArray;
    QStringList charItems = characters.split("|", Qt::SkipEmptyParts);
    
    for (const QString &item : charItems) {
        QString trimmed = item.trimmed();
        if (trimmed.isEmpty()) continue;
        
        QJsonObject charObj;
        QRegularExpression re("([^(]+)\\s*(?:\\(([^)]+)\\))?");
        QRegularExpressionMatch match = re.match(trimmed);
        
        if (match.hasMatch()) {
            QString name = match.captured(1).trimmed();
            QString notes = match.captured(2).trimmed();
            
            charObj["name"] = name;
            
            if (!notes.isEmpty()) {
                QStringList noteList = notes.split(",", Qt::SkipEmptyParts);
                if (!noteList.isEmpty()) {
                    charObj["pose"] = noteList.value(0).trimmed();
                }
                if (noteList.size() >= 2) {
                    charObj["expression"] = noteList.value(1).trimmed();
                }
            }
        } else {
            charObj["name"] = trimmed;
        }
        
        charArray.append(charObj);
    }
    
    return charArray;
}

QJsonArray NovelDetailPage::parseDialogueToJson(const QString& dialogue)
{
    QJsonArray dialogueArray;
    QStringList dialogueItems = dialogue.split("|", Qt::SkipEmptyParts);
    
    for (const QString &item : dialogueItems) {
        QString trimmed = item.trimmed();
        if (trimmed.isEmpty()) continue;
        
        QJsonObject dialogueObj;
        int colonPos = trimmed.indexOf(":");
        
        if (colonPos > 0) {
            dialogueObj["speaker"] = trimmed.left(colonPos).trimmed();
            dialogueObj["text"] = trimmed.mid(colonPos + 1).trimmed();
        } else {
            dialogueObj["speaker"] = "narration";
            dialogueObj["text"] = trimmed;
        }
        
        dialogueArray.append(dialogueObj);
    }
    
    return dialogueArray;
}

void NovelDetailPage::onRefreshBibleClicked()
{
    if (m_bibleSectionWidget) {
        m_bibleSectionWidget->refreshBible();
    }
}

bool NovelDetailPage::isBibleGenerating() const
{
    return m_isBibleImageGenerationRunning || BibleImageService::instance()->isGenerating();
}

void NovelDetailPage::requestBibleRefresh()
{
    if (isBibleGenerating()) {
        if (!m_deferredBibleRefresh) {
            m_deferredBibleRefresh = true;
            LOG_DEBUG("NovelDetailPage", "Defer bible refresh until batch image generation completes");
        }
        return;
    }

    m_deferredBibleRefresh = false;
    if (m_bibleSectionWidget) {
        m_bibleSectionWidget->refreshBible();
    }
}

void NovelDetailPage::applyDeferredBibleRefresh()
{
    if (m_deferredBibleRefresh) {
        LOG_DEBUG("NovelDetailPage", "Applying deferred bible refresh after batch image generation");
    }
    m_deferredBibleRefresh = false;

    if (m_bibleSectionWidget) {
        m_bibleSectionWidget->refreshBible();
    }
}

void NovelDetailPage::onBibleDataChanged()
{
    requestBibleRefresh();
}

void NovelDetailPage::onCharacterCountChanged(int count)
{
    LOG_DEBUG("NovelDetailPage", QString("onCharacterCountChanged: %1 -> %2").arg(m_characterCount).arg(count));
    m_characterCount = count;
    updateBibleMetaLabel();
}

void NovelDetailPage::onSceneCountChanged(int count)
{
    LOG_DEBUG("NovelDetailPage", QString("onSceneCountChanged: %1 -> %2").arg(m_sceneCount).arg(count));
    m_sceneCount = count;
    updateBibleMetaLabel();
}

void NovelDetailPage::onBibleItemEditClicked(const QString &id, BibleType type)
{
    if (id.isEmpty()) {
        return;
    }
    
    LOG_INFO("NovelDetailPage", QString("Bible item edit clicked: id=%1, type=%2").arg(id).arg(static_cast<int>(type)));
    
    if (type == BibleType::Character) {
        Character character = CharacterExtractor::instance()->getCharacterById(id);
        if (!character.id().isEmpty()) {
            LOG_INFO("NovelDetailPage", QString("Editing character: %1").arg(character.name()));
        }
    } else {
        Scene scene = SceneExtractor::instance()->getSceneBySceneId(m_currentNovel.id(), id);
        if (!scene.id().isEmpty()) {
            LOG_INFO("NovelDetailPage", QString("Editing scene: %1").arg(scene.name()));
        }
    }
}

void NovelDetailPage::onBibleItemDataChanged(const QString &id, const QStringList &details)
{
    Character character = CharacterExtractor::instance()->getCharacterById(id);
    if (!character.id().isEmpty()) {
        CharacterAppearance app = character.appearance();
        
        for (const QString &detail : details) {
            const QString gender = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("gender"), QString::fromUtf8("性别")});
            if (!gender.isEmpty()) {
                app.gender = gender;
            }

            const int age = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("age"), QString::fromUtf8("年龄")}).toInt();
            if (age > 0) {
                app.age = age;
            }

            const QString hairColor = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("hairColor"), QString::fromUtf8("发色")});
            if (!hairColor.isEmpty()) {
                app.hairColor = hairColor;
            }

            const QString eyeColor = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("eyeColor"), QString::fromUtf8("瞳色"), QString::fromUtf8("眼睛")});
            if (!eyeColor.isEmpty()) {
                app.eyeColor = eyeColor;
            }

            const QString bodyType = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("bodyType"), QString::fromUtf8("体型")});
            if (!bodyType.isEmpty()) {
                app.build = bodyType;
            }

            const QString hairStyle = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("hairStyle"), QString::fromUtf8("发型")});
            if (!hairStyle.isEmpty()) {
                app.hairStyle = hairStyle;
            }

            const QString clothing = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("clothing"), QString::fromUtf8("服饰")});
            if (!clothing.isEmpty()) {
                app.clothing = BibleUtils::splitCommaSeparatedList(clothing);
            }

            const QString accessories = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("accessories"), QString::fromUtf8("配饰")});
            if (!accessories.isEmpty()) {
                app.accessories = BibleUtils::splitCommaSeparatedList(accessories);
            }

            const QString features = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("features"), QStringLiteral("distinctiveFeatures"), QString::fromUtf8("明显特征"), QString::fromUtf8("特征")});
            if (!features.isEmpty()) {
                app.distinctiveFeatures = BibleUtils::splitCommaSeparatedList(features);
            }

            const QString personality = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("personality"), QString::fromUtf8("个性"), QString::fromUtf8("性格")});
            if (!personality.isEmpty()) {
                character.setPersonality(BibleUtils::splitCommaSeparatedList(personality));
            }
        }
        
        character.setAppearance(app);
        CharacterExtractor::instance()->updateCharacter(character);
        return;
    }
    
    Scene scene = SceneExtractor::instance()->getSceneBySceneId(m_currentNovel.id(), id);
    if (!scene.id().isEmpty()) {
        SceneDetails det = scene.details();

        for (const QString &detail : details) {
            const QString description = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("description"), QString::fromUtf8("描述")});
            if (!description.isEmpty()) {
                det.description = description;
            }

            const QString building = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("building"), QString::fromUtf8("建筑")});
            if (!building.isEmpty()) {
                det.building = building;
            }

            const QString color = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("color"), QString::fromUtf8("色调")});
            if (!color.isEmpty()) {
                det.color = color;
            }

            const QString landmark = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("landmark"), QString::fromUtf8("地标")});
            if (!landmark.isEmpty()) {
                det.landmark = landmark;
            }

            const QString layout = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("layout"), QString::fromUtf8("布局")});
            if (!layout.isEmpty()) {
                det.layout = layout;
            }

            const QString atmosphere = BibleUtils::extractDetailValue(detail, QStringList{QStringLiteral("atmosphere"), QString::fromUtf8("氛围")});
            if (!atmosphere.isEmpty()) {
                det.atmosphere = atmosphere;
            }
        }

        scene.setDetails(det);
        SceneExtractor::instance()->updateScene(scene);
    }
}

void NovelDetailPage::onCharacterDataChanged(const QString &id, const Character &character)
{
    Q_UNUSED(id)
    CharacterExtractor::instance()->updateCharacterSilent(character);
}

void NovelDetailPage::onSceneDataChanged(const QString &id, const Scene &scene)
{
    Q_UNUSED(id)
    SceneExtractor::instance()->updateSceneSilent(scene);
}

void NovelDetailPage::onBibleItemUploadClicked(const QString &id, const QString &imagePath, BibleType type)
{
    if (imagePath.isEmpty()) {
        onBibleItemDeleteImageClicked(id, type);
        return;
    }
    
    QString relativePath = FileStorage::instance()->saveReferenceImage(imagePath, m_currentNovel.id());
    if (relativePath.isEmpty()) {
        QMessageBox::warning(this, tr("保存失败"), tr("无法保存图片"));
        return;
    }
    
    if (type == BibleType::Character) {
        saveCharacterBibleImage(id, relativePath);
    } else {
        saveSceneBibleImage(m_currentNovel.id(), id, relativePath);
    }

    requestBibleRefresh();
}

void NovelDetailPage::onBibleItemDeleteImageClicked(const QString &id, BibleType type)
{
    bool confirmed = ConfirmDialog::showConfirm(this, tr("确认删除"), tr("确定删除此图片？"));
    if (!confirmed) {
        return;
    }
    
    if (type == BibleType::Character) {
        deleteCharacterBibleImage(id);
    } else {
        deleteSceneBibleImage(m_currentNovel.id(), id);
    }

    requestBibleRefresh();
}

void NovelDetailPage::onBibleItemDeleteRequested(const QString &id, BibleType type)
{
    QString title = (type == BibleType::Character) ? tr("删除角色") : tr("删除场景");
    QString message = (type == BibleType::Character)
        ? tr("确定删除此角色？关联数据将被清除。")
        : tr("确定删除此场景？关联数据将被清除。");

    if (!ConfirmDialog::showConfirm(this, title, message)) {
        return;
    }

    int savedMainVScrollPos = 0;
    if (m_mainScrollArea) {
        savedMainVScrollPos = m_mainScrollArea->verticalScrollBar()->value();
    }

    bool success = false;
    if (type == BibleType::Character) {
        success = CharacterExtractor::instance()->deleteCharacter(id);
    } else {
        Scene scene = SceneExtractor::instance()->getSceneBySceneId(m_currentNovel.id(), id);
        success = !scene.id().isEmpty() && SceneExtractor::instance()->deleteScene(scene.id());
    }

    if (!success) {
        QMessageBox::warning(this, tr("删除失败"), tr("无法删除，请重试"));
        return;
    }

    if (m_bibleSectionWidget) {
        m_bibleSectionWidget->refreshBible();
    }

    QTimer::singleShot(0, this, [this, savedMainVScrollPos]() {
        if (m_mainScrollArea) {
            m_mainScrollArea->verticalScrollBar()->setValue(savedMainVScrollPos);
        }
    });
}

void NovelDetailPage::onExportClicked()
{
    QMessageBox::information(this, tr("导出功能"), tr("导出功能开发中..."));
}

void NovelDetailPage::onPanelCardClicked(int panelNumber)
{
    if (!m_storyboardContainer) {
        return;
    }
    
    QVBoxLayout *containerLayout = qobject_cast<QVBoxLayout*>(m_storyboardContainer->layout());
    if (!containerLayout) {
        return;
    }
    
    for (int i = 0; i < containerLayout->count(); ++i) {
        QLayoutItem *item = containerLayout->itemAt(i);
        if (QWidget *widget = item->widget()) {
            StoryboardItem *storyboardItem = qobject_cast<StoryboardItem*>(widget);
            if (storyboardItem && storyboardItem->panelNumber() == panelNumber) {
                QScrollArea *scrollArea = qobject_cast<QScrollArea*>(m_storyboardContainer->parentWidget()->parentWidget());
                if (scrollArea) {
                    scrollArea->ensureWidgetVisible(storyboardItem);
                }
                storyboardItem->setStyleSheet(storyboardItem->styleSheet() + 
                    " QFrame { border: 2px solid #9333EA; }");
                QTimer::singleShot(2000, [storyboardItem]() {
                    storyboardItem->setStyleSheet(storyboardItem->styleSheet().replace(
                        " border: 2px solid #9333EA;", ""));
                });
                break;
            }
        }
    }
}

#include "moc_NovelDetailPage.cpp"
