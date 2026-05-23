#include "components/ExportDetailDialog.h"
#include "components/EditorStyles.h"
#include "utils/StatusHelper.h"
#include "utils/ExportUtils.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPropertyAnimation>
#include <QUrl>
#include <QVBoxLayout>

namespace {
    constexpr int DIALOG_WIDTH = 560;
    constexpr int DETAIL_ITEM_HEIGHT = 72;

    const QString DETAIL_ITEM_STYLE = R"(
        #detailItem {
            background: #0a6366f1;
            border-radius: 12px;
            border: 1px solid #146366f1;
        }
    )";

    const QString OPEN_BTN_STYLE = R"(
        QPushButton {
            padding: 12px 28px;
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

    const QString CLOSE_BTN_STYLE = R"(
        QPushButton {
            padding: 12px 28px;
            font-size: 14px;
            background: #f1f5f9;
            color: #64748b;
            border: none;
            border-radius: 12px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #e2e8f0;
            color: #475569;
        }
    )";

    QString formatBytes(qint64 size)
    {
        if (size <= 0) return QStringLiteral("—");
        if (size < 1024) return QString("%1 B").arg(size);
        if (size < 1024 * 1024) return QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
        if (size < 1024 * 1024 * 1024) return QString("%1 MB").arg(size / 1024.0 / 1024.0, 0, 'f', 1);
        return QString("%1 GB").arg(size / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2);
    }

    QString formatDateTime(const QString &raw)
    {
        if (raw.isEmpty()) return QStringLiteral("—");
        QDateTime dt = QDateTime::fromString(raw, Qt::ISODate);
        if (!dt.isValid()) return raw;
        return dt.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
    }
}

ExportDetailDialog::ExportDetailDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void ExportDetailDialog::setupUI()
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(DIALOG_WIDTH);

    QWidget *container = new QWidget(this);
    container->setObjectName("exportDetailContainer");
    container->setStyleSheet(R"(
        #exportDetailContainer {
            background: #ffffff;
            border-radius: 20px;
            border: 1px solid #e2e8f0;
        }
    )");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(container);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20);
    outerLayout->addWidget(container);

    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(20);

    QWidget *titleRow = new QWidget();
    titleRow->setStyleSheet("background: transparent;");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleRow);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(10);

    QLabel *titleLabel = new QLabel(tr("导出详情"));
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1e293b; background: transparent;");

    m_exportIdLabel = new QLabel();
    m_exportIdLabel->setStyleSheet("font-size: 12px; color: #94a3b8; background: transparent;");

    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(m_exportIdLabel);
    titleLayout->addStretch();

    layout->addWidget(titleRow);

    QWidget *grid = new QWidget();
    grid->setStyleSheet("background: transparent;");
    QGridLayout *gridLayout = new QGridLayout(grid);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(12);
    gridLayout->addWidget(createDetailItem(tr("作品 ID"), &m_novelIdLabel), 0, 0);
    gridLayout->addWidget(createDetailItem(tr("格式"), &m_formatLabel), 0, 1);
    gridLayout->addWidget(createDetailItem(tr("状态"), &m_statusLabel), 1, 0);
    gridLayout->addWidget(createDetailItem(tr("文件大小"), &m_fileSizeLabel), 1, 1);
    gridLayout->addWidget(createDetailItem(tr("创建时间"), &m_createdAtLabel), 2, 0, 1, 2);
    gridLayout->addWidget(createDetailItem(tr("文件路径"), &m_fileUrlLabel), 3, 0, 1, 2);

    layout->addWidget(grid);

    QWidget *btnRow = new QWidget();
    btnRow->setStyleSheet("background: transparent;");
    QHBoxLayout *btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(12);

    m_openBtn = new QPushButton(tr("打开文件"));
    m_openBtn->setIcon(QIcon(renderSvg(QStringLiteral(":/icons/fruit_strawberry.svg"), 18)));
    m_openBtn->setIconSize(QSize(18, 18));
    m_openBtn->setStyleSheet(OPEN_BTN_STYLE);
    m_openBtn->setFocusPolicy(Qt::NoFocus);
    m_openBtn->setCursor(Qt::PointingHandCursor);
    connect(m_openBtn, &QPushButton::clicked, this, [this]() {
        if (!m_fileUrlLabel) return;
        const QString path = m_fileUrlLabel->text().trimmed();
        if (path.isEmpty() || path == QLatin1String("—")) return;
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });

    m_closeBtn = new QPushButton(tr("关闭"));
    m_closeBtn->setStyleSheet(CLOSE_BTN_STYLE);
    m_closeBtn->setFocusPolicy(Qt::NoFocus);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &ExportDetailDialog::animateFadeOut);

    btnLayout->addStretch();
    btnLayout->addWidget(m_openBtn);
    btnLayout->addWidget(m_closeBtn);

    layout->addWidget(btnRow);
}

QWidget* ExportDetailDialog::createDetailItem(const QString &labelText, QLabel **valueLabel)
{
    QWidget *widget = new QWidget();
    widget->setObjectName("detailItem");
    widget->setStyleSheet(DETAIL_ITEM_STYLE);
    widget->setMinimumHeight(DETAIL_ITEM_HEIGHT);

    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(6);

    QLabel *label = new QLabel(labelText);
    label->setStyleSheet("font-size: 12px; color: #64748b; background: transparent;");

    *valueLabel = new QLabel();
    (*valueLabel)->setStyleSheet("font-size: 14px; color: #1e293b; background: transparent;");
    (*valueLabel)->setWordWrap(true);
    (*valueLabel)->setMinimumHeight(24);

    layout->addWidget(label);
    layout->addWidget(*valueLabel);

    return widget;
}

void ExportDetailDialog::setData(const ExportResult &result)
{
    if (m_exportIdLabel)
        m_exportIdLabel->setText(result.id);
    if (m_novelIdLabel)
        m_novelIdLabel->setText(result.novelId);
    if (m_formatLabel)
        m_formatLabel->setText(ExportUtils::exportFormatLabel(result.format));
    if (m_statusLabel)
        m_statusLabel->setText(StatusHelper::Job::statusLabel(result.status));
    if (m_fileSizeLabel)
        m_fileSizeLabel->setText(formatBytes(result.fileSize));
    if (m_createdAtLabel)
        m_createdAtLabel->setText(formatDateTime(result.createdAt));

    const QString fileUrl = result.fileUrl;
    if (m_fileUrlLabel)
        m_fileUrlLabel->setText(fileUrl.isEmpty() ? QStringLiteral("—") : fileUrl);

    if (m_openBtn)
        m_openBtn->setEnabled(!fileUrl.isEmpty());
}

void ExportDetailDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    animateFadeIn();
}

void ExportDetailDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        animateFadeOut();
        return;
    }
    QDialog::keyPressEvent(event);
}

void ExportDetailDialog::animateFadeIn()
{
    QPropertyAnimation *anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(150);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ExportDetailDialog::animateFadeOut()
{
    QPropertyAnimation *anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(100);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    connect(anim, &QPropertyAnimation::finished, this, &QDialog::accept);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}
