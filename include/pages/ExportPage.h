#ifndef EXPORTPAGE_H
#define EXPORTPAGE_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLayout>
#include <QScrollArea>
#include <QListWidget>
#include <QSettings>
#include <QVariantMap>

class ExportPage : public QWidget
{
    Q_OBJECT

public:
    explicit ExportPage(QWidget *parent = nullptr);
    ~ExportPage();

private slots:
    void onSearchClicked();
    void onHistoryItemClicked(int index);
    void validateInput();

private:
    // ========== UI 初始化 ==========
    void setupUI();
    QScrollArea* createScrollArea();
    QWidget* createContentWidget();
    QWidget* createHeader();
    QWidget* createSearchCard();
    QWidget* createHistoryCard();
    
    // ========== 辅助方法 ==========
    QWidget* createTransparentWidget();
    QLabel* createLabel(const QString &text, const QString &style, int fontSize = -1, bool bold = false);
    void setupLayout(QLayout *layout, int left, int top, int right, int bottom, int spacing = 0);
    QWidget* createCard(const QString &objectName);
    
    // ========== 搜索区域组件 ==========
    QWidget* createSearchArea();
    QWidget* createSearchInputRow();
    QWidget* createResultArea();
    QWidget* createResultHeader();
    QWidget* createResultDetails();
    QWidget* createDetailItem(const QString &labelText, QLabel **valueLabel);
    
    // ========== 历史区域组件 ==========
    QWidget* createHistoryHeader();
    QWidget* createHistoryList();
    
    // ========== 业务逻辑 ==========
    void loadHistory();
    void saveHistory(const QString &exportId);
    void rebuildHistoryList();
    QString formatBytes(qint64 size);
    QString formatDateTime(const QString &dateTime);
    QString formatLabel(const QString &format);
    QString statusLabel(const QString &status);
    void showError(const QString &message);
    void showResult(const QVariantMap &data);
    void clearMessages();

    // ========== 成员变量 ==========
    QLineEdit *m_exportIdEdit;
    QPushButton *m_searchBtn;
    QLabel *m_errorLabel;
    QWidget *m_resultWidget;
    QLabel *m_resultNovelIdLabel;
    QLabel *m_resultFormatLabel;
    QLabel *m_resultStatusLabel;
    QLabel *m_resultFileSizeLabel;
    QLabel *m_resultCreatedAtLabel;
    QLabel *m_resultFileUrlLabel;
    QPushButton *m_downloadBtn;
    QListWidget *m_historyList;
    
    QStringList m_historyData;
    static const int MAX_HISTORY = 10;
};

#endif // EXPORTPAGE_H
