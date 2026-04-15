#include "ExportPage.h"
#include "components/EditorStyles.h"
#include "utils/StatusHelper.h"
#include "services/ExportService.h"
#include "ServiceContainer.h"
#include "EncodingUtils.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QFont>

namespace {
    using namespace StatusHelper;
    constexpr int INPUT_HEIGHT = 50;
    constexpr int DETAIL_ITEM_HEIGHT = 72;
    const QString COLOR_PRIMARY = "#84cc16";
    const QString COLOR_ERROR = "#ef4444";
    const QString COLOR_TEXT = "#1e293b";
    const QString COLOR_TEXT_LIGHT = "#64748b";
    const QString COLOR_BORDER = "#d4d4d4";
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
    
    const QString BTN_SUCCESS_STYLE = R"(
        QPushButton {
            padding: 14px 24px;
            font-size: 14px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #10b981, stop:1 #34d399);
            color: white;
            border: none;
            border-radius: 12px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #059669, stop:1 #10b981);
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
    
    const QString DETAIL_ITEM_STYLE = R"(
        #detailItem {
            background: #0a6366f1;
            border-radius: 12px;
            border: 1px solid #146366f1;
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
    , m_resultWidget(nullptr)
    , m_resultNovelIdLabel(nullptr)
    , m_resultFormatLabel(nullptr)
    , m_resultStatusLabel(nullptr)
    , m_resultFileSizeLabel(nullptr)
    , m_resultCreatedAtLabel(nullptr)
    , m_downloadBtn(nullptr)
    , m_historyList(nullptr)
{
    setupUI();
    loadHistory();
}

ExportPage::~ExportPage()
{
}


QWidget* ExportPage::createTransparentWidget()
{
    
    QWidget *widget = new QWidget();
    widget->setStyleSheet("background: transparent;");
    return widget;
}

QLabel* ExportPage::createLabel(const QString &text, const QString &style, int fontSize, bool bold)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet(style);
    if (fontSize > 0) {
        label->setFont(QFont("Microsoft YaHei", fontSize, bold ? QFont::Bold : QFont::Normal));
    }
    return label;
}

void ExportPage::setupLayout(QLayout *layout, int left, int top, int right, int bottom, int spacing)
{
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(spacing);
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
    iconLabel->setStyleSheet(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
            stop:0 #84cc16, stop:1 #65a30d);
        border-radius: 14px;
        color: white;
        font-size: 24px;
    )");
    iconLabel->setText(QString::fromUtf8("📦"));
    iconLabel->setFont(QFont(QStringLiteral("Segoe UI Emoji"), 24));
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = createLabel(tr("导出中心"), "font-size: 26px; font-weight: bold; color: #1e293b; background: transparent;", 26, true);

    titleLayout->addWidget(iconLabel);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    QLabel *descLabel = createLabel(tr("查看导出结果、下载文件并跟踪导出历史。支持 PDF、Webtoon 和资源包格式。"), "font-size: 15px; color: #64748b; background: transparent; padding-left: 60px;");
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
    layout->addWidget(createResultArea());

    return card;
}

QWidget* ExportPage::createSearchArea()
{
    QWidget *searchRow = createTransparentWidget();
    QHBoxLayout *searchLayout = new QHBoxLayout(searchRow);
    setupLayout(searchLayout, 0, 0, 0, 0, 16);

    m_exportIdEdit = new QLineEdit();
    m_exportIdEdit->setPlaceholderText(tr("请输入导出任务ID"));
    m_exportIdEdit->setStyleSheet(INPUT_STYLE);
    m_exportIdEdit->setMinimumHeight(INPUT_HEIGHT);
    connect(m_exportIdEdit, &QLineEdit::textChanged, this, &ExportPage::validateInput);

    m_searchBtn = new QPushButton(tr("查询"));
    m_searchBtn->setStyleSheet(BTN_PRIMARY_STYLE);
    m_searchBtn->setCursor(Qt::PointingHandCursor);
    m_searchBtn->setMinimumHeight(INPUT_HEIGHT);
    m_searchBtn->setEnabled(false);
    connect(m_searchBtn, &QPushButton::clicked, this, &ExportPage::onSearchClicked);

    searchLayout->addWidget(m_exportIdEdit, 1);
    searchLayout->addWidget(m_searchBtn);

    return searchRow;
}

QWidget* ExportPage::createResultArea()
{
    m_resultWidget = createCard("resultCard");
    QVBoxLayout *resultLayout = new QVBoxLayout(m_resultWidget);
    setupLayout(resultLayout, 32, 28, 32, 28, 18);

    resultLayout->addWidget(createResultHeader());
    resultLayout->addWidget(createResultDetails());

    QWidget *buttonRow = createTransparentWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonRow);
    setupLayout(buttonLayout, 0, 0, 0, 0, 0);
    buttonLayout->addStretch();

    m_downloadBtn = new QPushButton(tr("下载"));
    connect(m_downloadBtn, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl("https://example.com/download"));
    });
    buttonLayout->addWidget(m_downloadBtn);

    resultLayout->addWidget(buttonRow);
    m_resultWidget->setVisible(false);
    return m_resultWidget;
}

QWidget* ExportPage::createResultHeader()
{
    QWidget *headerRow = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    setupLayout(headerLayout, 0, 0, 0, 0, 0);

    QLabel *resultTitle = createLabel(tr("导出结果"), "font-size: 16px; font-weight: bold; color: #1e293b; background: transparent;");

    QLabel *resultIdLabel = new QLabel();
    resultIdLabel->setObjectName("resultIdLabel");
    resultIdLabel->setStyleSheet("font-size: 13px; color: #94a3b8; background: transparent;");

    headerLayout->addWidget(resultTitle);
    headerLayout->addWidget(resultIdLabel);
    headerLayout->addStretch();

    return headerRow;
}

QWidget* ExportPage::createResultDetails()
{
    QWidget *detailsGrid = createTransparentWidget();
    QGridLayout *gridLayout = new QGridLayout(detailsGrid);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(14);
    gridLayout->addWidget(createDetailItem(tr("作品ID"), &m_resultNovelIdLabel), 0, 0);
    gridLayout->addWidget(createDetailItem(tr("格式"), &m_resultFormatLabel), 0, 1);
    gridLayout->addWidget(createDetailItem(tr("状态"), &m_resultStatusLabel), 1, 0);
    gridLayout->addWidget(createDetailItem(tr("文件大小"), &m_resultFileSizeLabel), 1, 1);
    gridLayout->addWidget(createDetailItem(tr("创建时间"), &m_resultCreatedAtLabel), 2, 0, 1, 2);

    return detailsGrid;
}

QWidget* ExportPage::createDetailItem(const QString &labelText, QLabel **valueLabel)
{
    QWidget *widget = new QWidget();
    widget->setObjectName("detailItem");
    widget->setStyleSheet(DETAIL_ITEM_STYLE);
    widget->setMinimumHeight(DETAIL_ITEM_HEIGHT);

    QVBoxLayout *layout = new QVBoxLayout(widget);
    setupLayout(layout, 16, 12, 16, 12, 6);

    QLabel *label = createLabel(labelText, "font-size: 12px; color: #64748b; background: transparent;");

    *valueLabel = new QLabel();
    (*valueLabel)->setStyleSheet("font-size: 14px; color: #1e293b; background: transparent;");
    (*valueLabel)->setWordWrap(true);
    (*valueLabel)->setMinimumHeight(24);

    layout->addWidget(label);
    layout->addWidget(*valueLabel);

    return widget;
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

    QLabel *titleLabel = createLabel(tr("历史记录"), "font-size: 18px; font-weight: bold; color: #1e293b; background: transparent;");

    QLabel *hintLabel = createLabel(tr("本地缓存仅保留最近 10 条记录。"), "font-size: 13px; color: #94a3b8; background: transparent;");

    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(hintLabel);

    return widget;
}

QWidget* ExportPage::createHistoryList()
{
    m_historyList = new QListWidget();
    m_historyList->setMinimumHeight(240);
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

    m_historyList->clear();
    for (const QString &id : m_historyData) {
        QListWidgetItem *item = new QListWidgetItem(tr("导出任务: %1").arg(id));
        m_historyList->addItem(item);
    }
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

    m_historyList->clear();
    for (const QString &id : m_historyData) {
        QListWidgetItem *item = new QListWidgetItem(tr("导出任务: %1").arg(id));
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
        showError(tr("请输入导出任务ID"));
        return;
    }

    clearMessages();
    m_searchBtn->setText(tr("查询中..."));
    m_searchBtn->setEnabled(false);

    ExportService* exportService = ServiceContainer::instance()->exportService();
    if (!exportService) {
        showError(tr("服务不可用"));
        m_searchBtn->setText(tr("查询"));
        m_searchBtn->setEnabled(true);
        return;
    }

    ExportResult result = exportService->getById(exportId);
    
    if (result.id.isEmpty()) {
    showError(tr("未找到导出记录: %1").arg(exportId));
        m_searchBtn->setText(tr("查询"));
        m_searchBtn->setEnabled(true);
        return;
    }

    QVariantMap data;
    data["id"] = result.id;
    data["novelId"] = result.novelId;
    data["format"] = result.format;
    data["status"] = result.status;
    data["fileSize"] = result.fileSize;
    data["createdAt"] = result.createdAt;
    data["fileUrl"] = result.fileUrl;

    showResult(data);
    saveHistory(exportId);

    m_searchBtn->setText(tr("查询"));
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
    if (!m_searchBtn || !m_exportIdEdit) {
        return;
    }

    bool isValid = !m_exportIdEdit->text().trimmed().isEmpty();
    m_searchBtn->setEnabled(isValid);
}

QString ExportPage::formatBytes(qint64 size)
{
    if (size <= 0) return QStringLiteral("-");
    if (size < 1024) return QString("%1 B").arg(size);
    if (size < 1024 * 1024) return QString("%1 KB").arg((double)size / 1024, 0, 'f', 1);
    if (size < 1024 * 1024 * 1024) return QString("%1 MB").arg((double)size / 1024 / 1024, 0, 'f', 1);
    return QString("%1 GB").arg((double)size / 1024 / 1024 / 1024, 0, 'f', 2);
}

QString ExportPage::formatDateTime(const QString &dateTime)
{
    if (dateTime.isEmpty()) return QStringLiteral("-");
    QDateTime dt = QDateTime::fromString(dateTime, Qt::ISODate);
    if (!dt.isValid()) return dateTime;
    return dt.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
}

QString ExportPage::formatLabel(const QString &format)
{
    if (format == "pdf") return "PDF";
    if (format == "webtoon") return "Webtoon";
    if (format == "resources") return "Resources (ZIP)";
    return format;
}

QString ExportPage::statusLabel(const QString &status)
{
    return Job::statusLabel(status);
}

void ExportPage::showError(const QString &message)
{
    if (!m_errorLabel) {
        return;
    }
    clearMessages();
    m_errorLabel->setText(message);
    m_errorLabel->setVisible(true);
}

void ExportPage::showResult(const QVariantMap &data)
{
    if (!m_resultWidget || !m_resultNovelIdLabel || !m_resultFormatLabel ||
        !m_resultStatusLabel || !m_resultFileSizeLabel || !m_resultCreatedAtLabel) {
        return;
    }

    clearMessages();

    QLabel *resultIdLabel = m_resultWidget->findChild<QLabel*>("resultIdLabel");
    if (resultIdLabel) {
        resultIdLabel->setText(data["id"].toString());
    }

    m_resultNovelIdLabel->setText(data["novelId"].toString());
    m_resultFormatLabel->setText(formatLabel(data["format"].toString()));
    m_resultStatusLabel->setText(statusLabel(data["status"].toString()));
    m_resultFileSizeLabel->setText(formatBytes(data["fileSize"].toLongLong()));
    m_resultCreatedAtLabel->setText(formatDateTime(data["createdAt"].toString()));

    m_resultWidget->setVisible(true);
}

void ExportPage::clearMessages()
{
    m_resultWidget->setVisible(false);
}






