#include "utils/BibleUtils.h"
#include <QRegularExpression>
#include <QUuid>

namespace BibleUtils {

QStringList mergeStringLists(const QStringList& base, const QStringList& incoming)
{
    QStringList result;
    QSet<QString> seen;
    auto addIfValid = [&result, &seen](const QString& item) {
        QString trimmed = item.trimmed();
        if (!trimmed.isEmpty() && !seen.contains(trimmed)) {
            seen.insert(trimmed);
            result.append(trimmed);
        }
    };
    for (const QString& item : base) addIfValid(item);
    for (const QString& item : incoming) addIfValid(item);
    return result;
}

QStringList mergeJsonArraysToStringList(const QJsonArray& base, const QJsonArray& incoming)
{
    return mergeStringLists(JsonUtils::jsonArrayToStringList(base), JsonUtils::jsonArrayToStringList(incoming));
}

QString extractDetailValue(const QString& detail, const QStringList& labels)
{
    for (const QString& label : labels) {
        if (label.isEmpty()) continue;
        const QRegularExpression pattern(QStringLiteral("%1\\s*[:：]\\s*([^，,；;、\\n]+)")
                                             .arg(QRegularExpression::escape(label)));
        const QRegularExpressionMatch match = pattern.match(detail);
        if (match.hasMatch()) return match.captured(1).trimmed();
    }
    return QString();
}

QStringList splitCommaSeparatedList(const QString& value)
{
    QStringList result;
    for (const QString& item : value.split(QRegularExpression(QStringLiteral("[,，、;；]")), Qt::SkipEmptyParts)) {
        const QString trimmed = item.trimmed();
        if (!trimmed.isEmpty()) result << trimmed;
    }
    return result;
}

QStringList jsonArrayToStringList(const QJsonArray& array)
{
    QStringList result;
    for (const QJsonValue& v : array) result << v.toString();
    return result;
}

QJsonObject mergeObjectPreserve(const QJsonObject& base, const QJsonObject& incoming)
{
    QJsonObject result = base;
    for (auto it = incoming.constBegin(); it != incoming.constEnd(); ++it) {
        const QString& key = it.key();
        const QJsonValue& value = it.value();
        if (value.isNull() || value.isUndefined()) continue;
        if (value.isArray()) {
            QStringList merged = mergeJsonArraysToStringList(result[key].toArray(), value.toArray());
            result[key] = QJsonArray::fromStringList(merged);
        } else if (value.isObject()) {
            result[key] = mergeObjectPreserve(result[key].toObject(), value.toObject());
        } else if (result[key].isNull() || result[key].isUndefined()) {
            result[key] = value;
        }
    }
    return result;
}

QJsonArray mergeObjectArray(const QJsonArray& base, const QJsonArray& incoming, const QString& keyField)
{
    QJsonArray result;
    QMap<QString, QJsonObject> index;
    for (const QJsonValue& item : base) {
        if (item.isNull()) continue;
        QJsonObject obj = item.toObject();
        result.append(obj);
        if (keyField.isEmpty() || obj.contains(keyField)) {
            QString key = keyField.isEmpty() ? QString::number(result.size() - 1) : obj[keyField].toString();
            index[key] = obj;
        }
    }
    for (const QJsonValue& item : incoming) {
        if (item.isNull()) continue;
        QJsonObject obj = item.toObject();
        QString key = keyField.isEmpty() ? QString() : obj[keyField].toString();
        if (!key.isEmpty() && index.contains(key)) {
            QJsonObject target = index[key];
            for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
                const QString& k = it.key();
                const QJsonValue& v = it.value();
                if (v.isNull()) continue;
                if (v.isArray()) {
                    QStringList merged = mergeJsonArraysToStringList(target[k].toArray(), v.toArray());
                    target[k] = QJsonArray::fromStringList(merged);
                } else if (v.isObject()) {
                    target[k] = mergeObjectPreserve(target[k].toObject(), v.toObject());
                } else if (!target.contains(k) || target[k].isNull()) {
                    target[k] = v;
                }
            }
            index[key] = target;
            for (int i = 0; i < result.size(); ++i) {
                QJsonObject ro = result[i].toObject();
                if (ro[keyField].toString() == key) {
                    result[i] = target;
                    break;
                }
            }
        } else {
            result.append(obj);
            if (!key.isEmpty()) index[key] = obj;
        }
    }
    return result;
}

QJsonObject mergeVisualCharacteristics(const QJsonObject& base, const QJsonObject& incoming)
{
    return mergeObjectPreserve(base, incoming);
}

QJsonObject mergeSpatialLayout(const QJsonObject& base, const QJsonObject& incoming)
{
    QJsonObject result = base;
    for (auto it = incoming.constBegin(); it != incoming.constEnd(); ++it) {
        const QString& key = it.key();
        const QJsonValue& value = it.value();
        if (value.isNull()) continue;
        if (key == "keyAreas" && value.isArray()) {
            result[key] = mergeObjectArray(result[key].toArray(), value.toArray(), "name");
        } else if (value.isArray()) {
            QStringList merged = mergeJsonArraysToStringList(result[key].toArray(), value.toArray());
            result[key] = QJsonArray::fromStringList(merged);
        } else if (value.isObject()) {
            result[key] = mergeObjectPreserve(result[key].toObject(), value.toObject());
        } else if (!result.contains(key) || result[key].isNull()) {
            result[key] = value;
        }
    }
    return result;
}

QJsonArray mergeVariations(const QJsonArray& base, const QJsonArray& incoming, const QString& keyField)
{
    return mergeObjectArray(base, incoming, keyField);
}

// ==================== Inference ====================

namespace Inference {

bool containsAny(const QString& text, const QStringList& tokens)
{
    for (const QString& token : tokens) {
        if (!token.isEmpty() && text.contains(token)) return true;
    }
    return false;
}

QString inferSceneType(const QString& name, const QString& description)
{
    QString combined = (name + " " + description).toLower();
    QStringList indoorKeywords = {
        QString::fromUtf8("房间"), QString::fromUtf8("卧室"), QString::fromUtf8("教室"),
        QString::fromUtf8("办公室"), QString::fromUtf8("客厅"), QString::fromUtf8("堂屋"),
        QString::fromUtf8("厨房"), QString::fromUtf8("书房"), QString::fromUtf8("门内"),
        QString::fromUtf8("屋内"), QString::fromUtf8("室内"), QString::fromUtf8("老宅")
    };
    QStringList outdoorKeywords = {
        QString::fromUtf8("街道"), QString::fromUtf8("古街"), QString::fromUtf8("广场"),
        QString::fromUtf8("公园"), QString::fromUtf8("野外"), QString::fromUtf8("室外"),
        QString::fromUtf8("户外"), QString::fromUtf8("巷"), QString::fromUtf8("路")
    };
    QStringList transitionalKeywords = {
        QString::fromUtf8("门口"), QString::fromUtf8("门槛"), QString::fromUtf8("门边"),
        QString::fromUtf8("门外"), QString::fromUtf8("门内"), QString::fromUtf8("入口"),
        QString::fromUtf8("出口"), QString::fromUtf8("木门"), QString::fromUtf8("门扉")
    };
    if (containsAny(combined, transitionalKeywords) && (containsAny(combined, indoorKeywords) || containsAny(combined, outdoorKeywords)))
        return "indoor-outdoor";
    if (containsAny(combined, indoorKeywords)) return "indoor";
    if (containsAny(combined, outdoorKeywords)) return "outdoor";
    if (combined.contains("城市") || combined.contains("商场") || combined.contains("地铁") || combined.contains("车站"))
        return "urban";
    if (combined.contains("乡村") || combined.contains("农田") || combined.contains("村庄"))
        return "rural";
    if (combined.contains("森林") || combined.contains("山") || combined.contains("海") || combined.contains("河"))
        return "nature";
    return "outdoor";
}

QString inferSetting(const QString& name, const QString& description)
{
    QString combined = (name + " " + description).toLower();
    QStringList ancientKeywords = {
        QString::fromUtf8("古代"), QString::fromUtf8("古城"), QString::fromUtf8("古街"),
        QString::fromUtf8("古镇"), QString::fromUtf8("戏袍"), QString::fromUtf8("戏服"),
        QString::fromUtf8("煤油灯"), QString::fromUtf8("老宅"), QString::fromUtf8("旧宅"),
        QString::fromUtf8("堂屋"), QString::fromUtf8("土灶"), QString::fromUtf8("木门"),
        QString::fromUtf8("纸糊窗"), QString::fromUtf8("青石板"), QString::fromUtf8("传统民居"),
        QString::fromUtf8("祠堂"), QString::fromUtf8("门楼"), QString::fromUtf8("瓦房")
    };
    if (combined.contains("现代") || combined.contains("商场") || combined.contains("地铁") || combined.contains("高楼"))
        return "modern";
    if (containsAny(combined, ancientKeywords) || combined.contains("宫殿") || combined.contains("朝代"))
        return "ancient";
    if (combined.contains("未来") || combined.contains("太空") || combined.contains("飞船"))
        return "future";
    if (combined.contains("奇幻") || combined.contains("魔法"))
        return "fantasy";
    return QString();
}

QString inferTimeOfDay(const QString& name, const QString& description)
{
    QString combined = (name + " " + description).toLower();
    if (combined.contains("黑夜") || combined.contains("夜幕") || combined.contains("夜色") ||
        combined.contains("夜雨") || combined.contains("雷雨") || combined.contains("深夜") ||
        (combined.contains("灯火") && combined.contains("雨")))
        return "night";
    if (combined.contains("黎明") || combined.contains("拂晓")) return "dawn";
    if (combined.contains("早晨") || combined.contains("清晨")) return "morning";
    if (combined.contains("正午") || combined.contains("中午")) return "noon";
    if (combined.contains("午后") || combined.contains("下午")) return "afternoon";
    if (combined.contains("黄昏") || combined.contains("傍晚")) return "dusk";
    if (combined.contains("夜晚") || combined.contains("深夜") || combined.contains("夜里")) return "night";
    if (combined.contains("午夜")) return "midnight";
    if (combined.contains("煤油灯") || combined.contains("灯火")) return "night";
    return QString();
}

QString inferWeather(const QString& name, const QString& description)
{
    QString combined = (name + " " + description).toLower();
    if (combined.contains("暴雨") || combined.contains("大雨") || combined.contains("雷雨") || combined.contains("雨幕")) return "rainy";
    if (combined.contains("雨")) return "rainy";
    if (combined.contains("雪")) return "snowy";
    if (combined.contains("雾")) return "foggy";
    if (combined.contains("暴风") || combined.contains("风暴")) return "stormy";
    if (combined.contains("多云") || combined.contains("阴")) return "cloudy";
    return QString();
}

QString inferBuilding(const QString& setting)
{
    if (setting == "ancient") return "传统民居或古旧木构建筑";
    if (setting == "future") return "未来感建筑";
    if (setting == "fantasy") return "奇幻建筑";
    return QString();
}

QString inferColorScheme(const QString& timeOfDay, const QString& weather)
{
    if (timeOfDay.isEmpty() && weather.isEmpty()) return QString();
    if (timeOfDay == "night" || timeOfDay == "midnight") {
        if (weather == "rainy" || weather == "foggy") return "冷灰主调 + 朱红点色 + 灯火暖黄";
        return "深色主调 + 局部暖光";
    }
    if (weather == "sunny") return "暖色调 + 高光";
    if (weather == "rainy" || weather == "foggy") return "灰蓝主调 + 低饱和暖点";
    return QString();
}

QString inferAtmosphere(const QString& timeOfDay, const QString& type)
{
    if (timeOfDay == "night") return "压抑、紧张";
    if (type == "indoor") return "封闭、压迫";
    return QString();
}

QString inferLayout(const QString& type)
{
    if (type == "indoor") return "封闭式室内空间";
    if (type == "indoor-outdoor") return "门口/门廊过渡空间";
    if (type == "outdoor") return "开放式街巷空间";
    return "混合空间";
}

QString inferSpaceSize(const QString& name, const QString& description)
{
    QString combined = (name + " " + description).toLower();
    if (combined.contains("狭小") || combined.contains("狭窄") || combined.contains("小房间")) return "狭小";
    if (combined.contains("宽敞") || combined.contains("大厅") || combined.contains("广场")) return "宽敞";
    if (combined.contains("巨大") || combined.contains("广阔") || combined.contains("无边")) return "巨大";
    return QString();
}

QString inferNarrativeRole(const QString& name, const QString& description)
{
    QString combined = (name + " " + description).toLower();
    if (combined.contains("开场") || combined.contains("起始") || combined.contains("第一章") ||
        combined.contains("主要") || combined.contains("重要") || combined.contains("核心"))
        return "primary";
    if (combined.contains("背景") || combined.contains("次要")) return "background";
    return QString();
}

int inferAge(const QString& role, const QString& name)
{
    const QString combined = (role + " " + name).toLower();
    static const QStringList elderTokens = {
        QString::fromUtf8("老年"), QString::fromUtf8("老人"), QString::fromUtf8("老者"),
        QString::fromUtf8("爷爷"), QString::fromUtf8("祖父"), QString::fromUtf8("外公"),
        QString::fromUtf8("奶奶"), QString::fromUtf8("祖母"), QString::fromUtf8("外婆")
    };
    if (containsAny(combined, elderTokens)) return 65;
    static const QStringList matureMaleTokens = {
        QString::fromUtf8("伯伯"), QString::fromUtf8("叔叔"), QString::fromUtf8("伯"),
        QString::fromUtf8("叔"), QString::fromUtf8("舅舅"), QString::fromUtf8("舅")
    };
    if (containsAny(combined, matureMaleTokens)) return 50;
    static const QStringList matureFemaleTokens = {
        QString::fromUtf8("阿姨"), QString::fromUtf8("姑姑"), QString::fromUtf8("婶婶"),
        QString::fromUtf8("姨"), QString::fromUtf8("姑"), QString::fromUtf8("婶")
    };
    if (containsAny(combined, matureFemaleTokens)) return 45;
    static const QStringList parentTokens = {
        QString::fromUtf8("父亲"), QString::fromUtf8("母亲"), QString::fromUtf8("爸爸"),
        QString::fromUtf8("妈妈"), QString::fromUtf8("家长")
    };
    if (containsAny(combined, parentTokens)) return 45;
    static const QStringList youngAdultTokens = {
        QString::fromUtf8("青年"), QString::fromUtf8("成人"), QString::fromUtf8("大学生")
    };
    if (containsAny(combined, youngAdultTokens)) return 22;
    static const QStringList teenTokens = {
        QString::fromUtf8("少年"), QString::fromUtf8("学生"), QString::fromUtf8("男孩"), QString::fromUtf8("女孩")
    };
    if (containsAny(combined, teenTokens)) return 16;
    static const QStringList childTokens = {
        QString::fromUtf8("儿童"), QString::fromUtf8("孩子")
    };
    if (containsAny(combined, childTokens)) return 8;
    return 0;
}

} // namespace Inference

// ==================== Translation ====================

namespace Translation {

QString translateSceneType(const QString& type)
{
    static const QMap<QString, QString> map = {
        {"indoor", QString::fromUtf8("室内")},
        {"outdoor", QString::fromUtf8("室外")},
        {"urban", QString::fromUtf8("城市")},
        {"rural", QString::fromUtf8("乡村")},
        {"nature", QString::fromUtf8("自然")},
        {"fantasy", QString::fromUtf8("奇幻")},
        {"scifi", QString::fromUtf8("科幻")}
    };
    return lookup(map, type);
}

QString translateTimeOfDay(const QString& timeOfDay)
{
    static const QMap<QString, QString> map = {
        {"dawn", QString::fromUtf8("黎明")},
        {"morning", QString::fromUtf8("早晨")},
        {"noon", QString::fromUtf8("中午")},
        {"afternoon", QString::fromUtf8("下午")},
        {"dusk", QString::fromUtf8("傍晚")},
        {"evening", QString::fromUtf8("傍晚")},
        {"night", QString::fromUtf8("夜晚")},
        {"midnight", QString::fromUtf8("午夜")},
        {"day", QString::fromUtf8("白天")}
    };
    return lookup(map, timeOfDay);
}

QString translateWeather(const QString& weather)
{
    static const QMap<QString, QString> map = {
        {"sunny", QString::fromUtf8("晴朗")},
        {"clear", QString::fromUtf8("晴朗")},
        {"cloudy", QString::fromUtf8("多云")},
        {"overcast", QString::fromUtf8("阴天")},
        {"rainy", QString::fromUtf8("雨天")},
        {"snowy", QString::fromUtf8("雪天")},
        {"foggy", QString::fromUtf8("雾天")},
        {"stormy", QString::fromUtf8("暴风")}
    };
    return lookup(map, weather);
}

QString translateNarrativeRole(const QString& role)
{
    static const QMap<QString, QString> map = {
        {"primary-setting", QString::fromUtf8("主要场景")},
        {"primary", QString::fromUtf8("主要场景")},
        {"secondary", QString::fromUtf8("次要场景")},
        {"secondary-setting", QString::fromUtf8("次要场景")},
        {"background", QString::fromUtf8("背景场景")},
        {"transitional", QString::fromUtf8("过渡场景")},
        {"symbolic", QString::fromUtf8("象征场景")}
    };
    return lookup(map, role);
}

QString preferZh(const QString& zhValue, const QString& enValue, QString(*translator)(const QString&))
{
    return zhValue.isEmpty() ? translator(enValue) : zhValue;
}

} // namespace Translation

// ==================== SceneTextFilter ====================

namespace SceneTextFilter {

bool isDynamicSceneText(const QString& text)
{
    const QString normalized = text.trimmed();
    if (normalized.isEmpty()) return true;

    static const QStringList humanTokens = {
        QString::fromUtf8("我"), QString::fromUtf8("你"), QString::fromUtf8("他"),
        QString::fromUtf8("她"), QString::fromUtf8("他们"), QString::fromUtf8("她们"),
        QString::fromUtf8("人物"), QString::fromUtf8("角色"), QString::fromUtf8("主角"),
        QString::fromUtf8("配角"), QString::fromUtf8("男人"), QString::fromUtf8("女人"),
        QString::fromUtf8("少年"), QString::fromUtf8("少女"), QString::fromUtf8("老人"),
        QString::fromUtf8("孩子"), QString::fromUtf8("母亲"), QString::fromUtf8("父亲"),
        QString::fromUtf8("摊主"), QString::fromUtf8("爷爷")
    };
    static const QStringList dialogueTokens = {
        QStringLiteral("'"), QStringLiteral("\""),
        QString::fromUtf8("“"), QString::fromUtf8("”"),
        QString::fromUtf8("："), QString::fromUtf8("说道"),
        QString::fromUtf8("问道"), QString::fromUtf8("喊道"), QString::fromUtf8("对白")
    };
    static const QStringList actionTokens = {
        QString::fromUtf8("蹲"), QString::fromUtf8("站"), QString::fromUtf8("走"),
        QString::fromUtf8("跑"), QString::fromUtf8("看"), QString::fromUtf8("闻"),
        QString::fromUtf8("抬头"), QString::fromUtf8("低头"), QString::fromUtf8("翻"),
        QString::fromUtf8("说"), QString::fromUtf8("笑"), QString::fromUtf8("哭"),
        QString::fromUtf8("坐"), QString::fromUtf8("躺"), QString::fromUtf8("冲"),
        QString::fromUtf8("扑"), QString::fromUtf8("望"), QString::fromUtf8("盯"),
        QString::fromUtf8("转身"), QString::fromUtf8("伸手"), QString::fromUtf8("抓"),
        QString::fromUtf8("抱"), QString::fromUtf8("拿"), QString::fromUtf8("握"),
        QString::fromUtf8("推"), QString::fromUtf8("拉"), QString::fromUtf8("开口"),
        QString::fromUtf8("叫"), QString::fromUtf8("喊"), QString::fromUtf8("喘"),
        QString::fromUtf8("跪"), QString::fromUtf8("倚"), QString::fromUtf8("靠在"),
        QString::fromUtf8("悬于"), QString::fromUtf8("紧贴"), QString::fromUtf8("垂落")
    };
    static const QStringList postureTokens = {
        QString::fromUtf8("目光"), QString::fromUtf8("眼底"), QString::fromUtf8("唇角"),
        QString::fromUtf8("指尖"), QString::fromUtf8("声音"), QString::fromUtf8("脊背"),
        QString::fromUtf8("肩"), QString::fromUtf8("手"), QString::fromUtf8("臂"),
        QString::fromUtf8("腿"), QString::fromUtf8("脚"), QString::fromUtf8("赤足"),
        QString::fromUtf8("眼"), QString::fromUtf8("眉"), QString::fromUtf8("脸"),
        QString::fromUtf8("唇")
    };
    static const QStringList cameraTokens = {
        QString::fromUtf8("特写"), QString::fromUtf8("近景"), QString::fromUtf8("中景"),
        QString::fromUtf8("远景"), QString::fromUtf8("镜头"), QString::fromUtf8("视角"),
        QString::fromUtf8("入画"), QString::fromUtf8("构图")
    };
    static const QStringList genericTokens = {
        QString::fromUtf8("氛围"), QString::fromUtf8("配色"), QString::fromUtf8("布局"),
        QString::fromUtf8("环境"), QString::fromUtf8("场景"), QString::fromUtf8("画面"),
        QString::fromUtf8("空间"), QString::fromUtf8("整体"), QString::fromUtf8("感觉"),
        QString::fromUtf8("风格")
    };

    auto containsAny = [&normalized](const QStringList& tokens) {
        for (const QString& token : tokens) {
            if (!token.isEmpty() && normalized.contains(token)) return true;
        }
        return false;
    };

    return containsAny(humanTokens) || containsAny(dialogueTokens) ||
           containsAny(actionTokens) || containsAny(postureTokens) ||
           containsAny(cameraTokens) || containsAny(genericTokens);
}

QStringList filterStaticSceneList(const QStringList& items)
{
    QStringList filtered;
    for (const QString& item : items) {
        if (!isDynamicSceneText(item)) filtered << item.trimmed();
    }
    filtered.removeDuplicates();
    return filtered;
}

QString filterStaticSceneText(const QString& text)
{
    return isDynamicSceneText(text) ? QString() : text.trimmed();
}

QJsonArray toJsonArray(const QStringList& items)
{
    return QJsonArray::fromStringList(items);
}

} // namespace SceneTextFilter

// ==================== BibleUpdateStrategy ====================

namespace BibleUpdateStrategy {

QJsonObject resolveAppearanceObject(const QJsonObject& characterObj)
{
    QJsonObject appObj = characterObj["appearance"].toObject();
    if (appObj.isEmpty() && characterObj["appearance"].isString()) {
        QJsonParseError parseError;
        QJsonDocument appearanceDoc = QJsonDocument::fromJson(characterObj["appearance"].toString().toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && appearanceDoc.isObject())
            appObj = appearanceDoc.object();
    }
    if (appObj.isEmpty()) appObj = characterObj;
    return appObj;
}

QJsonObject resolveSceneDetailsObject(const QJsonObject& sceneObj)
{
    QJsonObject detailsObj = sceneObj["details"].toObject();
    if (detailsObj.isEmpty()) detailsObj = sceneObj;
    return detailsObj;
}

bool hasEmptyAppearanceFields(const CharacterAppearance& app)
{
    return app.gender.isEmpty() || app.age == 0 || app.hairColor.isEmpty() ||
           app.hairStyle.isEmpty() || app.eyeColor.isEmpty() || app.build.isEmpty() ||
           app.height.isEmpty() || app.clothing.isEmpty() || app.accessories.isEmpty() ||
           app.distinctiveFeatures.isEmpty() || app.aliases.isEmpty();
}

bool hasNewAppearanceData(const QJsonObject& appObj)
{
    return !appObj["gender"].toString().isEmpty() || appObj["age"].toInt() > 0 ||
           !appObj["hairColor"].toString().isEmpty() || !appObj["hairStyle"].toString().isEmpty() ||
           !appObj["eyeColor"].toString().isEmpty() || !appObj["build"].toString().isEmpty() ||
           !appObj["height"].toString().isEmpty() || !appObj["clothing"].toArray().isEmpty() ||
           !appObj["accessories"].toArray().isEmpty() || !appObj["distinctiveFeatures"].toArray().isEmpty() ||
           !appObj["aliases"].toArray().isEmpty();
}

bool hasCharacterAppearanceChanges(const CharacterAppearance& existing, const QJsonObject& appObj)
{
    auto scalarChanged = [&appObj](const QString& key, const QString& existingValue) {
        const QString incoming = appObj[key].toString().trimmed();
        return !incoming.isEmpty() && incoming != existingValue.trimmed();
    };
    auto intChanged = [&appObj](const QString& key, int existingValue) {
        const int incoming = appObj[key].toInt();
        return incoming > 0 && incoming != existingValue;
    };
    auto listChanged = [&appObj](const QString& key, const QStringList& existingValues) {
        const QStringList incoming = JsonUtils::jsonArrayToStringList(appObj[key].toArray());
        for (const QString& value : incoming) {
            if (!existingValues.contains(value)) return true;
        }
        return false;
    };
    return scalarChanged("gender", existing.gender) ||
           intChanged("age", existing.age) ||
           scalarChanged("hairColor", existing.hairColor) ||
           scalarChanged("hairStyle", existing.hairStyle) ||
           scalarChanged("eyeColor", existing.eyeColor) ||
           scalarChanged("build", existing.build) ||
           scalarChanged("height", existing.height) ||
           listChanged("clothing", existing.clothing) ||
           listChanged("accessories", existing.accessories) ||
           listChanged("distinctiveFeatures", existing.distinctiveFeatures) ||
           listChanged("aliases", existing.aliases);
}

bool isExplicitSource(const QString& source)
{
    return source.compare(QStringLiteral("explicit"), Qt::CaseInsensitive) == 0;
}

bool isInferredSource(const QString& source)
{
    return source.compare(QStringLiteral("inferred"), Qt::CaseInsensitive) == 0;
}

bool isManualSource(const QString& source)
{
    return source.compare(QStringLiteral("manual"), Qt::CaseInsensitive) == 0;
}

bool isLockedSource(const QString& source)
{
    return isManualSource(source) || isExplicitSource(source);
}

int sourcePriority(const QString& source)
{
    if (isManualSource(source)) return 3;
    if (isExplicitSource(source)) return 2;
    if (isInferredSource(source)) return 1;
    return 0;
}

int fieldSourcePolicyVersion()
{
    return 3;
}

QString fieldSource(const QJsonObject& sources, const QString& field)
{
    return sources.value(field).toString();
}

void setFieldSource(QJsonObject& sources, const QString& field, const QString& source)
{
    sources[field] = source;
}

QString normalizedSource(const QString& source, const QString& fallback)
{
    return source.isEmpty() ? fallback : source;
}

bool canReplaceSource(const QString& existingSource, const QString& incomingSource)
{
    return sourcePriority(existingSource) < sourcePriority(incomingSource);
}

void setUpdatedSource(QJsonObject& sources, const QString& field, const QString& incomingSource, const QString& fallback)
{
    setFieldSource(sources, field, normalizedSource(incomingSource, fallback));
}

void downgradeLegacyFieldSource(QJsonObject& sources, const QString& field)
{
    const QString current = fieldSource(sources, field);
    if (!current.isEmpty() && !isManualSource(current))
        setFieldSource(sources, field, QStringLiteral("inferred"));
}

bool shouldUpdateFromSource(const QString& existingSource, const QString& incomingSource)
{
    return !incomingSource.isEmpty() && sourcePriority(incomingSource) > sourcePriority(existingSource);
}

bool canAdoptFieldValue(bool hasExistingValue, const QString& existingSource, const QString& incomingSource)
{
    if (incomingSource.isEmpty() || isLockedSource(existingSource)) return false;
    return !hasExistingValue || shouldUpdateFromSource(existingSource, incomingSource);
}

bool canAdoptFieldValue(const QString& existingValue, const QString& existingSource, const QString& incomingSource)
{
    return canAdoptFieldValue(!existingValue.trimmed().isEmpty(), existingSource, incomingSource);
}

bool canAdoptFieldValue(int existingValue, const QString& existingSource, const QString& incomingSource)
{
    return canAdoptFieldValue(existingValue > 0, existingSource, incomingSource);
}

bool canAdoptFieldValue(const QStringList& existingValues, const QString& existingSource, const QString& incomingSource)
{
    return canAdoptFieldValue(!existingValues.isEmpty(), existingSource, incomingSource);
}

bool hasDirectTextEvidence(const QString& sourceText, const QString& value)
{
    const QString trimmedSource = sourceText.trimmed();
    const QString trimmedValue = value.trimmed();
    return !trimmedSource.isEmpty() && !trimmedValue.isEmpty() &&
           trimmedSource.contains(trimmedValue, Qt::CaseInsensitive);
}

bool hasDirectTextEvidence(const QString& sourceText, const QStringList& values)
{
    for (const QString& value : values) {
        if (hasDirectTextEvidence(sourceText, value)) return true;
    }
    return false;
}

QString sourceFromTextEvidence(const QString& sourceText, const QString& value, const QString& inferredValue)
{
    Q_UNUSED(inferredValue);
    if (hasDirectTextEvidence(sourceText, value)) return QStringLiteral("explicit");
    return QStringLiteral("inferred");
}

QString sourceFromTextEvidence(const QString& sourceText, const QStringList& values, const QStringList& inferredValues)
{
    Q_UNUSED(inferredValues);
    if (hasDirectTextEvidence(sourceText, values)) return QStringLiteral("explicit");
    return QStringLiteral("inferred");
}

bool hasCharacterPersonalityChanges(const QStringList& existing, const QJsonArray& incomingArray)
{
    const QStringList incoming = JsonUtils::jsonArrayToStringList(incomingArray);
    for (const QString& value : incoming) {
        if (!existing.contains(value)) return true;
    }
    return false;
}

bool hasEmptyDetailsFields(const SceneDetails& det)
{
    const bool indoorScene = isIndoorSceneType(det.type);
    return det.description.isEmpty() || det.building.isEmpty() || det.color.isEmpty() ||
           det.landmark.isEmpty() || det.layout.isEmpty() || det.atmosphere.isEmpty() ||
           det.anchorPoints.isEmpty() || det.signatureObjects.isEmpty() ||
           det.fixedColorBlocks.isEmpty() || det.consistencyRules.isEmpty() ||
           det.type.isEmpty() || det.typeZh.isEmpty() || det.setting.isEmpty() || det.timeOfDay.isEmpty() ||
           det.currentInterpretation.isEmpty() || det.confidence.isEmpty() ||
           det.status.isEmpty() || det.narrativeRoleZh.isEmpty() || det.evidence.isEmpty() || det.aliases.isEmpty() ||
           det.history.isEmpty() ||
           (!indoorScene && det.weather.isEmpty());
}

bool hasNewDetailsData(const QJsonObject& detailsObj)
{
    QStringList keys = {"description", "building", "color", "landmark", "layout",
                        "atmosphere", "type", "typeZh", "setting", "timeOfDay", "weather", "spaceSize",
                        "currentInterpretation", "confidence", "status", "narrativeRole", "narrativeRoleZh"};
    for (const QString& key : keys) {
        if (!detailsObj[key].toString().isEmpty()) return true;
    }
    QStringList arrayKeys = {"anchorPoints", "signatureObjects", "fixedColorBlocks",
                             "consistencyRules", "evidence", "aliases", "history"};
    for (const QString& key : arrayKeys) {
        if (!detailsObj[key].toArray().isEmpty()) return true;
    }
    return false;
}

bool hasIterationUpdates(const SceneDetails& existing, const QJsonObject& detailsObj)
{
    auto scalarChanged = [&detailsObj](const QString& key, const QString& existingValue) {
        QString incoming = detailsObj[key].toString().trimmed();
        return !incoming.isEmpty() && incoming != existingValue.trimmed();
    };
    auto listChanged = [&detailsObj](const QString& key, const QStringList& existingValues) {
        QStringList incoming = JsonUtils::jsonArrayToStringList(detailsObj[key].toArray());
        for (const QString& value : incoming) {
            if (!existingValues.contains(value)) return true;
        }
        return false;
    };
    return scalarChanged("currentInterpretation", existing.currentInterpretation) ||
           scalarChanged("confidence", existing.confidence) ||
           scalarChanged("status", existing.status) ||
           listChanged("evidence", existing.evidence) ||
           listChanged("aliases", existing.aliases) ||
           listChanged("history", existing.history);
}

void updateCharacterAppearance(CharacterAppearance& app, const QJsonObject& appObj)
{
    const QJsonObject incomingSources = appObj.value("fieldSources").toObject();
    auto updateScalar = [&appObj, &app, &incomingSources](QString& field, const QString& key) {
        const QString incoming = appObj[key].toString().trimmed();
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(app.fieldSources, key);
        if (!incoming.isEmpty() && canAdoptFieldValue(field, existingSource, incomingSource)) {
            field = incoming;
            setFieldSource(app.fieldSources, key, normalizedSource(incomingSource, QStringLiteral("explicit")));
        }
    };
    auto updateInt = [&appObj, &app, &incomingSources](int& field, const QString& key) {
        const int incoming = appObj[key].toInt();
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(app.fieldSources, key);
        if (incoming > 0 && canAdoptFieldValue(field, existingSource, incomingSource)) {
            field = incoming;
            setFieldSource(app.fieldSources, key, normalizedSource(incomingSource, QStringLiteral("explicit")));
        }
    };
    auto updateList = [&appObj, &app, &incomingSources](QStringList& field, const QString& key) {
        const QStringList incoming = JsonUtils::jsonArrayToStringList(appObj[key].toArray());
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(app.fieldSources, key);
        if (incoming.isEmpty()) return;
        if (!canAdoptFieldValue(field, existingSource, incomingSource)) return;
        field = (field.isEmpty() || isExplicitSource(incomingSource) || isManualSource(incomingSource))
            ? incoming
            : mergeStringLists(field, incoming);
        setFieldSource(app.fieldSources, key, normalizedSource(incomingSource, QStringLiteral("inferred")));
    };
    updateScalar(app.gender, "gender");
    updateInt(app.age, "age");
    updateScalar(app.hairColor, "hairColor");
    updateScalar(app.hairStyle, "hairStyle");
    updateScalar(app.eyeColor, "eyeColor");
    updateScalar(app.build, "build");
    updateScalar(app.height, "height");
    updateList(app.clothing, "clothing");
    updateList(app.accessories, "accessories");
    updateList(app.distinctiveFeatures, "distinctiveFeatures");
    updateList(app.aliases, "aliases");
}

void updateSceneDetails(SceneDetails& det, const QJsonObject& detailsObj)
{
    const bool indoorScene = isIndoorSceneType(detailsObj["type"].toString()) ||
                             isIndoorSceneType(det.type);
    const QJsonObject incomingSources = detailsObj.value("fieldSources").toObject();
    auto updateField = [&detailsObj, &det, &incomingSources](const QString& key, QString& field) {
        QString value = detailsObj[key].toString().trimmed();
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(det.fieldSources, key);
        if (!value.isEmpty() && canAdoptFieldValue(field, existingSource, incomingSource)) {
            field = value;
            setFieldSource(det.fieldSources, key, normalizedSource(incomingSource, QStringLiteral("explicit")));
        }
    };
    auto updateListField = [&detailsObj, &det, &incomingSources](const QString& key, QStringList& field) {
        QStringList values = JsonUtils::jsonArrayToStringList(detailsObj[key].toArray());
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(det.fieldSources, key);
        if (values.isEmpty()) return;
        if (!canAdoptFieldValue(field, existingSource, incomingSource)) return;
        field = (field.isEmpty() || isExplicitSource(incomingSource) || isManualSource(incomingSource))
            ? values
            : mergeStringLists(field, values);
        setFieldSource(det.fieldSources, key, normalizedSource(incomingSource, QStringLiteral("inferred")));
    };
    updateField("description", det.description);
    updateField("building", det.building);
    updateField("color", det.color);
    updateField("landmark", det.landmark);
    updateField("layout", det.layout);
    updateField("atmosphere", det.atmosphere);
    updateListField("anchorPoints", det.anchorPoints);
    updateListField("signatureObjects", det.signatureObjects);
    updateListField("fixedColorBlocks", det.fixedColorBlocks);
    updateListField("consistencyRules", det.consistencyRules);
    updateField("type", det.type);
    updateField("typeZh", det.typeZh);
    updateField("setting", det.setting);
    updateField("timeOfDay", det.timeOfDay);
    updateField("spaceSize", det.spaceSize);
    if (indoorScene) {
        det.weather.clear();
        det.weatherVariations = QJsonArray();
    } else {
        updateField("weather", det.weather);
    }
    updateField("currentInterpretation", det.currentInterpretation);
    updateField("confidence", det.confidence);
    updateField("status", det.status);
    updateField("narrativeRole", det.narrativeRole);
    updateField("narrativeRoleZh", det.narrativeRoleZh);
    updateListField("evidence", det.evidence);
    updateListField("aliases", det.aliases);
    updateListField("history", det.history);
}

} // namespace BibleUpdateStrategy

// ==================== Utils ====================

namespace Utils {

QString generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace Utils

} // namespace BibleUtils