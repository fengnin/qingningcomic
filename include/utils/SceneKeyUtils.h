#ifndef SCENEKEYUTILS_H
#define SCENEKEYUTILS_H

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include "models/Scene.h"

namespace SceneKeyUtils {

QStringList deduplicateList(const QStringList& list, int maxItems = -1);
bool containsAnyToken(const QString& text, const QStringList& tokens);
QString stripSceneModifiers(QString text);
QString normalizeSceneLabel(QString text);
QString extractSceneNounKey(const QString& text);
QString buildSceneCoreKey(const QString& text);
QString resolveSceneStableKey(const QString& name, const QString& description, const QString& setting, const QStringList& aliases = QStringList(), const QStringList& detailValues = QStringList());
QString resolveSceneDisplayName(const QStringList& candidates);
QStringList buildSceneAliasKeys(const QString& name, const SceneDetails& details);
QStringList buildSceneIdentityKeys(const QString& sceneId, const QString& name, const QString& description, const QString& setting, const QStringList& aliases, const QStringList& detailValues);
QStringList buildSceneIdentityKeys(const Scene& scene);
QStringList buildSceneIdentityKeys(const QJsonObject& scene);
QMap<QString, QString> buildSceneLookupMap(const QJsonArray& scenes);
QString resolveSceneLookupId(const QMap<QString, QString>& lookup, const QStringList& candidateKeys);


} // namespace SceneKeyUtils

#endif // SCENEKEYUTILS_H
