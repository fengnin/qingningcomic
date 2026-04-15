#include "components/ModeComboBox.h"
#include <QEvent>

namespace {
    const QString DROP_BTN_STYLE = R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #4d7c0f;
            font-size: 10px;
            border-radius: 4px;
        }
        QPushButton:hover { 
            background: #2684cc16; 
        }
        QPushButton:pressed { 
            background: #4084cc16; 
        }
    )";
    
    const QString COMBO_EDIT_STYLE = R"(
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
        QLineEdit:read-only { color: #374151; }
    )";
    
    const QString COMBO_CONTAINER_STYLE = R"(
        QWidget {
            background: white;
            border: 1px solid #d4d4d4;
            border-left: none;
            border-radius: 0 10px 10px 0;
        }
    )";
    
    const QString MENU_STYLE = R"(
        QMenu {
            background: white;
            border: 1px solid #d4d4d4;
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
            background: #2684cc16;
            color: #4d7c0f;
        }
    )";
}

ModeComboBox::ModeComboBox(QWidget *parent)
    : QWidget(parent)
    , m_lineEdit(nullptr)
    , m_dropBtn(nullptr)
    , m_menu(nullptr)
    , m_currentIndex(0)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    m_lineEdit = new QLineEdit();
    m_lineEdit->setReadOnly(true);
    m_lineEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_lineEdit->setStyleSheet(COMBO_EDIT_STYLE);
    m_lineEdit->setCursor(Qt::PointingHandCursor);
    m_lineEdit->installEventFilter(this);
    layout->addWidget(m_lineEdit, 1);
    
    QWidget *btnContainer = new QWidget();
    btnContainer->setFixedWidth(28);
    btnContainer->setStyleSheet(COMBO_CONTAINER_STYLE);
    
    QHBoxLayout *btnLayout = new QHBoxLayout(btnContainer);
    btnLayout->setContentsMargins(2, 0, 2, 0);
    
    QString dropArrow = QStringLiteral("▼");
    m_dropBtn = new QPushButton(dropArrow);
    m_dropBtn->setFixedSize(24, 36);
    m_dropBtn->setCursor(Qt::PointingHandCursor);
    m_dropBtn->setStyleSheet(DROP_BTN_STYLE);
    connect(m_dropBtn, &QPushButton::clicked, this, &ModeComboBox::onDropDownClicked);
    btnLayout->addWidget(m_dropBtn);
    
    layout->addWidget(btnContainer);
    
    m_menu = new QMenu(this);
    m_menu->setStyleSheet(MENU_STYLE);
}

void ModeComboBox::addItem(const QString &text)
{
    m_items.append(text);
    QAction *action = m_menu->addAction(text);
    action->setData(m_items.size() - 1);
    connect(action, &QAction::triggered, this, [this, action]() {
        int idx = action->data().toInt();
        setCurrentIndex(idx);
    });
    
    if (m_items.size() == 1) {
        m_lineEdit->setText(text);
    }
}

void ModeComboBox::setCurrentIndex(int index)
{
    if (index >= 0 && index < m_items.size()) {
        m_currentIndex = index;
        m_lineEdit->setText(m_items[index]);
        emit currentIndexChanged(index);
    }
}

QString ModeComboBox::currentText() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_items.size()) {
        return m_items[m_currentIndex];
    }
    return QString();
}

int ModeComboBox::findText(const QString &text) const
{
    return m_items.indexOf(text);
}

void ModeComboBox::setFixedHeight(int height)
{
    QWidget::setFixedHeight(height);
    if (m_lineEdit) m_lineEdit->setFixedHeight(height);
}

void ModeComboBox::onDropDownClicked()
{
    QPoint pos = mapToGlobal(QPoint(0, height()));
    m_menu->popup(pos);
}

bool ModeComboBox::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_lineEdit && event->type() == QEvent::MouseButtonPress) {
        onDropDownClicked();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}
