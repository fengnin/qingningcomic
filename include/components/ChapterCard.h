#ifndef CHAPTERCARD_H
#define CHAPTERCARD_H

#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

// 章节卡片组件，显示章节信息和状态
class ChapterCard : public QFrame
{
    Q_OBJECT

public:
    explicit ChapterCard(int chapterNumber, int panelCount, 
                        const QString &status, bool isActive, QWidget *parent = nullptr);
    
    void setActive(bool active);
    bool isActive() const { return m_isActive; }
    int chapterNumber() const { return m_chapterNumber; }

signals:
    void clicked(int chapterNumber);
    void deleteRequested(int chapterNumber);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupUI();
    
    QHBoxLayout* createHeaderRow();
    QWidget* createInfoRow();

    int m_chapterNumber;
    int m_panelCount;
    QString m_status;
    bool m_isActive;
};

#endif // CHAPTERCARD_H
