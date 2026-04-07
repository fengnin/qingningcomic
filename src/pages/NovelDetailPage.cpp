/**
 * @file NovelDetailPage.cpp
 * @brief 作品详情页面实现文件
 * 
 * 本文件包含 NovelDetailPage 作品详情主页面的实现
 * 组件已拆分到 components 目录：
 * - ChapterSpinBox: components/ChapterSpinBox.cpp
 * - ModeComboBox: components/ModeComboBox.cpp
 * - ChapterCard: components/ChapterCard.cpp
 * - StoryboardItem: components/StoryboardItem.cpp
 * - BibleItem: components/BibleItem.cpp
 * - PanelCard: components/PanelCard.cpp
 */

#include "NovelDetailPage.h"
#include "StyleManager.h"
#include "viewmodels/StoryboardViewModel.h"
#include "viewmodels/NovelViewModel.h"
#include "CharacterExtractor.h"
#include "SceneExtractor.h"
#include "BibleGenerator.h"
#include "BibleImageService.h"
#include "ImageService.h"
#include "FileStorage.h"
#include "Bible.h"
#include "Panel.h"
#include "EncodingUtils.h"
#include "Logger.h"
#include "SuccessDialog.h"
#include "ConfirmDialog.h"
#include "DatabaseManager.h"
#include "components/EditorStyles.h"
#include "components/AnalysisProgressWidget.h"
#include "components/AnalysisResultWidget.h"
#include "components/ImageViewerDialog.h"
#include "AnalysisStatusManager.h"
#include "utils/StatusHelper.h"
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

// ==================== 命名空间常量和样式定义 ====================

namespace {
    using namespace EditorStyles;
    
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
    
    const QString SPIN_BTN_STYLE = ARROW_BTN_STYLE;
    const QString DROP_BTN_STYLE = ARROW_BTN_STYLE;
    const QString SPIN_EDIT_STYLE = INPUT_LEFT_STYLE + "QLineEdit:read-only { color: #374151; }";
    const QString COMBO_EDIT_STYLE = INPUT_LEFT_STYLE + "QLineEdit:read-only { color: #374151; }";
    const QString SPIN_CONTAINER_STYLE = BTN_CONTAINER_RIGHT_STYLE;
    const QString COMBO_CONTAINER_STYLE = BTN_CONTAINER_RIGHT_STYLE;
    
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
}

namespace {
    const QString LABEL_STYLE = "QLabel#chapterTitle {"
        "font-size: 16px; font-weight: bold; color: #333333; background: transparent; border: none; }"
        "QLabel#chapterMeta {"
        "font-size: 14px; color: #666666; background: transparent; border: none; }";
    
    // ========== 编辑器样式常量 ==========
    // 关闭按钮样式
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
    
    // 取消按钮样式
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
    
    // 保存按钮样式
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
    
    // 编辑输入框样式
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
    
    // 编辑文本框样式
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
    
    // 编辑下拉框样式
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
    
    // 编辑下拉框箭头按钮样式
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
    
    // ========== 面板预览示例数据 ==========
    const QStringList SAMPLE_PANEL_DESCRIPTIONS = {
        QString::fromUtf8("青柠站在教室门口，阳光透过窗户洒在她的脸上。"),
        QString::fromUtf8("她微微皱眉，手中的信件被捏得有些皱。"),
        QString::fromUtf8("教室里传来同学们的欢声笑语，她却心不在焉。"),
        QString::fromUtf8("窗外的樱花树随风摇曳，花瓣飘落在窗台上。"),
        QString::fromUtf8("她深吸一口气，推开了教室的门。")
    };
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
    , m_refreshBibleBtn(nullptr)
    , m_characterCountLabel(nullptr)
    , m_sceneCountLabel(nullptr)
    , m_characterBibleContainer(nullptr)
    , m_sceneBibleContainer(nullptr)
    , m_panelPreviewContainer(nullptr)
    , m_panelPreviewTitleLabel(nullptr)
    , m_panelPreviewHintLabel(nullptr)
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
    
    StoryboardViewModel* vm = StoryboardViewModel::instance();
    
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
                m_generatePanelsBtn->setText(TR("开始生成"));
                
                QTimer::singleShot(100, this, [this]() {
                    refreshPanelPreview();
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
                m_generatePanelsBtn->setText(TR("开始生成"));
            }, Qt::QueuedConnection);
    
    // 连接角色/场景更新信号，刷新圣经 UI
    connect(CharacterExtractor::instance(), &CharacterExtractor::characterUpdated,
            this, [this](const QString& characterId, const QString& portraitPath) {
                Q_UNUSED(characterId);
                Q_UNUSED(portraitPath);
                if (m_currentNovel.id().isEmpty()) return;
                QTimer::singleShot(100, this, [this]() {
                    refreshBibleUI();
                });
            }, Qt::QueuedConnection);
    
    connect(SceneExtractor::instance(), &SceneExtractor::sceneUpdated,
            this, [this](const QString& sceneId, const QString& sceneName) {
                Q_UNUSED(sceneId);
                Q_UNUSED(sceneName);
                if (m_currentNovel.id().isEmpty()) return;
                QTimer::singleShot(100, this, [this]() {
                    refreshBibleUI();
                });
            }, Qt::QueuedConnection);
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
    m_currentNovel = novel;
    updateDisplay();
}

void NovelDetailPage::setChapterNumber(int chapterNumber)
{
    m_currentChapter = chapterNumber;
    if (m_chapterNumberSpin) {
        m_chapterNumberSpin->setValue(chapterNumber);
    }
    updateDisplay();
}

// ========== 辅助方法 ==========

QWidget* NovelDetailPage::createTransparentWidget()
{
    QWidget *widget = new QWidget();
    widget->setStyleSheet(TRANSPARENT_BG);
    return widget;
}

QLabel* NovelDetailPage::createLabel(const QString &text, const QString &color, int fontSize, bool bold)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet(QString("font-size: %1px; color: %2; background: transparent;").arg(fontSize).arg(color));
    label->setFont(QFont("Microsoft YaHei", fontSize, bold ? QFont::Bold : QFont::Normal));
    return label;
}

QLabel* NovelDetailPage::createSectionLabel(const QString &text)
{
    return createLabel(text, m_colorText, 14);
}

QPushButton* NovelDetailPage::createButton(const QString &text, const QString &style, int width, int height)
{
    QPushButton *btn = new QPushButton(text);
    btn->setStyleSheet(style);
    if (width > 0) btn->setFixedWidth(width);
    btn->setFixedHeight(height);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QPushButton* NovelDetailPage::createFeatureButton(const QString &text, int width)
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

void NovelDetailPage::setupLayout(QLayout *layout, int left, int top, int right, int bottom, int spacing)
{
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(spacing);
}

void NovelDetailPage::applyCardShadow(QWidget *card)
{
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 20));
    shadow->setOffset(0, 2);
    card->setGraphicsEffect(shadow);
}

// ========== 卡片组件 ==========

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
    setupLayout(layout, 0, 0, 0, 0, 0);
    
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
        m_chapterHintLabel->setText(TR("当前已完成 %1 章，将生成第 %2 章")
            .arg(m_completedChapterCount).arg(targetChapter));
    }
    if (m_addChapterBtn) {
        m_addChapterBtn->setText(QString(TR("生成第 %1 章")).arg(targetChapter));
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

// ========== UI 初始化 ==========

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

// ========== Section 创建 ==========

QWidget* NovelDetailPage::createHeaderSection()
{
    QFrame *header = new QFrame();
    header->setObjectName("headerSection");
    header->setStyleSheet(headerSectionStyle());
    
    QHBoxLayout *layout = new QHBoxLayout(header);
    setupLayout(layout, 24, 20, 24, 20, 12);
    
    m_titleLabel = createLabel(TR("小说标题 1"), m_colorTitle, 28, true);
    m_statusLabel = createStatusLabel("completed");
    m_metaLabel = createLabel(TR("类型：科幻  |  作品 ID：qingning-001  |  分镜: 1  |  当前分镜章节：第 1 章  |  章节: 2"), m_colorHint, 14);
    
    m_analyzeBtn = createButton(TR("重新分析"), secondaryButtonStyle(), 100, EditorStyles::Constants::BTN_HEIGHT);
    connect(m_analyzeBtn, &QPushButton::clicked, this, &NovelDetailPage::onAnalyzeClicked);
    
    m_viewExportsBtn = createButton(TR("查看导出记录"), primaryButtonStyle(), 130, EditorStyles::Constants::BTN_HEIGHT);
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
    
    // 标题区
    QWidget *headerRow = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    setupLayout(headerLayout, 0, 0, 0, 0, 0);
    
    // 标题区
    QWidget *titleContainer = createTransparentWidget();
    QVBoxLayout *titleLayout = new QVBoxLayout(titleContainer);
    setupLayout(titleLayout, 0, 0, 0, 0, 4);
    titleLayout->addWidget(createLabel(TR("章节分镜"), "#212121", 16, true));
    m_chapterCountLabel = createLabel(TR("已生成 0 章"), m_colorHint, 14);
    titleLayout->addWidget(m_chapterCountLabel);
    
    QPushButton *refreshBtn = createButton(TR("刷新列表"), refreshButtonStyle(), -1, EditorStyles::Constants::BTN_HEIGHT);
    refreshBtn->setObjectName("refreshListBtn");
    connect(refreshBtn, &QPushButton::clicked, this, &NovelDetailPage::onRefreshChaptersClicked);
    
    headerLayout->addWidget(titleContainer);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshBtn);
    layout->addWidget(headerRow);
    
    // 章节卡片区 - 使用滚动区域
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

// ========== 卡片创建 ==========

QWidget* NovelDetailPage::createAddChapterCard()
{
    QFrame *card = createFeatureCardFrame();
    QVBoxLayout *layout = new QVBoxLayout(card);
    setupLayout(layout, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, 16);
    
    layout->addWidget(createCardHeader(TR("追加新章节"), TR("复用当前圣经，保持角色/场景连续性")));
    
    layout->addWidget(createSectionLabel(TR("目标章节")));
    
    m_chapterNumberSpin = new ChapterSpinBox();
    m_chapterNumberSpin->setMinimum(1);
    m_chapterNumberSpin->setMaximum(9999);
    m_chapterNumberSpin->setValue(m_completedChapterCount + 1);
    m_chapterNumberSpin->setFixedHeight(EditorStyles::Constants::BTN_HEIGHT);
    connect(m_chapterNumberSpin, &ChapterSpinBox::valueChanged, this, &NovelDetailPage::onChapterNumberChanged);
    layout->addWidget(m_chapterNumberSpin);
    
    m_chapterHintLabel = createLabel(
        TR("当前已完成 %1 章，默认生成第 %2 章").arg(m_completedChapterCount).arg(m_completedChapterCount + 1),
        m_colorHint, 12);
    layout->addWidget(m_chapterHintLabel);
    
    // 章节文本
    layout->addWidget(createSectionLabel(TR("章节文本")));
    
    m_chapterTextEdit = new QTextEdit();
    m_chapterTextEdit->setPlaceholderText(TR("请粘贴章节文本，建议 6k 字以内"));
    m_chapterTextEdit->setMinimumHeight(100);
    m_chapterTextEdit->setStyleSheet(inputStyle());
    layout->addWidget(m_chapterTextEdit);
    
    QWidget *btnRow = createButtonRow(m_addChapterBtn, TR("生成第 %1 章").arg(m_completedChapterCount + 1), TR("分析任务 就绪"));
    connect(m_addChapterBtn, &QPushButton::clicked, this, &NovelDetailPage::onAddChapterClicked);
    layout->addWidget(btnRow);
    
    m_analysisProgress = new AnalysisProgressWidget();
    layout->addWidget(m_analysisProgress);
    
    m_analysisResult = new AnalysisResultWidget();
    layout->addWidget(m_analysisResult);
    
    QLabel *hintLabel = createLabel(TR("系统会沿用当前角色/场景圣经生成新的分镜，生成后可在圣经面板中替换参考图。"), m_colorHint, 12);
    hintLabel->setWordWrap(true);
    layout->addWidget(hintLabel);
    
    return card;
}

QWidget* NovelDetailPage::createGeneratePanelsCard()
{
    QFrame *card = createFeatureCardFrame();
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING);
    layout->setSpacing(12);
    
    layout->addWidget(createCardHeader(TR("面板批量生成"), TR("支持预览/高清双模式")));
    
    layout->addSpacing(8);
    layout->addWidget(createSectionLabel(TR("生成模式")));
    
    m_generateModeCombo = new ModeComboBox();
    m_generateModeCombo->addItem(TR("预览模式 (512×288)"));
    m_generateModeCombo->addItem(TR("高清模式 (1920×1080)"));
    m_generateModeCombo->setFixedHeight(EditorStyles::Constants::BTN_HEIGHT);
    layout->addWidget(m_generateModeCombo);
    
    // 按钮区
    layout->addSpacing(20);
    
    m_generatePanelsBtn = createFeatureButton(TR("开始生成"));
    connect(m_generatePanelsBtn, &QPushButton::clicked, this, &NovelDetailPage::onGeneratePanelsClicked);
    
    QWidget *btnRow = createButtonStatusRow(m_generatePanelsBtn, TR("生成任务 就绪"));
    layout->addWidget(btnRow);
    
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
    
    layout->addWidget(createCardHeader(TR("自然语言修改请求"), TR("自动解析 CR-DSL 并执行修改闭环")));
    
    layout->addWidget(createSectionLabel(TR("修改指令")));
    
    m_changeRequestEdit = new QTextEdit();
    m_changeRequestEdit->setPlaceholderText(TR("例如：把第 1 页第 1 个面板中的主角表情改为微笑"));
    m_changeRequestEdit->setFixedHeight(66);
    m_changeRequestEdit->setStyleSheet(inputStyle());
    layout->addWidget(m_changeRequestEdit);
    
    layout->addSpacing(14);
    
    // 按钮行
    QWidget *btnRow = createButtonRow(m_submitChangeRequestBtn, TR("提交修改请求"), TR("任务状态 就绪"));
    connect(m_submitChangeRequestBtn, &QPushButton::clicked, this, &NovelDetailPage::onSubmitChangeRequestClicked);
    layout->addWidget(btnRow);
    
    layout->addWidget(createLabel(TR("提交后会自动跟踪任务状态"), m_colorHint, 12));
    layout->addStretch();
    
    return card;
}

QWidget* NovelDetailPage::createExportCard()
{
    QFrame *card = createFeatureCardFrame();
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    setupLayout(cardLayout, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, EditorStyles::Constants::CARD_PADDING, 16);
    
    cardLayout->addWidget(createCardHeader(TR("导出高清成品"), TR("PDF / Webtoon 长图 / 资源包")));
    
    // 导出格式
    cardLayout->addWidget(createSectionLabel(TR("导出格式")));
    
    m_exportFormatCombo = new ModeComboBox();
    m_exportFormatCombo->addItem("PDF");
    m_exportFormatCombo->addItem(TR("Webtoon 长图"));
    m_exportFormatCombo->addItem(TR("资源包"));
    m_exportFormatCombo->setFixedHeight(EditorStyles::Constants::BTN_HEIGHT);
    cardLayout->addWidget(m_exportFormatCombo);
    
    // 按钮与状态区
    m_exportBtn = createFeatureButton(TR("开始导出"));
    connect(m_exportBtn, &QPushButton::clicked, this, &NovelDetailPage::onExportClicked);
    
    cardLayout->addWidget(createButtonStatusRow(m_exportBtn, TR("导出任务 就绪")));
    cardLayout->addStretch();
    
    return card;
}

// ========== 圣经区块 ==========

QWidget* NovelDetailPage::createBibleSection()
{
    QWidget *section = createTransparentWidget();
    
    QVBoxLayout *sectionLayout = new QVBoxLayout(section);
    setupLayout(sectionLayout, 0, 0, 0, 0, EditorStyles::Constants::CARD_SPACING);
    
    // 顶部标题区
    QWidget *headerRow = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    setupLayout(headerLayout, 0, 0, 0, 0, 0);
    
    // 左侧标题
    QLabel *sectionTitle = createLabel(TR("圣经总览"), m_colorTitle, 18, true);
    headerLayout->addWidget(sectionTitle);
    
    headerLayout->addStretch();
    
    // 右侧刷新按钮
    m_refreshBibleBtn = createButton(TR("刷新圣经"), refreshButtonStyle(), -1, EditorStyles::Constants::BTN_HEIGHT);
    connect(m_refreshBibleBtn, &QPushButton::clicked, this, &NovelDetailPage::onRefreshBibleClicked);
    
    headerLayout->addWidget(m_refreshBibleBtn);
    sectionLayout->addWidget(headerRow);
    
    // 双列布局：角色圣经 + 场景圣经
    QWidget *bibleGrid = createTransparentWidget();
    QHBoxLayout *bibleGridLayout = new QHBoxLayout(bibleGrid);
    setupLayout(bibleGridLayout, 0, 0, 0, 0, 20);
    
    bibleGridLayout->addWidget(createCharacterBibleCard(), 1);
    bibleGridLayout->addWidget(createSceneBibleCard(), 1);
    
    sectionLayout->addWidget(bibleGrid);
    
    // 面板预览区域
    sectionLayout->addWidget(createPanelPreviewCard());
    
    return section;
}

/**
 * @brief 创建分镜文本编辑区域
 */
QWidget* NovelDetailPage::createStoryboardSection()
{
    QWidget *section = createTransparentWidget();
    
    QVBoxLayout *sectionLayout = new QVBoxLayout(section);
    setupLayout(sectionLayout, 0, 0, 0, 0, EditorStyles::Constants::CARD_SPACING);
    
    // 顶部标题区
    QWidget *headerRow = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    setupLayout(headerLayout, 0, 0, 0, 0, 0);
    
    // 左侧：主标题 + 副标题
    QWidget *titleGroup = createTransparentWidget();
    QVBoxLayout *titleLayout = new QVBoxLayout(titleGroup);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(4);
    
    QLabel *sectionTitle = createLabel(TR("分镜文本编辑"), m_colorTitle, 18, true);
    titleLayout->addWidget(sectionTitle);
    
    m_storyboardCountLabel = createLabel(TR("共 0 个面板"), m_colorHint, 12);
    titleLayout->addWidget(m_storyboardCountLabel);
    
    headerLayout->addWidget(titleGroup);
    headerLayout->addStretch();
    
    // 右侧：提示文字
    QLabel *hintLabel = createLabel(TR("逐个修订场景描述与 Prompt，保存后即可复用最新文本生成面板"), m_colorHint, 14);
    hintLabel->setWordWrap(true);
    hintLabel->setMaximumWidth(300);
    headerLayout->addWidget(hintLabel);
    
    sectionLayout->addWidget(headerRow);
    
    // 内容区（滚动列表）
    sectionLayout->addWidget(createStoryboardCard());
    
    return section;
}

/**
 * @brief 创建分镜文本编辑卡片
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
    
    // 滚动内容容器
    m_storyboardContainer = new QWidget();
    m_storyboardContainer->setStyleSheet(TRANSPARENT_BG);
    m_storyboardContainer->setMinimumWidth(400);
    
    QVBoxLayout *containerLayout = new QVBoxLayout(m_storyboardContainer);
    containerLayout->setContentsMargins(0, 0, 8, 0);
    containerLayout->setSpacing(16);
    
    // 初始显示示例数据，后续由 refreshStoryboardItems 更新
    QList<QPair<int, QStringList>> storyboardItems = {
        {1, {"", "青柠站在教室门口，阳光透过窗户洒在她的脸上。", "中景", "平视", "青柠 (站立, 微笑)", "青柠: 这里好安静...", "", ""}},
        {2, {"", "她微微皱眉，手中的信件被捏得有些皱。", "特写", "俯视", "青柠 (皱眉, 疑惑)", "青柠: 这是怎么回事...", "", ""}},
        {3, {"", "教室里传来同学们的欢声笑语，她却心不在焉。", "远景", "平视", "青柠 (发呆) | 同学们 (欢笑)", "", "", ""}},
        {4, {"", "窗外的樱花树随风摇曳，花瓣飘落在窗台上。", "中景", "仰视", "", "", "", ""}}
    };
    
    for (const auto &item : storyboardItems) {
        StoryboardItem *storyboardItem = new StoryboardItem(
            item.first, item.second[0], item.second[1], item.second[2], item.second[3], 
            item.second[4], item.second[5], item.second.value(6, ""), item.second.value(7, "")
        );
        connect(storyboardItem, &StoryboardItem::dataChanged, this, &NovelDetailPage::onStoryboardDataChanged);
        containerLayout->addWidget(storyboardItem);
    }
    
    containerLayout->addStretch();
    
    scrollArea->setWidget(m_storyboardContainer);
    cardLayout->addWidget(scrollArea);
    
    return card;
}

QWidget* NovelDetailPage::createCharacterBibleCard()
{
    return createBibleCardWithItems(QString::fromUtf8("角色圣经"), BibleType::Character);
}

QWidget* NovelDetailPage::createSceneBibleCard()
{
    return createBibleCardWithItems(QString::fromUtf8("场景圣经"), BibleType::Scene);
}

QWidget* NovelDetailPage::createBibleCardWithItems(const QString& title, BibleType type)
{
    QWidget *container = createTransparentWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    setupLayout(containerLayout, 0, 0, 0, 0, 12);
    
    if (type == BibleType::Character) {
        m_characterBibleContainer = container;
        QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(m_currentNovel.id());
        populateCharacterBible(containerLayout, characters);
        
        m_characterCountLabel = createLabel(QString::fromUtf8("%1 个角色").arg(characters.size()), m_colorHint, 12);
        return createBibleCard(title, m_characterCountLabel, container);
    } else {
        m_sceneBibleContainer = container;
        QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(m_currentNovel.id());
        populateSceneBible(containerLayout, scenes);
        
        m_sceneCountLabel = createLabel(QString::fromUtf8("%1 个场景").arg(scenes.size()), m_colorHint, 12);
        return createBibleCard(title, m_sceneCountLabel, container);
    }
}

QWidget* NovelDetailPage::createPanelPreviewCard()
{
    QFrame *card = new QFrame();
    card->setObjectName("panelPreviewCard");
    card->setStyleSheet(panelPreviewCardStyle());
    
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    setupLayout(cardLayout, 20, 20, 20, 20, 16);
    
    QWidget *headerRow = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    setupLayout(headerLayout, 0, 0, 0, 0, 0);
    
    m_panelPreviewTitleLabel = createLabel(QString("面板预览・第 %1 章").arg(m_currentChapter), m_colorTitle, 18, true);
    headerLayout->addWidget(m_panelPreviewTitleLabel);
    
    headerLayout->addStretch();
    
    m_panelPreviewHintLabel = createLabel(TR("点击面板可打开编辑工具"), m_colorHint, 14);
    headerLayout->addWidget(m_panelPreviewHintLabel);
    
    cardLayout->addWidget(headerRow);
    
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(scrollAreaStyle());
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setMinimumHeight(200);
    scrollArea->setFixedHeight(220);
    
    m_panelPreviewContainer = new QWidget();
    m_panelPreviewContainer->setStyleSheet(TRANSPARENT_BG);
    
    QHBoxLayout *containerLayout = new QHBoxLayout(m_panelPreviewContainer);
    containerLayout->setContentsMargins(0, 0, 0, 8);
    containerLayout->setSpacing(16);
    
    scrollArea->setWidget(m_panelPreviewContainer);
    scrollArea->setAlignment(Qt::AlignLeft);
    cardLayout->addWidget(scrollArea);
    
    refreshPanelPreview();
    
    return card;
}

// ========== 其他 ==========

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
    if (m_currentNovel.id().isEmpty()) {
        m_titleLabel->setText("小说标题 1");
        m_metaLabel->setText("类型：科幻  |  作品 ID：qingning-001  |  分镜: 1  |  当前分镜章节：第 1 章  |  章节: 2");
        return;
    }
    
    m_titleLabel->setText(m_currentNovel.title());
    
    StoryboardViewModel* vm = StoryboardViewModel::instance();
    vm->loadStoryboards(m_currentNovel.id());
    QList<Storyboard> storyboards = vm->storyboards();
    
    m_completedChapterCount = storyboards.size();
    
    int maxChapterNumber = 0;
    for (const Storyboard& storyboard : storyboards) {
        if (storyboard.chapterNumber() > maxChapterNumber) {
            maxChapterNumber = storyboard.chapterNumber();
        }
    }
    int nextChapterNumber = maxChapterNumber > 0 ? maxChapterNumber + 1 : 1;
    
    m_metaLabel->setText(QString("类型：科幻  |  作品 ID：%1  |  分镜: %2  |  当前分镜章节：第 %3 章  |  章节: %4")
        .arg(m_currentNovel.id())
        .arg(m_completedChapterCount)
        .arg(m_currentChapter)
        .arg(m_completedChapterCount));
    
    if (m_chapterCountLabel) {
        m_chapterCountLabel->setText(TR("已生成 %1 章").arg(m_completedChapterCount));
    }
    
    if (m_chapterNumberSpin) {
        m_chapterNumberSpin->setValue(nextChapterNumber);
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
    
    vm->loadStoryboard(m_currentNovel.id(), m_currentChapter);
    refreshStoryboardItems();
    
    refreshPanelPreview();
    
    QTimer::singleShot(0, this, [this]() {
        refreshBibleUI();
    });
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
        delete card;  // 同步删除，确保布局立即更新
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
        m_chapterCountLabel->setText(TR("已生成 %1 章").arg(storyboards.size()));
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
    if (!ConfirmDialog::showConfirm(this, TR("确认删除"), 
            TR("确定要删除第 %1 章吗？\n此操作不可撤销。").arg(chapterNumber))) {
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
        SuccessDialog::showError(this, TR("删除失败"), TR("无法删除第 %1 章").arg(chapterNumber));
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
        {1, {"", "青柠站在教室门口，阳光透过窗户洒在她的脸上。", "中景", "平视", "青柠 (站立, 微笑)", "青柠: 这里好安静...", "", ""}},
        {2, {"", "她微微皱眉，手中的信件被捏得有些皱。", "特写", "俯视", "青柠 (皱眉, 疑惑)", "青柠: 这是怎么回事...", "", ""}},
        {3, {"", "教室里传来同学们的欢声笑语，她却心不在焉。", "远景", "平视", "青柠 (发呆) | 同学们 (欢笑)", "", "", ""}},
        {4, {"", "窗外的樱花树随风摇曳，花瓣飘落在窗台上。", "中景", "仰视", "", "", "", ""}}
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
    
    if (m_panelPreviewTitleLabel) {
        m_panelPreviewTitleLabel->setText(QString("面板预览・第 %1 章").arg(m_currentChapter));
    }
    
    QVBoxLayout *containerLayout = qobject_cast<QVBoxLayout*>(m_storyboardContainer->layout());
    if (!containerLayout) {
        LOG_WARNING("NovelDetailPage", "refreshStoryboardItems: containerLayout is null");
        return;
    }
    
    clearLayout(containerLayout);
    
    QList<QPair<int, QStringList>> storyboardItems = loadStoryboardFromDatabase();
    
    if (storyboardItems.isEmpty()) {
        storyboardItems = getSampleStoryboardItems();
    }
    
    int createdCount = 0;
    for (const auto &item : storyboardItems) {
        StoryboardItem *storyboardItem = new StoryboardItem(
            item.first, item.second[0], item.second[1], item.second[2], item.second[3], 
            item.second[4], item.second[5], item.second.value(6, ""), item.second.value(7, "")
        );
        connect(storyboardItem, &StoryboardItem::dataChanged, this, &NovelDetailPage::onStoryboardDataChanged);
        containerLayout->addWidget(storyboardItem);
        createdCount++;
    }
    
    if (m_storyboardCountLabel) {
        m_storyboardCountLabel->setText(TR("共 %1 个面板").arg(createdCount));
    }
    
    containerLayout->addStretch();
}

void NovelDetailPage::refreshPanelPreview()
{
    LOG_INFO("NovelDetailPage", "refreshPanelPreview called");
    
    if (!m_panelPreviewContainer) {
        LOG_WARNING("NovelDetailPage", "refreshPanelPreview: m_panelPreviewContainer is null");
        return;
    }
    
    QHBoxLayout *containerLayout = qobject_cast<QHBoxLayout*>(m_panelPreviewContainer->layout());
    if (!containerLayout) {
        LOG_WARNING("NovelDetailPage", "refreshPanelPreview: containerLayout is null");
        return;
    }
    
    clearLayout(containerLayout);
    
    QString novelId = m_currentNovel.id();
    if (novelId.isEmpty()) {
        populatePanelPreviewWithSample();
        return;
    }
    
    try {
        StoryboardViewModel* vm = StoryboardViewModel::instance();
        if (!vm) {
            LOG_WARNING("NovelDetailPage", "refreshPanelPreview: StoryboardViewModel is null");
            populatePanelPreviewWithSample();
            return;
        }
        
        vm->loadStoryboard(novelId, m_currentChapter, true);
        Storyboard storyboard = vm->currentStoryboard();
        if (storyboard.id().isEmpty()) {
            populatePanelPreviewWithSample();
            return;
        }
        
        QList<Panel> panels = vm->currentPanels();
        
        LOG_INFO("NovelDetailPage", QString("refreshPanelPreview: loaded %1 panels").arg(panels.size()));
        
        if (panels.isEmpty()) {
            populatePanelPreviewWithSample();
            return;
        }
        
        for (const Panel& panel : panels) {
            QString desc = panel.scene();
            if (desc.isEmpty()) {
                desc = panel.visualPrompt();
            }
            
            int panelNum = (panel.page() - 1) * 6 + panel.index() + 1;
            QString previewUrl = panel.previewUrl();
            LOG_DEBUG("NovelDetailPage", QString("Panel %1 previewUrl: %2").arg(panelNum).arg(previewUrl));
            PanelCard *panelCard = createPanelCard(panelNum, desc, panel.id(), previewUrl);
            containerLayout->addWidget(panelCard);
        }
        
        containerLayout->addStretch();
    } catch (const std::exception& e) {
        LOG_ERROR("NovelDetailPage", QString("refreshPanelPreview exception: %1").arg(e.what()));
        populatePanelPreviewWithSample();
    } catch (...) {
        LOG_ERROR("NovelDetailPage", "refreshPanelPreview unknown exception");
        populatePanelPreviewWithSample();
    }
}

PanelCard* NovelDetailPage::createPanelCard(int panelNum, const QString& description, const QString& panelId, const QString& previewUrl)
{
    PanelCard *panelCard = new PanelCard(m_currentChapter, panelNum, description);
    if (!panelId.isEmpty()) {
        panelCard->setPanelId(panelId);
    }
    if (!previewUrl.isEmpty()) {
        panelCard->setPreviewUrl(previewUrl);
    }
    connect(panelCard, &PanelCard::clicked, this, &NovelDetailPage::onPanelCardClicked);
    connect(panelCard, &PanelCard::imageClicked, this, [](const QString &imagePath) {
        ImageViewerDialog::showImage(nullptr, imagePath);
    });
    return panelCard;
}

void NovelDetailPage::populatePanelPreviewWithSample()
{
    QHBoxLayout *containerLayout = qobject_cast<QHBoxLayout*>(m_panelPreviewContainer->layout());
    if (!containerLayout) {
        return;
    }
    
    for (int i = 0; i < SAMPLE_PANEL_DESCRIPTIONS.size(); ++i) {
        PanelCard *panelCard = createPanelCard(i + 1, SAMPLE_PANEL_DESCRIPTIONS[i]);
        containerLayout->addWidget(panelCard);
    }
    
    containerLayout->addStretch();
}

QList<QPair<int, QStringList>> NovelDetailPage::loadStoryboardFromDatabase() const
{
    QList<QPair<int, QStringList>> items;
    QString novelId = m_currentNovel.id();
    
    if (novelId.isEmpty()) {
        return items;
    }
    
    Storyboard storyboard = StoryboardViewModel::instance()->currentStoryboard();
    
    if (storyboard.id().isEmpty()) {
        return items;
    }
    
    QList<Panel> panels = StoryboardViewModel::instance()->currentPanels();
    
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
    refreshPanelPreview();
}

// ========== 槽函数 ==========

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
    QMessageBox::information(this, TR("提示"), TR("重新分析功能暂未开放，敬请期待"));
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
        refreshBtn->setText(TR("刷新中..."));
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
        SuccessDialog::showWarning(this, TR("提示"), TR("已有任务在执行中，请等待完成"));
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
    
    QJsonArray existingCharacters = BibleGenerator::instance()->collectExistingCharacters(m_currentNovel.id());
    QJsonArray existingScenes = BibleGenerator::instance()->collectExistingScenes(m_currentNovel.id());
    
    StoryboardViewModel::instance()->startAnalysisWithBible(m_currentNovel.id(), text, chapterNumber, existingCharacters, existingScenes);
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
        SuccessDialog::showWarning(const_cast<NovelDetailPage*>(this), TR("提示"), TR("请输入章节文本内容后再分析小说"));
        return false;
    }
    return true;
}

void NovelDetailPage::setAnalysisStatus(AnalysisStatusManager::Status status, const QString& extraInfo)
{
    m_analysisStatus = status;
    
    QString buttonText = extraInfo;
    if (status == AnalysisStatusManager::Status::Processing) {
        buttonText = TR("正在生成...");
    } else if (status == AnalysisStatusManager::Status::Ready) {
        buttonText = QString(TR("生成第 %1 章")).arg(m_completedChapterCount + 1);
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
    setAnalysisStatus(AnalysisStatusManager::Status::Success);
    
    if (m_analysisProgress) {
        m_analysisProgress->setState(AnalysisProgressWidget::State::Completed);
    }
    
    if (m_analysisResult) {
        m_analysisResult->setResult(chapter, 0, 0, 0);
        m_analysisResult->showAnimated();
    }
    
    NovelViewModel::instance()->updateNovelStatus(m_currentNovel.id(), NovelStatus::Completed);
    
    m_completedChapterCount++;
    m_currentChapter = chapter;
    m_chapterTextEdit->clear();
    
    StoryboardViewModel::instance()->clearCache();
    
    updateDisplay();
    
    QTimer::singleShot(100, this, [this, chapter]() {
        refreshStoryboardItems();
        refreshPanelPreview();
        
        QTimer::singleShot(100, this, [this, chapter]() {
            startAutoImageGeneration(chapter);
        });
    });
}

void NovelDetailPage::handleAnalysisFailure(const QString& errorMessage)
{
    setAnalysisStatus(AnalysisStatusManager::Status::Failed);
    
    if (m_analysisProgress) {
        m_analysisProgress->setState(AnalysisProgressWidget::State::Failed);
        m_analysisProgress->setProgressText(errorMessage);
    }
    
    SuccessDialog::showError(this, TR("生成失败"), TR("分析小说失败：\n%1").arg(errorMessage));
}

void NovelDetailPage::onAnalysisFailed(const QString& novelId, const QString& error)
{
    if (!isCurrentNovel(novelId)) return;
    handleAnalysisFailure(error);
}

void NovelDetailPage::onGeneratePanelsClicked()
{
    if (m_currentNovel.id().isEmpty()) {
        QMessageBox::warning(this, TR("错误"), TR("请先选择小说"));
        return;
    }
    
    if (ImageService::instance()->isGenerating()) {
        QMessageBox::warning(this, TR("提示"), TR("已有生成任务在进行中"));
        return;
    }
    
    StoryboardViewModel* vm = StoryboardViewModel::instance();
    vm->loadStoryboard(m_currentNovel.id(), m_currentChapter);
    
    Storyboard currentStoryboard = vm->currentStoryboard();
    if (currentStoryboard.id().isEmpty()) {
        QMessageBox::warning(this, TR("错误"), TR("当前章节尚未生成分镜，请先分析小说"));
        return;
    }
    
    vm->loadPanels(currentStoryboard.id());
    QList<Panel> panels = vm->currentPanels();
    
    QStringList panelIds;
    for (const Panel& p : panels) {
        panelIds.append(p.id());
    }
    
    if (panelIds.isEmpty()) {
        QMessageBox::warning(this, TR("错误"), TR("没有找到可生成的面板"));
        return;
    }
    
    ImageService::GenerateMode mode = ImageService::GenerateMode::Preview;
    if (m_generateModeCombo && m_generateModeCombo->currentIndex() == 1) {
        mode = ImageService::GenerateMode::HD;
    }
    
    m_generatePanelsBtn->setEnabled(false);
    m_generatePanelsBtn->setText(TR("生成中..."));
    
    if (m_panelGenerateProgress) {
        m_panelGenerateProgress->reset();
        m_panelGenerateProgress->setState(AnalysisProgressWidget::State::Processing);
        m_panelGenerateProgress->setProgress(0);
        m_panelGenerateProgress->setProgressText(TR("正在生成面板图像 0/%1").arg(panelIds.size()));
    }
    
    LOG_INFO("NovelDetailPage", QString("Starting batch generation for %1 panels").arg(panelIds.size()));
    
    QString taskId = ImageService::instance()->enqueueBatchPanelImageGeneration(panelIds, mode);
    
    if (taskId.isEmpty()) {
        QMessageBox::warning(this, TR("错误"), TR("创建生成任务失败"));
        m_generatePanelsBtn->setEnabled(true);
        m_generatePanelsBtn->setText(TR("开始生成"));
        if (m_panelGenerateProgress) {
            m_panelGenerateProgress->setState(AnalysisProgressWidget::State::Failed);
            m_panelGenerateProgress->setProgressText(TR("创建生成任务失败"));
        }
        return;
    }
}

void NovelDetailPage::startAutoImageGeneration(int chapter)
{
    Q_UNUSED(chapter);
    
    QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(m_currentNovel.id());
    QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(m_currentNovel.id());
    
    int totalTasks = characters.size() + scenes.size();
    
    if (totalTasks == 0) {
        return;
    }
    
    if (m_analysisProgress) {
        m_analysisProgress->reset();
        m_analysisProgress->setState(AnalysisProgressWidget::State::Processing);
        m_analysisProgress->setProgress(0);
        m_analysisProgress->setProgressText(QString::fromUtf8("正在生成图像 0/%1").arg(totalTasks));
    }
    
    disconnect(BibleImageService::instance(), &BibleImageService::batchProgress, this, nullptr);
    disconnect(BibleImageService::instance(), &BibleImageService::allBibleImagesCompleted, this, nullptr);
    
    connect(BibleImageService::instance(), &BibleImageService::batchProgress,
            this, &NovelDetailPage::onBibleImageBatchProgress, Qt::QueuedConnection);
    connect(BibleImageService::instance(), &BibleImageService::allBibleImagesCompleted,
            this, &NovelDetailPage::onAllBibleImagesCompleted, Qt::QueuedConnection);
    
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
            QString::fromUtf8("正在生成%1 %2/%3").arg(type == "character" ? "角色肖像" : "场景参考图").arg(current).arg(total));
    }
}

void NovelDetailPage::onAllBibleImagesCompleted(int successCount, int failedCount)
{
    m_completedImageTasks = successCount + failedCount;
    onAllImageGenerationCompleted();
}

void NovelDetailPage::onAllImageGenerationCompleted()
{
    
    if (m_analysisProgress) {
        m_analysisProgress->setState(AnalysisProgressWidget::State::Completed);
        QJsonObject progressResult;
        progressResult["imageCount"] = m_completedImageTasks;
        m_analysisProgress->setResult(progressResult);
    }
    
    refreshPanelPreview();
    refreshBibleUI();
    
    m_pendingCharacters.clear();
    m_pendingScenes.clear();
    m_totalImageTasks = 0;
    m_completedImageTasks = 0;
}

void NovelDetailPage::onSubmitChangeRequestClicked()
{
    QMessageBox::information(this, TR("提示"), TR("自然语言修改请求功能暂未开放，敬请期待"));
}

void NovelDetailPage::onChapterNumberChanged(int value)
{
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
                if (noteList.size() >= 1) {
                    charObj["pose"] = noteList[0].trimmed();
                }
                if (noteList.size() >= 2) {
                    charObj["expression"] = noteList[1].trimmed();
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
    if (!m_refreshBibleBtn) return;
    
    QString originalText = m_refreshBibleBtn->text();
    m_refreshBibleBtn->setText(TR("刷新中..."));
    
    QApplication::processEvents();
    
    refreshBibleUI();
    
    m_refreshBibleBtn->setText(originalText);
}

void NovelDetailPage::onBibleDataChanged()
{
    refreshBibleUI();
}

void NovelDetailPage::clearBibleContainer(QWidget *container, const QString& logPrefix)
{
    Q_UNUSED(logPrefix);
    
    if (!container) return;
    
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout) return;
    
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->disconnect();
            widget->setParent(nullptr);
            widget->deleteLater();
        }
        delete item;
    }
}

QStringList NovelDetailPage::buildCharacterDetails(const Character& character) const
{
    QStringList details;
    
    QString genderText = character.appearance().gender == "male" ? TR("男") : 
                         character.appearance().gender == "female" ? TR("女") : character.appearance().gender;
    if (!genderText.isEmpty() || character.appearance().age > 0) {
        QString ageStr = character.appearance().age > 0 ? 
            QString::number(character.appearance().age) + TR("岁") : "";
        details << TR("外观：%1 · %2 · 发色：%3 · 瞳色：%4")
            .arg(genderText, ageStr, character.appearance().hairColor, character.appearance().eyeColor);
    }
    if (!character.appearance().build.isEmpty()) {
        details << TR("体型：%1").arg(character.appearance().build);
    }
    if (!character.appearance().hairStyle.isEmpty()) {
        details << TR("发型：%1").arg(character.appearance().hairStyle);
    }
    if (!character.appearance().clothing.isEmpty()) {
        details << TR("服饰：%1").arg(character.appearance().clothing.join(", "));
    }
    if (!character.appearance().distinctiveFeatures.isEmpty()) {
        details << TR("特征：%1").arg(character.appearance().distinctiveFeatures.join(", "));
    }
    if (!character.personality().isEmpty()) {
        details << TR("性格：%1").arg(character.personality().join(", "));
    }
    
    return details;
}

void NovelDetailPage::addBibleItemToLayout(QVBoxLayout* layout, const QString& name, 
                                            const QStringList& details, BibleType type)
{
    BibleItem *item = new BibleItem(name, details, type);
    connect(item, &BibleItem::editClicked, this, &NovelDetailPage::onBibleItemEditClicked);
    connect(item, &BibleItem::dataChanged, this, &NovelDetailPage::onBibleItemDataChanged);
    layout->addWidget(item);
}

void NovelDetailPage::populateCharacterBible(QVBoxLayout* layout, const QList<Character>& characters)
{
    if (!layout) return;
    
    if (characters.isEmpty()) {
        QLabel *emptyLabel = createLabel(TR("暂无角色数据"), m_colorHint, 14);
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
    } else {
        for (const Character& character : characters) {
            QStringList details = buildCharacterDetails(character);
            BibleItem *item = new BibleItem(character.name(), details, BibleType::Character);
            
            QString portraitPath = character.portraitPath();
            if (!portraitPath.isEmpty()) {
                QString fullPath = FileStorage::instance()->getFullPath(portraitPath);
                item->setImage(fullPath);
            }
            
            connect(item, &BibleItem::editClicked, this, &NovelDetailPage::onBibleItemEditClicked);
            connect(item, &BibleItem::dataChanged, this, &NovelDetailPage::onBibleItemDataChanged);
            connect(item, &BibleItem::imageClicked, this, [](const QString &imagePath) {
                ImageViewerDialog::showImage(nullptr, imagePath);
            });
            connect(item, &BibleItem::uploadClicked, this, &NovelDetailPage::onBibleItemUploadClicked);
            connect(item, &BibleItem::deleteImageClicked, this, &NovelDetailPage::onBibleItemDeleteImageClicked);
            layout->addWidget(item);
        }
    }
    layout->addStretch();
}

void NovelDetailPage::populateSceneBible(QVBoxLayout* layout, const QList<Scene>& scenes)
{
    if (!layout) return;
    
    if (scenes.isEmpty()) {
        QLabel *emptyLabel = createLabel(TR("暂无场景数据"), m_colorHint, 14);
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
    } else {
        for (const Scene& scene : scenes) {
            QStringList details = scene.details().toDisplayStrings();
            BibleItem *item = new BibleItem(scene.name(), details, BibleType::Scene);
            
            QString referencePath = scene.referenceImagePath();
            if (!referencePath.isEmpty()) {
                QString fullPath = FileStorage::instance()->getFullPath(referencePath);
                item->setImage(fullPath);
            }
            
            connect(item, &BibleItem::editClicked, this, &NovelDetailPage::onBibleItemEditClicked);
            connect(item, &BibleItem::dataChanged, this, &NovelDetailPage::onBibleItemDataChanged);
            connect(item, &BibleItem::imageClicked, this, [](const QString &imagePath) {
                ImageViewerDialog::showImage(nullptr, imagePath);
            });
            connect(item, &BibleItem::uploadClicked, this, &NovelDetailPage::onBibleItemUploadClicked);
            connect(item, &BibleItem::deleteImageClicked, this, &NovelDetailPage::onBibleItemDeleteImageClicked);
            layout->addWidget(item);
        }
    }
    layout->addStretch();
}

void NovelDetailPage::populateBibleContainer(QVBoxLayout *layout, const QList<BibleEntry>& entries, 
                                              BibleType type, const QString& emptyText)
{
    if (!layout) return;
    
    if (entries.isEmpty()) {
        QLabel *emptyLabel = createLabel(emptyText, m_colorHint, 14);
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
    } else {
        for (const BibleEntry &entry : entries) {
            BibleItem *item = new BibleItem(entry.name(), entry.toDisplayDetails(), type);
            connect(item, &BibleItem::editClicked, this, &NovelDetailPage::onBibleItemEditClicked);
            layout->addWidget(item);
        }
    }
    layout->addStretch();
}

void NovelDetailPage::refreshBibleUI()
{
    // 保存滚动条位置
    int hScrollValue = 0;
    int vScrollValue = 0;
    if (m_mainScrollArea) {
        hScrollValue = m_mainScrollArea->horizontalScrollBar()->value();
        vScrollValue = m_mainScrollArea->verticalScrollBar()->value();
    }
    
    clearBibleContainer(m_characterBibleContainer, "refreshBibleUI: Character");
    clearBibleContainer(m_sceneBibleContainer, "refreshBibleUI: Scene");
    
    QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(m_currentNovel.id());
    QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(m_currentNovel.id());
    
    if (m_characterBibleContainer) {
        populateCharacterBible(qobject_cast<QVBoxLayout*>(m_characterBibleContainer->layout()), characters);
    }
    
    if (m_sceneBibleContainer) {
        populateSceneBible(qobject_cast<QVBoxLayout*>(m_sceneBibleContainer->layout()), scenes);
    }
    
    if (m_characterCountLabel) {
        m_characterCountLabel->setText(QString::fromUtf8("%1 个角色").arg(characters.size()));
    }
    if (m_sceneCountLabel) {
        m_sceneCountLabel->setText(QString::fromUtf8("%1 个场景").arg(scenes.size()));
    }
    
    // 恢复滚动条位置
    if (m_mainScrollArea) {
        m_mainScrollArea->horizontalScrollBar()->setValue(hScrollValue);
        m_mainScrollArea->verticalScrollBar()->setValue(vScrollValue);
    }
}

void NovelDetailPage::onCharacterCountChanged(int count)
{
    Q_UNUSED(count);
}

void NovelDetailPage::onSceneCountChanged(int count)
{
    Q_UNUSED(count);
}

void NovelDetailPage::onBibleItemEditClicked(const QString &name)
{
    Q_UNUSED(name);
}

void NovelDetailPage::onBibleItemDataChanged(const QString &name, const QStringList &details)
{
    Character character = CharacterExtractor::instance()->getCharacterByName(m_currentNovel.id(), name);
    if (!character.id().isEmpty()) {
        CharacterAppearance app = character.appearance();
        
        for (const QString &detail : details) {
            if (detail.contains(QString::fromUtf8("\u5916\u89c2"))) {
                QRegularExpression genderRe(QString::fromUtf8("\u5916\u89c2\uff1a(\u7537|\u5973)"));
                QRegularExpressionMatch match = genderRe.match(detail);
                if (match.hasMatch()) {
                    app.gender = match.captured(1);
                }
                
                QRegularExpression ageRe(QString::fromUtf8("(\\d+)\u5c81"));
                match = ageRe.match(detail);
                if (match.hasMatch()) {
                    app.age = match.captured(1).toInt();
                }
                
                QRegularExpression hairColorRe(QString::fromUtf8("\u53d1\u8272\uff1a(\\S+)"));
                match = hairColorRe.match(detail);
                if (match.hasMatch()) {
                    app.hairColor = match.captured(1);
                }
                
                QRegularExpression eyeColorRe(QString::fromUtf8("\u77b3\u8272\uff1a(\\S+)"));
                match = eyeColorRe.match(detail);
                if (match.hasMatch()) {
                    app.eyeColor = match.captured(1);
                }
            }
            else if (detail.contains(QString::fromUtf8("\u4f53\u578b"))) {
                app.build = extractBibleValue(detail, QString::fromUtf8("\u4f53\u578b"));
            }
            else if (detail.contains(QString::fromUtf8("\u53d1\u578b"))) {
                app.hairStyle = extractBibleValue(detail, QString::fromUtf8("\u53d1\u578b"));
            }
            else if (detail.contains(QString::fromUtf8("\u670d\u9970"))) {
                QString value = extractBibleValue(detail, QString::fromUtf8("\u670d\u9970"));
                app.clothing = value.split(", ");
            }
            else if (detail.contains(QString::fromUtf8("\u7279\u5f81"))) {
                QString value = extractBibleValue(detail, QString::fromUtf8("\u7279\u5f81"));
                app.distinctiveFeatures = value.split(", ");
            }
            else if (detail.contains(QString::fromUtf8("\u6027\u683c"))) {
                QString value = extractBibleValue(detail, QString::fromUtf8("\u6027\u683c"));
                character.setPersonality(value.split(", "));
            }
        }
        
        character.setAppearance(app);
        CharacterExtractor::instance()->updateCharacter(character);
        return;
    }
    
    Scene scene = SceneExtractor::instance()->getSceneBySceneId(m_currentNovel.id(), name);
    if (!scene.id().isEmpty()) {
        SceneDetails det = scene.details();
        
        for (const QString &detail : details) {
            if (detail.contains(QString::fromUtf8("\u573a\u666f\u63cf\u8ff0"))) {
                det.description = extractBibleValue(detail, QString::fromUtf8("\u573a\u666f\u63cf\u8ff0"));
            }
            else if (detail.contains(QString::fromUtf8("\u5efa\u7b51"))) {
                det.building = extractBibleValue(detail, QString::fromUtf8("\u5efa\u7b51"));
            }
            else if (detail.contains(QString::fromUtf8("\u8272\u8c03"))) {
                det.color = extractBibleValue(detail, QString::fromUtf8("\u8272\u8c03"));
            }
            else if (detail.contains(QString::fromUtf8("\u5730\u6807"))) {
                det.landmark = extractBibleValue(detail, QString::fromUtf8("\u5730\u6807"));
            }
            else if (detail.contains(QString::fromUtf8("\u5e03\u5c40"))) {
                det.layout = extractBibleValue(detail, QString::fromUtf8("\u5e03\u5c40"));
            }
            else if (detail.contains(QString::fromUtf8("\u6c1b\u56f4"))) {
                det.atmosphere = extractBibleValue(detail, QString::fromUtf8("\u6c1b\u56f4"));
            }
        }
        
        scene.setDetails(det);
        SceneExtractor::instance()->updateScene(scene);
    }
}

void NovelDetailPage::onBibleItemUploadClicked(const QString &name, BibleType type)
{
    QString filter = TR("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)");
    QString filePath = QFileDialog::getOpenFileName(this, TR("选择参考图片"), 
                                                     QString(), filter);
    if (filePath.isEmpty()) {
        return;
    }
    
    QString relativePath = FileStorage::instance()->saveReferenceImage(filePath, m_currentNovel.id());
    if (relativePath.isEmpty()) {
        QMessageBox::warning(this, TR("错误"), TR("保存图片失败"));
        return;
    }
    
    if (type == BibleType::Character) {
        Character character = CharacterExtractor::instance()->getCharacterByName(m_currentNovel.id(), name);
        if (!character.id().isEmpty()) {
            character.setPortraitPath(relativePath);
            CharacterExtractor::instance()->updateCharacter(character);
        }
    } else {
        Scene scene = SceneExtractor::instance()->getSceneBySceneId(m_currentNovel.id(), name);
        if (!scene.id().isEmpty()) {
            scene.setReferenceImagePath(relativePath);
            SceneExtractor::instance()->updateScene(scene);
        }
    }
    
    refreshBibleUI();
}

void NovelDetailPage::onBibleItemDeleteImageClicked(const QString &name, BibleType type)
{
    bool confirmed = ConfirmDialog::showConfirm(this, TR("确认"), TR("确定要删除该参考图片吗？"));
    if (!confirmed) {
        return;
    }
    
    if (type == BibleType::Character) {
        Character character = CharacterExtractor::instance()->getCharacterByName(m_currentNovel.id(), name);
        if (!character.id().isEmpty()) {
            QString oldPath = character.portraitPath();
            if (!oldPath.isEmpty()) {
                FileStorage::instance()->deleteReferenceImage(oldPath);
            }
            character.setPortraitPath(QString());
            CharacterExtractor::instance()->updateCharacter(character);
        }
    } else {
        Scene scene = SceneExtractor::instance()->getSceneBySceneId(m_currentNovel.id(), name);
        if (!scene.id().isEmpty()) {
            QString oldPath = scene.referenceImagePath();
            if (!oldPath.isEmpty()) {
                FileStorage::instance()->deleteReferenceImage(oldPath);
            }
            scene.setReferenceImagePath(QString());
            SceneExtractor::instance()->updateScene(scene);
        }
    }
    
    refreshBibleUI();
}

QString NovelDetailPage::extractBibleValue(const QString &detail, const QString &key) const
{
    QString prefix = key + QString::fromUtf8("\uff1a");
    if (detail.startsWith(prefix)) {
        return detail.mid(prefix.length()).trimmed();
    }
    return QString();
}

void NovelDetailPage::onExportClicked()
{
    QMessageBox::information(this, TR("提示"), TR("导出功能暂未开放，敬请期待"));
}

void NovelDetailPage::onPanelCardClicked(int panelNumber)
{
    Q_UNUSED(panelNumber);
}
