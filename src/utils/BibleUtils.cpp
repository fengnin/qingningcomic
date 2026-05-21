#include "utils/BibleUtils.h"

namespace BibleUtils {

QJsonObject mergeObjectPreserve(const QJsonObject& base, const QJsonObject& incoming)
{
    QJsonObject result = base;

    for (auto it = incoming.constBegin(); it != incoming.constEnd(); ++it) {
        const QString& key = it.key();
        const QJsonValue& value = it.value();

        if (value.isNull() || value.isUndefined()) {
            continue;
        }

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
            if (!key.isEmpty()) {
                index[key] = obj;
            }
        }
    }

    return result;
}

namespace Inference {

bool containsAny(const QString& text, const QStringList& tokens)
{
    for (const QString& token : tokens) {
        if (!token.isEmpty() && text.contains(token)) {
            return true;
        }
    }
    return false;
}

bool isIndoorSceneType(const QString& type)
{
    return type == QLatin1String("indoor") || type == QLatin1String("indoor-outdoor");
}

QString inferSceneType(const QString& name, const QString& description)
{
    const QString combined = (name + QLatin1Char(' ') + description).toLower();
    static const QStringList indoorKeywords = {
        QString::fromUtf8("房间"), QString::fromUtf8("卧室"), QString::fromUtf8("教室"),
        QString::fromUtf8("办公室"), QString::fromUtf8("客厅"), QString::fromUtf8("堂屋"),
        QString::fromUtf8("厨房"), QString::fromUtf8("书房"), QString::fromUtf8("门内"),
        QString::fromUtf8("屋内"), QString::fromUtf8("室内"), QString::fromUtf8("老宅")
    };
    static const QStringList outdoorKeywords = {
        QString::fromUtf8("街道"), QString::fromUtf8("古街"), QString::fromUtf8("广场"),
        QString::fromUtf8("公园"), QString::fromUtf8("野外"), QString::fromUtf8("室外"),
        QString::fromUtf8("户外"), QString::fromUtf8("巷"), QString::fromUtf8("路")
    };
    static const QStringList transitionalKeywords = {
        QString::fromUtf8("门口"), QString::fromUtf8("门槛"), QString::fromUtf8("门边"),
        QString::fromUtf8("门外"), QString::fromUtf8("门内"), QString::fromUtf8("入口"),
        QString::fromUtf8("出口"), QString::fromUtf8("木门"), QString::fromUtf8("门扉")
    };

    if (containsAny(combined, transitionalKeywords) &&
        (containsAny(combined, indoorKeywords) || containsAny(combined, outdoorKeywords))) {
        return QStringLiteral("indoor-outdoor");
    }
    if (containsAny(combined, indoorKeywords)) return QStringLiteral("indoor");
    if (containsAny(combined, outdoorKeywords)) return QStringLiteral("outdoor");
    if (combined.contains(QString::fromUtf8("城市")) || combined.contains(QString::fromUtf8("商场")) ||
        combined.contains(QString::fromUtf8("地铁")) || combined.contains(QString::fromUtf8("车站"))) {
        return QStringLiteral("urban");
    }
    if (combined.contains(QString::fromUtf8("乡村")) || combined.contains(QString::fromUtf8("农田")) ||
        combined.contains(QString::fromUtf8("村庄"))) {
        return QStringLiteral("rural");
    }
    if (combined.contains(QString::fromUtf8("森林")) || combined.contains(QString::fromUtf8("山")) ||
        combined.contains(QString::fromUtf8("海")) || combined.contains(QString::fromUtf8("河"))) {
        return QStringLiteral("nature");
    }
    return QStringLiteral("outdoor");
}

QString inferSetting(const QString& name, const QString& description)
{
    const QString combined = (name + QLatin1Char(' ') + description).toLower();
    static const QStringList ancientKeywords = {
        QString::fromUtf8("古代"), QString::fromUtf8("古城"), QString::fromUtf8("古街"),
        QString::fromUtf8("古镇"), QString::fromUtf8("戏袍"), QString::fromUtf8("戏服"),
        QString::fromUtf8("煤油灯"), QString::fromUtf8("老宅"), QString::fromUtf8("旧宅"),
        QString::fromUtf8("堂屋"), QString::fromUtf8("土灶"), QString::fromUtf8("木门"),
        QString::fromUtf8("纸糊窗"), QString::fromUtf8("青石板"), QString::fromUtf8("传统民居"),
        QString::fromUtf8("祠堂"), QString::fromUtf8("门楼"), QString::fromUtf8("瓦房")
    };

    if (combined.contains(QString::fromUtf8("现代")) || combined.contains(QString::fromUtf8("商场")) ||
        combined.contains(QString::fromUtf8("地铁")) || combined.contains(QString::fromUtf8("高楼"))) {
        return QStringLiteral("modern");
    }
    if (containsAny(combined, ancientKeywords) || combined.contains(QString::fromUtf8("宫殿")) ||
        combined.contains(QString::fromUtf8("朝代"))) {
        return QStringLiteral("ancient");
    }
    if (combined.contains(QString::fromUtf8("未来")) || combined.contains(QString::fromUtf8("太空")) ||
        combined.contains(QString::fromUtf8("飞船"))) {
        return QStringLiteral("future");
    }
    if (combined.contains(QString::fromUtf8("奇幻")) || combined.contains(QString::fromUtf8("魔法"))) {
        return QStringLiteral("fantasy");
    }
    return QString();
}

QString inferTimeOfDay(const QString& name, const QString& description)
{
    const QString combined = (name + QLatin1Char(' ') + description).toLower();

    if (combined.contains(QString::fromUtf8("黑夜")) || combined.contains(QString::fromUtf8("夜幕")) ||
        combined.contains(QString::fromUtf8("夜色")) || combined.contains(QString::fromUtf8("夜雨")) ||
        combined.contains(QString::fromUtf8("雷雨")) || combined.contains(QString::fromUtf8("深夜")) ||
        (combined.contains(QString::fromUtf8("灯火")) && combined.contains(QString::fromUtf8("雨")))) {
        return QStringLiteral("night");
    }
    if (combined.contains(QString::fromUtf8("黎明")) || combined.contains(QString::fromUtf8("拂晓"))) return QStringLiteral("dawn");
    if (combined.contains(QString::fromUtf8("早晨")) || combined.contains(QString::fromUtf8("清晨"))) return QStringLiteral("morning");
    if (combined.contains(QString::fromUtf8("正午")) || combined.contains(QString::fromUtf8("中午"))) return QStringLiteral("noon");
    if (combined.contains(QString::fromUtf8("午后")) || combined.contains(QString::fromUtf8("下午"))) return QStringLiteral("afternoon");
    if (combined.contains(QString::fromUtf8("黄昏")) || combined.contains(QString::fromUtf8("傍晚"))) return QStringLiteral("dusk");
    if (combined.contains(QString::fromUtf8("夜晚")) || combined.contains(QString::fromUtf8("深夜")) ||
        combined.contains(QString::fromUtf8("夜里"))) return QStringLiteral("night");
    if (combined.contains(QString::fromUtf8("午夜"))) return QStringLiteral("midnight");
    if (combined.contains(QString::fromUtf8("煤油灯")) || combined.contains(QString::fromUtf8("灯火"))) {
        return QStringLiteral("night");
    }
    return QString();
}

QString inferWeather(const QString& name, const QString& description)
{
    const QString combined = (name + QLatin1Char(' ') + description).toLower();

    if (combined.contains(QString::fromUtf8("暴雨")) || combined.contains(QString::fromUtf8("大雨")) ||
        combined.contains(QString::fromUtf8("雷雨")) || combined.contains(QString::fromUtf8("雨幕"))) return QStringLiteral("rainy");
    if (combined.contains(QString::fromUtf8("雨"))) return QStringLiteral("rainy");
    if (combined.contains(QString::fromUtf8("雪"))) return QStringLiteral("snowy");
    if (combined.contains(QString::fromUtf8("雾"))) return QStringLiteral("foggy");
    if (combined.contains(QString::fromUtf8("暴风")) || combined.contains(QString::fromUtf8("风暴"))) return QStringLiteral("stormy");
    if (combined.contains(QString::fromUtf8("多云")) || combined.contains(QString::fromUtf8("阴"))) return QStringLiteral("cloudy");
    return QString();
}

QString inferBuilding(const QString& setting)
{
    if (setting == QLatin1String("ancient")) return QString::fromUtf8("传统民居或古旧木构建筑");
    if (setting == QLatin1String("future")) return QString::fromUtf8("未来感建筑");
    if (setting == QLatin1String("fantasy")) return QString::fromUtf8("奇幻建筑");
    return QString();
}

QString inferColorScheme(const QString& timeOfDay, const QString& weather)
{
    if (timeOfDay.isEmpty() && weather.isEmpty()) {
        return QString();
    }
    if (timeOfDay == QLatin1String("night") || timeOfDay == QLatin1String("midnight")) {
        if (weather == QLatin1String("rainy") || weather == QLatin1String("foggy"))
            return QString::fromUtf8("冷灰主调 + 朱红点色 + 灯火暖黄");
        return QString::fromUtf8("深色主调 + 局部暖光");
    }
    if (weather == QLatin1String("sunny")) return QString::fromUtf8("暖色调 + 高光");
    if (weather == QLatin1String("rainy") || weather == QLatin1String("foggy"))
        return QString::fromUtf8("灰蓝主调 + 低饱和暖点");
    return QString();
}

QString inferAtmosphere(const QString& timeOfDay, const QString& type)
{
    if (timeOfDay == QLatin1String("night")) return QString::fromUtf8("压抑、紧张");
    if (type == QLatin1String("indoor")) return QString::fromUtf8("封闭、压迫");
    return QString();
}

QString inferLayout(const QString& type)
{
    if (type == QLatin1String("indoor")) return QString::fromUtf8("封闭式室内空间");
    if (type == QLatin1String("indoor-outdoor")) return QString::fromUtf8("门口/门廊过渡空间");
    if (type == QLatin1String("outdoor")) return QString::fromUtf8("开放式街巷空间");
    return QString::fromUtf8("混合空间");
}

QString inferSpaceSize(const QString& name, const QString& description)
{
    const QString combined = (name + QLatin1Char(' ') + description).toLower();

    if (combined.contains(QString::fromUtf8("狭小")) || combined.contains(QString::fromUtf8("狭窄")) ||
        combined.contains(QString::fromUtf8("小房间"))) return QString::fromUtf8("狭小");
    if (combined.contains(QString::fromUtf8("宽敞")) || combined.contains(QString::fromUtf8("大厅")) ||
        combined.contains(QString::fromUtf8("广场"))) return QString::fromUtf8("宽敞");
    if (combined.contains(QString::fromUtf8("巨大")) || combined.contains(QString::fromUtf8("广阔")) ||
        combined.contains(QString::fromUtf8("无边"))) return QString::fromUtf8("巨大");
    return QString();
}

QString inferNarrativeRole(const QString& name, const QString& description)
{
    const QString combined = (name + QLatin1Char(' ') + description).toLower();

    if (combined.contains(QString::fromUtf8("开场")) || combined.contains(QString::fromUtf8("起始")) ||
        combined.contains(QString::fromUtf8("第一章")) || combined.contains(QString::fromUtf8("主要")) ||
        combined.contains(QString::fromUtf8("重要")) || combined.contains(QString::fromUtf8("核心"))) return QStringLiteral("primary");
    if (combined.contains(QString::fromUtf8("背景")) || combined.contains(QString::fromUtf8("次要"))) return QStringLiteral("background");
    return QString();
}

int inferAge(const QString& role, const QString& name)
{
    const QString combined = (role + QLatin1Char(' ') + name).toLower();

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
        QString::fromUtf8("少年"), QString::fromUtf8("学生"),
        QString::fromUtf8("男孩"), QString::fromUtf8("女孩")
    };
    if (containsAny(combined, teenTokens)) return 16;

    static const QStringList childTokens = {
        QString::fromUtf8("儿童"), QString::fromUtf8("孩子")
    };
    if (containsAny(combined, childTokens)) return 8;

    return 0;
}

namespace Translation {

QString translateSceneType(const QString& type)
{
    static const QMap<QString, QString> map = {
        {"indoor",  QString::fromUtf8("室内")},
        {"outdoor", QString::fromUtf8("室外")},
        {"urban",   QString::fromUtf8("城市")},
        {"rural",   QString::fromUtf8("乡村")},
        {"nature",  QString::fromUtf8("自然")},
        {"fantasy", QString::fromUtf8("奇幻")},
        {"scifi",   QString::fromUtf8("科幻")}
    };
    auto it = map.find(type);
    return (it != map.end()) ? it.value() : type;
}

QString translateTimeOfDay(const QString& timeOfDay)
{
    static const QMap<QString, QString> map = {
        {"dawn",      QString::fromUtf8("黎明")},
        {"morning",   QString::fromUtf8("早晨")},
        {"noon",      QString::fromUtf8("中午")},
        {"afternoon", QString::fromUtf8("下午")},
        {"dusk",      QString::fromUtf8("傍晚")},
        {"evening",   QString::fromUtf8("傍晚")},
        {"night",     QString::fromUtf8("夜晚")},
        {"midnight",  QString::fromUtf8("午夜")},
        {"day",       QString::fromUtf8("白天")}
    };
    auto it = map.find(timeOfDay);
    return (it != map.end()) ? it.value() : timeOfDay;
}

QString translateWeather(const QString& weather)
{
    static const QMap<QString, QString> map = {
        {"sunny",    QString::fromUtf8("晴朗")},
        {"clear",    QString::fromUtf8("晴朗")},
        {"cloudy",   QString::fromUtf8("多云")},
        {"overcast", QString::fromUtf8("阴天")},
        {"rainy",    QString::fromUtf8("雨天")},
        {"snowy",    QString::fromUtf8("雪天")},
        {"foggy",    QString::fromUtf8("雾天")},
        {"stormy",   QString::fromUtf8("暴风")}
    };
    auto it = map.find(weather);
    return (it != map.end()) ? it.value() : weather;
}

QString translateNarrativeRole(const QString& role)
{
    static const QMap<QString, QString> map = {
        {"primary-setting",   QString::fromUtf8("主要场景")},
        {"primary",           QString::fromUtf8("主要场景")},
        {"secondary",         QString::fromUtf8("次要场景")},
        {"secondary-setting", QString::fromUtf8("次要场景")},
        {"background",        QString::fromUtf8("背景场景")},
        {"transitional",      QString::fromUtf8("过渡场景")},
        {"symbolic",          QString::fromUtf8("象征场景")}
    };
    auto it = map.find(role);
    return (it != map.end()) ? it.value() : role;
}

} // namespace Translation

} // namespace Inference

namespace SceneTextFilter {

bool isDynamicSceneText(const QString& text)
{
    const QString normalized = text.trimmed();
    if (normalized.isEmpty()) {
        return true;
    }

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

    auto containsAnyLocal = [&normalized](const QStringList& tokens) {
        for (const QString& token : tokens) {
            if (!token.isEmpty() && normalized.contains(token)) {
                return true;
            }
        }
        return false;
    };

    return containsAnyLocal(humanTokens) ||
           containsAnyLocal(dialogueTokens) ||
           containsAnyLocal(actionTokens) ||
           containsAnyLocal(postureTokens) ||
           containsAnyLocal(cameraTokens) ||
           containsAnyLocal(genericTokens);
}

QStringList filterStaticSceneList(const QStringList& items)
{
    QStringList filtered;
    for (const QString& item : items) {
        if (!isDynamicSceneText(item)) {
            filtered << item.trimmed();
        }
    }
    filtered.removeDuplicates();
    return filtered;
}

QString filterStaticSceneText(const QString& text)
{
    return isDynamicSceneText(text) ? QString() : text.trimmed();
}

} // namespace SceneTextFilter

QJsonObject mergeSpatialLayout(const QJsonObject& base, const QJsonObject& incoming)
{
    QJsonObject result = base;

    for (auto it = incoming.constBegin(); it != incoming.constEnd(); ++it) {
        const QString& key = it.key();
        const QJsonValue& value = it.value();

        if (value.isNull()) continue;

        if (key == QLatin1String("keyAreas") && value.isArray()) {
            result[key] = mergeObjectArray(result[key].toArray(), value.toArray(), QStringLiteral("name"));
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

} // namespace BibleUtils
