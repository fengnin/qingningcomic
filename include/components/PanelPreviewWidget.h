#ifndef PANELPREVIEWWIDGET_H
#define PANELPREVIEWWIDGET_H

#include "models/Panel.h"
#include "services/ImageService.h"
#include <QWidget>
#include <QLabel>
#include <QMap>
#include <QScrollArea>

class QVBoxLayout;
class QHBoxLayout;
class PanelCard;

class PanelPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PanelPreviewWidget(QWidget* parent = nullptr);
    ~PanelPreviewWidget() = default;

    void setChapter(int chapter);
    void setPanels(const QList<Panel>& panels);
    void clear();
    void refresh();
    void beginBatchRefresh();
    void endBatchRefresh(bool refreshNow = true);

    int panelCount() const { return m_panelCount; }
    int currentChapter() const { return m_currentChapter; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void panelClicked(int panelNumber, const QString& panelId);
    void panelCountChanged(int count);

private slots:
    void onPanelCardClicked(int panelNumber);
    void onPanelGenerated(const ImageService::GenerateResult& result);
    void onPanelDescriptionChanged(int panelNumber, const QString& description);

private:
    void setupUI();
    void resetPanelView();
    void populateWithPanels(const QList<Panel>& panels);
    void populateEmptyState();
    void finishPopulate(int actualCount);
    QString panelDescription(const Panel& panel) const;
    int panelNumberFor(const Panel& panel) const;
    PanelCard* createPanelCard(int panelNum, const QString& description,
                               const QString& panelId = QString(),
                               const QString& previewUrl = QString());
    PanelCard* panelCardForId(const QString& panelId) const;
    void syncPanelPreview(const QString& panelId, const QString& previewUrl, int width = 0, int height = 0);
    bool savePanelDescription(const QString& panelId, const QString& description);

    QLabel* m_titleLabel = nullptr;
    QLabel* m_countLabel = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_panelContainer = nullptr;
    QHBoxLayout* m_panelLayout = nullptr;
    
    int m_currentChapter = 1;
    int m_panelCount = 0;
    QString m_storyboardId;
    QList<Panel> m_panels;
    QMap<QString, PanelCard*> m_panelCards;
    bool m_deferLiveUpdates = false;
};

#endif // PANELPREVIEWWIDGET_H
