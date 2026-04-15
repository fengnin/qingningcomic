#include "NovelPage.h"
#include "NovelUploadPage.h"
#include "viewmodels/NovelViewModel.h"
#include "components/EditorStyles.h"
#include "utils/StatusHelper.h"
#include "UserSession.h"
#include <QMessageBox>
#include <QDateTime>
#include <QScrollArea>
#include <QButtonGroup>
#include <QDialog>
#include <QFormLayout>
#include <QTextEdit>
#include <QComboBox>
#include <QMouseEvent>
#include <QTimer>
#include <QUuid>
#include <QFont>

namespace {
    using namespace EditorStyles;
    using EditorStyles::UI::createTransparentWidget;
    using EditorStyles::UI::createLabel;
    using EditorStyles::UI::setupLayout;
    
    // ========== 对话框样式 ==========
    const QString DIALOG_BASE_STYLE = "QDialog { background: #FFFFFF; }";
    const QString DIALOG_TITLE_STYLE = "font-size: 18px; font-weight: bold; color: #1F2937; background: transparent;";
    const QString DIALOG_SUBTITLE_STYLE = "font-size: 14px; color: #6B7280; background: transparent;";
    
    const QString WARNING_LABEL_STYLE = R"(
        font-size: 13px;
        color: #991B1B;
        background: #0def4444;
        border: 1px solid #26ef4444;
        border-radius: 8px;
        padding: 12px 14px;
    )";
    
    // ========== 空状态样式 ==========
    const QString EMPTY_CARD_STYLE = R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #f2f8fafc,
                stop:1 #e6eef2ff);
            border-radius: 24px;
            border: 2px dashed #4d6366f1;
        }
    )";
    
    const QString EMPTY_ICON_CONTAINER_STYLE = R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #266366f1,
                stop:1 #268b5cf6);
            border-radius: 50px;
        }
    )";
    
    const QString EMPTY_TITLE_STYLE = R"(
        font-size: 24px;
        font-weight: bold;
        color: #1e293b;
        background: transparent;
        border: none;
    )";
    
    const QString EMPTY_DESC_STYLE = R"(
        font-size: 15px;
        color: #b31e293b;
        background: transparent;
        border: none;
        line-height: 1.6;
    )";
    
    const QString EMPTY_TIP_STYLE = R"(
        font-size: 13px;
        color: #4d1e293b;
        background: #80ffffff;
        border: none;
        border-radius: 8px;
        padding: 8px 16px;
    )";
    
    // ========== 按钮样式模板 ==========
    const QString BTN_STYLE_TEMPLATE = R"(
        QPushButton {
            background: %1;
            color: %2;
            border: none;
            border-radius: 8px;
            font-size: 14px;
        }
        QPushButton:hover {
            background: %3;
        }
    )";
    
    // ========== 筛选项 ==========
    const QVector<FilterItem> FILTER_ITEMS = {
        {"all", QString::fromUtf8("全部")},
        {"completed", QString::fromUtf8("已完成")},
        {"analyzing", QString::fromUtf8("分析中")},
        {"created", QString::fromUtf8("已创建")}
    };
    
    // ========== 文本常量 ==========
    namespace Text {
        const QString HERO_TITLE = QString::fromUtf8("作品管理");
        const QString HERO_DESC = QString::fromUtf8("管理和查看您的小说作品");
        const QString JUMP_LABEL = QString::fromUtf8("跳转ID:");
        const QString JUMP_PLACEHOLDER = QString::fromUtf8("输入作品ID");
        const QString CREATE_BTN = QString::fromUtf8("创建新作品");
        const QString TOTAL_RECORD = QString::fromUtf8("共 %1 条记录");
        const QString EMPTY_TITLE = QString::fromUtf8("暂无作品");
        const QString EMPTY_DESC = QString::fromUtf8("还没有上传任何小说作品");
        const QString EMPTY_TIP = QString::fromUtf8("点击右上角「创建新作品」按钮开始创作");
        const QString DELETE_DIALOG_TITLE = QString::fromUtf8("确认删除作品？");
        const QString DELETE_SUCCESS_TITLE = QString::fromUtf8("删除成功");
        const QString DELETE_SUCCESS_MSG = QString::fromUtf8("作品已成功删除");
        const QString WARNING_DELETE = QString::fromUtf8("⚠️ 删除后所有关联数据（分镜、面板、导出任务）将被永久清除！");
        const QString BTN_CANCEL = QString::fromUtf8("取消");
        const QString BTN_CONFIRM_DELETE = QString::fromUtf8("确认删除");
        const QString BTN_OK = QString::fromUtf8("确定");
        const QString BTN_JUMP = QString::fromUtf8("跳转");
    }
    
    // ========== 辅助函数 ==========
    QLabel* createDialogLabel(const QString &text, const QString &style)
    {
        QLabel *label = new QLabel(text);
        label->setStyleSheet(style);
        label->setWordWrap(true);
        return label;
    }
    
    QLabel* createWarningLabel(const QString &text)
    {
        QLabel *label = new QLabel(text);
        label->setStyleSheet(WARNING_LABEL_STYLE);
        label->setWordWrap(true);
        return label;
    }
    
    QLabel* createMessageIcon(const QString &bgColor, const QString &iconColor, const QString &iconText)
    {
        QLabel *iconLabel = new QLabel();
        iconLabel->setFixedSize(48, 48);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("QLabel { background: %1; border-radius: 24px; font-size: 24px; color: %2; }")
            .arg(bgColor, iconColor));
        iconLabel->setText(iconText);
        iconLabel->setFont(QFont(QStringLiteral("Segoe UI Emoji"), 24));
        return iconLabel;
    }
    
    QPushButton* createButton(const QString &text, const QString &bgColor, 
                               const QString &textColor, const QString &hoverColor)
    {
        QPushButton *btn = new QPushButton(text);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(BTN_STYLE_TEMPLATE.arg(bgColor, textColor, hoverColor));
        return btn;
    }
    
    QPushButton* createDialogButton(const QString &text, const QString &bgColor, const QString &hoverColor)
    {
        return createButton(text, bgColor, "white", hoverColor);
    }
    
    QPushButton* createSecondaryButton(const QString &text)
    {
        return createButton(text, Colors::COLOR_CANCEL_BG, "#374151", Colors::COLOR_CANCEL_HOVER);
    }
    
    QPushButton* createActionButton(const QString &text, const QString &style)
    {
        QPushButton *btn = new QPushButton(text);
        btn->setStyleSheet(style);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    }
    
    QLineEdit* createStyledLineEdit(const QString &placeholder, int width, int height)
    {
        QLineEdit *edit = new QLineEdit();
        edit->setPlaceholderText(placeholder);
        edit->setStyleSheet(lineEditStyle());
        if (width > 0 && height > 0) {
            edit->setFixedSize(width, height);
        }
        return edit;
    }
    
    QPushButton* createStyledButton(const QString &text, const QString &style)
    {
        QPushButton *btn = new QPushButton(text);
        btn->setStyleSheet(style);
        return btn;
    }
}

NovelPage::NovelPage(QWidget *parent)
    : QWidget(parent)
    , m_userId(UserSession::instance()->currentUserId())
    , m_mainLayout(nullptr)
    , m_jumpInput(nullptr)
    , m_jumpBtn(nullptr)
    , m_createBtn(nullptr)
    , m_filterGroup(nullptr)
    , m_totalCountLabel(nullptr)
    , m_filterContainer(nullptr)
    , m_gridContainer(nullptr)
    , m_gridLayout(nullptr)
    , m_currentFilter("all")
{
    setupUI();
    loadNovelsFromDatabase();
}

NovelPage::~NovelPage()
{
}

void NovelPage::refresh()
{
    loadNovelsFromDatabase();
}

void NovelPage::setupUI()
{
    this->setObjectName("novelPage");
    this->setStyleSheet("#novelPage { background: transparent; }");
    
    m_mainLayout = new QVBoxLayout(this);
    setupLayout(m_mainLayout, 24, 24, 24, 24, 24);
    
    m_mainLayout->addWidget(createScrollArea());
}

QScrollArea* NovelPage::createScrollArea()
{
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(scrollAreaStyle());
    
    QWidget *scrollContent = new QWidget();
    scrollContent->setObjectName("scrollContent");
    scrollContent->setStyleSheet("#scrollContent { background: transparent; }");
    
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    setupLayout(scrollLayout, 0, 0, 0, 0, 24);
    
    scrollLayout->addWidget(createHeroSection());
    scrollLayout->addWidget(createFilterBar());
    scrollLayout->addWidget(createGridSection());
    scrollLayout->addStretch();
    
    scrollArea->setWidget(scrollContent);
    return scrollArea;
}


QWidget* NovelPage::createHeroSection()
{
    QWidget *heroWidget = new QWidget();
    heroWidget->setObjectName("heroSection");
    heroWidget->setStyleSheet(R"(
        #heroSection {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #F8FAFC, stop:1 #EEF2FF);
            border-radius: 16px;
            border: 1px solid #66c7d2fe;
        }
    )");
    
    QHBoxLayout *heroLayout = new QHBoxLayout(heroWidget);
    setupLayout(heroLayout, 24, 20, 24, 20, 10);
    
    heroLayout->addWidget(createHeroIcon());
    heroLayout->addWidget(createHeroText());
    heroLayout->addWidget(createTransparentWidget(), 1);
    heroLayout->addWidget(createJumpSection());
    heroLayout->addWidget(createCreateButton());
    
    return heroWidget;
}

QWidget* NovelPage::createHeroIcon()
{
    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(48, 48);
    iconLabel->setStyleSheet(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
            stop:0 #84cc16, stop:1 #65a30d);
        border-radius: 14px;
        color: white;
        font-size: 24px;
    )");
    iconLabel->setText(QString::fromUtf8("📚"));
    iconLabel->setFont(QFont(QStringLiteral("Segoe UI Emoji"), 24));
    iconLabel->setAlignment(Qt::AlignCenter);
    return iconLabel;
}

QWidget* NovelPage::createHeroText()
{
    QWidget *textContainer = createTransparentWidget();
    QVBoxLayout *textLayout = new QVBoxLayout(textContainer);
    setupLayout(textLayout, 0, 0, 0, 0, 4);
    
    QLabel *titleLabel = createLabel(Text::HERO_TITLE, "font-size: 24px; font-weight: bold; color: #1e293b; background: transparent;");
    QLabel *descLabel = createLabel(Text::HERO_DESC, "font-size: 14px; color: #333333; background: transparent;");
    
    textLayout->addWidget(titleLabel);
    textLayout->addWidget(descLabel);
    
    return textContainer;
}

QWidget* NovelPage::createJumpSection()
{
    QWidget *jumpWidget = createTransparentWidget();
    QHBoxLayout *jumpLayout = new QHBoxLayout(jumpWidget);
    setupLayout(jumpLayout, 0, 0, 0, 0, 8);
    
    QLabel *jumpLabel = createLabel(Text::JUMP_LABEL, "font-size: 12px; color: #4d7c0f; background: transparent;");
    m_jumpInput = createStyledLineEdit(Text::JUMP_PLACEHOLDER, 200, EditorStyles::Constants::INPUT_HEIGHT);
    m_jumpBtn = createStyledButton(Text::BTN_JUMP, secondaryButtonStyle());
    m_jumpBtn->setFixedSize(EditorStyles::Constants::BTN_JUMP_WIDTH, EditorStyles::Constants::INPUT_HEIGHT);
    
    jumpLayout->addWidget(jumpLabel);
    jumpLayout->addWidget(m_jumpInput);
    jumpLayout->addWidget(m_jumpBtn);
    
    connect(m_jumpBtn, &QPushButton::clicked, this, &NovelPage::onJumpClicked);
    
    return jumpWidget;
}

QWidget* NovelPage::createCreateButton()
{
    m_createBtn = createStyledButton(Text::CREATE_BTN, primaryButtonStyle());
    m_createBtn->setFixedSize(EditorStyles::Constants::BTN_CREATE_WIDTH, EditorStyles::Constants::INPUT_HEIGHT);
    
    connect(m_createBtn, &QPushButton::clicked, this, &NovelPage::onCreateNovelClicked);
    
    return m_createBtn;
}


QWidget* NovelPage::createFilterBar()
{
    QWidget *filterBarWidget = createTransparentWidget();
    QHBoxLayout *filterBarLayout = new QHBoxLayout(filterBarWidget);
    setupLayout(filterBarLayout, 0, 0, 0, 0, 16);
    
    m_filterContainer = createTransparentWidget();
    QHBoxLayout *chipsLayout = new QHBoxLayout(m_filterContainer);
    setupLayout(chipsLayout, 0, 0, 0, 0, 10);
    
    m_filterGroup = new QButtonGroup(this);
    
    const auto &items = getFilterItems();
    for (int i = 0; i < items.size(); ++i) {
        bool isActive = (items[i].status == m_currentFilter);
        QPushButton *chip = createFilterChip(items[i].label, items[i].status, isActive);
        m_filterGroup->addButton(chip, i);
        chipsLayout->addWidget(chip);
    }
    
    chipsLayout->addStretch();
    
    m_totalCountLabel = createLabel(Text::TOTAL_RECORD, "font-size: 13px; color: #991e293b;");
    
    filterBarLayout->addWidget(m_filterContainer, 1);
    filterBarLayout->addWidget(m_totalCountLabel);
    
    connect(m_filterGroup, &QButtonGroup::idClicked, [this](int id) {
        const auto &items = getFilterItems();
        if (id >= 0 && id < items.size()) {
            onFilterClicked(items[id].status);
        }
    });
    
    return filterBarWidget;
}

QPushButton* NovelPage::createFilterChip(const QString &label, const QString &status, bool isActive)
{
    QPushButton *chip = new QPushButton(label);
    chip->setCheckable(true);
    chip->setChecked(isActive);
    chip->setProperty("status", status);
    chip->setStyleSheet(R"(
        QPushButton {
            padding: 8px 14px;
            border-radius: 999px;
            border: 1px solid #66a3e635;
            background: #ebf7fee7;
            font-size: 13px;
            color: #4d7c0f;
        }
        QPushButton:hover {
            border-color: #8084cc16;
            background: #33a3e635;
        }
        QPushButton:checked {
            border-color: #8084cc16;
            background: #2684cc16;
            color: #4d7c0f;
        }
    )");
    return chip;
}


QWidget* NovelPage::createGridSection()
{
    QWidget *gridSectionWidget = new QWidget();
    gridSectionWidget->setObjectName("gridSection");
    gridSectionWidget->setStyleSheet(R"(
        #gridSection {
            background: #f2f8fafc;
            border-radius: 24px;
            border: 1px solid #4dc7d2fe;
        }
    )");
    
    QVBoxLayout *gridSectionLayout = new QVBoxLayout(gridSectionWidget);
    setupLayout(gridSectionLayout, 24, 24, 24, 32, 20);
    
    m_gridContainer = createTransparentWidget();
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(20);
    
    gridSectionLayout->addWidget(m_gridContainer);
    
    return gridSectionWidget;
}


QWidget* NovelPage::createNovelCard(const Novel &novel)
{
    QWidget *cardWidget = new QWidget();
    cardWidget->setObjectName("novelCard");
    cardWidget->setProperty("novelId", novel.id());
    cardWidget->setCursor(Qt::PointingHandCursor);
    cardWidget->setStyleSheet(cardStyle());
    
    QVBoxLayout *cardLayout = new QVBoxLayout(cardWidget);
    setupLayout(cardLayout, 20, 20, 20, 20, 16);
    
    cardLayout->addWidget(createCardHeader(novel));
    cardLayout->addWidget(createCardDetails(novel));
    cardLayout->addWidget(createCardActions(novel));
    cardLayout->addWidget(createCardFooter(novel));
    
    cardWidget->installEventFilter(this);
    
    return cardWidget;
}

QWidget* NovelPage::createCardHeader(const Novel &novel)
{
    QWidget *headerWidget = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    setupLayout(headerLayout, 0, 0, 0, 0, 12);
    
    QLabel *titleLabel = createLabel(novel.title(), "font-size: 18px; font-weight: 600; color: #1e293b;");
    titleLabel->setWordWrap(true);
    
    QString status = Novel::statusToString(novel.status());
    QLabel *statusLabel = createStatusLabel(status);
    
    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(statusLabel);
    
    return headerWidget;
}

QWidget* NovelPage::createCardDetails(const Novel &novel)
{
    QWidget *detailsWidget = createTransparentWidget();
    QGridLayout *detailsLayout = new QGridLayout(detailsWidget);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->setSpacing(12);
    
    QString createTime = novel.createdAt().toString("yyyy-MM-dd hh:mm");
    detailsLayout->addWidget(createDetailRow(QString::fromUtf8("创建时间"), createTime), 0, 0);
    
    return detailsWidget;
}

QWidget* NovelPage::createCardFooter(const Novel &novel)
{
    QWidget *footerWidget = createTransparentWidget();
    QHBoxLayout *footerLayout = new QHBoxLayout(footerWidget);
    footerLayout->setContentsMargins(0, 0, 0, 0);
    footerLayout->setSpacing(8);
    
    QLabel *idLabel = createLabel(QString::fromUtf8("ID: "), "font-size: 12px; color: #8c1e293b;");
    
    QLabel *idValue = createLabel(novel.id(), "background: #40a3e635; padding: 4px 6px; border-radius: 6px; color: #4d7c0f; font-size: 12px;");
    
    footerLayout->addWidget(idLabel);
    footerLayout->addStretch();
    footerLayout->addWidget(idValue);
    
    return footerWidget;
}

QWidget* NovelPage::createCardActions(const Novel &novel)
{
    QWidget *actionsWidget = createTransparentWidget();
    actionsWidget->setStyleSheet("border-top: 1px dashed #80c7d2fe; padding-top: 12px;");
    
    QHBoxLayout *actionsLayout = new QHBoxLayout(actionsWidget);
    actionsLayout->setContentsMargins(0, 12, 0, 0);
    actionsLayout->setSpacing(8);
    
    QPushButton *editBtn = createActionButton(QString::fromUtf8("编辑"), novelCardEditButtonStyle());
    QPushButton *deleteBtn = createActionButton(QString::fromUtf8("删除"), novelCardDeleteButtonStyle());
    
    actionsLayout->addStretch();
    actionsLayout->addWidget(editBtn);
    actionsLayout->addWidget(deleteBtn);
    
    QString editNovelId = novel.id();
    QString deleteNovelId = novel.id();
    
    connect(editBtn, &QPushButton::clicked, this, [this, editNovelId]() {
        onEditNovelClicked(editNovelId);
    });
    
    connect(deleteBtn, &QPushButton::clicked, this, [this, deleteNovelId]() {
        onDeleteNovelClicked(deleteNovelId);
    });
    
    return actionsWidget;
}

QWidget* NovelPage::createDetailRow(const QString &labelText, const QString &valueText)
{
    QWidget *rowWidget = createTransparentWidget();
    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    setupLayout(rowLayout, 0, 0, 0, 0, 8);
    
    QLabel *label = createLabel(labelText, "font-size: 12px; color: #8c1e293b;");
    QLabel *value = createLabel(valueText, "font-size: 14px; color: #1e293b;");
    
    rowLayout->addWidget(label);
    rowLayout->addWidget(value);
    rowLayout->addStretch();
    
    return rowWidget;
}

QLabel* NovelPage::createStatusLabel(const QString &status)
{
    QLabel *statusLabel = new QLabel(StatusHelper::Novel::label(status));
    
    QString baseStyle = "padding: 6px 12px; border-radius: 999px; font-size: 12px; font-weight: 600;";
    
    StatusHelper::StatusStyle style = StatusHelper::Novel::style(status);
    statusLabel->setStyleSheet(QString("QLabel { %1 background: %2; color: %3; }")
        .arg(baseStyle, style.bgColor, style.textColor));
    
    return statusLabel;
}

QWidget* NovelPage::createEmptyState()
{
    QWidget *emptyWidget = createTransparentWidget();
    emptyWidget->setStyleSheet("background: transparent;");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(emptyWidget);
    setupLayout(mainLayout, 0, 80, 0, 80, 0);
    mainLayout->setAlignment(Qt::AlignCenter);
    
    QWidget *card = new QWidget();
    card->setStyleSheet(EMPTY_CARD_STYLE);
    card->setFixedSize(420, 320);
    
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 40, 40, 40);
    cardLayout->setSpacing(20);
    cardLayout->setAlignment(Qt::AlignCenter);
    
    QWidget *iconContainer = new QWidget();
    iconContainer->setFixedSize(100, 100);
    iconContainer->setStyleSheet(EMPTY_ICON_CONTAINER_STYLE);
    QVBoxLayout *iconLayout = new QVBoxLayout(iconContainer);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->setAlignment(Qt::AlignCenter);
    QLabel *iconLabel = new QLabel();
    iconLabel->setText(QChar(0x2728));
    iconLabel->setStyleSheet("font-size: 48px; background: transparent; border: none;");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLayout->addWidget(iconLabel);
    
    QLabel *titleLabel = new QLabel(Text::EMPTY_TITLE);
    titleLabel->setStyleSheet(EMPTY_TITLE_STYLE);
    titleLabel->setAlignment(Qt::AlignCenter);
    
    QLabel *descLabel = new QLabel(Text::EMPTY_DESC);
    descLabel->setStyleSheet(EMPTY_DESC_STYLE);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    
    QLabel *tipLabel = new QLabel(Text::EMPTY_TIP);
    tipLabel->setStyleSheet(EMPTY_TIP_STYLE);
    tipLabel->setAlignment(Qt::AlignCenter);
    
    cardLayout->addWidget(iconContainer, 0, Qt::AlignCenter);
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(descLabel);
    cardLayout->addSpacing(10);
    cardLayout->addWidget(tipLabel, 0, Qt::AlignCenter);
    
    mainLayout->addWidget(card, 0, Qt::AlignCenter);
    
    return emptyWidget;
}


void NovelPage::loadNovelsFromDatabase()
{
    NovelViewModel::instance()->loadNovels(m_userId, 1, 100);
    m_novels = NovelViewModel::instance()->novels();
    
    updateFilterStats();
    renderNovelGrid();
}

QList<Novel> NovelPage::filterNovels() const
{
    QList<Novel> result;
    for (const Novel &novel : m_novels) {
        QString statusStr = Novel::statusToString(novel.status());
        
        if (m_currentFilter == "all") {
            result.append(novel);
        } else if (statusStr == m_currentFilter) {
            result.append(novel);
        }
    }
    return result;
}

void NovelPage::renderNovelGrid()
{
    clearGridLayout();
    
    QList<Novel> filteredNovels = filterNovels();
    m_totalCountLabel->setText(QString("Total %1 records").arg(filteredNovels.size()));
    
    if (filteredNovels.isEmpty()) {
        m_gridLayout->addWidget(createEmptyState(), 0, 0, 1, EditorStyles::Constants::GRID_COLUMNS);
        return;
    }
    
    renderNovelCards(filteredNovels);
}

void NovelPage::clearGridLayout()
{
    QLayoutItem *item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void NovelPage::renderNovelCards(const QList<Novel> &novels)
{
    int col = 0, row = 0;
    for (const Novel &novel : novels) {
        m_gridLayout->addWidget(createNovelCard(novel), row, col);
        if (++col >= EditorStyles::Constants::GRID_COLUMNS) {
            col = 0;
            row++;
        }
    }
    
    for (int i = col; col > 0 && i < EditorStyles::Constants::GRID_COLUMNS; ++i) {
        QWidget *spacer = new QWidget();
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_gridLayout->addWidget(spacer, row, i);
    }
}

void NovelPage::updateFilterStats()
{
    m_filterStats.clear();
    
    for (const Novel &novel : m_novels) {
        QString status = Novel::statusToString(novel.status());
        m_filterStats[status] = m_filterStats.value(status, 0) + 1;
    }
    
    QList<QAbstractButton *> buttons = m_filterGroup->buttons();
    const auto &items = getFilterItems();
    
    for (int i = 0; i < buttons.size() && i < items.size(); ++i) {
        QString label = items[i].label;
        if (items[i].status != "all" && m_filterStats.contains(items[i].status)) {
            label += QString(" (%1)").arg(m_filterStats[items[i].status]);
        }
        buttons[i]->setText(label);
    }
}

const QVector<FilterItem>& NovelPage::getFilterItems() const
{
    return FILTER_ITEMS;
}


Novel* NovelPage::findNovelById(const QString &novelId)
{
    for (int i = 0; i < m_novels.size(); ++i) {
        if (m_novels[i].id() == novelId) {
            return &m_novels[i];
        }
    }
    return nullptr;
}

void NovelPage::removeNovelFromList(const QString &novelId)
{
    for (int i = 0; i < m_novels.size(); ++i) {
        if (m_novels[i].id() == novelId) {
            m_novels.removeAt(i);
            return;
        }
    }
}


void NovelPage::onCreateNovelClicked()
{
    emit createNovelRequested();
}

void NovelPage::onJumpClicked()
{
    QString novelId = m_jumpInput->text().trimmed();
    if (novelId.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请输入要跳转的作品ID"));
        return;
    }
    QMessageBox::information(this, tr("跳转成功"), tr("正在跳转到作品: %1").arg(novelId));
}

void NovelPage::onFilterClicked(const QString &status)
{
    m_currentFilter = status;
    
    QList<QAbstractButton *> buttons = m_filterGroup->buttons();
    const auto &items = getFilterItems();
    
    for (int i = 0; i < buttons.size() && i < items.size(); ++i) {
        buttons[i]->setChecked(items[i].status == status);
    }
    
    renderNovelGrid();
}

void NovelPage::onNovelCardClicked(const QString &novelId)
{
    emit showNovelDetail(novelId);
}

void NovelPage::onDeleteNovelClicked(const QString &novelId)
{
    Novel* novel = findNovelById(novelId);
    QString title = novel ? novel->title() : QString();
    showDeleteConfirmDialog(novelId, title);
}

void NovelPage::onEditNovelClicked(const QString &novelId)
{
    emit editNovelRequested(novelId);
}

void NovelPage::deleteNovel(const QString &novelId)
{
    NovelViewModel::instance()->deleteNovel(novelId);
    removeNovelFromList(novelId);
    updateFilterStats();
    renderNovelGrid();
    showSuccessMessage(Text::DELETE_SUCCESS_TITLE, Text::DELETE_SUCCESS_MSG);
}

void NovelPage::showDeleteConfirmDialog(const QString &novelId, const QString &title)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QString::fromUtf8("确认删除"));
    dialog.setMinimumWidth(420);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.setStyleSheet(DIALOG_BASE_STYLE);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(32, 28, 32, 24);
    mainLayout->setSpacing(20);
    
    mainLayout->addWidget(createDialogLabel(Text::DELETE_DIALOG_TITLE, DIALOG_TITLE_STYLE));
    mainLayout->addWidget(createDialogLabel(QString::fromUtf8("您即将删除作品「%1」，此操作不可恢复。").arg(title), DIALOG_SUBTITLE_STYLE));
    mainLayout->addWidget(createWarningLabel(Text::WARNING_DELETE));
    
    QPushButton *cancelBtn = createSecondaryButton(Text::BTN_CANCEL);
    QPushButton *confirmBtn = createDialogButton(Text::BTN_CONFIRM_DELETE, Colors::COLOR_ERROR, Colors::COLOR_ERROR_HOVER);
    cancelBtn->setFixedSize(80, 36);
    confirmBtn->setFixedSize(100, 36);
    
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(confirmBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(confirmBtn);
    mainLayout->addLayout(btnLayout);
    
    if (dialog.exec() == QDialog::Accepted) {
        deleteNovel(novelId);
    }
}

void NovelPage::showSuccessMessage(const QString &title, const QString &message)
{
    showMessageDialog(title, message, MessageType::Success);
}

void NovelPage::showErrorMessage(const QString &title, const QString &message)
{
    showMessageDialog(title, message, MessageType::Error);
}

void NovelPage::showMessageDialog(const QString &title, const QString &message, MessageType type)
{
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.setMinimumWidth(360);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.setStyleSheet(DIALOG_BASE_STYLE);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(32, 28, 32, 24);
    mainLayout->setSpacing(16);
    
    bool isSuccess = (type == MessageType::Success);
    QString bgColor = isSuccess ? "#1a22c55e" : "#1aef4444";
    QString iconColor = isSuccess ? Colors::COLOR_SUCCESS : Colors::COLOR_ERROR;
    QString iconText = isSuccess ? QString::fromUtf8("✅") : QString::fromUtf8("❌");
    QString btnBgColor = isSuccess ? Colors::COLOR_SUCCESS : Colors::COLOR_ERROR;
    QString btnHoverColor = isSuccess ? Colors::COLOR_SUCCESS_HOVER : Colors::COLOR_ERROR_HOVER;
    
    mainLayout->addWidget(createMessageIcon(bgColor, iconColor, iconText), 0, Qt::AlignCenter);
    mainLayout->addWidget(createDialogLabel(title, DIALOG_TITLE_STYLE), 0, Qt::AlignCenter);
    mainLayout->addWidget(createDialogLabel(message, DIALOG_SUBTITLE_STYLE), 0, Qt::AlignCenter);
    mainLayout->addSpacing(8);
    
    QPushButton *okBtn = createDialogButton(Text::BTN_OK, btnBgColor, btnHoverColor);
    okBtn->setFixedSize(100, 36);
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    mainLayout->addWidget(okBtn, 0, Qt::AlignCenter);
    
    dialog.exec();
}

bool NovelPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::MouseButtonRelease) {
        return false;
    }

    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    if (mouseEvent->button() != Qt::LeftButton) {
        return false;
    }
    
    QString novelId = findNovelIdFromWidget(qobject_cast<QWidget*>(watched));
    if (!novelId.isEmpty()) {
        QTimer::singleShot(0, this, [this, novelId]() {
            onNovelCardClicked(novelId);
        });
        return true;
    }
    
    return false;
}

QString NovelPage::findNovelIdFromWidget(QWidget *widget) const
{
    while (widget) {
        QVariant novelId = widget->property("novelId");
        if (novelId.isValid()) {
            return novelId.toString();
        }
        widget = widget->parentWidget();
    }
    return QString();
}

