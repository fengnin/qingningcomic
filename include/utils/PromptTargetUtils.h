#ifndef PROMPTTARGETUTILS_H
#define PROMPTTARGETUTILS_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace PromptTargetUtils {

bool containsAny(const QString& text, const QStringList& keywords);
bool isSceneFocusedPrompt(const QString& text);
bool isCharacterFocusedPrompt(const QString& text);
bool isRemovalIntentText(const QString& text);
bool isRotationIntent(const QString& editIntent);
bool isLocalObjectMovementIntent(const QString& editIntent);
bool isSceneEditIntent(const QString& editIntent);
bool isCharacterEditIntent(const QString& editIntent);
bool shouldPreferSceneReferenceForEditIntent(const QString& editIntent, const QString& editPrompt);
QString resolveTargetFromEditMetadata(const QJsonObject& panelJson);
QString resolveEditPromptTarget(const QJsonObject& panelJson, const QString& explicitTarget, const QString& editPrompt, const QString& panelSceneText);
QString classifyPanelPromptTarget(const QJsonObject& panelJson);


} // namespace PromptTargetUtils

#endif // PROMPTTARGETUTILS_H
