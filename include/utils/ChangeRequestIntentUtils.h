#ifndef CHANGEREQUESTINTENTUTILS_H
#define CHANGEREQUESTINTENTUTILS_H

#include <QJsonObject>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace ChangeRequestIntentUtils {

bool containsAnyKeywordLower(const QString& lowerText, const QStringList& keywords);
QString extractFirstTrimmedValue(const QJsonObject& params, const QStringList& candidateKeys);
QString effectiveEditSourceText(const QJsonObject& params, const QString& sourcePrompt);
bool isBackgroundEditIntentText(const QString& text);
bool isAttributeEditIntentText(const QString& text);
QString buildRemovalSubjectPrompt(const QString& subject);
QString buildReplacementSubjectPrompt(const QString& subject, const QString& replacementDescription);
QString buildMovementSubjectPrompt(const QString& subject, const QString& destinationDescription);
QString extractLocalObjectSubjectFromParams(const QJsonObject& params, const QString& sourcePrompt);
QString extractLocalObjectDestinationFromParams(const QJsonObject& params, const QString& sourcePrompt);
QString extractBackgroundSubject(const QJsonObject& params);
QString buildBackgroundEditPrompt(const QJsonObject& params);
bool isLightingEditIntentText(const QString& text);
QString extractLightingSubject(const QJsonObject& params);
QString buildLightingEditPrompt(const QJsonObject& params);
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
