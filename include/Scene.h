#ifndef SCENE_H
#define SCENE_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QHash>
#include <QMap>
#include "utils/JsonUtils.h"

namespace SceneJsonParser {
    struct ParsedVisualData {
        QString building;
        QString color;
        QString atmosphere;
        QString landmark;
        QString layout;
        QString spaceSize;
        QJsonArray keyAreas;
        QJsonArray timeVariations;
        QJsonArray weatherVariations;
    };
    
    inline ParsedVisualData parseVisualCharacteristics(const QJsonObject& json)
    {
        ParsedVisualData data;
        
        QJsonObject visualChars = json["visualCharacteristics"].toObject();
        if (!visualChars.isEmpty()) {
            data.building = visualChars["architecture"].toString();
            data.color = visualChars["colorScheme"].toString();
            data.atmosphere = visualChars["atmosphere"].toString();
            
            QJsonArray landmarksArray = visualChars["keyLandmarks"].toArray();
            if (!landmarksArray.isEmpty()) {
                QStringList landmarks;
                for (const QJsonValue& v : landmarksArray) {
                    landmarks << v.toString();
                }
                data.landmark = landmarks.join(", ");
            }
        }
        
        QJsonObject spatialLayout = json["spatialLayout"].toObject();
        if (!spatialLayout.isEmpty()) {
            data.layout = spatialLayout["layout"].toString();
            data.spaceSize = spatialLayout["size"].toString();
            data.keyAreas = spatialLayout["keyAreas"].toArray();
        }
        
        data.timeVariations = json["timeVariations"].toArray();
        data.weatherVariations = json["weatherVariations"].toArray();
        
        return data;
    }
    
    inline void writeVisualCharacteristics(QJsonObject& json, 
                                            const QString& building,
                                            const QString& color,
                                            const QString& atmosphere,
                                            const QString& landmark,
                                            const QString& layout,
                                            const QString& spaceSize = QString(),
                                            const QJsonArray& keyAreas = QJsonArray(),
                                            const QJsonArray& timeVariations = QJsonArray(),
                                            const QJsonArray& weatherVariations = QJsonArray())
    {
        QJsonObject visualChars;
        if (!building.isEmpty()) {
            visualChars["architecture"] = building;
        }
        if (!color.isEmpty()) {
            visualChars["colorScheme"] = color;
        }
        if (!atmosphere.isEmpty()) {
            visualChars["atmosphere"] = atmosphere;
        }
        if (!landmark.isEmpty()) {
            QJsonArray landmarks;
            for (const QString& l : landmark.split(", ")) {
                if (!l.isEmpty()) {
                    landmarks << l;
                }
            }
            if (!landmarks.isEmpty()) {
                visualChars["keyLandmarks"] = landmarks;
            }
        }
        if (!visualChars.isEmpty()) {
            json["visualCharacteristics"] = visualChars;
        }
        
        if (!layout.isEmpty() || !spaceSize.isEmpty() || !keyAreas.isEmpty()) {
            QJsonObject spatialLayout;
            if (!layout.isEmpty()) {
                spatialLayout["layout"] = layout;
            }
            if (!spaceSize.isEmpty()) {
                spatialLayout["size"] = spaceSize;
            }
            if (!keyAreas.isEmpty()) {
                spatialLayout["keyAreas"] = keyAreas;
            }
            json["spatialLayout"] = spatialLayout;
        }
        
        if (!timeVariations.isEmpty()) {
            json["timeVariations"] = timeVariations;
        }
        if (!weatherVariations.isEmpty()) {
            json["weatherVariations"] = weatherVariations;
        }
    }
}

struct SceneDetails
{
    QString description;
    QString building;
    QString color;
    QString landmark;
    QString layout;
    QString atmosphere;
    QString type;
    QString setting;
    QString timeOfDay;
    QString weather;
    QString spaceSize;
    QString narrativeRole;
    QStringList details;
    
    QJsonObject visualCharacteristics;
    QJsonObject spatialLayout;
    QJsonArray timeVariations;
    QJsonArray weatherVariations;
    
    QJsonObject toJson() const
    {
        QJsonObject json;
        if (!description.isEmpty()) json["description"] = description;
        if (!building.isEmpty()) json["building"] = building;
        if (!color.isEmpty()) json["color"] = color;
        if (!landmark.isEmpty()) json["landmark"] = landmark;
        if (!layout.isEmpty()) json["layout"] = layout;
        if (!atmosphere.isEmpty()) json["atmosphere"] = atmosphere;
        if (!type.isEmpty()) json["type"] = type;
        if (!setting.isEmpty()) json["setting"] = setting;
        if (!timeOfDay.isEmpty()) json["timeOfDay"] = timeOfDay;
        if (!weather.isEmpty()) json["weather"] = weather;
        if (!spaceSize.isEmpty()) json["spaceSize"] = spaceSize;
        if (!narrativeRole.isEmpty()) json["narrativeRole"] = narrativeRole;
        if (!details.isEmpty()) json["details"] = QJsonArray::fromStringList(details);
        
        if (!visualCharacteristics.isEmpty()) {
            json["visualCharacteristics"] = visualCharacteristics;
        }
        if (!spatialLayout.isEmpty()) {
            json["spatialLayout"] = spatialLayout;
        }
        if (!timeVariations.isEmpty()) {
            json["timeVariations"] = timeVariations;
        }
        if (!weatherVariations.isEmpty()) {
            json["weatherVariations"] = weatherVariations;
        }
        
        return json;
    }
    
    static SceneDetails fromJson(const QJsonObject& json)
    {
        SceneDetails det;
        det.description = json["description"].toString();
        det.building = json["building"].toString();
        det.color = json["color"].toString();
        det.landmark = json["landmark"].toString();
        det.layout = json["layout"].toString();
        det.atmosphere = json["atmosphere"].toString();
        det.type = json["type"].toString();
        det.setting = json["setting"].toString();
        det.timeOfDay = json["timeOfDay"].toString();
        det.weather = json["weather"].toString();
        det.spaceSize = json["spaceSize"].toString();
        det.narrativeRole = json["narrativeRole"].toString();
        
        QJsonArray detailsArray = json["details"].toArray();
        for (const QJsonValue& v : detailsArray) {
            det.details << v.toString();
        }
        
        det.visualCharacteristics = json["visualCharacteristics"].toObject();
        det.spatialLayout = json["spatialLayout"].toObject();
        det.timeVariations = json["timeVariations"].toArray();
        det.weatherVariations = json["weatherVariations"].toArray();
        
        return det;
    }
    
    QStringList toDisplayStrings() const
    {
        QStringList lines;
        
        auto addLine = [&lines](const QString& key, const QString& value) {
            if (!value.isEmpty()) {
                lines << QString::fromUtf8("%1\uff1a%2").arg(key, value);
            }
        };
        
        // 枚举值转换：英文 -> 中文
        auto translateType = [](const QString& v) -> QString {
            static const QMap<QString, QString> map = {
                {"indoor", QString::fromUtf8("\u5ba4\u5185")},
                {"outdoor", QString::fromUtf8("\u5ba4\u5916")},
                {"urban", QString::fromUtf8("\u57ce\u5e02")},
                {"rural", QString::fromUtf8("\u4e61\u6751")},
                {"nature", QString::fromUtf8("\u81ea\u7136")},
                {"fantasy", QString::fromUtf8("\u5947\u5e7b")},
                {"historical", QString::fromUtf8("\u5386\u53f2")}
            };
            return map.value(v.toLower(), v);
        };
        
        auto translateSetting = [](const QString& v) -> QString {
            static const QMap<QString, QString> map = {
                {"modern", QString::fromUtf8("\u73b0\u4ee3")},
                {"ancient", QString::fromUtf8("\u53e4\u4ee3")},
                {"medieval", QString::fromUtf8("\u4e2d\u4e16\u7eaa")},
                {"future", QString::fromUtf8("\u672a\u6765")},
                {"contemporary", QString::fromUtf8("\u5f53\u4ee3")},
                {"fantasy", QString::fromUtf8("\u5947\u5e7b")},
                {"scifi", QString::fromUtf8("\u79d1\u5e7b")},
                {"post-apocalyptic", QString::fromUtf8("\u672b\u4e16")}
            };
            return map.value(v.toLower(), v);
        };
        
        auto translateTimeOfDay = [](const QString& v) -> QString {
            static const QMap<QString, QString> map = {
                {"dawn", QString::fromUtf8("\u9ece\u660e")},
                {"morning", QString::fromUtf8("\u65e9\u6668")},
                {"noon", QString::fromUtf8("\u6b63\u5348")},
                {"afternoon", QString::fromUtf8("\u4e0b\u5348")},
                {"dusk", QString::fromUtf8("\u9ec4\u660f")},
                {"evening", QString::fromUtf8("\u508d\u665a")},
                {"night", QString::fromUtf8("\u591c\u665a")},
                {"midnight", QString::fromUtf8("\u5348\u591c")}
            };
            return map.value(v.toLower(), v);
        };
        
        auto translateWeather = [](const QString& v) -> QString {
            static const QMap<QString, QString> map = {
                {"sunny", QString::fromUtf8("\u6674\u6717")},
                {"cloudy", QString::fromUtf8("\u591a\u4e91")},
                {"rainy", QString::fromUtf8("\u96e8\u5929")},
                {"snowy", QString::fromUtf8("\u96ea\u5929")},
                {"foggy", QString::fromUtf8("\u96fe\u5929")},
                {"stormy", QString::fromUtf8("\u66b4\u98ce\u96e8")},
                {"windy", QString::fromUtf8("\u5927\u98ce")}
            };
            return map.value(v.toLower(), v);
        };
        
        auto translateNarrativeRole = [](const QString& v) -> QString {
            static const QMap<QString, QString> map = {
                {"primary", QString::fromUtf8("\u4e3b\u8981\u573a\u666f")},
                {"secondary", QString::fromUtf8("\u8fc7\u6e21\u573a\u666f")},
                {"background", QString::fromUtf8("\u80cc\u666f\u573a\u666f")}
            };
            if (v.contains("-", Qt::CaseInsensitive)) {
                QString result = v;
                for (auto it = map.begin(); it != map.end(); ++it) {
                    result.replace(it.key(), it.value(), Qt::CaseInsensitive);
                }
                return result;
            }
            return map.value(v.toLower(), v);
        };
        
        addLine(QString::fromUtf8("\u573a\u666f\u63cf\u8ff0"), description);
        addLine(QString::fromUtf8("\u5efa\u7b51"), building);
        addLine(QString::fromUtf8("\u8272\u8c03"), color);
        addLine(QString::fromUtf8("\u5730\u6807"), landmark);
        addLine(QString::fromUtf8("\u5e03\u5c40"), layout);
        addLine(QString::fromUtf8("\u6c1b\u56f4"), atmosphere);
        addLine(QString::fromUtf8("\u7a7a\u95f4\u5c3a\u5bf8"), spaceSize);
        
        if (!type.isEmpty()) {
            addLine(QString::fromUtf8("\u7c7b\u578b"), translateType(type));
        }
        if (!setting.isEmpty()) {
            addLine(QString::fromUtf8("\u80cc\u666f"), translateSetting(setting));
        }
        if (!timeOfDay.isEmpty()) {
            addLine(QString::fromUtf8("\u65f6\u6bb5"), translateTimeOfDay(timeOfDay));
        }
        if (!weather.isEmpty()) {
            addLine(QString::fromUtf8("\u5929\u6c14"), translateWeather(weather));
        }
        if (!narrativeRole.isEmpty()) {
            addLine(QString::fromUtf8("\u53d9\u4e8b\u89d2\u8272"), translateNarrativeRole(narrativeRole));
        }
        
        if (!timeVariations.isEmpty()) {
            QStringList tvDescs;
            for (const auto& tvVal : timeVariations) {
                QJsonObject tv = tvVal.toObject();
                QString tod = translateTimeOfDay(tv["timeOfDay"].toString());
                QString desc = tv["description"].toString();
                if (!tod.isEmpty()) {
                    if (desc.isEmpty()) {
                        tvDescs << tod;
                    } else {
                        tvDescs << QString("%1：%2").arg(tod, desc);
                    }
                }
            }
            if (!tvDescs.isEmpty()) {
                addLine(QString::fromUtf8("\u65f6\u95f4\u53d8\u5316"), tvDescs.join(" | "));
            }
        }
        
        if (!weatherVariations.isEmpty()) {
            QStringList wvDescs;
            for (const auto& wvVal : weatherVariations) {
                QJsonObject wv = wvVal.toObject();
                QString w = translateWeather(wv["weather"].toString());
                QString desc = wv["description"].toString();
                if (!w.isEmpty()) {
                    if (desc.isEmpty()) {
                        wvDescs << w;
                    } else {
                        wvDescs << QString("%1：%2").arg(w, desc);
                    }
                }
            }
            if (!wvDescs.isEmpty()) {
                addLine(QString::fromUtf8("\u5929\u6c14\u53d8\u5316"), wvDescs.join(" | "));
            }
        }
        
        return lines;
    }
};

class Scene
{
public:
    Scene() = default;
    
    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }
    
    QString novelId() const { return m_novelId; }
    void setNovelId(const QString& novelId) { m_novelId = novelId; }
    
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    QString sceneId() const { return m_sceneId; }
    void setSceneId(const QString& sceneId) { m_sceneId = sceneId; }
    
    SceneDetails details() const { return m_details; }
    void setDetails(const SceneDetails& details) { m_details = details; }
    
    QStringList tags() const { return m_tags; }
    void setTags(const QStringList& tags) { m_tags = tags; }
    
    QString referenceImagePath() const { return m_referenceImagePath; }
    void setReferenceImagePath(const QString& path) { m_referenceImagePath = path; }
    
    QJsonObject toJson() const
    {
        QJsonObject json;
        json["id"] = m_id;
        json["novelId"] = m_novelId;
        json["name"] = m_name;
        json["sceneId"] = m_sceneId;
        json["details"] = m_details.toJson();
        json["tags"] = QJsonArray::fromStringList(m_tags);
        json["referenceImagePath"] = m_referenceImagePath;
        return json;
    }
    
    static Scene fromJson(const QJsonObject& json)
    {
        Scene scene;
        scene.m_id = json["id"].toString();
        scene.m_novelId = json["novelId"].toString();
        scene.m_name = json["name"].toString();
        scene.m_sceneId = json["sceneId"].toString();
        scene.m_details = SceneDetails::fromJson(json["details"].toObject());
        scene.m_tags = JsonUtils::jsonArrayToStringList(json["tags"].toArray());
        scene.m_referenceImagePath = json["referenceImagePath"].toString();
        return scene;
    }
    
    QVariantMap toVariantMap() const
    {
        QVariantMap map;
        map["id"] = m_id;
        map["novel_id"] = m_novelId;
        map["name"] = m_name;
        map["scene_id"] = m_sceneId;
        map["details"] = JsonUtils::jsonToString(m_details.toJson());
        map["tags"] = JsonUtils::jsonToString(QJsonArray::fromStringList(m_tags));
        map["reference_image_path"] = m_referenceImagePath;
        return map;
    }
    
    static Scene fromVariantMap(const QVariantMap& map)
    {
        Scene scene;
        scene.m_id = map["id"].toString();
        scene.m_novelId = map["novel_id"].toString();
        scene.m_name = map["name"].toString();
        scene.m_sceneId = map["scene_id"].toString();
        
        QJsonObject detailsJson = JsonUtils::variantToJson(map["details"]);
        if (!detailsJson.isEmpty()) {
            scene.m_details = SceneDetails::fromJson(detailsJson);
        }
        
        scene.m_tags = JsonUtils::variantToStringList(map["tags"]);
        scene.m_referenceImagePath = map["reference_image_path"].toString();
        return scene;
    }
    
private:
    QString m_id;
    QString m_novelId;
    QString m_name;
    QString m_sceneId;
    SceneDetails m_details;
    QStringList m_tags;
    QString m_referenceImagePath;
};

#endif // SCENE_H
