#pragma once

#include <QString>

namespace LogSummaryUtils {

inline QString summarizeText(const QString& text, int maxLength = 180)
{
    const QString trimmed = text.trimmed();
    if (maxLength <= 0) {
        return QString();
    }
    return trimmed.left(maxLength);
}

} // namespace LogSummaryUtils
