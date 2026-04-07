#include "components/ChapterSpinBox.h"

namespace {
    const QString SPIN_BTN_STYLE = R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #4d7c0f;
            font-size: 8px;
            border-radius: 4px;
        }
        QPushButton:hover { 
            background: #2684cc16; 
        }
        QPushButton:pressed { 
            background: #4084cc16; 
        }
    )";
    
    const QString SPIN_EDIT_STYLE = R"(
        QLineEdit {
            border-radius: 10px 0 0 10px;
            border: 1px solid #d4d4d4;
            border-right: none;
            background: white;
            padding: 0 14px;
            font-size: 14px;
            color: #374151;
        }
        QLineEdit:focus { 
            border-color: #84cc16; 
        }
    )";
    
    const QString SPIN_CONTAINER_STYLE = R"(
        QWidget {
            background: white;
            border: 1px solid #d4d4d4;
            border-left: none;
            border-radius: 0 10px 10px 0;
        }
    )";
}

ChapterSpinBox::ChapterSpinBox(QWidget *parent)
    : QWidget(parent)
    , m_lineEdit(nullptr)
    , m_upBtn(nullptr)
    , m_downBtn(nullptr)
    , m_value(1)
    , m_minimum(1)
    , m_maximum(9999)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    m_lineEdit = new QLineEdit();
    m_lineEdit->setText(QString::number(m_value));
    m_lineEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_lineEdit->setStyleSheet(SPIN_EDIT_STYLE);
    connect(m_lineEdit, &QLineEdit::textChanged, [this](const QString &text) {
        bool ok;
        int v = text.toInt(&ok);
        if (ok && v >= m_minimum && v <= m_maximum) {
            m_value = v;
            emit valueChanged(v);
        }
    });
    layout->addWidget(m_lineEdit, 1);
    
    QWidget *btnContainer = new QWidget();
    btnContainer->setFixedWidth(28);
    btnContainer->setStyleSheet(SPIN_CONTAINER_STYLE);
    
    QVBoxLayout *btnLayout = new QVBoxLayout(btnContainer);
    btnLayout->setContentsMargins(2, 0, 2, 0);
    btnLayout->setSpacing(0);
    
    QString upArrow = QString::fromUtf8("\u25b2");
    m_upBtn = new QPushButton(upArrow);
    m_upBtn->setFixedSize(24, 20);
    m_upBtn->setCursor(Qt::PointingHandCursor);
    m_upBtn->setStyleSheet(SPIN_BTN_STYLE);
    connect(m_upBtn, &QPushButton::clicked, this, &ChapterSpinBox::onUpClicked);
    btnLayout->addWidget(m_upBtn);
    
    QString downArrow = QString::fromUtf8("\u25bc");
    m_downBtn = new QPushButton(downArrow);
    m_downBtn->setFixedSize(24, 20);
    m_downBtn->setCursor(Qt::PointingHandCursor);
    m_downBtn->setStyleSheet(SPIN_BTN_STYLE);
    connect(m_downBtn, &QPushButton::clicked, this, &ChapterSpinBox::onDownClicked);
    btnLayout->addWidget(m_downBtn);
    
    layout->addWidget(btnContainer);
    setFocusProxy(m_lineEdit);
}

void ChapterSpinBox::setValue(int value)
{
    if (value >= m_minimum && value <= m_maximum) {
        m_value = value;
        m_lineEdit->setText(QString::number(value));
    }
}

void ChapterSpinBox::setFixedHeight(int height)
{
    QWidget::setFixedHeight(height);
    if (m_lineEdit) m_lineEdit->setFixedHeight(height);
}

void ChapterSpinBox::setExistingChapters(const QSet<int>& chapters)
{
    m_existingChapters = chapters;
}

void ChapterSpinBox::setExistingChapters(const QList<int>& chapters)
{
    m_existingChapters.clear();
    for (int ch : chapters) {
        m_existingChapters.insert(ch);
    }
}

void ChapterSpinBox::onUpClicked()
{
    int newValue = m_value + 1;
    while (newValue <= m_maximum && m_existingChapters.contains(newValue)) {
        newValue++;
    }
    
    if (newValue <= m_maximum) {
        m_value = newValue;
        m_lineEdit->setText(QString::number(m_value));
        emit valueChanged(m_value);
    }
}

void ChapterSpinBox::onDownClicked()
{
    int newValue = m_value - 1;
    while (newValue >= m_minimum && m_existingChapters.contains(newValue)) {
        newValue--;
    }
    
    if (newValue >= m_minimum) {
        m_value = newValue;
        m_lineEdit->setText(QString::number(m_value));
        emit valueChanged(m_value);
    }
}
