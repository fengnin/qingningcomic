#include "services/ExportService.h"
#include "data/DatabaseManager.h"
#include "services/StoryboardService.h"
#include "data/FileStorage.h"
#include "models/Task.h"
#include "utils/ExportRenderer.h"
#include "utils/ExportDataHelper.h"
#include "utils/ExportUtils.h"
#include "utils/Logger.h"
#include <QUuid>
#include <QDateTime>

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

TaskType exportFormatToTaskType(ExportUtils::ExportFormat format)
{
    switch (format) {
    case ExportUtils::ExportFormat::Webtoon:   return TaskType::ExportWebtoon;
    case ExportUtils::ExportFormat::Resources: return TaskType::ExportResources;
    default:                                   return TaskType::ExportPdf;
    }
}

QString writeExportJobRow(DatabaseManager* db, const QString& novelId, TaskType taskType)
{
    TaskData job;
    job.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    job.type = taskType;
    job.novelId = novelId;
    job.markAsStarted();
    job.createdAt = QDateTime::currentDateTime();

    QVariantMap row = job.toDatabaseRow();
    row["created_at"] = mysqlLocalTimestamp();
    db->insert("jobs", row);
    return job.id;
}

void updateExportJobRow(DatabaseManager* db, const QString& jobId, bool success, const QString& error = {})
{
    QVariantMap data;
    data["status"] = success ? QStringLiteral("Completed") : QStringLiteral("Failed");
    if (!success && !error.isEmpty()) {
        data["error_message"] = error;
    }
    db->update("jobs", data, QStringLiteral("id = ?"), QVariantList{jobId});
}

} // namespace

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
    return ExportDataHelper::exportResultFromMap(
        m_db->selectOne("exports", "id = ?", QVariantList{exportId}));
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
    return exportResultsFromRows(
        m_db->selectAll("exports", "novel_id = ?", QVariantList{novelId}, "created_at DESC"));
}

// Loads the novel, storyboard, and panels needed for export. Returns false and emits error on failure.
bool ExportService::loadExportData(const QString& novelId, int chapterNumber,
                                   Novel* outNovel, Storyboard* outStoryboard, QList<Panel>* outPanels)
{
    StoryboardService* storyboardService = StoryboardService::instance();
    if (!storyboardService) {
        emitError("exportCurrentStory", tr("StoryboardService 未初始化"));
        return false;
    }

    *outNovel = Novel::fromVariantMap(m_db->selectOne("novels", "id = ?", QVariantList{novelId}));
    if (outNovel->id().isEmpty()) {
        emitError("exportCurrentStory", tr("作品不存在"));
        return false;
    }

    *outStoryboard = storyboardService->getStoryboardByChapter(novelId, chapterNumber);
    if (outStoryboard->id().isEmpty()) {
        emitError("exportCurrentStory", tr("当前作品没有可导出的分镜"));
        return false;
    }

    *outPanels = storyboardService->getPanels(outStoryboard->id());
    if (outPanels->isEmpty()) {
        emitError("exportCurrentStory", tr("当前章节没有可导出的面板"));
        return false;
    }

    return true;
}

bool ExportService::exportCurrentStory(const QString& novelId, int chapterNumber, const QString& format,
                                       QString* outExportId, QString* outFilePath)
{
    if (!checkConnection("exportCurrentStory")) {
        return false;
    }

    Novel novel;
    Storyboard storyboard;
    QList<Panel> panels;
    if (!loadExportData(novelId, chapterNumber, &novel, &storyboard, &panels)) {
        return false;
    }

    const ExportUtils::ExportFormat exportFormat = parseExportFormat(format);
    const QString jobId = writeExportJobRow(m_db, novelId, exportFormatToTaskType(exportFormat));

    const ExportResult record = createExport(novelId, ExportUtils::exportFormatToString(exportFormat));
    if (record.id.isEmpty()) {
        updateExportJobRow(m_db, jobId, false, tr("创建导出记录失败"));
        return false;
    }

    const QByteArray data = renderExportData(exportFormat, novel, storyboard, panels);
    if (data.isEmpty()) {
        updateExport(record.id, "failed");
        updateExportJobRow(m_db, jobId, false, tr("导出数据为空"));
        emit exportFailed(record.id, tr("导出数据为空"));
        return false;
    }

    const QString relativePath = FileStorage::instance()->saveExportFile(
        record.id, data, ExportUtils::exportFormatToString(exportFormat));
    if (relativePath.isEmpty()) {
        const QString saveError = FileStorage::instance()->lastError();
        updateExport(record.id, "failed");
        updateExportJobRow(m_db, jobId, false, saveError);
        emit exportFailed(record.id, saveError);
        return false;
    }

    if (!updateExport(record.id, "completed", data.size())) {
        updateExportJobRow(m_db, jobId, false, tr("更新导出状态失败"));
        return false;
    }

    // Write file_path and download_url — not covered by updateExport()
    QVariantMap pathData;
    pathData["file_path"] = relativePath;
    pathData["download_url"] = FileStorage::instance()->getFullPath(relativePath);
    m_db->update("exports", pathData, "id = ?", QVariantList{record.id});

    updateExportJobRow(m_db, jobId, true);

    const QString fullPath = FileStorage::instance()->getFullPath(relativePath);
    if (outExportId)  *outExportId  = record.id;
    if (outFilePath)  *outFilePath  = fullPath;

    emit exportCompleted(record.id, fullPath);
    return true;
}

QList<ExportResult> ExportService::getRecentExports(int limit)
{
    if (!checkConnection("getRecentExports")) {
        return QList<ExportResult>();
    }
    return exportResultsFromRows(
        m_db->selectAll("exports", QString(), QVariantList(), "created_at DESC", limit));
}
