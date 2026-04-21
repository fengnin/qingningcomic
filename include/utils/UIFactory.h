#ifndef UI_FACTORY_H
#define UI_FACTORY_H

#include <QString>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QGraphicsDropShadowEffect>

namespace UIFactory {

inline QPushButton* createButton(const QString& text, QWidget* parent = nullptr)
{
    QPushButton* btn = new QPushButton(text, parent);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

inline QPushButton* createButton(const QString& text, const QString& style, QWidget* parent = nullptr)
{
    QPushButton* btn = createButton(text, parent);
    if (!style.isEmpty()) {
        btn->setStyleSheet(style);
    }
    return btn;
}

inline QPushButton* createButton(const QString& text, int width, int height, QWidget* parent = nullptr)
{
    QPushButton* btn = createButton(text, parent);
    btn->setFixedSize(width, height);
    return btn;
}

inline QPushButton* createButton(const QString& text, int width, int height, 
                                  const QString& style, QWidget* parent = nullptr)
{
    QPushButton* btn = createButton(text, width, height, parent);
    if (!style.isEmpty()) {
        btn->setStyleSheet(style);
    }
    return btn;
}

inline QLabel* createLabel(const QString& text, QWidget* parent = nullptr)
{
    return new QLabel(text, parent);
}

inline QLabel* createLabel(const QString& text, const QString& style, QWidget* parent = nullptr)
{
    QLabel* label = new QLabel(text, parent);
    if (!style.isEmpty()) {
        label->setStyleSheet(style);
    }
    return label;
}

inline QLabel* createLabel(const QString& text, int width, int height, QWidget* parent = nullptr)
{
    QLabel* label = new QLabel(text, parent);
    label->setFixedSize(width, height);
    return label;
}

inline QLineEdit* createLineEdit(const QString& placeholder = QString(), QWidget* parent = nullptr)
{
    QLineEdit* edit = new QLineEdit(parent);
    if (!placeholder.isEmpty()) {
        edit->setPlaceholderText(placeholder);
    }
    return edit;
}

inline QTextEdit* createTextEdit(const QString& placeholder = QString(), QWidget* parent = nullptr)
{
    QTextEdit* edit = new QTextEdit(parent);
    if (!placeholder.isEmpty()) {
        edit->setPlaceholderText(placeholder);
    }
    return edit;
}

inline QSpinBox* createSpinBox(int min, int max, int value = 0, QWidget* parent = nullptr)
{
    QSpinBox* spin = new QSpinBox(parent);
    spin->setRange(min, max);
    spin->setValue(value);
    return spin;
}

inline QComboBox* createComboBox(const QStringList& items, QWidget* parent = nullptr)
{
    QComboBox* combo = new QComboBox(parent);
    combo->addItems(items);
    return combo;
}

inline QWidget* createTransparentWidget(QWidget* parent = nullptr)
{
    QWidget* w = new QWidget(parent);
    w->setStyleSheet("background: transparent;");
    return w;
}

inline QWidget* createWidgetWithLayout(QLayout* layout, QWidget* parent = nullptr)
{
    QWidget* w = new QWidget(parent);
    w->setLayout(layout);
    return w;
}

inline QVBoxLayout* createVBoxLayout(int margin = 0, int spacing = 0)
{
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
    return layout;
}

inline QVBoxLayout* createVBoxLayout(int left, int top, int right, int bottom, int spacing = 0)
{
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(spacing);
    return layout;
}

inline QHBoxLayout* createHBoxLayout(int margin = 0, int spacing = 0)
{
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
    return layout;
}

inline QHBoxLayout* createHBoxLayout(int left, int top, int right, int bottom, int spacing = 0)
{
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(spacing);
    return layout;
}

inline QWidget* createLabeledField(const QString& labelText, QWidget* field, 
                                    const QString& labelStyle = QString(), QWidget* parent = nullptr)
{
    QWidget* container = createTransparentWidget(parent);
    QVBoxLayout* layout = createVBoxLayout(0, 4);
    
    QLabel* label = createLabel(labelText, labelStyle);
    layout->addWidget(label);
    layout->addWidget(field);
    
    container->setLayout(layout);
    return container;
}

inline QWidget* createLabeledLineEdit(const QString& labelText, QLineEdit*& lineEdit,
                                       const QString& labelStyle = QString(), QWidget* parent = nullptr)
{
    lineEdit = createLineEdit();
    return createLabeledField(labelText, lineEdit, labelStyle, parent);
}

inline QWidget* createLabeledTextEdit(const QString& labelText, QTextEdit*& textEdit,
                                       const QString& labelStyle = QString(), QWidget* parent = nullptr)
{
    textEdit = createTextEdit();
    return createLabeledField(labelText, textEdit, labelStyle, parent);
}

inline QWidget* createLabeledComboBox(const QString& labelText, QComboBox*& comboBox,
                                       const QStringList& items, const QString& labelStyle = QString(),
                                       QWidget* parent = nullptr)
{
    comboBox = createComboBox(items);
    return createLabeledField(labelText, comboBox, labelStyle, parent);
}

inline QWidget* createLabeledSpinBox(const QString& labelText, QSpinBox*& spinBox,
                                      int min, int max, int value, const QString& labelStyle = QString(),
                                      QWidget* parent = nullptr)
{
    spinBox = createSpinBox(min, max, value);
    return createLabeledField(labelText, spinBox, labelStyle, parent);
}

inline QScrollArea* createScrollArea(bool widgetResizable = true, QWidget* parent = nullptr)
{
    QScrollArea* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(widgetResizable);
    scroll->setFrameShape(QFrame::NoFrame);
    return scroll;
}

inline QFrame* createSeparator(QFrame::Shape shape = QFrame::HLine, QWidget* parent = nullptr)
{
    QFrame* line = new QFrame(parent);
    line->setFrameShape(shape);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

inline QGraphicsDropShadowEffect* createShadowEffect(int blurRadius = 20, 
                                                      const QColor& color = QColor(0, 0, 0, 50),
                                                      const QPointF& offset = QPointF(0, 4))
{
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(blurRadius);
    shadow->setColor(color);
    shadow->setOffset(offset);
    return shadow;
}

inline QWidget* createButtonRow(const QStringList& buttonTexts, 
                                 QList<QPushButton*>& buttons,
                                 int spacing = 8, QWidget* parent = nullptr)
{
    QWidget* row = createTransparentWidget(parent);
    QHBoxLayout* layout = createHBoxLayout(0, spacing);
    
    for (const QString& text : buttonTexts) {
        QPushButton* btn = createButton(text);
        buttons.append(btn);
        layout->addWidget(btn);
    }
    
    layout->addStretch();
    row->setLayout(layout);
    return row;
}

}

#endif // UI_FACTORY_H
