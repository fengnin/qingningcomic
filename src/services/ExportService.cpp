#include "services/ExportService.h"
#include "data/DatabaseManager.h"
#include "services/StoryboardService.h"
#include "data/FileStorage.h"
#include "utils/ExportRenderer.h"
#include "utils/ExportDataHelper.h"
#include "utils/ExportUtils.h"
#include "utils/Logger.h"
#include <QUuid>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

namespace {
QString mysqlLocalTimestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

QList<ExportResult> exportResultsFromRows(const QList<QVariantMap>& rows)
{
    QList<ExportResult> results;
    results.reserve(rows.size());
    for (const QVariantMap& data : rows) {
        results.append(ExportDataHelper::exportResultFromMap(data));
    }
    return results;
}
}

ExportUtils::ExportFormat ExportService::parseExportFormat(const QString& format) const
{
    if (format == QLatin1String("webtoon")) {
        return ExportUtils::ExportFormat::Webtoon;
    }
    if (format == QLatin1String("resources")) {
        return ExportUtils::ExportFormat::Resources;
    }
    return ExportUtils::ExportFormat::Pdf;
}

QByteArray ExportService::renderExportData(ExportUtils::ExportFormat exportFormat, const Novel& novel,
                                           const Storyboard& storyboard, const QList<Panel>& panels) const
{
    switch (exportFormat) {
    case ExportUtils::ExportFormat::Pdf:
        return ExportRenderer::exportPanelsToPdf(novel, storyboard, panels);
    case ExportUtils::ExportFormat::Webtoon:
        return ExportRenderer::exportPanelsToWebtoon(panels);
    case ExportUtils::ExportFormat::Resources:
        return ExportRenderer::exportPanelsToResourcesZip(novel, storyboard, panels);
    }
    return QByteArray();
}

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
    
    return ExportDataHelper::exportResultFromMap(data);
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
    result.createdAt = mysqlLocalTimestamp();
    
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
    if (!checkConnection("getExportsByNovel")) {
        return QList<ExportResult>();
    }
    
    QList<QVariantMap> rows = m_db->selectAll("exports", "novel_id = ?", QVariantList{novelId}, "created_at DESC");
    return exportResultsFromRows(rows);
}

bool ExportService::exportCurrentStory(const QString& novelId, int chapterNumber, const QString& format,
                                       QString* outExportId, QString* outFilePath)
{
    if (!checkConnection("exportCurrentStory")) {
        return false;
    }

    StoryboardService* storyboardService = StoryboardService::instance();
    if (!storyboardService) {
        emitError("exportCurrentStory", tr("StoryboardService 未初始化"));
        return false;
    }

    Novel novel = Novel::fromVariantMap(m_db->selectOne("novels", "id = ?", QVariantList{novelId}));
    if (novel.id().isEmpty()) {
        emitError("exportCurrentStory", tr("作品不存在"));
        return false;
    }

    Storyboard storyboard = storyboardService->getStoryboardByChapter(novelId, chapterNumber);
    if (storyboard.id().isEmpty()) {
        emitError("exportCurrentStory", tr("当前作品没有可导出的分镜"));
        return false;
    }

    QList<Panel> panels = storyboardService->getPanels(storyboard.id());
    if (panels.isEmpty()) {
        emitError("exportCurrentStory", tr("当前章节没有可导出的面板"));
        return false;
    }

    const ExportUtils::ExportFormat exportFormat = parseExportFormat(format);

    const ExportResult record = createExport(novelId, ExportUtils::exportFormatToString(exportFormat));
    if (record.id.isEmpty()) {
        return false;
    }

    const QByteArray data = renderExportData(exportFormat, novel, storyboard, panels);

    if (data.isEmpty()) {
        updateExport(record.id, "failed");
        emit exportFailed(record.id, tr("导出数据为空"));
        return false;
    }

    const QString relativePath = FileStorage::instance()->saveExportFile(record.id, data, ExportUtils::exportFormatToString(exportFormat));
    if (relativePath.isEmpty()) {
        updateExport(record.id, "failed");
        emit exportFailed(record.id, FileStorage::instance()->lastError());
        return false;
    }

    if (!updateExport(record.id, "completed", data.size())) {
        return false;
    }

    QVariantMap updateData = ExportDataHelper::buildExportUpdateData(QStringLiteral("completed"), data.size());
    updateData["file_path"] = relativePath;
    updateData["download_url"] = FileStorage::instance()->getFullPath(relativePath);
    m_db->update("exports", updateData, "id = ?", QVariantList{record.id});

    if (outExportId) {
        *outExportId = record.id;
    }
    if (outFilePath) {
        *outFilePath = FileStorage::instance()->getFullPath(relativePath);
    }

    emit exportCompleted(record.id, FileStorage::instance()->getFullPath(relativePath));
    return true;
}

QList<ExportResult> ExportService::getRecentExports(int limit)
{
    if (!checkConnection("getRecentExports")) {
        return QList<ExportResult>();
    }
    
    QList<QVariantMap> rows = m_db->selectAll("exports", QString(), QVariantList(), "created_at DESC", limit);
    return exportResultsFromRows(rows);
}
