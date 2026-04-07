#ifndef CHAPTERSPINBOX_H
#define CHAPTERSPINBOX_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSet>

// 自定义数字输入框，带三角形上下箭头
class ChapterSpinBox : public QWidget
{
    Q_OBJECT

public:
    explicit ChapterSpinBox(QWidget *parent = nullptr);
    
    void setValue(int value);
    int value() const { return m_value; }
    void setMinimum(int min) { m_minimum = min; }
    void setMaximum(int max) { m_maximum = max; }
    void setFixedHeight(int height);
    // 设置已存在的章节号列表，用于跳过
    void setExistingChapters(const QSet<int>& chapters);
    void setExistingChapters(const QList<int>& chapters);

signals:
    void valueChanged(int value);

private slots:
    void onUpClicked();
    void onDownClicked();

private:
    QLineEdit *m_lineEdit;
    QPushButton *m_upBtn;
    QPushButton *m_downBtn;
    int m_value;
    int m_minimum;
    int m_maximum;
    QSet<int> m_existingChapters;  // 已存在的章节号
};

#endif // CHAPTERSPINBOX_H
