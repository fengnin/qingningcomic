#ifndef PANELEDITORWIDGET_H
#define PANELEDITORWIDGET_H

#include <QFrame>
#include <QPixmap>

class QLabel;
class QTextEdit;
class QPushButton;
class QNetworkAccessManager;
class QNetworkReply;
class ImageService;

class PanelEditorWidget : public QFrame
{
    Q_OBJECT

public:
    enum EditMode {
        Inpaint = 0,
        Outpaint = 1,
        BgSwap = 2
    };
    
    enum class State {
        Ready,
        Processing,
        Success,
        Error
    };

    explicit PanelEditorWidget(QWidget *parent = nullptr);
    ~PanelEditorWidget();
    
    void setPanelInfo(int chapterNumber, int panelNumber, const QString &panelId);
    void setSceneDescription(const QString &description);
    void setPreviewPixmap(const QPixmap &pixmap);
    void setPreviewUrl(const QString &url);
    void setState(State state);
    
    QString sceneDescription() const;
    int editMode() const { return static_cast<int>(m_currentEditMode); }
    QString editInstruction() const;
    QString maskFilePath() const { return m_maskFilePath; }

signals:
    void sceneDescriptionChanged(const QString &description);
    void editSubmitted(int editMode, const QString &instruction, const QString &maskPath);
    void imageGenerated(const QString &imageUrl);
    void closed();

private slots:
    void onSaveSceneClicked();
    void onEditModeClicked(int mode);
    void onSubmitEditClicked();
    void onSelectMaskFileClicked();
    void onImageDownloadFinished(QNetworkReply *reply);

private:
    void setupUI();
    void setupConnections();
    
    QWidget* createHeader();
    QWidget* createHeaderPreview();
    QWidget* createHeaderInfo();
    QPushButton* createCloseButton();
    QWidget* createSceneDescSection();
    QWidget* createEditModeSection();
    QWidget* createInstructionSection();
    QWidget* createMaskFileRow();
    QWidget* createFooter();
    
    QWidget* createSection(const QString& title);
    QLabel* createInfoLabel(const QString& text);
    QPushButton* createModeButton(const QString& text, int mode);
    QPushButton* createActionButton(const QString& text);
    
    void updateModeButtons(EditMode activeMode);
    void loadPreviewFromUrl(const QString &url);
    
    QLabel *m_previewLabel = nullptr;
    QLabel *m_idLabel = nullptr;
    QLabel *m_posLabel = nullptr;
    QTextEdit *m_sceneDescEdit = nullptr;
    QTextEdit *m_editInstructionEdit = nullptr;
    QPushButton *m_saveSceneBtn = nullptr;
    QPushButton *m_submitEditBtn = nullptr;
    QPushButton *m_selectMaskBtn = nullptr;
    QLabel *m_maskFileLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    
    QPushButton *m_inpaintBtn = nullptr;
    QPushButton *m_outpaintBtn = nullptr;
    QPushButton *m_bgSwapBtn = nullptr;
    
    int m_chapterNumber = 0;
    int m_panelNumber = 0;
    QString m_panelId;
    QString m_maskFilePath;
    EditMode m_currentEditMode = EditMode::Inpaint;
    State m_state = State::Ready;
    
    QNetworkAccessManager *m_networkManager = nullptr;
    ImageService *m_imageService = nullptr;
};

#endif // PANELEDITORWIDGET_H
