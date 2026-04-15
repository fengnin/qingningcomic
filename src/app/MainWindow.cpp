#include "MainWindow.h"
#include "SidebarWidget.h"
#include "DashboardPage.h"
#include "NovelPage.h"
#include "NovelUploadPage.h"
#include "ExportPage.h"
#include "NovelDetailPage.h"
#include "Novel.h"
#include "viewmodels/NovelViewModel.h"
#include "StyleManager.h"
#include "Logger.h"
#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QFont>

namespace {
    constexpr int SIDEBAR_MIN_WIDTH = 280;
    constexpr int WINDOW_MIN_WIDTH = 1400;
    constexpr int WINDOW_MIN_HEIGHT = 900;
    constexpr int WINDOW_DEFAULT_WIDTH = 1600;
    constexpr int WINDOW_DEFAULT_HEIGHT = 1000;
    
    const QString STYLE_TRANSPARENT = "background: transparent;";
    const QString STYLE_BACK_BTN = R"(
        QPushButton { background: transparent; border: none; color: #b45309; font-size: 14px; padding: 8px 12px; border-radius: 6px; }
        QPushButton:hover { background: #fef3c7; }
    )";
    const QString STYLE_TOP_BAR = "QWidget { background: white; border-bottom: 1px solid #e5e5e5; }";
    const QString STYLE_CONTENT_AREA = "background-color: #fafafa;";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_breadcrumbLabel(nullptr)
    , m_pageStack(nullptr)
    , m_sidebar(nullptr)
    , m_dashboardPage(nullptr)
    , m_uploadPage(nullptr)
    , m_novelPage(nullptr)
    , m_exportPage(nullptr)
    , m_novelDetailPage(nullptr)
    , m_backBtn(nullptr)
{
    setupUI();
    initPages();
    connectPageSignals();
    
    setMinimumSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
    resize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    setupLayoutMargins(mainLayout, 0, 0, 0, 0, 0);
    
    m_sidebar = new SidebarWidget(this);
    m_sidebar->setFixedWidth(SIDEBAR_MIN_WIDTH);
    
    connect(m_sidebar, &SidebarWidget::navItemClicked,
            this, &MainWindow::onNavItemSelected);
    
    QWidget *rightArea = createTransparentWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightArea);
    setupLayoutMargins(rightLayout, 0, 0, 0, 0, 0);
    
    createTopBar();
    createContentArea();
    
    rightLayout->addWidget(m_backBtn ? m_backBtn : new QWidget());
    rightLayout->addWidget(m_breadcrumbLabel ? m_breadcrumbLabel : new QWidget());
    rightLayout->addStretch();
    rightLayout->addWidget(m_pageStack ? m_pageStack : new QWidget(), 1);
    
    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(rightArea, 1);
    
    setCentralWidget(centralWidget);
}

void MainWindow::createTopBar()
{
    QWidget *topBar = createTransparentWidget();
    topBar->setObjectName("topBar");
    topBar->setStyleSheet(STYLE_TOP_BAR);
    topBar->setFixedHeight(56);
    
    QHBoxLayout *topBarLayout = new QHBoxLayout(topBar);
    setupLayoutMargins(topBarLayout, 16, 8, 16, 8, 10);
    
    m_backBtn = new QPushButton(QString::fromUtf8("返回"));
    m_backBtn->setStyleSheet(STYLE_BACK_BTN);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setVisible(false);
    
    connect(m_backBtn, &QPushButton::clicked, this, &MainWindow::onBackToNovelPage);
    
    QString breadcrumbText = QString::fromUtf8("总览");
    m_breadcrumbLabel = createStyledLabel(breadcrumbText,
        "font-size: 16px; font-weight: bold; color: #451a03; background: transparent;", 16, true);
    
    topBarLayout->addWidget(m_backBtn);
    topBarLayout->addWidget(m_breadcrumbLabel);
    topBarLayout->addStretch();
}

void MainWindow::createContentArea()
{
    m_pageStack = new QStackedWidget(this);
    m_pageStack->setStyleSheet(STYLE_CONTENT_AREA);
}

void MainWindow::initPages()
{
    m_dashboardPage = new DashboardPage(m_pageStack);
    m_uploadPage = new NovelUploadPage(m_pageStack);
    m_novelPage = new NovelPage(m_pageStack);
    m_exportPage = new ExportPage(m_pageStack);
    m_novelDetailPage = new NovelDetailPage(m_pageStack);
    
    m_pageStack->addWidget(m_dashboardPage);
    m_pageStack->addWidget(m_uploadPage);
    m_pageStack->addWidget(m_novelPage);
    m_pageStack->addWidget(m_exportPage);
    m_pageStack->addWidget(m_novelDetailPage);
    
    m_pageStack->setCurrentIndex(0);
}

void MainWindow::connectPageSignals()
{
    auto *uploadPage = qobject_cast<NovelUploadPage*>(m_uploadPage);
    if (uploadPage) {
        connect(uploadPage, &NovelUploadPage::novelUploaded,
                this, &MainWindow::onNovelUploaded);
        connect(uploadPage, &NovelUploadPage::viewDetailsRequested,
                this, &MainWindow::onShowNovelDetail);
    }
    
    auto *novelPage = qobject_cast<NovelPage*>(m_novelPage);
    if (novelPage) {
        connect(novelPage, &NovelPage::showNovelDetail,
                this, &MainWindow::onShowNovelDetail);
        connect(novelPage, &NovelPage::editNovelRequested,
                this, &MainWindow::onEditNovelRequested);
        connect(novelPage, &NovelPage::createNovelRequested,
                this, [this]() { switchToPage(1, QString::fromUtf8("新建作品")); });
    }
    
    auto *detailPage = qobject_cast<NovelDetailPage*>(m_novelDetailPage);
    if (detailPage) {
        connect(detailPage, &NovelDetailPage::backClicked,
                this, &MainWindow::onBackToNovelPage);
    }
}

void MainWindow::refreshPage(int pageIndex)
{
    if (!m_pageStack) return;
    
    switch (pageIndex) {
        case 1: {
            auto *page = qobject_cast<NovelUploadPage*>(m_uploadPage);
            if (page) page->resetToCreateMode();
            break;
        }
        case 2: {
            auto *page = qobject_cast<NovelPage*>(m_novelPage);
            if (page) page->refresh();
            break;
        }
        default:
            break;
    }
}

void MainWindow::onNavItemSelected(int pageIndex, const QString &breadcrumb)
{
    switchToPage(pageIndex, breadcrumb);
}

void MainWindow::switchToPage(int pageIndex, const QString &breadcrumb)
{
    if (!m_pageStack || !m_breadcrumbLabel || !m_backBtn) return;
    
    refreshPage(pageIndex);
    m_pageStack->setCurrentIndex(pageIndex);
    m_breadcrumbLabel->setText(breadcrumb);
    
    bool showBack = (pageIndex == 4);
    m_backBtn->setVisible(showBack);
    
    if (m_sidebar) {
        int navIndex = (pageIndex == 4) ? 2 : pageIndex;
        m_sidebar->setActiveNav(navIndex);
    }
    
    QFontMetrics fm(m_breadcrumbLabel->font());
    int textWidth = fm.horizontalAdvance(breadcrumb);
    m_breadcrumbLabel->setMinimumWidth(textWidth + 20);
}

void MainWindow::onShowNovelDetail(const QString &novelId)
{
    if (novelId.isEmpty()) {
        LOG_WARNING("MainWindow", "onShowNovelDetail: novelId is empty");
        return;
    }
    
    if (!m_novelDetailPage) {
        LOG_ERROR("MainWindow", "onShowNovelDetail: m_novelDetailPage is null");
        return;
    }
    
    NovelViewModel* vm = NovelViewModel::instance();
    if (!vm) {
        LOG_ERROR("MainWindow", "onShowNovelDetail: NovelViewModel is null");
        return;
    }
    
    vm->loadNovel(novelId);
    Novel novel = vm->currentNovel();
    
    if (novel.id().isEmpty()) {
        LOG_WARNING("MainWindow", QString("onShowNovelDetail: Novel not found: %1").arg(novelId));
        return;
    }
    
    auto *detailPage = qobject_cast<NovelDetailPage*>(m_novelDetailPage);
    if (!detailPage) {
        LOG_ERROR("MainWindow", "onShowNovelDetail: Failed to cast to NovelDetailPage");
        return;
    }
    
    detailPage->setNovel(novel);
    
    switchToPage(4, novel.title());
}

void MainWindow::onBackToNovelPage()
{
    switchToPage(2, QString::fromUtf8("作品列表"));
}

void MainWindow::onNovelUploaded(const QString &novelId, int chapterNumber)
{
    Q_UNUSED(chapterNumber)
    
    auto *novelPageWidget = qobject_cast<NovelPage*>(m_novelPage);
    if (novelPageWidget) novelPageWidget->refresh();
    
    auto *detailPageWidget = qobject_cast<NovelDetailPage*>(m_novelDetailPage);
    if (detailPageWidget && !novelId.isEmpty()) {
        NovelViewModel::instance()->loadNovel(novelId);
        Novel novel = NovelViewModel::instance()->currentNovel();
        if (!novel.id().isEmpty()) detailPageWidget->setNovel(novel);
    }
}

void MainWindow::onEditNovelRequested(const QString &novelId)
{
    if (novelId.isEmpty()) {
        LOG_WARNING("MainWindow", "onEditNovelRequested: novelId is empty");
        return;
    }
    
    NovelViewModel* vm = NovelViewModel::instance();
    if (!vm) {
        LOG_ERROR("MainWindow", "onEditNovelRequested: NovelViewModel is null");
        return;
    }
    
    vm->loadNovel(novelId);
    Novel novel = vm->currentNovel();
    
    if (novel.id().isEmpty()) {
        LOG_WARNING("MainWindow", QString("onEditNovelRequested: Novel not found: %1").arg(novelId));
        return;
    }
    
    auto *uploadPage = qobject_cast<NovelUploadPage*>(m_uploadPage);
    if (!uploadPage) {
        LOG_ERROR("MainWindow", "onEditNovelRequested: Failed to cast to NovelUploadPage");
        return;
    }
    
    switchToPage(1, QString::fromUtf8("编辑作品"));
    
    uploadPage->setNovelForEdit(novel);
}

QLabel* MainWindow::createStyledLabel(const QString &text, const QString &style, int fontSize, bool bold)
{
    QLabel *label = new QLabel(text);
    QFont font(bold ? QStringLiteral("Microsoft YaHei") : QStringLiteral("Segoe UI"), fontSize > 0 ? fontSize : -1);
    if (bold) font.setBold(true);
    label->setFont(font);
    label->setStyleSheet(style);
    label->setWordWrap(false);
    return label;
}

QWidget* MainWindow::createTransparentWidget()
{
    QWidget *widget = new QWidget();
    widget->setStyleSheet(STYLE_TRANSPARENT);
    return widget;
}

void MainWindow::setupLayoutMargins(QBoxLayout *layout, int left, int top, int right, int bottom, int spacing)
{
    layout->setContentsMargins(left, top, right, bottom);
    if (spacing > 0) layout->setSpacing(spacing);
}
