#ifndef CHANGEREQUESTUIUTILS_H
#define CHANGEREQUESTUIUTILS_H

#include <QString>
#include <QJsonObject>

namespace ChangeRequestUiUtils {

inline QString normalizeNaturalLanguage(const QString& text)
{
    return text.trimmed();
}

inline bool hasNaturalLanguage(const QString& text)
{
    return !normalizeNaturalLanguage(text).isEmpty();
}

inline QJsonObject buildSubmissionContext(
    const QString& storyboardId,
    int chapterNumber,
    const QString& targetPanelId = QString(),
    int targetPanelNumber = 0)
{
    QJsonObject context;
    context["storyboardId"] = storyboardId;
    context["chapterNumber"] = chapterNumber;
    if (!targetPanelId.isEmpty()) {
        context["targetPanelId"] = targetPanelId;
    }
    if (targetPanelNumber > 0) {
        context["targetPanelNumber"] = targetPanelNumber;
    }
    return context;
}

}

#endif // CHANGEREQUESTUIUTILS_H
