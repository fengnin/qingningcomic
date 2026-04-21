#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include "pages/NovelDetailPage.h"
#include "components/ModeComboBox.h"

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);
    ~DashboardPage();

public slots:
    void updateJobStats(int total, int running, int completed, int failed);
    void refresh();

private:
    // ========== 初始化 ==========
    void setupUI();
    void setupConnections();
    void loadStatsFromDatabase();
    void startPolling();
    void stopPolling();
    void refreshActiveJobs();
    void refreshJobLists();
    void clearLayout(QLayout *layout);
    
    // ========== Hero区域 ==========
    QWidget* createHeroSection();
    QWidget* createHeroTextSection();
    QWidget* createStatsContainer();
    QWidget* createStatCard(const QString &label, const QString &bgColor, QLabel **valuePtr);
    
    // ========== 控制区域 ==========
    QWidget* createControlsSection();
    QWidget* createFilterGroup();
    QWidget* createFilter(const QString &label, const QStringList &items, ModeComboBox **comboPtr);
    QPushButton* createRefreshButton();
    
    // ========== 内容区域 ==========
    QWidget* createContentGrid();
    QWidget* createCard(const QString &objectName);
    QWidget* createCardHeader(const QString &title, const QString &subtitle);
    QWidget* createStandardCard(const QString &title, const QString &subtitle);
    
    // ========== 任务卡片 ==========
    struct JobItemData {
        QString type;
        QString status;
        QString time;
        int progress = 0;
        bool showProgress = false;
    };
    
    QWidget* createJobOverviewCard();
    void populateJobOverviewList();
    
    QWidget* createActiveJobsCard();
    void populateActiveJobsList();
    void populateJobList(QVBoxLayout *layout, const QString &whereClause,
                          const QVariantList &whereValues, const QString &emptyText,
                          const QString &orderBy, int limit, bool showProgress);
    
    QWidget* createJobItem(const JobItemData &data);
    QWidget* createTimelineJobItem(const QString &type, const QString &status,
                                    const QString &time, int progress);
    QWidget* createStandardJobItem(const QString &type, const QString &status, const QString &time);
    QWidget* createActiveJobsFooter();
    
    // ========== 指引卡片 ==========
    QWidget* createGuideCard();
    QWidget* createGuideStepList();
    QWidget* createGuideStep(int stepNumber, const QString &title, const QString &description);
    QLabel* createStepNumberLabel(int stepNumber);
    QWidget* createStepContent(const QString &title, const QString &description);

private:
    struct StatLabels {
        QLabel *total = nullptr;
        QLabel *running = nullptr;
        QLabel *completed = nullptr;
        QLabel *failed = nullptr;
        
        void setValues(int t, int r, int c, int f) {
            if (total) total->setText(QString::number(t));
            if (running) running->setText(QString::number(r));
            if (completed) completed->setText(QString::number(c));
            if (failed) failed->setText(QString::number(f));
        }
    } m_statLabels;
    
    QVBoxLayout *m_jobOverviewLayout = nullptr;
    QVBoxLayout *m_activeJobsLayout = nullptr;
    
    ModeComboBox *m_typeCombo = nullptr;
    ModeComboBox *m_statusCombo = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QTimer *m_refreshTimer = nullptr;
};

#endif // DASHBOARDPAGE_H
