#include "services/StoryboardService.h"
#include "services/ServiceContainer.h"
#include "data/DatabaseManager.h"
#include "utils/SingletonUtils.h"
#include "data/FileStorage.h"
#include "api/StorageClient.h"
#include "data/TransactionScope.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include <QUuid>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QSqlError>

const QString StoryboardService::PANEL_SELECT_FIELDS = 
    "id, storyboard_id, page, panel_index, content, visual_prompt, preview_image_path, hd_image_path";

DEFINE_SINGLETON_INSTANCE_WITH_ARG(StoryboardService, DatabaseManager::instance())

StoryboardService::StoryboardService(DatabaseManager* db, QObject* parent)
    : BaseService(db, parent)
{
}

StoryboardService::~StoryboardService()
{
}

QString StoryboardService::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

Storyboard StoryboardService::buildStoryboardFromQuery(QSqlQuery& query) const
{
    Storyboard sb;
    sb.setId(query.value(0).toString());
    sb.setNovelId(query.value(1).toString());
    sb.setChapterNumber(query.value(2).toInt());
    sb.setTotalPages(query.value(3).toInt());
    sb.setPanelCount(query.value(4).toInt());
    sb.setVersion(query.value(5).toInt());
    return sb;
}

void StoryboardService::setPanelImageUrl(Panel& panel, const QString& s3Key, bool isPreview) const
{
    if (s3Key.isEmpty()) {
        return;
    }
    
    QString url;
    if (StorageClient::instance()->isInitialized()) {
        url = StorageClient::instance()->getPresignedUrl(s3Key);
    } else {
        url = FileStorage::instance()->getFullPath(s3Key);
    }
    
    if (isPreview) {
        panel.setPreviewUrl(url);
    } else {
        panel.setHdUrl(url);
    }
}

Panel StoryboardService::buildPanelFromQuery(QSqlQuery& query) const
{
    Panel panel;
    panel.setId(query.value(0).toString());
    panel.setStoryboardId(query.value(1).toString());
    panel.setPage(query.value(2).toInt());
    panel.setIndex(query.value(3).toInt());
    panel.setRawContent(parseJsonObject(query.value(4).toString()));
    panel.setVisualPrompt(query.value(5).toString());
    
    QString previewS3Key = query.value(6).toString();
    QString hdS3Key = query.value(7).toString();
    panel.setPreviewS3Key(previewS3Key);
    panel.setHdS3Key(hdS3Key);
    
    setPanelImageUrl(panel, previewS3Key, true);
    setPanelImageUrl(panel, hdS3Key, false);
    
    return panel;
}

Panel StoryboardService::buildPanelFromMap(const QVariantMap& map) const
{
    Panel panel;
    panel.setId(map["id"].toString());
    panel.setStoryboardId(map["storyboard_id"].toString());
    panel.setPage(map["page"].toInt());
    panel.setIndex(map["panel_index"].toInt());
    
    QJsonObject content = parseJsonObject(map["content"].toString());
    panel.setContent(content);
    panel.setVisualPrompt(map["visual_prompt"].toString());
    
    QString previewS3Key = map["preview_image_path"].toString();
    QString hdS3Key = map["hd_image_path"].toString();
    panel.setPreviewS3Key(previewS3Key);
    panel.setHdS3Key(hdS3Key);
    
    setPanelImageUrl(panel, previewS3Key, true);
    setPanelImageUrl(panel, hdS3Key, false);
    
    return panel;
}

QString StoryboardService::findStoryboardId(const QString& novelId, int chapterNumber)
{
    QSqlQuery query(m_db->database());
    query.prepare("SELECT id FROM storyboards WHERE novel_id = :novel_id AND chapter_number = :chapter");
    query.bindValue(":novel_id", novelId);
    query.bindValue(":chapter", chapterNumber);
    
    return (query.exec() && query.next()) ? query.value(0).toString() : QString();
}

QString StoryboardService::createStoryboard(const QString& novelId, int chapterNumber, const QJsonObject& data)
{
    QString id = generateId();
    
    QSqlQuery query(m_db->database());
    query.prepare("INSERT INTO storyboards (id, novel_id, chapter_number, version, total_pages, panel_count) "
                  "VALUES (:id, :novel_id, :chapter, 1, :pages, :count)");
    query.bindValue(":id", id);
    query.bindValue(":novel_id", novelId);
    query.bindValue(":chapter", chapterNumber);
    query.bindValue(":pages", data["totalPages"].toInt());
    query.bindValue(":count", data["panels"].toArray().size());
    
    if (!query.exec()) {
        emitError("createStoryboard", query.lastError().text());
        return QString();
    }
    return id;
}

bool StoryboardService::updateNovelStoryboard(const QString& novelId, const QString& storyboardId)
{
    QSqlQuery query(m_db->database());
    query.prepare("UPDATE novels SET storyboard_id = :sid, status = 'completed' WHERE id = :nid");
    query.bindValue(":sid", storyboardId);
    query.bindValue(":nid", novelId);
    return query.exec();
}

bool StoryboardService::updateStoryboardPanelCount(const QString& storyboardId, int panelCount)
{
    QSqlQuery query(m_db->database());
    query.prepare("UPDATE storyboards SET panel_count = :count WHERE id = :id");
    query.bindValue(":count", panelCount);
    query.bindValue(":id", storyboardId);
    
    if (!query.exec()) {
        emitError("updateStoryboardPanelCount", query.lastError().text());
        return false;
    }
    return true;
}

bool StoryboardService::deletePanelsByStoryboardId(const QString& storyboardId)
{
    QSqlQuery query(m_db->database());
    query.prepare("DELETE FROM panels WHERE storyboard_id = :id");
    query.bindValue(":id", storyboardId);
    
    if (!query.exec()) {
        emitError("deletePanels", query.lastError().text());
        return false;
    }
    return true;
}

bool StoryboardService::insertPanel(const QString& storyboardId, const QJsonObject& panelJson)
{
    QString panelId = panelJson["id"].toString();
    if (panelId.isEmpty()) {
        panelId = generateId();
    }
    
    QSqlQuery query(m_db->database());
    query.prepare("INSERT INTO panels (id, storyboard_id, page, panel_index, content, visual_prompt, preview_image_path, hd_image_path) "
                  "VALUES (:id, :storyboard_id, :page, :index, :content, :prompt, :preview_path, :hd_path)");
    query.bindValue(":id", panelId);
    query.bindValue(":storyboard_id", storyboardId);
    query.bindValue(":page", panelJson["page"].toInt());
    query.bindValue(":index", panelJson["index"].toInt());
    query.bindValue(":content", serializeJson(panelJson));
    query.bindValue(":prompt", panelJson["visualPrompt"].toString());
    query.bindValue(":preview_path", panelJson["previewS3Key"].toString());
    query.bindValue(":hd_path", panelJson["hdS3Key"].toString());
    
    if (!query.exec()) {
        emitError("insertPanel", query.lastError().text());
        return false;
    }
    return true;
}

bool StoryboardService::saveStoryboard(const QString& novelId, const QJsonObject& data, int chapterNumber,
                                       bool updateNovelStatus)
{
    if (!checkConnection("saveStoryboard")) {
        return false;
    }
    
    TransactionScope transaction(m_db);
    if (!transaction.isActive()) {
        emitError("saveStoryboard", "Failed to begin transaction: " + m_db->lastError());
        return false;
    }
    
    QString storyboardId = findStoryboardId(novelId, chapterNumber);
    
    if (storyboardId.isEmpty()) {
        storyboardId = createStoryboard(novelId, chapterNumber, data);
        if (storyboardId.isEmpty()) {
            LOG_ERROR("StoryboardService", "createStoryboard failed, rolling back");
            return false;
        }
    } else {
        if (!deletePanelsByStoryboardId(storyboardId)) {
            LOG_ERROR("StoryboardService", "deletePanelsByStoryboardId failed, rolling back");
            return false;
        }
    }
    
    QJsonArray panels = data["panels"].toArray();
    for (int i = 0; i < panels.size(); ++i) {
        if (!insertPanel(storyboardId, panels[i].toObject())) {
            LOG_ERROR("StoryboardService", QString("insertPanel failed at index %1, rolling back").arg(i));
            return false;
        }
    }
    
    if (!updateStoryboardPanelCount(storyboardId, panels.size())) {
        LOG_ERROR("StoryboardService", "updateStoryboardPanelCount failed, rolling back");
        return false;
    }
    
    if (updateNovelStatus) {
        if (!updateNovelStoryboard(novelId, storyboardId)) {
            LOG_ERROR("StoryboardService", "updateNovelStoryboard failed, rolling back");
            return false;
        }
    }
    
    if (!transaction.commit()) {
        emitError("saveStoryboard", "Failed to commit transaction");
        return false;
    }
    
    emit storyboardSaved(novelId);
    return true;
}

QList<Storyboard> StoryboardService::queryStoryboards(const QString& novelId, int chapterNumber, 
                                                       const QString& orderBy, bool limitOne)
{
    QList<Storyboard> list;
    if (!checkConnection("queryStoryboards")) {
        return list;
    }
    
    QString sql = "SELECT id, novel_id, chapter_number, total_pages, panel_count, version "
                  "FROM storyboards WHERE novel_id = :id";
    if (chapterNumber >= 0) {
        sql += " AND chapter_number = :chapter";
    }
    if (!orderBy.isEmpty()) {
        sql += " ORDER BY " + orderBy;
    }
    if (limitOne) {
        sql += " LIMIT 1";
    }
    
    QSqlQuery query(m_db->database());
    query.prepare(sql);
    query.bindValue(":id", novelId);
    if (chapterNumber >= 0) {
        query.bindValue(":chapter", chapterNumber);
    }
    
    if (query.exec()) {
        while (query.next()) {
            list.append(buildStoryboardFromQuery(query));
        }
    }
    return list;
}

Storyboard StoryboardService::getStoryboardByChapter(const QString& novelId, int chapterNumber)
{
    QList<Storyboard> list = queryStoryboards(novelId, chapterNumber, QString(), true);
    return list.isEmpty() ? Storyboard() : list.first();
}

QList<Storyboard> StoryboardService::getAllStoryboards(const QString& novelId)
{
    return queryStoryboards(novelId, -1, "chapter_number", false);
}

QList<Panel> StoryboardService::getPanels(const QString& storyboardId)
{
    QList<Panel> list;
    
    QList<QVariantMap> results = m_db->selectAll("panels", "storyboard_id = ?", 
        QVariantList() << storyboardId, "page, panel_index");
    
    for (const QVariantMap& map : results) {
        list.append(buildPanelFromMap(map));
    }
    
    return list;
}

Panel StoryboardService::getPanel(const QString& panelId)
{
    Panel panel;
    
    QVariantMap result = m_db->selectOne("panels", "id = ?", QVariantList() << panelId);
    
    if (!result.isEmpty()) {
        panel = buildPanelFromMap(result);
    }
    
    return panel;
}

bool StoryboardService::updatePanel(const QString& panelId, const QJsonObject& content)
{
    QString contentStr = serializeJson(content);
    QString previewPath = content["previewS3Key"].toString();
    QString hdPath = content["hdS3Key"].toString();
    
    QVariantMap data;
    data["content"] = contentStr;
    data["preview_image_path"] = previewPath;
    data["hd_image_path"] = hdPath;
    
    if (!m_db->update("panels", data, "id = ?", QVariantList() << panelId)) {
        emitError("updatePanel", m_db->lastError());
        return false;
    }
    
    return true;
}

bool StoryboardService::deleteStoryboard(const QString& novelId, int chapterNumber)
{
    if (!checkConnection("deleteStoryboard")) {
        return false;
    }
    
    QString storyboardId = findStoryboardId(novelId, chapterNumber);
    if (storyboardId.isEmpty()) {
        return true;
    }
    
    if (!deletePanelsByStoryboardId(storyboardId)) {
        return false;
    }
    
    QSqlQuery query(m_db->database());
    query.prepare("DELETE FROM storyboards WHERE id = ?");
    query.addBindValue(storyboardId);
    
    if (!query.exec()) {
        emitError("deleteStoryboard", query.lastError().text());
        return false;
    }

    QSqlQuery updateQuery(m_db->database());
    updateQuery.prepare("UPDATE novels SET storyboard_id = NULL WHERE id = ?");
    updateQuery.addBindValue(novelId);
    if (!updateQuery.exec()) {
        emitError("deleteStoryboard", updateQuery.lastError().text());
    }

    return true;
}
