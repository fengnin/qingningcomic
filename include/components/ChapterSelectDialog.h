#ifndef CHAPTERSELECTDIALOG_H
#define CHAPTERSELECTDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "components/ChapterSpinBox.h"

class ChapterSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChapterSelectDialog(QWidget *parent = nullptr);
    
    void setTitle(const QString &title);
    void setValue(int value);
    void setRange(int min, int max);
    void setExistingChapters(const QSet<int>& chapters);
    void setExistingChapters(const QList<int>& chapters);
    
    int value() const;
    
    static int selectChapter(QWidget *parent, int currentValue = 1, int min = 1, int max = 9999);

private:
    void setupUI();
    void setupConnections();
    
    QLabel *m_titleLabel;
    ChapterSpinBox *m_spinBox;
    QPushButton *m_cancelBtn;
    QPushButton *m_okBtn;
};

#endif // CHAPTERSELECTDIALOG_H
