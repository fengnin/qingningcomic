#include "ConfirmDialog.h"
#include "StyleManager.h"
#include "EncodingUtils.h"
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>

namespace {
    constexpr int DIALOG_WIDTH = 400;
    constexpr int DIALOG_MIN_HEIGHT = 180;
    constexpr int BORDER_RADIUS = 16;
    constexpr int BUTTON_WIDTH = 90;
    constexpr int BUTTON_HEIGHT = 36;
    constexpr int ICON_SIZE = 40;
    constexpr int H_PADDING = 24;
    constexpr int V_PADDING = 24;
    constexpr int ELEMENT_SPACING = 16;
    
    const QString WARNING_COLOR = "#facc15";
    const QString WARNING_HOVER = "#eab308";
    const QString WARNING_PRESSED = "#ca8a04";
    const QString WARNING_BG = "#fef9c3";
}

bool ConfirmDialog::showConfirm(QWidget *parent, const QString &title, const QString &message)
{
    ConfirmDialog dialog(parent);
    dialog.setTitle(title);
    dialog.setMessage(message);
    dialog.adjustSize();
    return dialog.exec() == QDialog::Accepted;
}

ConfirmDialog::ConfirmDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    setupUI();
}

ConfirmDialog::~ConfirmDialog()
{
}

void ConfirmDialog::setupUI()
{
    m_container = new QWidget(this);
    m_container->setObjectName("dialogContainer");
    m_container->setStyleSheet(QString(
        "#dialogContainer {"
        "  background-color: #FFFFFF;"
        "  border-radius: %1px;"
        "  border: 1px solid #e5e5e5;"
        "}"
    ).arg(BORDER_RADIUS));
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 8);
    m_container->setGraphicsEffect(shadow);
    
    QVBoxLayout *containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(H_PADDING, V_PADDING, H_PADDING, V_PADDING);
    containerLayout->setSpacing(ELEMENT_SPACING);
    
    createHeader(containerLayout);
    createMessageArea(containerLayout);
    createButtonArea(containerLayout);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_container);
}

void ConfirmDialog::createHeader(QVBoxLayout *parentLayout)
{
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);
    
    m_iconLabel = new QLabel();
    m_iconLabel->setText("!");
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setFixedSize(ICON_SIZE, ICON_SIZE);
    m_iconLabel->setFont(QFont("Arial", 18, QFont::Bold));
    m_iconLabel->setStyleSheet(QString(
        "QLabel {"
        "  background-color: %1;"
        "  color: %2;"
        "  border-radius: %3px;"
        "}"
    ).arg(WARNING_BG, WARNING_COLOR).arg(ICON_SIZE / 2));
    
    m_titleLabel = new QLabel();
    m_titleLabel->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    m_titleLabel->setStyleSheet("color: #171717; background: transparent;");
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    headerLayout->addWidget(m_iconLabel);
    headerLayout->addWidget(m_titleLabel, 1);
    
    parentLayout->addLayout(headerLayout);
}

void ConfirmDialog::createMessageArea(QVBoxLayout *parentLayout)
{
    m_messageLabel = new QLabel();
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setFont(QFont("Microsoft YaHei", 11));
    m_messageLabel->setStyleSheet("color: #525252; background: transparent; line-height: 1.5;");
    m_messageLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    
    parentLayout->addWidget(m_messageLabel);
    parentLayout->addStretch();
}

void ConfirmDialog::createButtonArea(QVBoxLayout *parentLayout)
{
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);
    buttonLayout->setContentsMargins(0, 8, 0, 0);
    
    buttonLayout->addStretch();
    
    m_cancelBtn = new QPushButton(tr("取消"));
    m_cancelBtn->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setFont(QFont("Microsoft YaHei", 10));
    m_cancelBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #f5f5f5;"
        "  color: #525252;"
        "  border: 1px solid #e5e5e5;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #eeeeee;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #e5e5e5;"
        "}"
    );
    
    m_confirmBtn = new QPushButton(tr("确定"));
    m_confirmBtn->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    m_confirmBtn->setCursor(Qt::PointingHandCursor);
    m_confirmBtn->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    m_confirmBtn->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: #1a2e05;"
        "  border: none;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %2;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %3;"
        "}"
    ).arg(WARNING_COLOR, WARNING_HOVER, WARNING_PRESSED));
    
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_confirmBtn);
    
    parentLayout->addLayout(buttonLayout);
    
    connect(m_confirmBtn, &QPushButton::clicked, this, &ConfirmDialog::onConfirmClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &ConfirmDialog::onCancelClicked);
}

void ConfirmDialog::setTitle(const QString &title)
{
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void ConfirmDialog::setMessage(const QString &message)
{
    if (m_messageLabel) {
        m_messageLabel->setText(message);
    }
}

void ConfirmDialog::adjustSize()
{
    if (!m_messageLabel) return;
    
    QFontMetrics msgFm(m_messageLabel->font());
    int msgWidth = DIALOG_WIDTH - H_PADDING * 2;
    int msgHeight = msgFm.boundingRect(0, 0, msgWidth, 0, 
                                        Qt::TextWordWrap | Qt::AlignCenter, 
                                        m_messageLabel->text()).height();
    msgHeight = qMax(msgHeight, 30);
    
    int totalHeight = V_PADDING
                    + ICON_SIZE
                    + ELEMENT_SPACING
                    + msgHeight
                    + ELEMENT_SPACING
                    + BUTTON_HEIGHT + 8
                    + V_PADDING;
    
    totalHeight = qMax(totalHeight, DIALOG_MIN_HEIGHT);
    
    setFixedSize(DIALOG_WIDTH, totalHeight);
    m_container->setFixedSize(DIALOG_WIDTH, totalHeight);
}

void ConfirmDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(150);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ConfirmDialog::onConfirmClicked()
{
    m_confirmed = true;
    animateClose();
}

void ConfirmDialog::onCancelClicked()
{
    m_confirmed = false;
    animateClose();
}

void ConfirmDialog::animateClose()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(100);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
        m_confirmed ? accept() : reject();
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
