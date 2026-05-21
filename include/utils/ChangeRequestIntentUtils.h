#ifndef CHANGEREQUESTINTENTUTILS_H
#define CHANGEREQUESTINTENTUTILS_H

#include <QJsonObject>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include "utils/BibleUtils.h"
#include "utils/LocalEditPromptUtils.h"

namespace ChangeRequestIntentUtils {

inline QString extractFirstTrimmedValue(const QJsonObject& params, const QStringList& candidateKeys)
{
    for (const QString& key : candidateKeys) {
        const QString value = params.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return QString();
}

inline QString effectiveEditSourceText(const QJsonObject& params, const QString& sourcePrompt)
{
    const QString sourceText = params.value(QStringLiteral("sourceText")).toString().trimmed();
    if (!sourceText.isEmpty()) {
        return sourceText;
    }

    const QString rawPrompt = params.value(QStringLiteral("rawPrompt")).toString().trimmed();
    if (!rawPrompt.isEmpty()) {
        return rawPrompt;
    }

    return sourcePrompt;
}

bool isBackgroundEditIntentText(const QString& text);
bool isAttributeEditIntentText(const QString& text);
QString buildRemovalSubjectPrompt(const QString& subject);
QString buildReplacementSubjectPrompt(const QString& subject, const QString& replacementDescription);
QString buildMovementSubjectPrompt(const QString& subject, const QString& destinationDescription);
QString extractLocalObjectSubjectFromParams(const QJsonObject& params, const QString& sourcePrompt);
QString extractLocalObjectDestinationFromParams(const QJsonObject& params, const QString& sourcePrompt);
QString extractBackgroundSubject(const QJsonObject& params);
QString buildBackgroundEditPrompt(const QJsonObject& params);
QString extractRemovalSubject(const QJsonObject& params);
QString cleanupReplacementTarget(QString text);
int lastLocalContextMarkerIndex(const QString& text);
QString cleanupReplacementSource(QString text);
QString cleanupPlacementTarget(QString text);
QString extractMatchedCapture(const QString& text, const QRegularExpression& pattern, int captureIndex);
QString extractMatchedSourceText(const QString& text, const QRegularExpression& pattern, int captureIndex);
QString extractMatchedTargetText(const QString& text, const QRegularExpression& pattern, int captureIndex);
QString extractReplacementSourceFromText(const QString& text);
QString extractReplacementTargetFromText(const QString& text);
QString extractPlacementSourceFromText(const QString& text);
QString extractPlacementTargetFromText(const QString& text);

}

#endif // CHANGEREQUESTINTENTUTILS_H
