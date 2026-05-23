#ifndef PANELCARD_H
#define PANELCARD_H

#include "components/EditorCardBase.h"
#include <QMouseEvent>
#include <QPixmap>

class PanelCard : public EditorCardBase
{
    Q_OBJECT

public:
    explicit PanelCard(int chapterNumber, int panelNumber, const QString &description, QWidget *parent = nullptr);
    ~PanelCard();
    
    void setDescription(const QString &description);
    void setPreviewUrl(const QString &url);
    void setPanelId(const QString &panelId) { m_panelId = panelId; }
    void setImageSize(int width, int height);
    QString panelId() const { return m_panelId; }
    int panelNumber() const { return m_panelNumber; }
    int chapterNumber() const { return m_chapterNumber; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void clicked(int panelNumber);
    void dataChanged(int panelNumber, const QString &description);
    void imageClicked(const QString &imagePath);

private slots:
    void onAsyncImageLoaded(const QString& id, const QString& cacheKey, const QPixmap& pixmap);

private:
    enum class PreviewState {
        Empty,
        Loading,
        Loaded,
        Error
    };

    void setupUI();
    void setupCardAppearance();
    void setupMainLayout();

    QWidget* createPreviewSection();
    QWidget* createNumberLabel();
    QWidget* createDescriptionLabel();
    
    QPixmap loadPixmapFromUrl(const QString &url);
    void setPreviewPixmap(const QPixmap &pixmap);
    void setPreviewText(const QString &text);
    void setPreviewState(PreviewState state);
    
    QLabel *m_previewLabel = nullptr;
    QLabel *m_numberLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    QLabel *m_sizeLabel = nullptr;
    int m_chapterNumber;
    int m_panelNumber;
    QString m_description;

    QString m_panelId;
    QString m_previewUrl;
    QString m_currentImagePath;
    QString m_loadingImageId;
    PreviewState m_previewState = PreviewState::Empty;
};

#endif // PANELCARD_H
