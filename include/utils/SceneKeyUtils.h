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

inline QStringList deduplicateList(const QStringList& list, int maxItems = -1)
{
    QStringList result;
    QSet<QString> seen;
    for (QString item : list) {
        item = item.trimmed();
        if (item.isEmpty() || seen.contains(item)) {
            continue;
        }
        seen.insert(item);
        result << item;
    }
    return maxItems > 0 ? result.mid(0, maxItems) : result;
}

inline bool containsAnyToken(const QString& text, const QStringList& tokens)
{
    for (const QString& token : tokens) {
        if (!token.isEmpty() && text.contains(token)) {
            return true;
        }
    }
    return false;
}

QString stripSceneModifiers(QString text);

QString normalizeSceneLabel(QString text);

QString extractSceneNounKey(const QString& text);

QString buildSceneCoreKey(const QString& text);

inline QString resolveSceneStableKey(const QString& name,
                                    const QString& description,
                                    const QString& setting,
                                    const QStringList& aliases = QStringList(),
                                    const QStringList& detailValues = QStringList())
{
    QStringList candidates;
    candidates << name << setting << description;
    candidates << aliases << detailValues;

    for (const QString& candidate : candidates) {
        const QString normalized = normalizeSceneLabel(candidate);
        const QString coreKey = buildSceneCoreKey(normalized.isEmpty() ? candidate : normalized);
        if (!coreKey.isEmpty()) {
            return coreKey;
        }
    }

    return QString();
}

inline QString resolveSceneDisplayName(const QStringList& candidates)
{
    for (const QString& candidate : candidates) {
        const QString normalized = normalizeSceneLabel(candidate);
        if (!normalized.isEmpty()) {
            return normalized;
        }
    }
    return QString();
}

inline QStringList buildSceneAliasKeys(const QString& name, const SceneDetails& details)
{
    QStringList keys;

    auto addKey = [&keys](const QString& value) {
        const QString trimmed = value.trimmed();
        if (trimmed.isEmpty()) {
            return;
        }

        const QString normalized = normalizeSceneLabel(trimmed);
        if (!normalized.isEmpty() && !keys.contains(normalized)) {
            keys << normalized;
        }

        const QString coreKey = buildSceneCoreKey(trimmed);
        if (!coreKey.isEmpty() && !keys.contains(coreKey)) {
            keys << coreKey;
        }
    };

    addKey(name);
    addKey(details.description);
    addKey(details.building);
    addKey(details.landmark);
    addKey(details.layout);
    addKey(details.setting);

    for (const QString& alias : details.aliases) {
        addKey(alias);
    }

    return deduplicateList(keys);
}

inline QStringList buildSceneIdentityKeys(const QString& sceneId,
                                          const QString& name,
                                          const QString& description,
                                          const QString& setting,
                                          const QStringList& aliases,
                                          const QStringList& detailValues)
{
    QStringList keys;

    if (!sceneId.trimmed().isEmpty()) {
        keys << sceneId.trimmed();
    }

    SceneDetails details;
    details.description = description;
    details.setting = setting;
    details.aliases = aliases;

    const QStringList mergedDetails = deduplicateList(detailValues);
    for (const QString& value : mergedDetails) {
        details.details << value;
    }

    keys << buildSceneAliasKeys(name, details);

    if (!description.trimmed().isEmpty()) {
        const QString coreKey = buildSceneCoreKey(description);
        if (!coreKey.isEmpty()) {
            keys << coreKey;
        }
    }

    for (const QString& value : mergedDetails) {
        const QString normalized = normalizeSceneLabel(value);
        if (!normalized.isEmpty()) {
            keys << normalized;
        }
        const QString coreKey = buildSceneCoreKey(value);
        if (!coreKey.isEmpty()) {
            keys << coreKey;
        }
    }

    return deduplicateList(keys);
}

inline QStringList buildSceneIdentityKeys(const Scene& scene)
{
    const SceneDetails details = scene.details();
    const QStringList detailValues = {
        details.landmark,
        details.building,
        details.layout,
        details.setting
    };
    QStringList keys = buildSceneIdentityKeys(scene.id(), scene.name(), details.description, details.setting,
                                  details.aliases, detailValues);
    if (!scene.sceneId().trimmed().isEmpty()) {
        keys.prepend(scene.sceneId().trimmed());
    }
    return deduplicateList(keys);
}

inline QStringList buildSceneIdentityKeys(const QJsonObject& scene)
{
    const QStringList detailValues = {
        scene.value("description").toString(),
        scene.value("details").toObject().value("description").toString(),
        scene.value("landmark").toString(),
        scene.value("building").toString(),
        scene.value("layout").toString(),
        scene.value("setting").toString()
    };

    QStringList aliases;
    const QJsonObject details = scene.value("details").toObject();
    for (const QJsonValue& aliasValue : details.value("aliases").toArray()) {
        aliases << aliasValue.toString();
    }
    for (const QJsonValue& aliasValue : scene.value("aliases").toArray()) {
        aliases << aliasValue.toString();
    }

    QStringList keys = buildSceneIdentityKeys(
        scene.value("id").toString(),
        scene.value("name").toString(),
        scene.value("description").toString(),
        scene.value("setting").toString(),
        aliases,
        detailValues
    );
    const QString sceneStableId = scene.value("sceneId").toString().trimmed();
    const QString sceneStableIdAlt = scene.value("scene_id").toString().trimmed();
    if (!sceneStableIdAlt.isEmpty()) {
        keys.prepend(sceneStableIdAlt);
    }
    if (!sceneStableId.isEmpty()) {
        keys.prepend(sceneStableId);
    }
    return deduplicateList(keys);
}

inline QMap<QString, QString> buildSceneLookupMap(const QJsonArray& scenes)
{
    QMap<QString, QString> lookup;
    for (const QJsonValue& sceneVal : scenes) {
        const QJsonObject sceneObj = sceneVal.toObject();
        const QString sceneId = sceneObj.value("id").toString().trimmed();
        if (sceneId.isEmpty()) {
            continue;
        }

        const QString sceneStableId = sceneObj.value("sceneId").toString().trimmed();
        const QString sceneStableIdAlt = sceneObj.value("scene_id").toString().trimmed();

        for (const QString& key : buildSceneIdentityKeys(sceneObj)) {
            if (!key.isEmpty()) {
                lookup[key] = sceneId;
            }
        }
        if (!sceneStableId.isEmpty()) {
            lookup[sceneStableId] = sceneId;
        }
        if (!sceneStableIdAlt.isEmpty()) {
            lookup[sceneStableIdAlt] = sceneId;
        }
    }
    return lookup;
}

inline QString resolveSceneLookupId(const QMap<QString, QString>& lookup, const QStringList& candidateKeys)
{
    for (const QString& key : candidateKeys) {
        if (!key.isEmpty() && lookup.contains(key)) {
            return lookup.value(key);
        }
    }

    QString bestMatchId;
    int bestMatchLen = 0;

    for (auto it = lookup.constBegin(); it != lookup.constEnd(); ++it) {
        const QString lookupKey = it.key();
        if (lookupKey.length() < 2) {
            continue;
        }
        for (const QString& candidate : candidateKeys) {
            if (candidate.isEmpty()) {
                continue;
            }
            if (lookupKey.contains(candidate) || candidate.contains(lookupKey)) {
                if (lookupKey.length() > bestMatchLen) {
                    bestMatchLen = lookupKey.length();
                    bestMatchId = it.value();
                }
            }
        }
    }

    return bestMatchId;
}

} // namespace SceneKeyUtils

#endif // SCENEKEYUTILS_H
