#ifndef NOVELUPLOADPAGE_H
#define NOVELUPLOADPAGE_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QScrollArea>
#include "models/Novel.h"
#include "components/ChapterSpinBox.h"

class NovelService;

class NovelUploadPage : public QWidget
{
    Q_OBJECT

public:
    explicit NovelUploadPage(QWidget *parent = nullptr);
    ~NovelUploadPage();
    
    void setNovelForEdit(const Novel &novel);
    void resetToCreateMode();
    bool hasActiveProgressState() const;

signals:
    void backClicked();
    void novelUploaded(const QString& novelId, int chapterNumber);
    void viewDetailsRequested(const QString& novelId);

private slots:
    void onUploadClicked();
    void onSelectFileClicked();
    void validateInput();
    void onAnalysisCompleted(const QString& novelId, int chapterNumber);
    void onAnalysisFailed(const QString& novelId, const QString& error);

private:
    void setupUI();
    void connectAnalysisSignals();
    QScrollArea* createScrollArea();
    QWidget* createMainCard();
    QWidget* createHeaderSection();
    QWidget* createMessageSection();
    QWidget* createFormSection();
    QWidget* createActionSection();
    QWidget* createTipsSection();
    
    bool validateFormData(const QString &title, const QString &text);
    void showMessage(const QString &message, bool isError);
    void showError(const QString &message);
    void showSuccess(const QString &novelId);
    void clearMessages();
    
    QVariantMap buildMetadata(const QString &genre);
    void setButtonLoadingState(bool loading);
    QString getSuccessMessage() const;
    QString getErrorMessage() const;
    
    bool updateExistingNovel(const QString &title, const QString &text, const QString &genre);
    bool createNewNovel(const QString& title, const QString& text, const QString& genre, int chapterNumber);
    void startAnalysis(int chapterNumber, const QString& text);
    
    void setWaitingAnalysisState();
    void resetAfterAnalysis();
    bool isCurrentAnalysis(const QString& novelId) const;
    
    int showChapterNumberDialog();

    bool m_isEditMode = false;
    bool m_isWaitingAnalysis = false;
    bool m_analysisCompleted = false;
    QString m_editNovelId;
    QString m_pendingNovelId;
    QLabel *m_headerTitleLabel = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_genreEdit = nullptr;
    QTextEdit *m_textEdit = nullptr;
    QPushButton *m_uploadBtn = nullptr;
    QPushButton *m_selectFileBtn = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_errorLabel = nullptr;
    QLabel *m_successLabel = nullptr;
};

#endif
