#include "components/SuccessDialog.h"
#include "utils/EncodingUtils.h"
#include "utils/UIFactory.h"
#include <QPropertyAnimation>
#include <QKeyEvent>

namespace {
    constexpr int DIALOG_WIDTH = 440;
    constexpr int DIALOG_HEIGHT = 320;
    constexpr int HEADER_HEIGHT = 80;
    constexpr int FOOTER_HEIGHT = 60;
    constexpr int BUTTON_WIDTH = 100;
    constexpr int BUTTON_HEIGHT = 36;
    constexpr int BORDER_RADIUS = 16;
    constexpr int FADE_IN_DURATION = 150;
    constexpr int FADE_OUT_DURATION = 100;
    
    const QMap<SuccessDialog::Type, QString> ICON_TEXTS = {
        {SuccessDialog::Success, QString::fromUtf8("✓")},
        {SuccessDialog::Warning, "!"},
        {SuccessDialog::Error, QString::fromUtf8("×")},
        {SuccessDialog::Info, "i"}
    };
    
    const QMap<SuccessDialog::Type, SuccessDialog::ColorScheme> COLOR_SCHEMES = {
        {SuccessDialog::Success, {"#4CAF50", "#43A047", "#388E3C"}},
        {SuccessDialog::Warning, {"#FF9800", "#F57C00", "#EF6C00"}},
        {SuccessDialog::Error, {"#F44336", "#D32F2F", "#C62828"}},
        {SuccessDialog::Info, {"#2196F3", "#1976D2", "#1565C0"}}
    };
}

void SuccessDialog::showSuccess(QWidget *parent, const QString &title, const QString &message)
{
    showDialog(parent, Success, title, message);
}

void SuccessDialog::showWarning(QWidget *parent, const QString &title, const QString &message)
{
    showDialog(parent, Warning, title, message);
}

void SuccessDialog::showError(QWidget *parent, const QString &title, const QString &message)
{
    showDialog(parent, Error, title, message);
}

void SuccessDialog::showInfo(QWidget *parent, const QString &title, const QString &message)
{
    showDialog(parent, Info, title, message);
}

void SuccessDialog::showDialog(QWidget *parent, Type type, const QString &title, const QString &message)
{
    SuccessDialog *dialog = new SuccessDialog(parent);
    dialog->setType(type);
    dialog->setTitle(title);
    dialog->setMessage(message);
    dialog->exec();
    dialog->deleteLater();
}

SuccessDialog::SuccessDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void SuccessDialog::setupUI()
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(DIALOG_WIDTH, DIALOG_HEIGHT);
    
    m_container = new QWidget(this);
    m_container->setObjectName("dialogContainer");
    m_container->setGeometry(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT);
    
    QVBoxLayout *containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    createHeader(containerLayout);
    createContent(containerLayout);
    createFooter(containerLayout);
    
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

void SuccessDialog::createHeader(QVBoxLayout *parentLayout)
{
    QWidget *headerWidget = UIFactory::createTransparentWidget();
    headerWidget->setFixedHeight(HEADER_HEIGHT);
    headerWidget->setObjectName("headerWidget");
    
    QHBoxLayout *headerLayout = UIFactory::createHBoxLayout(24, 24, 24, 16, 12);
    
    m_iconLabel = UIFactory::createLabel(QString());
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setObjectName("iconLabel");
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setFont(QFont("Segoe UI Symbol", 20, QFont::Bold));
    
    m_titleLabel = UIFactory::createLabel(QString());
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setFont(QFont("Microsoft YaHei", 16, QFont::Bold));

    headerLayout->addWidget(m_iconLabel);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    
    headerWidget->setLayout(headerLayout);
    parentLayout->addWidget(headerWidget);
}

void SuccessDialog::createContent(QVBoxLayout *parentLayout)
{
    QWidget *contentWidget = UIFactory::createTransparentWidget();
    contentWidget->setObjectName("contentWidget");
    
    QVBoxLayout *contentLayout = UIFactory::createVBoxLayout(24, 8, 24, 16, 16);
    
    m_messageLabel = UIFactory::createLabel(QString());
    m_messageLabel->setObjectName("messageLabel");
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setTextFormat(Qt::PlainText);
    m_messageLabel->setFont(QFont("Microsoft YaHei", 11));
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setMinimumWidth(DIALOG_WIDTH - 80);
    
    contentLayout->addWidget(m_messageLabel);
    
    m_detailsWidget = UIFactory::createTransparentWidget();
    m_detailsWidget->setObjectName("detailsWidget");
    m_detailsLayout = UIFactory::createVBoxLayout(20, 16, 20, 16, 12);
    m_detailsWidget->hide();
    
    contentLayout->addWidget(m_detailsWidget);
    contentLayout->addStretch();
    
    contentWidget->setLayout(contentLayout);
    parentLayout->addWidget(contentWidget);
}

void SuccessDialog::createFooter(QVBoxLayout *parentLayout)
{
    QWidget *footerWidget = UIFactory::createTransparentWidget();
    footerWidget->setFixedHeight(FOOTER_HEIGHT);
    footerWidget->setObjectName("footerWidget");
    
    QHBoxLayout *footerLayout = UIFactory::createHBoxLayout(24, 0, 24, 16, 0);
    footerLayout->addStretch();
    
    m_closeBtn = UIFactory::createButton(QString::fromUtf8("确定"), BUTTON_WIDTH, BUTTON_HEIGHT);
    m_closeBtn->setObjectName("closeBtn");
    m_closeBtn->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setDefault(true);
    
    footerLayout->addWidget(m_closeBtn);
    footerWidget->setLayout(footerLayout);
    parentLayout->addWidget(footerWidget);
    
    connect(m_closeBtn, &QPushButton::clicked, this, &SuccessDialog::onCloseClicked);
}

void SuccessDialog::applyStyle()
{
    ColorScheme scheme = getColorScheme();
    
    m_container->setStyleSheet(QString(R"(
        #dialogContainer {
            background-color: #FFFFFF;
            border-radius: %1px;
            border: 1px solid #E0E0E0;
        }
        #headerWidget, #contentWidget, #footerWidget {
            background-color: transparent;
        }
        #iconLabel {
            color: %2;
        }
        #titleLabel { 
            color: #212121; 
        }
        #messageLabel { 
            color: #616161; 
            line-height: 1.6; 
        }
        #detailsWidget { 
            background-color: #F8F8F8; 
            border-radius: 8px; 
        }
        #closeBtn {
            background-color: %3;
            color: white;
            border: none;
            border-radius: 8px;
        }
        #closeBtn:hover { 
            background-color: %4; 
        }
        #closeBtn:pressed { 
            background-color: %5; 
        }
    )").arg(BORDER_RADIUS).arg(scheme.primary).arg(scheme.primary).arg(scheme.hover).arg(scheme.pressed));
}

QString SuccessDialog::getIconText() const
{
    return ICON_TEXTS.value(m_type, QString::fromUtf8("?"));
}

SuccessDialog::ColorScheme SuccessDialog::getColorScheme() const
{
    return COLOR_SCHEMES.value(m_type, {"#4CAF50", "#43A047", "#388E3C"});
}

void SuccessDialog::setType(Type type)
{
    m_type = type;
    applyStyle();
    m_iconLabel->setText(getIconText());
}

void SuccessDialog::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void SuccessDialog::setMessage(const QString &message)
{
    m_messageLabel->setText(message);
    m_messageLabel->adjustSize();
}

void SuccessDialog::setDetails(const QStringList &details)
{
    if (details.isEmpty()) {
        m_detailsWidget->hide();
        return;
    }
    
    qDeleteAll(m_detailsWidget->findChildren<QLabel*>());
    
    QFont detailFont("Microsoft YaHei", 11);
    for (const QString &detail : details) {
        QLabel *detailLabel = new QLabel(detail);
        detailLabel->setFont(detailFont);
        detailLabel->setStyleSheet("color: #616161; background: transparent;");
        m_detailsLayout->addWidget(detailLabel);
    }
    
    m_detailsWidget->show();
}

void SuccessDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    animateFadeIn();
}

void SuccessDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        onCloseClicked();
        return;
    }
    QDialog::keyPressEvent(event);
}

void SuccessDialog::animateFadeIn()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(FADE_IN_DURATION);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void SuccessDialog::animateFadeOut()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(FADE_OUT_DURATION);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    connect(animation, &QPropertyAnimation::finished, this, &QDialog::accept);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void SuccessDialog::onCloseClicked()
{
    animateFadeOut();
}
