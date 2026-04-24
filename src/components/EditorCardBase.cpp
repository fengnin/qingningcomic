#include "components/EditorCardBase.h"
#include "components/EditorStyles.h"
#include <QHBoxLayout>
#include <QApplication>

namespace {
    using EditorStyles::TRANSPARENT_BG;
}

EditorCardBase::EditorCardBase(QWidget *parent)
    : QFrame(parent)
    , m_overlayWidget(nullptr)
    , m_editorCard(nullptr)
{
    setFrameShape(QFrame::NoFrame);
}

void EditorCardBase::showEditorCard()
{
    if (!m_overlayWidget) {
        setupEditorCard();
    }
    
    if (!m_overlayWidget || !m_editorCard) {
        return;
    }
    
    syncDataToEditor();
    
    QWidget *parent = m_overlayWidget->parentWidget();
    if (parent) {
        m_overlayWidget->setGeometry(parent->rect());
    }
    
    updateEditorCardPosition();
    
    m_editorCard->show();
    m_overlayWidget->show();
    m_overlayWidget->raise();
}

void EditorCardBase::hideEditorCard()
{
    if (m_overlayWidget) {
        m_overlayWidget->hide();
    }
}

void EditorCardBase::updateEditorCardPosition()
{
    if (!m_overlayWidget || !m_editorCard) return;
    
    QRect overlayRect = m_overlayWidget->rect();
    
    int cardWidth = m_editorCard->minimumWidth();
    int cardHeight = m_editorCard->minimumHeight();
    
    if (cardWidth <= 0) cardWidth = m_editorCard->width();
    if (cardHeight <= 0) cardHeight = m_editorCard->height();
    
    if (cardWidth <= 0) cardWidth = 480;
    if (cardHeight <= 0) cardHeight = 600;
    
    int x = (overlayRect.width() - cardWidth) / 2;
    int y = (overlayRect.height() - cardHeight) / 2;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    
    m_editorCard->setFixedSize(cardWidth, cardHeight);
    m_editorCard->move(x, y);
}

QWidget* EditorCardBase::createOverlay()
{
    QWidget *parentWindow = window();
    if (!parentWindow) {
        parentWindow = this;
        while (parentWindow->parentWidget()) {
            parentWindow = parentWindow->parentWidget();
        }
    }
    
    QWidget *overlay = new QWidget(parentWindow);
    overlay->setStyleSheet("background: #4d000000;");
    overlay->hide();
    return overlay;
}

QFrame* EditorCardBase::createCardBase(int width, int height)
{
    QFrame *card = new QFrame(m_overlayWidget);
    card->setFixedSize(width, height);
    card->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 16px;
            border: 1px solid #E8E8F0;
        }
    )");
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 10);
    card->setGraphicsEffect(shadow);
    
    return card;
}

QWidget* EditorCardBase::createTitleRow(const QString &title)
{
    QWidget *row = new QWidget();
    row->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *titleLabel = createLabel(title, "#333333", 16, true);
    layout->addWidget(titleLabel);
    layout->addStretch();
    
    return row;
}

QWidget* EditorCardBase::createTextEditSection(const QString &title, QTextEdit *&edit, 
                                                const QString &content, int height,
                                                const QString &placeholder, const QString &hint)
{
    QWidget *section = new QWidget();
    section->setStyleSheet(TRANSPARENT_BG);
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    
    if (hint.isEmpty()) {
        layout->addWidget(createLabel(title, "#333333", 12, true));
    } else {
        QWidget *titleRow = new QWidget();
        titleRow->setStyleSheet(TRANSPARENT_BG);
        QHBoxLayout *titleLayout = new QHBoxLayout(titleRow);
        titleLayout->setContentsMargins(0, 0, 0, 0);
        titleLayout->setSpacing(8);
        titleLayout->addWidget(createLabel(title, "#333333", 12, true));
        titleLayout->addWidget(createLabel(hint, "#9CA3AF", 11));
        titleLayout->addStretch();
        layout->addWidget(titleRow);
    }
    
    edit = new QTextEdit();
    edit->setPlainText(content);
    edit->setStyleSheet(EditorStyles::textEditStyle());
    edit->setMinimumHeight(height);
    edit->setMaximumHeight(height);
    if (!placeholder.isEmpty()) {
        edit->setPlaceholderText(placeholder);
    }
    layout->addWidget(edit);
    
    return section;
}

QWidget* EditorCardBase::createButtonRow(QPushButton *&primaryBtn, const QString &primaryText,
                                          QPushButton *&secondaryBtn, const QString &secondaryText)
{
    QWidget *row = new QWidget();
    row->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    
    primaryBtn = new QPushButton(primaryText);
    primaryBtn->setStyleSheet(EditorStyles::saveButtonStyle());
    primaryBtn->setCursor(Qt::PointingHandCursor);
    primaryBtn->setMinimumHeight(40);
    layout->addWidget(primaryBtn);
    
    secondaryBtn = new QPushButton(secondaryText);
    secondaryBtn->setStyleSheet(EditorStyles::cancelButtonStyle());
    secondaryBtn->setCursor(Qt::PointingHandCursor);
    secondaryBtn->setMinimumHeight(40);
    layout->addWidget(secondaryBtn);
    
    layout->addStretch();
    return row;
}

QLabel* EditorCardBase::createLabel(const QString &text, const QString &color, int fontSize, bool bold)
{
    QLabel *label = new QLabel(text);
    QString style = QString("font-size: %1px; color: %2; background: transparent; border: none;")
                        .arg(fontSize).arg(color);
    if (bold) style += " font-weight: bold;";
    label->setStyleSheet(style);
    return label;
}

QFrame* EditorCardBase::createSeparator()
{
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background: #E5E7EB; border: none; max-height: 1px;");
    return separator;
}
