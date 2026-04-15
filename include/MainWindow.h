#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class SidebarWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNavItemSelected(int pageIndex, const QString &breadcrumb);
    void onShowNovelDetail(const QString &novelId);
    void onBackToNovelPage();
    void onNovelUploaded(const QString &novelId, int chapterNumber);
    void onEditNovelRequested(const QString &novelId);

private:
    void setupUI();
    void createContentArea();
    void createTopBar();
    
    void switchToPage(int pageIndex, const QString &breadcrumb);
    
    // 页面管理方法
    void initPages();
    void connectPageSignals();
    void refreshPage(int pageIndex);
    
    // 辅助方法
    QLabel* createStyledLabel(const QString &text, const QString &style, int fontSize = -1, bool bold = false);
    QWidget* createTransparentWidget();
    void setupLayoutMargins(QBoxLayout *layout, int left, int top, int right, int bottom, int spacing = 0);

    QLabel *m_breadcrumbLabel;
    QStackedWidget *m_pageStack;
    SidebarWidget *m_sidebar;
    
    QWidget *m_dashboardPage;
    QWidget *m_uploadPage;
    QWidget *m_novelPage;
    QWidget *m_exportPage;
    QWidget *m_novelDetailPage;
    QPushButton *m_backBtn;
};

#endif // MAINWINDOW_H
