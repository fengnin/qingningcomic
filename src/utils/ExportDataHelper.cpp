#include "utils/ExportDataHelper.h"
#include "services/ExportService.h"

namespace ExportDataHelper {

static void fillCommonExportResultFields(ExportResult& result, const QVariantMap& data)
{
    result.id = data["id"].toString();
    result.novelId = data["novel_id"].toString();
    result.format = data["format"].toString();
    result.status = data["status"].toString();
    result.fileSize = data["file_size"].toLongLong();
    result.fileUrl = data["download_url"].toString();
    if (result.fileUrl.isEmpty()) {
        result.fileUrl = data["file_path"].toString();
    }
    result.createdAt = data["created_at"].toString();
    result.errorMessage = data["error_message"].toString();
}

ExportResult exportResultFromMap(const QVariantMap& data)
{
    ExportResult result;
    fillCommonExportResultFields(result, data);
    return result;
}

void fillExportResultFromMap(ExportResult& result, const QVariantMap& data)
{
    fillCommonExportResultFields(result, data);
}

QVariantMap buildExportUpdateData(const QString& status, qint64 fileSize)
{
    QVariantMap data;
    data["status"] = status;
    if (fileSize > 0) {
        data["file_size"] = fileSize;
    }
    return data;
}

}
