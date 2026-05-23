#ifndef EXPORTPAGE_H
#define EXPORTPAGE_H

#include "services/ExportService.h"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLayout>
#include <QScrollArea>
#include <QListWidget>
#include <QSettings>

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
    void setupUI();
    QScrollArea* createScrollArea();
    QWidget* createContentWidget();
    QWidget* createHeader();
    QWidget* createSearchCard();
    QWidget* createHistoryCard();
    QWidget* createCard(const QString &objectName);

    QWidget* createSearchArea();
    QWidget* createSearchInputRow();

    QWidget* createHistoryHeader();
    QWidget* createHistoryList();

    void loadHistory();
    void saveHistory(const QString &exportId);
    void rebuildHistoryList();
    void showError(const QString &message);
    void showResult(const ExportResult &result);
    void clearMessages();

    QLineEdit *m_exportIdEdit;
    QPushButton *m_searchBtn;
    QLabel *m_errorLabel;
    QListWidget *m_historyList;

    QStringList m_historyData;
    static const int MAX_HISTORY = 10;
};

#endif // EXPORTPAGE_H
