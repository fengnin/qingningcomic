#ifndef STORYBOARDSERVICE_H
#define STORYBOARDSERVICE_H

#include "services/BaseService.h"
#include "Panel.h"
#include "Storyboard.h"
#include "utils/SingletonUtils.h"
#include <QSqlQuery>

class StoryboardService : public BaseService
{
    Q_OBJECT
    DECLARE_SINGLETON_MEMBERS(StoryboardService)

public:
    explicit StoryboardService(DatabaseManager* db = nullptr, QObject* parent = nullptr);
    ~StoryboardService();
    
    bool saveStoryboard(const QString& novelId, const QJsonObject& data, int chapterNumber = 1);
    Storyboard getStoryboardByChapter(const QString& novelId, int chapterNumber);
    QList<Storyboard> getAllStoryboards(const QString& novelId);
    QList<Panel> getPanels(const QString& storyboardId);
    Panel getPanel(const QString& panelId);
    bool updatePanel(const QString& panelId, const QJsonObject& content);
    bool deleteStoryboard(const QString& novelId, int chapterNumber);

signals:
    void storyboardSaved(const QString& novelId);

private:
    QString generateId();
    
    Storyboard buildStoryboardFromQuery(QSqlQuery& query) const;
    Panel buildPanelFromQuery(QSqlQuery& query) const;
    Panel buildPanelFromMap(const QVariantMap& map) const;
    void setPanelImageUrl(Panel& panel, const QString& s3Key, bool isPreview) const;
    
    QString findStoryboardId(const QString& novelId, int chapterNumber);
    QString createStoryboard(const QString& novelId, int chapterNumber, const QJsonObject& data);
    bool updateNovelStoryboard(const QString& novelId, const QString& storyboardId);
    bool updateStoryboardPanelCount(const QString& storyboardId, int panelCount);
    bool deletePanelsByStoryboardId(const QString& storyboardId);
    bool insertPanel(const QString& storyboardId, const QJsonObject& panelJson);
    
    QList<Storyboard> queryStoryboards(const QString& novelId, int chapterNumber, const QString& orderBy, bool limitOne);
    
    static const QString PANEL_SELECT_FIELDS;
};

#endif // STORYBOARDSERVICE_H
