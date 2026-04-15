#ifndef NOVELPAGE_H
#define NOVELPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QButtonGroup>
#include <QEvent>
#include "Novel.h"
#include "NovelService.h"

// 筛选项数据结构
struct FilterItem {
    QString status;
    QString label;
};

class NovelPage : public QWidget
{
    Q_OBJECT

public:
    explicit NovelPage(QWidget *parent = nullptr);
    ~NovelPage();
    
    void setUserId(const QString &userId) { m_userId = userId; }
    QString userId() const { return m_userId; }
    
    void refresh();

signals:
    void showNovelDetail(const QString &novelId);
    void createNovelRequested();
    void editNovelRequested(const QString &novelId);  // 请求编辑小说

private slots:
    void onCreateNovelClicked();
    void onJumpClicked();
    void onFilterClicked(const QString &status);
    void onNovelCardClicked(const QString &novelId);
    void onDeleteNovelClicked(const QString &novelId);
    void onEditNovelClicked(const QString &novelId);

private:
    // ========== UI 初始化 ==========
    void setupUI();
    QScrollArea* createScrollArea();
    QWidget* createHeroSection();
    QWidget* createHeroIcon();
    QWidget* createHeroText();
    QWidget* createJumpSection();
    QWidget* createCreateButton();
    QWidget* createFilterBar();
    QWidget* createGridSection();
    
    // ========== 卡片组件 ==========
    QWidget* createNovelCard(const Novel &novel);
    QWidget* createCardHeader(const Novel &novel);
    QWidget* createCardDetails(const Novel &novel);
    QWidget* createCardFooter(const Novel &novel);
    QWidget* createCardActions(const Novel &novel);
    QLabel* createStatusLabel(const QString &status);
    QWidget* createDetailRow(const QString &labelText, const QString &valueText);
    QWidget* createEmptyState();
    QPushButton* createFilterChip(const QString &label, const QString &status, bool isActive);
    
    // ========== 业务逻辑 ==========
    void loadNovelsFromDatabase();
    QList<Novel> filterNovels() const;
    void renderNovelGrid();
    void clearGridLayout();
    void renderNovelCards(const QList<Novel> &novels);
    void updateFilterStats();
    Novel* findNovelById(const QString &novelId);
    void removeNovelFromList(const QString &novelId);
    void deleteNovel(const QString &novelId);
    
    // ========== 对话框辅助方法 ==========
    enum class MessageType { Success, Error };
    void showMessageDialog(const QString &title, const QString &message, MessageType type);
    void showDeleteConfirmDialog(const QString &novelId, const QString &title);
    void showSuccessMessage(const QString &title, const QString &message);
    void showErrorMessage(const QString &title, const QString &message);
    const QVector<FilterItem>& getFilterItems() const;
    QString findNovelIdFromWidget(QWidget *widget) const;
    bool eventFilter(QObject *watched, QEvent *event) override;

    // ========== 成员变量 ==========
    QString m_userId;
    QVBoxLayout *m_mainLayout;
    QLineEdit *m_jumpInput;
    QPushButton *m_jumpBtn;
    QPushButton *m_createBtn;
    QButtonGroup *m_filterGroup;
    QLabel *m_totalCountLabel;
    QWidget *m_filterContainer;
    QWidget *m_gridContainer;
    QGridLayout *m_gridLayout;
    QList<Novel> m_novels;
    QString m_currentFilter;
    QMap<QString, int> m_filterStats;
};

#endif // NOVELPAGE_H
