#ifndef ANALYSISTEXTUTILS_H
#define ANALYSISTEXTUTILS_H

#include <QString>

namespace AnalysisTextUtils {

inline QString resolveCharacterSourceText(const QString& currentText, const QString& fallbackText)
{
    const QString trimmedCurrent = currentText.trimmed();
    if (!trimmedCurrent.isEmpty()) {
        return currentText;
    }
    return fallbackText;
}

}

#endif // ANALYSISTEXTUTILS_H
