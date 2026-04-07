#ifndef EDITORCARDBASE_H
#define EDITORCARDBASE_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>

class EditorCardBase : public QFrame
{
    Q_OBJECT

public:
    explicit EditorCardBase(QWidget *parent = nullptr);
    virtual ~EditorCardBase() = default;

protected:
    void showEditorCard();
    void hideEditorCard();
    void updateEditorCardPosition();
    
    QWidget* createOverlay();
    QFrame* createCardBase(int width, int height);
    QWidget* createTitleRow(const QString &title);
    QWidget* createTextEditSection(const QString &title, QTextEdit *&edit, 
                                    const QString &content, int height,
                                    const QString &placeholder = QString(),
                                    const QString &hint = QString());
    QWidget* createButtonRow(QPushButton *&primaryBtn, const QString &primaryText,
                              QPushButton *&secondaryBtn, const QString &secondaryText);
    
    QLabel* createLabel(const QString &text, const QString &color, int fontSize, bool bold = false);
    QFrame* createSeparator();
    
    QWidget *m_overlayWidget;
    QFrame *m_editorCard;
    
private slots:
    virtual void onCloseButtonClicked() { hideEditorCard(); }
    
private:
    virtual void setupEditorCard() = 0;
    virtual void syncDataToEditor() = 0;
};

#endif // EDITORCARDBASE_H
