#ifndef PANELPREVIEWWIDGET_H
#define PANELPREVIEWWIDGET_H

#include "models/Panel.h"
#include "services/ImageService.h"
#include <QWidget>
#include <QLabel>
#include <QMap>

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

    int panelCount() const { return m_panelCount; }
    int currentChapter() const { return m_currentChapter; }

signals:
    void panelClicked(int panelNumber, const QString& panelId);
    void panelCountChanged(int count);

private slots:
    void onPanelCardClicked(int panelNumber);
    void onPanelGenerated(const ImageService::GenerateResult& result);
    void onPanelDescriptionChanged(int panelNumber, const QString& description);

private:
    void setupUI();
    void populateWithPanels(const QList<Panel>& panels);
    void populateWithSample();
    void finishPopulate(int actualCount);
    PanelCard* createPanelCard(int panelNum, const QString& description, 
                                const QString& panelId = QString(), 
                                const QString& previewUrl = QString());

    QLabel* m_titleLabel = nullptr;
    QLabel* m_countLabel = nullptr;
    QWidget* m_container = nullptr;
    QHBoxLayout* m_containerLayout = nullptr;
    
    int m_currentChapter = 1;
    int m_panelCount = 0;
    QString m_storyboardId;
    QList<Panel> m_panels;
    QMap<QString, PanelCard*> m_panelCards;
};

#endif // PANELPREVIEWWIDGET_H
