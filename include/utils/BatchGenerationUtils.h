#pragma once

#include <QString>

namespace BatchGenerationUtils {

inline bool isBatchGenerationSuccessful(int failedCount, const QString& errorMessage)
{
    return failedCount == 0 && errorMessage.trimmed().isEmpty();
}

inline QString buildBatchFailureMessage(int successCount, int failedCount, int totalCount, const QString& errorMessage)
{
    if (!errorMessage.trimmed().isEmpty()) {
        return errorMessage.trimmed();
    }

    if (failedCount > 0) {
        return QStringLiteral("Batch generation completed with %1 failed panel(s) out of %2")
            .arg(failedCount)
            .arg(totalCount);
    }

    if (successCount < totalCount) {
        return QStringLiteral("Batch generation did not complete all panels");
    }

    return QString();
}

} // namespace BatchGenerationUtils
