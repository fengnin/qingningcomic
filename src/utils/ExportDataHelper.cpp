#include "utils/ExportDataHelper.h"
#include "services/ExportService.h"

namespace ExportDataHelper {

ExportResult exportResultFromMap(const QVariantMap& data)
{
    ExportResult result;
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
    return result;
}

}
