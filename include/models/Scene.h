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

inline bool isIndoorSceneType(const QString& type)
{
    return type == QStringLiteral("indoor") || type == QString::fromUtf8("室内");
}

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
    QStringList anchorPoints;
    QStringList signatureObjects;
    QStringList fixedColorBlocks;
    QStringList consistencyRules;
    QString type;
    QString typeZh;
    QString setting;
    QString timeOfDay;
    QString weather;
    QString spaceSize;
    QString currentInterpretation;
    QString confidence;
    QString status;
    QString narrativeRole;
    QString narrativeRoleZh;
    QStringList details;
    QStringList evidence;
    QStringList aliases;
    QStringList history;
    
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
        if (!anchorPoints.isEmpty()) json["anchorPoints"] = QJsonArray::fromStringList(anchorPoints);
        if (!signatureObjects.isEmpty()) json["signatureObjects"] = QJsonArray::fromStringList(signatureObjects);
        if (!fixedColorBlocks.isEmpty()) json["fixedColorBlocks"] = QJsonArray::fromStringList(fixedColorBlocks);
        if (!consistencyRules.isEmpty()) json["consistencyRules"] = QJsonArray::fromStringList(consistencyRules);
        if (!type.isEmpty()) json["type"] = type;
        if (!typeZh.isEmpty()) json["typeZh"] = typeZh;
        if (!setting.isEmpty()) json["setting"] = setting;
        if (!timeOfDay.isEmpty()) json["timeOfDay"] = timeOfDay;
        if (!weather.isEmpty()) json["weather"] = weather;
        if (!spaceSize.isEmpty()) json["spaceSize"] = spaceSize;
        if (!currentInterpretation.isEmpty()) json["currentInterpretation"] = currentInterpretation;
        if (!confidence.isEmpty()) json["confidence"] = confidence;
        if (!status.isEmpty()) json["status"] = status;
        if (!narrativeRole.isEmpty()) json["narrativeRole"] = narrativeRole;
        if (!narrativeRoleZh.isEmpty()) json["narrativeRoleZh"] = narrativeRoleZh;
        if (!details.isEmpty()) json["details"] = QJsonArray::fromStringList(details);
        if (!evidence.isEmpty()) json["evidence"] = QJsonArray::fromStringList(evidence);
        if (!aliases.isEmpty()) json["aliases"] = QJsonArray::fromStringList(aliases);
        if (!history.isEmpty()) json["history"] = QJsonArray::fromStringList(history);
        
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
        det.anchorPoints = JsonUtils::jsonArrayToStringList(json["anchorPoints"].toArray());
        det.signatureObjects = JsonUtils::jsonArrayToStringList(json["signatureObjects"].toArray());
        det.fixedColorBlocks = JsonUtils::jsonArrayToStringList(json["fixedColorBlocks"].toArray());
        det.consistencyRules = JsonUtils::jsonArrayToStringList(json["consistencyRules"].toArray());
        det.type = json["type"].toString();
        det.typeZh = json["typeZh"].toString();
        det.setting = json["setting"].toString();
        det.timeOfDay = json["timeOfDay"].toString();
        det.weather = json["weather"].toString();
        det.spaceSize = json["spaceSize"].toString();
        det.currentInterpretation = json["currentInterpretation"].toString();
        det.confidence = json["confidence"].toString();
        det.status = json["status"].toString();
        det.narrativeRole = json["narrativeRole"].toString();
        det.narrativeRoleZh = json["narrativeRoleZh"].toString();
        det.evidence = JsonUtils::jsonArrayToStringList(json["evidence"].toArray());
        det.aliases = JsonUtils::jsonArrayToStringList(json["aliases"].toArray());
        det.history = JsonUtils::jsonArrayToStringList(json["history"].toArray());
        if (isIndoorSceneType(det.type)) {
            det.weather.clear();
            det.weatherVariations = QJsonArray();
        }
        
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
    
    QStringList toDisplayStrings() const;
    
    // 判断场景详情是否有空字段
    inline bool hasEmptyFields() const
    {
        const bool indoorScene = isIndoorSceneType(type);
        return description.isEmpty() || building.isEmpty() || color.isEmpty() ||
               landmark.isEmpty() || layout.isEmpty() || atmosphere.isEmpty() ||
               anchorPoints.isEmpty() || signatureObjects.isEmpty() || fixedColorBlocks.isEmpty() ||
               consistencyRules.isEmpty() ||
               spaceSize.isEmpty() || type.isEmpty() || setting.isEmpty() || timeOfDay.isEmpty() ||
               currentInterpretation.isEmpty() || confidence.isEmpty() || status.isEmpty() ||
               evidence.isEmpty() || aliases.isEmpty() || history.isEmpty() ||
               narrativeRole.isEmpty() ||
               (!indoorScene && weather.isEmpty());
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
