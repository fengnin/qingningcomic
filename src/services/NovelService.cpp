#include "NovelService.h"
#include "TransactionScope.h"
#include "ServiceContainer.h"
#include "DatabaseManager.h"
#include "Logger.h"
#include <QUuid>
#include <QSqlQuery>
#include <QSqlError>

const QString NovelService::TABLE_NOVELS = "novels";
const QString NovelService::TABLE_STORYBOARDS = "storyboards";
const QString NovelService::TABLE_PANELS = "panels";

DEFINE_SINGLETON_INSTANCE_WITH_ARG(NovelService, DatabaseManager::instance())

NovelService::NovelService(DatabaseManager* db, QObject* parent)
    : BaseService(db, parent)
{
}

NovelService::~NovelService()
{
}

WhereClauseResult NovelService::buildUserWhereClause(const QString& userId, const QString& statusFilter)
{
    WhereClauseResult result;
    result.clause = "user_id = ?";
    result.values << userId;
    
    if (!statusFilter.isEmpty()) {
        result.clause += " AND status = ?";
        result.values << statusFilter;
    }
    
    return result;
}

Novel NovelService::buildNovelFromMap(const QVariantMap& data)
{
    Novel novel;
    novel.setId(data["id"].toString());
    novel.setUserId(data["user_id"].toString());
    novel.setTitle(data["title"].toString());
    novel.setStatus(Novel::stringToStatus(data["status"].toString()));
    novel.setCreatedAt(data["created_at"].toDateTime());
    novel.setUpdatedAt(data["updated_at"].toDateTime());
    novel.setStoryboardId(data["storyboard_id"].toString());
    
    QString metadataJson = data["metadata"].toString();
    if (!metadataJson.isEmpty()) {
        novel.setMetadata(parseJsonToMap(metadataJson));
    }
    
    return novel;
}

Novel NovelService::getNovelById(const QString& novelId)
{
    if (!checkConnection("getNovelById")) {
        return Novel();
    }
    
    QVariantMap data = m_db->selectOne(TABLE_NOVELS, "id = ?", QVariantList{novelId});
    return data.isEmpty() ? Novel() : buildNovelFromMap(data);
}

NovelPageResult NovelService::getNovels(const QString& userId, int page, int pageSize, const QString& statusFilter)
{
    NovelPageResult result;
    result.page = page;
    result.pageSize = pageSize;
    
    if (!checkConnection("getNovels")) {
        return result;
    }
    
    WhereClauseResult where = buildUserWhereClause(userId, statusFilter);
    
    result.total = m_db->count(TABLE_NOVELS, where.clause, where.values);
    result.totalPages = (result.total + pageSize - 1) / pageSize;
    
    if (result.total == 0) {
        return result;
    }
    
    QList<QVariantMap> rows = m_db->selectAll(TABLE_NOVELS, where.clause, where.values, 
        "updated_at DESC", pageSize);
    
    for (const QVariantMap& data : rows) {
        result.novels.append(buildNovelFromMap(data));
    }
    
    return result;
}

Novel NovelService::createNovel(const QString& userId, const QString& title, const QString& text, const QVariantMap& metadata)
{
    if (!checkConnection("createNovel")) {
        return Novel();
    }
    
    QString novelId = generateNovelId();
    
    QVariantMap novelMetadata = metadata;
    novelMetadata["word_count"] = text.length();
    
    QVariantMap data;
    data["id"] = novelId;
    data["user_id"] = userId;
    data["title"] = title;
    data["status"] = Novel::statusToString(NovelStatus::Created);
    data["metadata"] = serializeJson(novelMetadata);
    
    TransactionScope transaction(m_db);
    if (!transaction.isActive()) {
        emitError("createNovel", "Failed to begin transaction: " + m_db->lastError());
        return Novel();
    }
    
    if (m_db->insert(TABLE_NOVELS, data) == -1) {
        emitError("createNovel", m_db->lastError());
        return Novel();
    }
    
    if (!text.isEmpty()) {
        if (!saveText(novelId, text)) {
            LOG_ERROR("NovelService", QString("saveText failed for novel %1, rolling back").arg(novelId));
            return Novel();
        }
    }
    
    if (!transaction.commit()) {
        emitError("createNovel", "Failed to commit transaction");
        return Novel();
    }
    
    Novel novel = getNovelById(novelId);
    emit novelCreated(novel);
    
    LOG_INFO("NovelService", QString("Created novel: %1, id=%2").arg(title, novelId));
    return novel;
}

bool NovelService::updateNovel(const QString& novelId, const QVariantMap& data)
{
    if (!checkConnection("updateNovel")) {
        return false;
    }
    
    if (!m_db->update(TABLE_NOVELS, data, "id = ?", QVariantList{novelId})) {
        emitError("updateNovel", m_db->lastError());
        return false;
    }
    
    emit novelUpdated(novelId);
    return true;
}

bool NovelService::deleteNovel(const QString& novelId)
{
    if (!checkConnection("deleteNovel")) {
        return false;
    }
    
    TransactionScope transaction(m_db);
    if (!transaction.isActive()) {
        emitError("deleteNovel", "Failed to begin transaction: " + m_db->lastError());
        return false;
    }
    
    QSqlQuery query(m_db->database());
    query.prepare("DELETE FROM novels WHERE id = ?");
    query.addBindValue(novelId);
    
    if (!query.exec()) {
        emitError("deleteNovel", query.lastError().text());
        return false;
    }
    
    if (!transaction.commit()) {
        emitError("deleteNovel", "Failed to commit transaction");
        return false;
    }
    
    emit novelDeleted(novelId);
    LOG_INFO("NovelService", QString("Deleted novel: %1 (cascade by FK)").arg(novelId));
    return true;
}

bool NovelService::updateStatus(const QString& novelId, NovelStatus status)
{
    if (!checkConnection("updateStatus")) {
        return false;
    }
    
    QVariantMap data;
    data["status"] = Novel::statusToString(status);
    
    if (!m_db->update(TABLE_NOVELS, data, "id = ?", QVariantList{novelId})) {
        emitError("updateStatus", m_db->lastError());
        return false;
    }
    
    emit statusChanged(novelId, status);
    return true;
}

int NovelService::countNovels(const QString& userId, const QString& status)
{
    if (!checkConnection("countNovels")) {
        return 0;
    }
    
    WhereClauseResult where = buildUserWhereClause(userId, status);
    return m_db->count(TABLE_NOVELS, where.clause, where.values);
}

bool NovelService::saveText(const QString& novelId, const QString& text)
{
    if (!checkConnection("saveText")) {
        return false;
    }
    
    QSqlQuery query(m_db->database());
    query.prepare("UPDATE novels SET original_text = ?, updated_at = NOW() WHERE id = ?");
    query.addBindValue(text);
    query.addBindValue(novelId);
    
    if (!query.exec()) {
        emitError("saveText", query.lastError().text());
        return false;
    }
    
    emit novelUpdated(novelId);
    return true;
}

QString NovelService::loadText(const QString& novelId)
{
    if (!checkConnection("loadText")) {
        return QString();
    }
    
    QVariantMap data = m_db->selectOne(TABLE_NOVELS, "id = ?", QVariantList{novelId});
    return data.value("original_text").toString();
}

bool NovelService::exists(const QString& novelId)
{
    if (!checkConnection("exists")) {
        return false;
    }
    
    return m_db->count(TABLE_NOVELS, "id = ?", QVariantList{novelId}) > 0;
}

bool NovelService::hasPermission(const QString& novelId, const QString& userId)
{
    if (!checkConnection("hasPermission")) {
        return false;
    }
    
    return m_db->count(TABLE_NOVELS, "id = ? AND user_id = ?", QVariantList{novelId, userId}) > 0;
}

QString NovelService::generateNovelId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
