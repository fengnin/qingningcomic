#include "pages/DashboardPage.h"
#include "data/DatabaseManager.h"
#include "utils/Logger.h"
#include "utils/StatusHelper.h"
#include "viewmodels/StoryboardViewModel.h"
#include "viewmodels/NovelViewModel.h"
#include "services/TaskQueue.h"
#include "components/EditorStyles.h"
#include "utils/EncodingUtils.h"
#include "utils/UserSession.h"
#include <QGridLayout>
#include <QFont>
#include <QTimer>
#include <QMap>

using namespace EditorStyles;
using namespace StatusHelper;

namespace {
    using EditorStyles::UI::createTransparentWidget;
    using EditorStyles::UI::createLabel;
    using EditorStyles::UI::setupLayout;
    
    struct StatCardData {
        QString label;
        QString bgColor;
    };
    
    const StatCardData STAT_CARDS[] = {
        {QString::fromUtf8("任务数"), Colors::DASHBOARD_STAT_TOTAL_BG},
        {QString::fromUtf8("进行中"), Colors::DASHBOARD_STAT_RUNNING_BG},
        {QString::fromUtf8("已完成"), Colors::DASHBOARD_STAT_COMPLETED_BG},
        {QString::fromUtf8("失败"), Colors::DASHBOARD_STAT_FAILED_BG}
    };
    const int STAT_CARDS_COUNT = sizeof(STAT_CARDS) / sizeof(StatCardData);
    
    struct GuideStepData {
        int number;
        QString title;
        QString description;
    };
    
    const GuideStepData GUIDE_STEPS[] = {
        {1, QString::fromUtf8("上传或选择作品"), QString::fromUtf8("在「项目空间」确认作品信息，确保已经分析小说。")},
        {2, QString::fromUtf8("提交自然语言修改"), QString::fromUtf8("作品详情页 → 修改请求，输入自然语言即可触发 CR-DSL 解析。")},
        {3, QString::fromUtf8("高清生成与导出"), QString::fromUtf8("选择预览/高清模式生成面板，完成后在导出中心获取 PDF / Webtoon。")}
    };
    const int GUIDE_STEPS_COUNT = sizeof(GUIDE_STEPS) / sizeof(GuideStepData);
    
    struct FilterOption {
        QString label;
        QStringList items;
    };
    
    const FilterOption TYPE_FILTER = {
        QString::fromUtf8("类型"),
        {
            QString::fromUtf8("全部类型"),
            QString::fromUtf8("分析小说"),
            QString::fromUtf8("面板预览"),
            QString::fromUtf8("面板高清"),
            QString::fromUtf8("修改请求"),
            QString::fromUtf8("面板编辑"),
            QString::fromUtf8("角色标准像"),
            QString::fromUtf8("导出 PDF"),
            QString::fromUtf8("导出 Webtoon"),
            QString::fromUtf8("导出资源包")
        }
    };
    
    const FilterOption STATUS_FILTER = {
        QString::fromUtf8("状态"),
        {
            QString::fromUtf8("全部状态"),
            QString::fromUtf8("排队中"),
            QString::fromUtf8("进行中"),
            QString::fromUtf8("已完成"),
            QString::fromUtf8("失败")
        }
    };
    
    const QString CATEGORY_DASHBOARD = QString::fromUtf8("总览");
    const QString EMPTY_JOBS_TEXT = QString::fromUtf8("暂无任务记录");
    const QString EMPTY_ACTIVE_JOBS_TEXT = QString::fromUtf8("暂无进行中的任务");
    const QString ACTIVE_JOBS_WHERE = "status IN ('running', 'in_progress', 'pending')";
    
    const QString CARD_TITLE_JOB_OVERVIEW = QString::fromUtf8("任务总览");
    const QString CARD_SUBTITLE_JOB_OVERVIEW = QString::fromUtf8("最新任务按时间排序");
    const QString CARD_TITLE_ACTIVE_JOBS = QString::fromUtf8("进行中的修改");
    const QString CARD_SUBTITLE_ACTIVE_JOBS = QString::fromUtf8("CR / 导出 / 面板任务进展");
    const QString CARD_TITLE_GUIDE = QString::fromUtf8("操作指引");
    const QString CARD_SUBTITLE_GUIDE = QString::fromUtf8("三步完成创作闭环");
    const QString FOOTER_TEXT_ACTIVE_JOBS = QString::fromUtf8("所有任务均可在 作品详情 → 修改请求 / 导出中心 中触发，执行进度将在此同步更新。");
    const QString HERO_DESC_TEXT = QString::fromUtf8("实时掌握小说转漫画链路的任务状态，查看修改闭环、高清批跑与导出中心的运行情况。");

    bool isCountedDashboardJobStatus(const QString& status)
    {
        const QString normalized = status.trimmed().toLower();
        if (normalized.isEmpty()) {
            return false;
        }
        return normalized != "cancelled";
    }
}

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
    QTimer::singleShot(100, this, &DashboardPage::loadStatsFromDatabase);
}

void DashboardPage::setupConnections()
{
    StoryboardViewModel* vm = StoryboardViewModel::instance();
    NovelViewModel* novelVm = NovelViewModel::instance();
    TaskQueue* taskQueue = TaskQueue::instance();
    
    connect(vm, &StoryboardViewModel::storyboardsLoaded,
            this, [this](const QString&, const QList<Storyboard>&) { refresh(); }, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::analysisStarted,
            this, &DashboardPage::refresh, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::analysisCompleted,
            this, [this]() { QTimer::singleShot(500, this, &DashboardPage::refresh); }, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::analysisFailed,
            this, [this]() { QTimer::singleShot(500, this, &DashboardPage::refresh); }, Qt::QueuedConnection);

    if (taskQueue) {
        connect(taskQueue, &TaskQueue::taskEnqueued,
                this, [this](const QString&) { onTaskQueueChanged(); }, Qt::QueuedConnection);
        connect(taskQueue, &TaskQueue::taskStarted,
                this, [this](const QString&) { onTaskQueueChanged(); }, Qt::QueuedConnection);
        connect(taskQueue, &TaskQueue::taskProgress,
                this, [this](const QString&, int, const QString&) { onTaskQueueChanged(); }, Qt::QueuedConnection);
        connect(taskQueue, &TaskQueue::taskCompleted,
                this, [this](const QString&, const QJsonObject&) { onTaskQueueChanged(); }, Qt::QueuedConnection);
        connect(taskQueue, &TaskQueue::taskFailed,
                this, [this](const QString&, const QString&) { onTaskQueueChanged(); }, Qt::QueuedConnection);
        connect(taskQueue, &TaskQueue::taskCancelled,
                this, [this](const QString&) { onTaskQueueChanged(); }, Qt::QueuedConnection);
    }
    
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &DashboardPage::refreshActiveJobs);

    m_syncRefreshTimer = new QTimer(this);
    m_syncRefreshTimer->setSingleShot(true);
    connect(m_syncRefreshTimer, &QTimer::timeout, this, &DashboardPage::refresh);
    
    connect(vm, &StoryboardViewModel::analysisStarted,
            this, [this]() { startPolling(); }, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::analysisCompleted,
            this, [this]() { QTimer::singleShot(1000, this, &DashboardPage::stopPolling); }, Qt::QueuedConnection);
    connect(vm, &StoryboardViewModel::analysisFailed,
            this, [this]() { QTimer::singleShot(1000, this, &DashboardPage::stopPolling); }, Qt::QueuedConnection);

    if (novelVm) {
        connect(novelVm, &NovelViewModel::novelCreated,
                this, [this](const ::Novel&) { refresh(); }, Qt::QueuedConnection);
        connect(novelVm, &NovelViewModel::novelDeleted,
                this, [this](const QString&) { refresh(); }, Qt::QueuedConnection);
        connect(novelVm, &NovelViewModel::novelUpdated,
                this, [this](const QString&) { refresh(); }, Qt::QueuedConnection);
    }
}

DashboardPage::~DashboardPage()
{
    stopPolling();
}

void DashboardPage::scheduleRefresh(int delayMs)
{
    if (!m_syncRefreshTimer) {
        refresh();
        return;
    }

    m_syncRefreshTimer->start(qMax(0, delayMs));
}

void DashboardPage::onTaskQueueChanged()
{
    scheduleRefresh(150);
}

void DashboardPage::startPolling()
{
    if (m_refreshTimer && !m_refreshTimer->isActive()) {
        m_refreshTimer->start(2500);
    }
}

void DashboardPage::stopPolling()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

void DashboardPage::updateJobStats(int total, int running, int completed, int failed)
{
    m_statLabels.setValues(total, running, completed, failed);
}

void DashboardPage::refresh()
{
    loadStatsFromDatabase();
    refreshJobLists();
}

void DashboardPage::refreshActiveJobs()
{
    if (m_activeJobsLayout) {
        populateActiveJobsList();
    }
}

void DashboardPage::refreshJobLists()
{
    populateJobOverviewList();
    populateActiveJobsList();
}

void DashboardPage::clearLayout(QLayout *layout)
{
    if (!layout) return;
    
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->disconnect();
            widget->setParent(nullptr);
            delete widget;
        }
        delete item;
    }
}

void DashboardPage::loadStatsFromDatabase()
{
    DatabaseManager* db = DatabaseManager::instance();
    QString userId = UserSession::instance()->currentUserId();
    QList<QVariantMap> jobs = db->selectAll(
        "jobs",
        "novel_id IN (SELECT id FROM novels WHERE user_id = ?)",
        QVariantList{userId});

    int totalJobs = 0;
    int runningJobs = 0;
    int completedJobs = 0;
    int failedJobs = 0;

    for (const QVariantMap& job : jobs) {
        QString status = job.value("status").toString().toLower();
        if (!isCountedDashboardJobStatus(status)) {
            continue;
        }

        ++totalJobs;
        if (status == "running" || status == "in_progress" || status == "pending") {
            runningJobs++;
        } else if (status == "completed") {
            completedJobs++;
        } else if (status == "failed") {
            failedJobs++;
        }
    }

    updateJobStats(totalJobs, runningJobs, completedJobs, failedJobs);
}

// ========== UI 初始化 ==========

void DashboardPage::setupUI()
{
    setObjectName("dashboardPage");
    setStyleSheet(dashboardPageBackgroundStyle());
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setupLayout(mainLayout, 20, 20, 24, 32, 24);
    
    mainLayout->addWidget(createHeroSection());
    mainLayout->addWidget(createControlsSection());
    mainLayout->addWidget(createContentGrid(), 1);
}

QWidget* DashboardPage::createHeroSection()
{
    QWidget *heroWidget = new QWidget();
    heroWidget->setObjectName("heroSection");
    heroWidget->setStyleSheet(dashboardHeroSectionStyle());
    
    QVBoxLayout *heroLayout = new QVBoxLayout(heroWidget);
    setupLayout(heroLayout, 28, 28, 28, 28, 16);
    
    heroLayout->addWidget(createHeroTextSection());
    heroLayout->addWidget(createStatsContainer());
    
    return heroWidget;
}

QWidget* DashboardPage::createHeroTextSection()
{
    QWidget *textWidget = createTransparentWidget();
    QVBoxLayout *textLayout = new QVBoxLayout(textWidget);
    setupLayout(textLayout, 0, 0, 0, 0, 8);
    
    // 标题行
    QWidget *titleRow = createTransparentWidget();
    QHBoxLayout *titleLayout = new QHBoxLayout(titleRow);
    setupLayout(titleLayout, 0, 0, 0, 0, 12);
    
    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(Constants::DASHBOARD_ICON_SIZE, Constants::DASHBOARD_ICON_SIZE);
    iconLabel->setStyleSheet(dashboardIconStyle());
    iconLabel->setText(QString::fromUtf8("🏠"));
    iconLabel->setAlignment(Qt::AlignCenter);
    
    QLabel *titleLabel = createLabel(
        QString::fromUtf8("工作台"),
        dashboardTitleStyle(), 
        Constants::DASHBOARD_SIZE_TITLE, true);
    
    titleLayout->addWidget(iconLabel);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    
    textLayout->addWidget(titleRow);
    
    // 描述文字
    QString descText = HERO_DESC_TEXT;
    QLabel *descLabel = createLabel(descText, dashboardDescStyle(), Constants::DASHBOARD_SIZE_NORMAL);
    descLabel->setWordWrap(true);
    descLabel->setIndent(60);
    textLayout->addWidget(descLabel);
    
    return textWidget;
}

QWidget* DashboardPage::createStatsContainer()
{
    QWidget *container = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    setupLayout(layout, 0, 8, 0, 0, Constants::CARD_SPACING);
    layout->setAlignment(Qt::AlignLeft);
    
    QLabel** labelPtrs[] = {
        &m_statLabels.total, &m_statLabels.running,
        &m_statLabels.completed, &m_statLabels.failed
    };
    
    for (int i = 0; i < STAT_CARDS_COUNT; ++i) {
        const auto &item = STAT_CARDS[i];
        layout->addWidget(createStatCard(item.label, item.bgColor, labelPtrs[i]));
    }
    
    layout->addStretch();
    
    return container;
}

QWidget* DashboardPage::createStatCard(const QString &label, const QString &bgColor, QLabel **valuePtr)
{
    QWidget *card = new QWidget();
    card->setFixedSize(Constants::DASHBOARD_STAT_CARD_WIDTH, Constants::DASHBOARD_STAT_CARD_HEIGHT);
    card->setObjectName("statCard");
    card->setStyleSheet(dashboardStatCardStyle(bgColor));
    
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    setupLayout(cardLayout, 12, 12, 12, 12, 6);
    cardLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    
    QLabel *labelWidget = new QLabel(label);
    labelWidget->setStyleSheet(dashboardStatCardLabelStyle());
    labelWidget->setFont(QFont("Microsoft YaHei", Constants::DASHBOARD_SIZE_TINY));
    
    QLabel *valueWidget = new QLabel("0");
    valueWidget->setStyleSheet(dashboardStatCardValueStyle());
    valueWidget->setFont(QFont("Microsoft YaHei", Constants::DASHBOARD_SIZE_STAT_VALUE, QFont::Bold));
    
    *valuePtr = valueWidget;
    
    cardLayout->addWidget(labelWidget);
    cardLayout->addWidget(valueWidget);
    
    return card;
}

// ========== 控制区域 ==========

QWidget* DashboardPage::createControlsSection()
{
    QWidget *controlsWidget = createTransparentWidget();
    QHBoxLayout *controlsLayout = new QHBoxLayout(controlsWidget);
    setupLayout(controlsLayout, 0, 0, 0, 0, 16);
    
    controlsLayout->addWidget(createFilterGroup());
    controlsLayout->addStretch();
    controlsLayout->addWidget(createRefreshButton());
    
    return controlsWidget;
}

QWidget* DashboardPage::createFilterGroup()
{
    QWidget *selectGroup = createTransparentWidget();
    QHBoxLayout *selectLayout = new QHBoxLayout(selectGroup);
    setupLayout(selectLayout, 0, 0, 0, 0, 12);
    
    selectLayout->addWidget(createFilter(TYPE_FILTER.label, TYPE_FILTER.items, &m_typeCombo));
    selectLayout->addWidget(createFilter(STATUS_FILTER.label, STATUS_FILTER.items, &m_statusCombo));
    
    return selectGroup;
}

QWidget* DashboardPage::createFilter(const QString &label, const QStringList &items, ModeComboBox **comboPtr)
{
    QWidget *filterWidget = createTransparentWidget();
    QVBoxLayout *filterLayout = new QVBoxLayout(filterWidget);
    setupLayout(filterLayout, 0, 0, 0, 0, 6);
    
    filterLayout->addWidget(createLabel(label, dashboardFilterLabelStyle()));
    
    ModeComboBox *combo = new ModeComboBox();
    for (const QString &item : items) {
        combo->addItem(item);
    }
    combo->setFixedHeight(40);
    *comboPtr = combo;
    
    filterLayout->addWidget(combo);
    
    return filterWidget;
}

QPushButton* DashboardPage::createRefreshButton()
{
    m_refreshBtn = new QPushButton(TR("刷新列表"));
    m_refreshBtn->setStyleSheet(refreshButtonStyle());
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &DashboardPage::refresh);
    return m_refreshBtn;
}

// ========== 内容区域 ==========

QWidget* DashboardPage::createContentGrid()
{
    QWidget *gridWidget = createTransparentWidget();
    QHBoxLayout *gridLayout = new QHBoxLayout(gridWidget);
    setupLayout(gridLayout, 0, 0, 0, 0, 24);
    
    QWidget *cards[] = {createJobOverviewCard(), createActiveJobsCard(), createGuideCard()};
    for (QWidget *card : cards) {
        card->setMinimumWidth(320);
        gridLayout->addWidget(card, 1);
    }
    
    return gridWidget;
}

QWidget* DashboardPage::createCard(const QString &objectName)
{
    QWidget *card = new QWidget();
    card->setObjectName(objectName);
    card->setStyleSheet(dashboardCardStyle());
    return card;
}

QWidget* DashboardPage::createCardHeader(const QString &title, const QString &subtitle)
{
    QWidget *header = createTransparentWidget();
    header->setFixedHeight(24);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    setupLayout(headerLayout, 0, 0, 0, 0);
    
    headerLayout->addWidget(createLabel(title, dashboardCardTitleStyle()));
    headerLayout->addStretch();
    headerLayout->addWidget(createLabel(subtitle, dashboardCardSubtitleStyle()));
    
    return header;
}

QWidget* DashboardPage::createStandardCard(const QString &title, const QString &subtitle)
{
    QWidget *card = createCard("cardStandard");
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    setupLayout(cardLayout, Constants::CARD_PADDING, 
                Constants::CARD_PADDING,
                Constants::CARD_PADDING, 
                Constants::CARD_PADDING,
                Constants::CARD_SPACING);
    
    cardLayout->addWidget(createCardHeader(title, subtitle));
    
    return card;
}


QWidget* DashboardPage::createJobOverviewCard()
{
    QWidget *card = createStandardCard(CARD_TITLE_JOB_OVERVIEW, CARD_SUBTITLE_JOB_OVERVIEW);
    QVBoxLayout *cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
    
    QWidget *jobListWidget = createTransparentWidget();
    m_jobOverviewLayout = new QVBoxLayout(jobListWidget);
    setupLayout(m_jobOverviewLayout, 0, 0, 0, 0, Constants::DASHBOARD_JOB_ITEM_SPACING);
    jobListWidget->setMinimumHeight(Constants::DASHBOARD_JOB_LIST_MIN_HEIGHT);
    
    populateJobOverviewList();
    
    cardLayout->addWidget(jobListWidget);
    
    return card;
}

void DashboardPage::populateJobOverviewList()
{
    const QList<QVariantMap> jobs = loadDashboardJobs(QString(), QVariantList(),
                                                      "created_at DESC",
                                                      Constants::DASHBOARD_MAX_OVERVIEW_JOBS);
    renderJobList(m_jobOverviewLayout, jobs, EMPTY_JOBS_TEXT, false);
}

void DashboardPage::populateActiveJobsList()
{
    const QList<QVariantMap> jobs = loadDashboardJobs(ACTIVE_JOBS_WHERE, QVariantList(),
                                                      "created_at DESC",
                                                      Constants::DASHBOARD_MAX_ACTIVE_JOBS);
    renderJobList(m_activeJobsLayout, jobs, EMPTY_ACTIVE_JOBS_TEXT, true);
}

QList<QVariantMap> DashboardPage::loadDashboardJobs(const QString &whereClause,
                                                    const QVariantList &whereValues,
                                                    const QString &orderBy,
                                                    int limit) const
{
    DatabaseManager* db = DatabaseManager::instance();
    QString userId = UserSession::instance()->currentUserId();
    
    QString finalWhere = "novel_id IN (SELECT id FROM novels WHERE user_id = ?)";
    QVariantList finalValues = QVariantList{userId};
    
    if (!whereClause.isEmpty()) {
        finalWhere += " AND " + whereClause;
        finalValues.append(whereValues);
    }
    
    return db->selectAll("jobs", finalWhere, finalValues, orderBy, limit);
}

void DashboardPage::renderJobList(QVBoxLayout *layout, const QList<QVariantMap> &jobs,
                                  const QString &emptyText, bool showProgress)
{
    if (!layout) return;

    clearLayout(layout);

    if (jobs.isEmpty()) {
        QLabel *emptyLabel = createLabel(emptyText, dashboardEmptyStateStyle());
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setMinimumHeight(60);
        layout->addWidget(emptyLabel);
        layout->addStretch();
        return;
    }

    for (const QVariantMap& job : jobs) {
        layout->addWidget(createJobItemWidget(buildJobItemData(job, showProgress)));
    }

    layout->addStretch();
}

DashboardPage::JobItemData DashboardPage::buildJobItemData(const QVariantMap &job, bool showProgress) const
{
    JobItemData data;
    data.type = StatusHelper::Job::typeLabel(job.value("type").toString());
    data.status = StatusHelper::Job::statusLabel(job.value("status").toString());
    data.time = job.value("created_at").toString();
    data.progress = job.value("progress").toInt();
    data.showProgress = showProgress;
    return data;
}

QWidget* DashboardPage::createJobItemWidget(const JobItemData &data)
{
    return data.showProgress 
        ? createTimelineJobItem(data.type, data.status, data.time, data.progress)
        : createStandardJobItem(data.type, data.status, data.time);
}

QWidget* DashboardPage::createTimelineJobItem(const QString &type, const QString &status,
                                               const QString &time, int progress)
{
    QWidget *item = createTransparentWidget();
    item->setMinimumHeight(Constants::DASHBOARD_TIMELINE_ITEM_HEIGHT);
    QHBoxLayout *layout = new QHBoxLayout(item);
    setupLayout(layout, 0, 0, 0, 0, Constants::CARD_SPACING);
    
    QLabel *dot = new QLabel();
    dot->setFixedSize(Constants::DASHBOARD_TIMELINE_DOT_SIZE, Constants::DASHBOARD_TIMELINE_DOT_SIZE);
    dot->setStyleSheet(dashboardTimelineDotStyle());
    
    QString metaText = QString("%1   %2%   %3").arg(status).arg(progress).arg(time);
    
    layout->addWidget(dot);
    layout->addWidget(createLabel(type, dashboardTimelineTitleStyle()));
    layout->addWidget(createLabel(metaText, dashboardTimelineMetaStyle()));
    layout->addStretch();
    
    return item;
}

QWidget* DashboardPage::createStandardJobItem(const QString &type, const QString &status, 
                                               const QString &time)
{
    QWidget *item = new QWidget();
    item->setObjectName("jobItem");
    item->setStyleSheet(dashboardJobItemStyle());
    item->setMinimumHeight(Constants::DASHBOARD_JOB_ITEM_HEIGHT);
    
    QVBoxLayout *layout = new QVBoxLayout(item);
    setupLayout(layout, 16, 12, 16, 12, 8);
    
    QWidget *header = createTransparentWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    setupLayout(headerLayout, 0, 0, 0, 0);
    
    headerLayout->addWidget(createLabel(type, dashboardJobTypeStyle()));
    headerLayout->addStretch();
    headerLayout->addWidget(createLabel(status, dashboardJobMetaStyle()));
    
    layout->addWidget(header);
    layout->addWidget(createLabel(QString::fromUtf8("创建时间: %1").arg(time), dashboardJobMetaStyle()));
    
    return item;
}

QWidget* DashboardPage::createActiveJobsCard()
{
    QWidget *card = createStandardCard(CARD_TITLE_ACTIVE_JOBS, CARD_SUBTITLE_ACTIVE_JOBS);
    QVBoxLayout *cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
    
    QWidget *timelineWidget = createTransparentWidget();
    m_activeJobsLayout = new QVBoxLayout(timelineWidget);
    setupLayout(m_activeJobsLayout, 0, 0, 0, Constants::DASHBOARD_JOB_ITEM_SPACING);
    m_activeJobsLayout->setAlignment(Qt::AlignTop);
    timelineWidget->setMinimumHeight(Constants::DASHBOARD_JOB_LIST_MIN_HEIGHT);
    
    populateActiveJobsList();
    
    cardLayout->addWidget(timelineWidget);
    cardLayout->addWidget(createActiveJobsFooter());
    
    return card;
}

QWidget* DashboardPage::createActiveJobsFooter()
{
    QWidget *footer = new QWidget();
    footer->setObjectName("activeJobsFooter");
    footer->setStyleSheet(dashboardActiveJobsFooterStyle());
    
    QVBoxLayout *footerLayout = new QVBoxLayout(footer);
    setupLayout(footerLayout, 12, 12, 12, 12);
    
    QLabel *footerLabel = createLabel(FOOTER_TEXT_ACTIVE_JOBS, dashboardFooterLabelStyle());
    footerLabel->setWordWrap(true);
    
    footerLayout->addWidget(footerLabel);
    
    return footer;
}



QWidget* DashboardPage::createGuideCard()
{
    QWidget *card = createCard("cardGuide");
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    setupLayout(cardLayout, Constants::CARD_PADDING, 
                Constants::CARD_PADDING,
                Constants::CARD_PADDING, 
                Constants::CARD_PADDING,
                Constants::CARD_SPACING);
    
    cardLayout->addWidget(createCardHeader(CARD_TITLE_GUIDE, CARD_SUBTITLE_GUIDE));
    cardLayout->addWidget(createGuideStepList());
    cardLayout->addStretch();
    
    return card;
}

QWidget* DashboardPage::createGuideStepList()
{
    QWidget *actionListWidget = createTransparentWidget();
    QVBoxLayout *actionListLayout = new QVBoxLayout(actionListWidget);
    setupLayout(actionListLayout, 0, 0, 0, 0, 14);
    
    for (int i = 0; i < GUIDE_STEPS_COUNT; ++i) {
        const auto &step = GUIDE_STEPS[i];
        actionListLayout->addWidget(createGuideStep(step.number, step.title, step.description));
    }
    
    return actionListWidget;
}

QWidget* DashboardPage::createGuideStep(int stepNumber, const QString &title, const QString &description)
{
    QWidget *step = createTransparentWidget();
    QHBoxLayout *stepLayout = new QHBoxLayout(step);
    setupLayout(stepLayout, 0, 0, 0, 0, 16);
    
    stepLayout->addWidget(createStepNumberLabel(stepNumber));
    stepLayout->addWidget(createStepContent(title, description), 1);
    
    return step;
}

QLabel* DashboardPage::createStepNumberLabel(int stepNumber)
{
    QLabel *stepNum = new QLabel(QString::number(stepNumber));
    stepNum->setFixedSize(Constants::DASHBOARD_STEP_NUMBER_SIZE, 
                          Constants::DASHBOARD_STEP_NUMBER_SIZE);
    stepNum->setAlignment(Qt::AlignCenter);
    stepNum->setStyleSheet(dashboardStepNumberStyle());
    return stepNum;
}

QWidget* DashboardPage::createStepContent(const QString &title, const QString &description)
{
    QWidget *stepContent = createTransparentWidget();
    QVBoxLayout *stepContentLayout = new QVBoxLayout(stepContent);
    setupLayout(stepContentLayout, 0, 0, 0, 0, 4);
    
    QLabel *stepTitle = createLabel(title, dashboardStepTitleStyle());
    QLabel *stepDesc = createLabel(description, dashboardStepDescStyle());
    stepDesc->setWordWrap(true);
    
    stepContentLayout->addWidget(stepTitle);
    stepContentLayout->addWidget(stepDesc);
    
    return stepContent;
}
