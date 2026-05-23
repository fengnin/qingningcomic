#include "pages/ExportPage.h"
#include "components/EditorStyles.h"
#include "components/ExportDetailDialog.h"
#include "utils/StatusHelper.h"
#include "services/ExportService.h"
#include "services/ServiceContainer.h"
#include "utils/EncodingUtils.h"
#include <QGraphicsDropShadowEffect>
#include <QFont>

using EditorStyles::UI::createTransparentWidget;
using EditorStyles::UI::createLabel;
using EditorStyles::UI::setupLayout;

namespace {
    using namespace StatusHelper;
    constexpr int INPUT_HEIGHT = 50;
    constexpr int SEARCH_BTN_WIDTH = 140;
    constexpr int HISTORY_LIST_HEIGHT = 160;
    const QString INPUT_STYLE = R"(
        QLineEdit {
            padding: 14px 16px;
            font-size: 15px;
            border: 2px solid #d4d4d4;
            border-radius: 12px;
            background: #ffffff;
            color: #1e293b;
        }
        QLineEdit:focus {
            border-color: #84cc16;
            background: #f7fee7;
        }
        QLineEdit::placeholder {
            color: #94a3b8;
        }
    )";
    
    const QString BTN_PRIMARY_STYLE = R"(
        QPushButton {
            padding: 14px 32px;
            font-size: 15px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #84cc16, stop:1 #65a30d);
            color: white;
            border: none;
            border-radius: 12px;
            font-weight: bold;
        }
        QPushButton:hover:!disabled {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #65a30d, stop:1 #4d7c0f);
        }
        QPushButton:disabled {
            background: #cbd5e1;
            color: #94a3b8;
        }
    )";
    
    const QString ERROR_LABEL_STYLE = R"(
        QLabel {
            padding: 14px 18px;
            background: #14ef4444;
            border: 1px solid #33ef4444;
            border-radius: 12px;
            color: #dc2626;
            font-size: 14px;
        }
    )";
    
    const QString HISTORY_LIST_STYLE = R"(
        QListWidget {
            border: none;
            background: transparent;
            outline: none;
        }
        QListWidget::item {
            padding: 14px 18px;
            margin-bottom: 10px;
            border-radius: 12px;
            border: 1px solid #1a6366f1;
            background: #0a6366f1;
            color: #334155;
            font-size: 14px;
        }
        QListWidget::item:hover {
            background: #146366f1;
            border-color: #336366f1;
        }
        QListWidget::item:selected {
            background: #1f6366f1;
            border-color: #4d6366f1;
        }
        QScrollBar:vertical {
            background: #F3F4F6;
            width: 8px;
            margin: 4px 2px 4px 0;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #D1D5DB;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #9CA3AF;
        }
        QScrollBar::handle:vertical:pressed {
            background: #84cc16;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
            background: transparent;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
    )";
}

ExportPage::ExportPage(QWidget *parent)
    : QWidget(parent)
    , m_exportIdEdit(nullptr)
    , m_searchBtn(nullptr)
    , m_errorLabel(nullptr)
    , m_historyList(nullptr)
{
    setupUI();
    loadHistory();

    ExportService *exportService = ServiceContainer::instance()->exportService();
    if (exportService) {
        connect(exportService, &ExportService::exportCompleted,
                this, [this](const QString &exportId, const QString &) {
            saveHistory(exportId);
        });
    }
}

ExportPage::~ExportPage()
{
}


QWidget* ExportPage::createCard(const QString &objectName)
{
    QWidget *card = new QWidget();
    card->setObjectName(objectName);
    card->setStyleSheet(QString(R"(
        #%1 {
            background: #f2ffffff;
            border-radius: 24px;
            border: 1px solid #1a6366f1;
        }
    )").arg(objectName));
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(99, 102, 241, 25));
    shadow->setOffset(0, 10);
    card->setGraphicsEffect(shadow);
    
    return card;
}


void ExportPage::setupUI()
{
    this->setObjectName("exportPage");
    this->setStyleSheet(R"(
        #exportPage {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #fafbff,
                stop:0.5 #f5f3ff,
                stop:1 #ede9fe);
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setupLayout(mainLayout, 0, 0, 0, 0, 0);

    mainLayout->addWidget(createScrollArea());
}

QScrollArea* ExportPage::createScrollArea()
{
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(R"(
        QScrollArea {
            border: none;
            background: transparent;
        }
    )" + EditorStyles::scrollBarStyle());
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scrollArea->setWidget(createContentWidget());
    return scrollArea;
}

QWidget* ExportPage::createContentWidget()
{
    QWidget *contentWidget = new QWidget();
    contentWidget->setObjectName("exportContent");
    contentWidget->setStyleSheet("#exportContent { background: transparent; }");

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    setupLayout(contentLayout, 40, 40, 40, 60, 28);

    contentLayout->addWidget(createHeader());
    contentLayout->addWidget(createSearchCard());
    contentLayout->addWidget(createHistoryCard());
    contentLayout->addStretch();

    return contentWidget;
}


QWidget* ExportPage::createHeader()
{
    QWidget *header = createTransparentWidget();
    QVBoxLayout *layout = new QVBoxLayout(header);
    setupLayout(layout, 0, 0, 0, 0, 12);

    QWidget *titleRow = createTransparentWidget();
    QHBoxLayout *titleLayout = new QHBoxLayout(titleRow);
    setupLayout(titleLayout, 0, 0, 0, 0, 12);

    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(48, 48);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setPixmap(renderSvg(QStringLiteral(":/icons/nav_export.svg"), 48));

    QLabel *titleLabel = createLabel("导出中心", "font-size: 26px; font-weight: bold; color: #1e293b; background: transparent;", 26, true);

    titleLayout->addWidget(iconLabel);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    QLabel *descLabel = createLabel("查询导出结果、下载文件、追踪导出历史。支持 PDF、Webtoon 长图、资源包三种格式。", "font-size: 15px; color: #64748b; background: transparent; padding-left: 60px;");
    descLabel->setWordWrap(true);

    layout->addWidget(titleRow);
    layout->addWidget(descLabel);

    return header;
}


QWidget* ExportPage::createSearchCard()
{
    QWidget *card = createCard("searchCard");
    QVBoxLayout *layout = new QVBoxLayout(card);
    setupLayout(layout, 32, 28, 32, 28, 24);

    layout->addWidget(createSearchArea());

    return card;
}

QWidget* ExportPage::createSearchArea()
{
    QWidget *widget = createTransparentWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    setupLayout(layout, 0, 0, 0, 0, 14);

    QLabel *label = createLabel("导出 ID", "font-size: 14px; font-weight: 600; color: #374151; background: transparent;");
    layout->addWidget(label);
    layout->addWidget(createSearchInputRow());

    m_errorLabel = new QLabel();
    m_errorLabel->setStyleSheet(ERROR_LABEL_STYLE);
    m_errorLabel->setVisible(false);
    m_errorLabel->setWordWrap(true);
    layout->addWidget(m_errorLabel);

    return widget;
}

QWidget* ExportPage::createSearchInputRow()
{
    QWidget *searchRow = createTransparentWidget();
    QHBoxLayout *searchLayout = new QHBoxLayout(searchRow);
    setupLayout(searchLayout, 0, 0, 0, 0, 16);

    m_exportIdEdit = new QLineEdit();
    m_exportIdEdit->setPlaceholderText("输入导出任务 ID");
    m_exportIdEdit->setStyleSheet(INPUT_STYLE);
    m_exportIdEdit->setMinimumHeight(INPUT_HEIGHT);
    connect(m_exportIdEdit, &QLineEdit::textChanged, this, &ExportPage::validateInput);

    m_searchBtn = new QPushButton("查询");
    m_searchBtn->setIcon(QIcon(renderSvg(QStringLiteral(":/icons/fruit_apple.svg"), 18)));
    m_searchBtn->setIconSize(QSize(18, 18));
    m_searchBtn->setEnabled(false);
    m_searchBtn->setStyleSheet(BTN_PRIMARY_STYLE);
    m_searchBtn->setMinimumSize(SEARCH_BTN_WIDTH, INPUT_HEIGHT);
    m_searchBtn->setCursor(Qt::PointingHandCursor);
    connect(m_searchBtn, &QPushButton::clicked, this, &ExportPage::onSearchClicked);

    searchLayout->addWidget(m_exportIdEdit, 1);
    searchLayout->addWidget(m_searchBtn);

    return searchRow;
}

QWidget* ExportPage::createHistoryCard()
{
    QWidget *card = createCard("historyCard");
    QVBoxLayout *layout = new QVBoxLayout(card);
    setupLayout(layout, 32, 28, 32, 28, 24);

    layout->addWidget(createHistoryHeader());
    layout->addWidget(createHistoryList());

    return card;
}

QWidget* ExportPage::createHistoryHeader()
{
    QWidget *widget = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    setupLayout(layout, 0, 0, 0, 0, 0);

    QLabel *titleLabel = createLabel("最近的导出记录", "font-size: 18px; font-weight: bold; color: #1e293b; background: transparent;");

    QLabel *hintLabel = createLabel("本地缓存，仅保存最近 10 项", "font-size: 13px; color: #94a3b8; background: transparent;");

    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(hintLabel);

    return widget;
}

QWidget* ExportPage::createHistoryList()
{
    m_historyList = new QListWidget();
    m_historyList->setMinimumHeight(HISTORY_LIST_HEIGHT);
    m_historyList->setStyleSheet(HISTORY_LIST_STYLE);
    
    connect(m_historyList, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
        int index = m_historyList->row(item);
        onHistoryItemClicked(index);
    });

    return m_historyList;
}


void ExportPage::loadHistory()
{
    QSettings settings("QingningComic", "ExportHistory");
    m_historyData = settings.value("recentExports", QStringList()).toStringList();
    rebuildHistoryList();
}

void ExportPage::saveHistory(const QString &exportId)
{
    m_historyData.removeAll(exportId);
    m_historyData.prepend(exportId);
    if (m_historyData.size() > MAX_HISTORY) {
        m_historyData = m_historyData.mid(0, MAX_HISTORY);
    }

    QSettings settings("QingningComic", "ExportHistory");
    settings.setValue("recentExports", m_historyData);
    rebuildHistoryList();
}

void ExportPage::rebuildHistoryList()
{
    m_historyList->clear();
    for (const QString &id : m_historyData) {
        QListWidgetItem *item = new QListWidgetItem(id);
        item->setIcon(QIcon(renderSvg(getRotatingExportIcon(), 24)));
        m_historyList->addItem(item);
    }
}

void ExportPage::onSearchClicked()
{
    if (!m_searchBtn || !m_exportIdEdit) {
        return;
    }

    QString exportId = m_exportIdEdit->text().trimmed();
    if (exportId.isEmpty()) {
        showError(TR("请输入导出 ID"));
        return;
    }

    clearMessages();
    m_searchBtn->setText(TR("查询中..."));
    m_searchBtn->setEnabled(false);

    ExportService* exportService = ServiceContainer::instance()->exportService();
    if (!exportService) {
        showError(TR("导出服务未初始化"));
        m_searchBtn->setText(TR("查询"));
        m_searchBtn->setEnabled(true);
        return;
    }

    ExportResult result = exportService->getById(exportId);

    if (result.id.isEmpty()) {
        showError(TR("未找到导出记录: %1").arg(exportId));
        m_searchBtn->setText(TR("查询"));
        m_searchBtn->setEnabled(true);
        return;
    }

    showResult(result);
    saveHistory(exportId);

    m_searchBtn->setText(TR("查询"));
    m_searchBtn->setEnabled(true);
    validateInput();
}

void ExportPage::onHistoryItemClicked(int index)
{
    if (index >= 0 && index < m_historyData.size()) {
        QString exportId = m_historyData[index];
        m_exportIdEdit->setText(exportId);
        onSearchClicked();
    }
}

void ExportPage::validateInput()
{
    bool isValid = !m_exportIdEdit->text().trimmed().isEmpty();
    m_searchBtn->setEnabled(isValid);
}

void ExportPage::showError(const QString &message)
{
    clearMessages();
    m_errorLabel->setText("❌ " + message);
    m_errorLabel->setVisible(true);
}

void ExportPage::showResult(const ExportResult &result)
{
    clearMessages();
    ExportDetailDialog *dialog = new ExportDetailDialog(this);
    dialog->setData(result);
    dialog->exec();
    dialog->deleteLater();
}

void ExportPage::clearMessages()
{
    if (m_errorLabel) {
        m_errorLabel->setVisible(false);
    }
}
