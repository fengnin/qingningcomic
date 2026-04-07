#ifndef ANALYSISRESULTWIDGET_H
#define ANALYSISRESULTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QPropertyAnimation>

class AnalysisResultWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnalysisResultWidget(QWidget *parent = nullptr);
    
    void setResult(int chapterNumber, int panelCount, int characterCount, int sceneCount);
    void setResult(const QJsonObject &result);
    void clear();
    void showAnimated();
    void hideAnimated();

signals:
    void viewDetailsClicked();

private:
    void setupUI();
    void updateDisplay();

    QVBoxLayout *m_mainLayout;
    QWidget *m_contentWidget;
    
    QLabel *m_titleLabel;
    QLabel *m_chapterLabel;
    QWidget *m_statsContainer;
    QLabel *m_panelCountLabel;
    QLabel *m_characterCountLabel;
    QLabel *m_sceneCountLabel;
    
    int m_chapterNumber = 0;
    int m_panelCount = 0;
    int m_characterCount = 0;
    int m_sceneCount = 0;
    
    QPropertyAnimation *m_showAnimation;
    QPropertyAnimation *m_hideAnimation;
};

#endif // ANALYSISRESULTWIDGET_H
