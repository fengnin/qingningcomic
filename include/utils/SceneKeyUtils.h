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

inline QString stripSceneModifiers(QString text)
{
    static const QStringList leadingTokens = {
        QString::fromUtf8("老式"),
        QString::fromUtf8("老旧"),
        QString::fromUtf8("破旧"),
        QString::fromUtf8("破败"),
        QString::fromUtf8("陈旧"),
        QString::fromUtf8("昏暗"),
        QString::fromUtf8("昏黄"),
        QString::fromUtf8("阴暗"),
        QString::fromUtf8("阴冷"),
        QString::fromUtf8("冷清"),
        QString::fromUtf8("空荡"),
        QString::fromUtf8("空旷"),
        QString::fromUtf8("狭窄"),
        QString::fromUtf8("宽敞"),
        QString::fromUtf8("简陋"),
        QString::fromUtf8("整洁"),
        QString::fromUtf8("凌乱"),
        QString::fromUtf8("潮湿"),
        QString::fromUtf8("废弃"),
        QString::fromUtf8("荒芜"),
        QString::fromUtf8("静谧"),
        QString::fromUtf8("安静"),
        QString::fromUtf8("寂静"),
        QString::fromUtf8("繁忙"),
        QString::fromUtf8("喧闹"),
        QString::fromUtf8("拥挤"),
        QString::fromUtf8("明亮"),
        QString::fromUtf8("清冷"),
        QString::fromUtf8("雨中"),
        QString::fromUtf8("雪中"),
        QString::fromUtf8("风中"),
        QString::fromUtf8("夜色中"),
        QString::fromUtf8("灯下"),
        QString::fromUtf8("黑暗中"),
        QString::fromUtf8("暴雨"),
        QString::fromUtf8("大雨"),
        QString::fromUtf8("小雨"),
        QString::fromUtf8("倾盆"),
        QString::fromUtf8("狂风"),
        QString::fromUtf8("细雨"),
        QString::fromUtf8("阴雨"),
        QString::fromUtf8("晴朗"),
        QString::fromUtf8("阴沉")
    };

    bool changed = true;
    while (changed) {
        changed = false;
        for (const QString& token : leadingTokens) {
            if (text.startsWith(token)) {
                text = text.mid(token.length()).trimmed();
                changed = true;
                break;
            }
        }
        text.remove(QRegularExpression(QStringLiteral(R"(^[：:\s]+)")));
    }
    return text.trimmed();
}

inline QString normalizeSceneLabel(QString text)
{
    QString result = text.trimmed();
    if (result.isEmpty()) {
        return result;
    }

    static const QStringList prefixes = {
        QString::fromUtf8("场景"),
        QString::fromUtf8("背景"),
        QString::fromUtf8("环境"),
        QStringLiteral("scene"),
        QStringLiteral("setting"),
        QStringLiteral("location")
    };

    for (const QString& prefix : prefixes) {
        if (result.startsWith(prefix, Qt::CaseInsensitive)) {
            QString stripped = result.mid(prefix.length()).trimmed();
            stripped.remove(QRegularExpression(QStringLiteral(R"(^[：:\s]+)")));
            if (!stripped.isEmpty()) {
                result = stripped;
            }
            break;
        }
    }

    const int punctuationPos = result.indexOf(QRegularExpression(QStringLiteral(R"([：:；;。！？!?，,、\n\r])")));
    if (punctuationPos > 0) {
        result = result.left(punctuationPos).trimmed();
    }

    static const QStringList shotSuffixes = {
        QString::fromUtf8("全景"),
        QString::fromUtf8("特写"),
        QString::fromUtf8("近景"),
        QString::fromUtf8("中景"),
        QString::fromUtf8("远景"),
        QString::fromUtf8("大远景"),
        QString::fromUtf8("大全景"),
        QString::fromUtf8("俯视"),
        QString::fromUtf8("仰视"),
        QString::fromUtf8("鸟瞰"),
        QString::fromUtf8("侧面")
    };

    for (const QString& suffix : shotSuffixes) {
        if (result.endsWith(suffix) && result.length() > suffix.length()) {
            result.chop(suffix.length());
            result = result.trimmed();
            break;
        }
    }

    return stripSceneModifiers(result);
}

inline QString extractSceneNounKey(const QString& text)
{
    static const QStringList nouns = {
        QString::fromUtf8("外廊"),
        QString::fromUtf8("客厅"),
        QString::fromUtf8("厨房"),
        QString::fromUtf8("卧室"),
        QString::fromUtf8("走廊"),
        QString::fromUtf8("玄关"),
        QString::fromUtf8("门口"),
        QString::fromUtf8("门廊"),
        QString::fromUtf8("门前"),
        QString::fromUtf8("楼道"),
        QString::fromUtf8("楼梯"),
        QString::fromUtf8("楼层"),
        QString::fromUtf8("阳台"),
        QString::fromUtf8("天台"),
        QString::fromUtf8("院子"),
        QString::fromUtf8("庭院"),
        QString::fromUtf8("后院"),
        QString::fromUtf8("前院"),
        QString::fromUtf8("街道"),
        QString::fromUtf8("小路"),
        QString::fromUtf8("小径"),
        QString::fromUtf8("荒径"),
        QString::fromUtf8("山路"),
        QString::fromUtf8("巷子"),
        QString::fromUtf8("巷道"),
        QString::fromUtf8("广场"),
        QString::fromUtf8("空地"),
        QString::fromUtf8("田野"),
        QString::fromUtf8("荒野"),
        QString::fromUtf8("郊野"),
        QString::fromUtf8("村落"),
        QString::fromUtf8("河边"),
        QString::fromUtf8("湖边"),
        QString::fromUtf8("海边"),
        QString::fromUtf8("树林"),
        QString::fromUtf8("森林"),
        QString::fromUtf8("桥面"),
        QString::fromUtf8("村口"),
        QString::fromUtf8("村道"),
        QString::fromUtf8("学校"),
        QString::fromUtf8("教室"),
        QString::fromUtf8("办公室"),
        QString::fromUtf8("病房"),
        QString::fromUtf8("诊室"),
        QString::fromUtf8("车站"),
        QString::fromUtf8("站台"),
        QString::fromUtf8("剧场"),
        QString::fromUtf8("舞台"),
        QString::fromUtf8("后台"),
        QString::fromUtf8("前台"),
        QString::fromUtf8("看台"),
        QString::fromUtf8("观众席"),
        QString::fromUtf8("休息区"),
        QString::fromUtf8("候场区"),
        QString::fromUtf8("商店"),
        QString::fromUtf8("店铺"),
        QString::fromUtf8("仓库"),
        QString::fromUtf8("大厅"),
        QString::fromUtf8("前厅"),
        QString::fromUtf8("房间"),
        QString::fromUtf8("屋内"),
        QString::fromUtf8("屋外"),
        QString::fromUtf8("住宅"),
        QString::fromUtf8("居民楼"),
        QString::fromUtf8("单元楼"),
        QString::fromUtf8("入口"),
        QString::fromUtf8("出口"),
        QString::fromUtf8("转角"),
        QString::fromUtf8("交界处")
    };

    int bestPos = -1;
    QString bestKey;
    for (const QString& noun : nouns) {
        const int pos = text.lastIndexOf(noun);
        if (pos < 0) {
            continue;
        }
        if (pos > bestPos || (pos == bestPos && noun.length() > bestKey.length())) {
            bestPos = pos;
            bestKey = text.mid(pos, noun.length());
        }
    }

    return bestKey;
}

inline QString buildSceneCoreKey(const QString& text)
{
    QString result = text.trimmed();
    if (result.isEmpty()) {
        return result;
    }

    const int punctuationPos = result.indexOf(QRegularExpression(QStringLiteral(R"([：:；;。！？!?，,、\n\r])")));
    if (punctuationPos > 0) {
        result = result.left(punctuationPos).trimmed();
    }

    result = stripSceneModifiers(result);
    if (result.isEmpty()) {
        return result;
    }

    const QString nounKey = extractSceneNounKey(result);
    if (!nounKey.isEmpty()) {
        return nounKey;
    }

    static const QStringList abstractTokens = {
        QString::fromUtf8("背景"),
        QString::fromUtf8("画面"),
        QString::fromUtf8("投影"),
        QString::fromUtf8("记忆"),
        QString::fromUtf8("幻象"),
        QString::fromUtf8("闪回"),
        QString::fromUtf8("镜头"),
        QString::fromUtf8("特写"),
        QString::fromUtf8("全景"),
        QString::fromUtf8("近景"),
        QString::fromUtf8("中景"),
        QString::fromUtf8("远景")
    };

    if (containsAnyToken(result, abstractTokens)) {
        return QString();
    }

    return result;
}

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
