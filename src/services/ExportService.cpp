#include "services/ExportService.h"
#include "data/DatabaseManager.h"
#include "utils/Logger.h"
#include <QUuid>
#include <QDateTime>

ExportService::ExportService(DatabaseManager* db, QObject* parent)
    : BaseService(db, parent)
{
}

ExportResult ExportService::getById(const QString& exportId)
{
    if (!checkConnection("getById")) {
        return ExportResult();
    }
    
    QVariantMap data = m_db->selectOne("exports", "id = ?", QVariantList{exportId});
    
    if (data.isEmpty()) {
        return ExportResult();
    }
    
    ExportResult result;
    result.id = data["id"].toString();
    result.novelId = data["novel_id"].toString();
    result.format = data["format"].toString();
    result.status = data["status"].toString();
    result.fileSize = data["file_size"].toLongLong();
    result.fileUrl = data["file_url"].toString();
    result.createdAt = data["created_at"].toString();
    result.errorMessage = data["error_message"].toString();
    
    return result;
}

ExportResult ExportService::createExport(const QString& novelId, const QString& format)
{
    if (!checkConnection("createExport")) {
        return ExportResult();
    }
    
    ExportResult result;
    result.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    result.novelId = novelId;
    result.format = format;
    result.status = "pending";
    result.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QVariantMap data;
    data["id"] = result.id;
    data["novel_id"] = novelId;
    data["format"] = format;
    data["status"] = "pending";
    data["created_at"] = result.createdAt;
    
    if (m_db->insert("exports", data) == -1) {
        emitError("createExport", m_db->lastError());
        return ExportResult();
    }
    
    emit exportCreated(result);
    LOG_INFO("ExportService", QString("Created export: %1 for novel %2").arg(result.id, novelId));
    return result;
}

bool ExportService::updateExport(const QString& exportId, const QString& status, qint64 fileSize)
{
    if (!checkConnection("updateExport")) {
        return false;
    }
    
    QVariantMap data;
    data["status"] = status;
    if (fileSize > 0) {
        data["file_size"] = fileSize;
    }
    
    if (!m_db->update("exports", data, "id = ?", QVariantList{exportId})) {
        emitError("updateExport", m_db->lastError());
        return false;
    }
    
    emit exportUpdated(exportId, status);
    LOG_INFO("ExportService", QString("Updated export %1: status=%2").arg(exportId, status));
    return true;
}

QString ExportService::getFilePath(const QString& exportId, const QString& format)
{
    return QString("exports/%1/%2.%3").arg(exportId, exportId, format);
}

QList<ExportResult> ExportService::getExportsByNovel(const QString& novelId)
{
    QList<ExportResult> results;
    
    if (!checkConnection("getExportsByNovel")) {
        return results;
    }
    
    QList<QVariantMap> rows = m_db->selectAll("exports", "novel_id = ?", QVariantList{novelId}, "created_at DESC");
    
    for (const QVariantMap& data : rows) {
        ExportResult result;
        result.id = data["id"].toString();
        result.novelId = data["novel_id"].toString();
        result.format = data["format"].toString();
        result.status = data["status"].toString();
        result.fileSize = data["file_size"].toLongLong();
        result.fileUrl = data["file_url"].toString();
        result.createdAt = data["created_at"].toString();
        result.errorMessage = data["error_message"].toString();
        results.append(result);
    }
    
    return results;
}

QList<ExportResult> ExportService::getRecentExports(int limit)
{
    QList<ExportResult> results;
    
    if (!checkConnection("getRecentExports")) {
        return results;
    }
    
    QList<QVariantMap> rows = m_db->selectAll("exports", QString(), QVariantList(), "created_at DESC", limit);
    
    for (const QVariantMap& data : rows) {
        ExportResult result;
        result.id = data["id"].toString();
        result.novelId = data["novel_id"].toString();
        result.format = data["format"].toString();
        result.status = data["status"].toString();
        result.fileSize = data["file_size"].toLongLong();
        result.fileUrl = data["file_url"].toString();
        result.createdAt = data["created_at"].toString();
        result.errorMessage = data["error_message"].toString();
        results.append(result);
    }
    
    return results;
}
