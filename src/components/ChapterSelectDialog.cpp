#include "components/ChapterSelectDialog.h"
#include "components/EditorStyles.h"
#include <QDialogButtonBox>

using namespace EditorStyles;

namespace {
    const QString DIALOG_STYLE = R"(
        QDialog {
            background: white;
            border-radius: 12px;
        }
    )";
    
    const QString TITLE_STYLE = R"(
        QLabel {
            color: #1f2937;
            font-size: 15px;
            font-weight: bold;
        }
    )";
    
    const QString BUTTON_STYLE = R"(
        QPushButton {
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
        }
    )";
    
    const QString CANCEL_BUTTON_STYLE = BUTTON_STYLE + R"(
        QPushButton {
            background: #f3f4f6;
            color: #6b7280;
            border: none;
        }
        QPushButton:hover {
            background: #e5e7eb;
        }
    )";
    
    const QString OK_BUTTON_STYLE = BUTTON_STYLE + R"(
        QPushButton {
            background: #84cc16;
            color: white;
            border: none;
        }
        QPushButton:hover {
            background: #a3e635;
        }
    )";
}

ChapterSelectDialog::ChapterSelectDialog(QWidget *parent)
    : QDialog(parent)
    , m_titleLabel(nullptr)
    , m_spinBox(nullptr)
    , m_cancelBtn(nullptr)
    , m_okBtn(nullptr)
{
    setupUI();
    setupConnections();
}

void ChapterSelectDialog::setupUI()
{
    setWindowTitle(QString::fromUtf8("选择章节"));
    setFixedSize(220, 150);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setStyleSheet(DIALOG_STYLE);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(10);
    
    m_titleLabel = new QLabel(QString::fromUtf8("选择章节"));
    m_titleLabel->setStyleSheet(TITLE_STYLE);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    m_spinBox = new ChapterSpinBox();
    m_spinBox->setMinimum(1);
    m_spinBox->setMaximum(9999);
    m_spinBox->setValue(1);
    m_spinBox->setFixedHeight(40);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    
    m_cancelBtn = new QPushButton(QString::fromUtf8("取消"));
    m_cancelBtn->setFixedSize(85, 36);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet(CANCEL_BUTTON_STYLE);
    
    m_okBtn = new QPushButton(QString::fromUtf8("确定"));
    m_okBtn->setFixedSize(85, 36);
    m_okBtn->setCursor(Qt::PointingHandCursor);
    m_okBtn->setStyleSheet(OK_BUTTON_STYLE);
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_okBtn);
    
    layout->addWidget(m_titleLabel, 0, Qt::AlignCenter);
    layout->addWidget(m_spinBox, 0, Qt::AlignCenter);
    layout->addLayout(btnLayout);
}

void ChapterSelectDialog::setupConnections()
{
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void ChapterSelectDialog::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void ChapterSelectDialog::setValue(int value)
{
    m_spinBox->setValue(value);
}

void ChapterSelectDialog::setRange(int min, int max)
{
    m_spinBox->setMinimum(min);
    m_spinBox->setMaximum(max);
}

void ChapterSelectDialog::setExistingChapters(const QSet<int>& chapters)
{
    m_spinBox->setExistingChapters(chapters);
}

void ChapterSelectDialog::setExistingChapters(const QList<int>& chapters)
{
    m_spinBox->setExistingChapters(chapters);
}

int ChapterSelectDialog::value() const
{
    return m_spinBox->value();
}

int ChapterSelectDialog::selectChapter(QWidget *parent, int currentValue, int min, int max)
{
    ChapterSelectDialog dialog(parent);
    dialog.setRange(min, max);
    dialog.setValue(currentValue);
    
    if (dialog.exec() == QDialog::Accepted) {
        return dialog.value();
    }
    return -1;
}
