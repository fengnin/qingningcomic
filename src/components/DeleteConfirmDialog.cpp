#include "components/DeleteConfirmDialog.h"
#include "utils/UIFactory.h"
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QPropertyAnimation>

namespace {
    constexpr int DIALOG_WIDTH = 420;
    constexpr int DIALOG_MIN_HEIGHT = 180;
    constexpr int BUTTON_WIDTH = 100;
    constexpr int BUTTON_HEIGHT = 36;
    constexpr int H_PADDING = 32;
    constexpr int V_PADDING = 28;
    constexpr int ELEMENT_SPACING = 20;

    const QString DIALOG_BASE_STYLE = "QDialog { background: #FFFFFF; }";
    const QString DIALOG_TITLE_STYLE = "font-size: 18px; font-weight: bold; color: #1F2937; background: transparent;";
    const QString DIALOG_SUBTITLE_STYLE = "font-size: 14px; color: #6B7280; background: transparent;";
    const QString CANCEL_BUTTON_STYLE = R"(
        QPushButton {
          background-color: #f5f5f5;
          color: #525252;
          border: 1px solid #e5e5e5;
          border-radius: 8px;
        }
        QPushButton:hover {
          background-color: #eeeeee;
        }
        QPushButton:pressed {
          background-color: #e5e5e5;
        }
    )";
    const QString CONFIRM_BUTTON_STYLE = R"(
        QPushButton {
          background-color: #ef4444;
          color: white;
          border: none;
          border-radius: 8px;
        }
        QPushButton:hover {
          background-color: #dc2626;
        }
        QPushButton:pressed {
          background-color: #b91c1c;
        }
    )";

    const QString WARNING_LABEL_STYLE = R"(
        font-size: 13px;
        color: #991B1B;
        background: #0def4444;
        border: 1px solid #26ef4444;
        border-radius: 8px;
        padding: 12px 14px;
    )";

    QString defaultWarningText()
    {
        return QString::fromUtf8("⚠️ 删除后所有关联数据将被永久清除！");
    }

    QString defaultConfirmText()
    {
        return QString::fromUtf8("确认删除");
    }

    QPushButton* createActionButton(const QString& text, const QString& style, bool bold = false)
    {
        QPushButton* button = UIFactory::createButton(text, BUTTON_WIDTH, BUTTON_HEIGHT);
        button->setFont(QFont("Microsoft YaHei", 10, bold ? QFont::Bold : QFont::Normal));
        button->setStyleSheet(style);
        return button;
    }
}

bool DeleteConfirmDialog::showConfirm(QWidget *parent, const QString &title, const QString &message,
                                      const QString &warningText, const QString &confirmText)
{
    DeleteConfirmDialog dialog(parent);
    dialog.setTitle(title);
    dialog.setMessage(message);
    dialog.setWarningText(warningText.isEmpty() ? defaultWarningText() : warningText);
    dialog.setConfirmText(confirmText.isEmpty() ? defaultConfirmText() : confirmText);
    dialog.updateFixedSize();
    return dialog.exec() == QDialog::Accepted;
}

DeleteConfirmDialog::DeleteConfirmDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("确认删除"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupUI();
}

void DeleteConfirmDialog::setupUI()
{
    setStyleSheet(DIALOG_BASE_STYLE);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(H_PADDING, V_PADDING, H_PADDING, 24);
    mainLayout->setSpacing(ELEMENT_SPACING);

    createHeader(mainLayout);
    createMessageArea(mainLayout);
    createWarningArea(mainLayout);
    createButtonArea(mainLayout);
}

void DeleteConfirmDialog::createHeader(QVBoxLayout *parentLayout)
{
    m_titleLabel = UIFactory::createLabel(QString());
    m_titleLabel->setFont(QFont("Microsoft YaHei", 18, QFont::Bold));
    m_titleLabel->setStyleSheet(DIALOG_TITLE_STYLE);
    m_titleLabel->setWordWrap(true);
    parentLayout->addWidget(m_titleLabel);
}

void DeleteConfirmDialog::createMessageArea(QVBoxLayout *parentLayout)
{
    m_messageLabel = UIFactory::createLabel(QString());
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setFont(QFont("Microsoft YaHei", 14));
    m_messageLabel->setStyleSheet(DIALOG_SUBTITLE_STYLE);
    m_messageLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

    parentLayout->addWidget(m_messageLabel);
}

void DeleteConfirmDialog::createWarningArea(QVBoxLayout *parentLayout)
{
    m_warningLabel = UIFactory::createLabel(QString());
    m_warningLabel->setWordWrap(true);
    m_warningLabel->setFont(QFont("Microsoft YaHei", 13));
    m_warningLabel->setStyleSheet(WARNING_LABEL_STYLE);
    parentLayout->addWidget(m_warningLabel);
}

void DeleteConfirmDialog::createButtonArea(QVBoxLayout *parentLayout)
{
    QHBoxLayout *buttonLayout = UIFactory::createHBoxLayout(0, 12);
    buttonLayout->addStretch();

    m_cancelBtn = createActionButton(QString::fromUtf8("取消"), CANCEL_BUTTON_STYLE);
    m_confirmBtn = createActionButton(defaultConfirmText(), CONFIRM_BUTTON_STYLE, true);

    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_confirmBtn);

    parentLayout->addLayout(buttonLayout);

    connect(m_confirmBtn, &QPushButton::clicked, this, &DeleteConfirmDialog::onConfirmClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &DeleteConfirmDialog::onCancelClicked);
}

void DeleteConfirmDialog::setTitle(const QString &title)
{
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void DeleteConfirmDialog::setMessage(const QString &message)
{
    if (m_messageLabel) {
        m_messageLabel->setText(message);
    }
}

void DeleteConfirmDialog::setWarningText(const QString &warningText)
{
    if (m_warningLabel) {
        m_warningLabel->setText(warningText);
    }
}

void DeleteConfirmDialog::setConfirmText(const QString &confirmText)
{
    if (m_confirmBtn) {
        m_confirmBtn->setText(confirmText);
    }
}

void DeleteConfirmDialog::updateFixedSize()
{
    const int contentWidth = DIALOG_WIDTH - H_PADDING * 2;
    int messageHeight = 30;
    int warningHeight = 30;

    if (m_messageLabel) {
        QFontMetrics messageFm(m_messageLabel->font());
        messageHeight = qMax(30, messageFm.boundingRect(0, 0, contentWidth, 0,
            Qt::TextWordWrap | Qt::AlignCenter, m_messageLabel->text()).height());
    }

    if (m_warningLabel) {
        QFontMetrics warningFm(m_warningLabel->font());
        warningHeight = qMax(30, warningFm.boundingRect(0, 0, contentWidth, 0,
            Qt::TextWordWrap | Qt::AlignCenter, m_warningLabel->text()).height() + 8);
    }

    int totalHeight = V_PADDING
                    + 32
                    + ELEMENT_SPACING
                    + messageHeight
                    + ELEMENT_SPACING
                    + warningHeight
                    + ELEMENT_SPACING
                    + BUTTON_HEIGHT
                    + 8
                    + V_PADDING;
    totalHeight = qMax(totalHeight, DIALOG_MIN_HEIGHT);

    setFixedSize(DIALOG_WIDTH, totalHeight);
}

void DeleteConfirmDialog::onConfirmClicked()
{
    m_confirmed = true;
    animateClose();
}

void DeleteConfirmDialog::onCancelClicked()
{
    m_confirmed = false;
    animateClose();
}

void DeleteConfirmDialog::animateClose()
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
