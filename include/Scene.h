#ifndef SCENE_H
#define SCENE_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include "utils/JsonUtils.h"

namespace SceneJsonParser {
    struct ParsedVisualData {
        QString building;
        QString color;
        QString atmosphere;
        QString landmark;
        QString layout;
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
        }
        
        return data;
    }
    
    inline void writeVisualCharacteristics(QJsonObject& json, 
                                            const QString& building,
                                            const QString& color,
                                            const QString& atmosphere,
                                            const QString& landmark,
                                            const QString& layout)
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
        
        if (!layout.isEmpty()) {
            QJsonObject spatialLayout;
            spatialLayout["layout"] = layout;
            json["spatialLayout"] = spatialLayout;
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
    QStringList details;
    
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["description"] = description;
        obj["type"] = type;
        obj["setting"] = setting;
        obj["timeOfDay"] = timeOfDay;
        obj["weather"] = weather;
        obj["details"] = QJsonArray::fromStringList(details);
        
        SceneJsonParser::writeVisualCharacteristics(obj, building, color, 
                                                      atmosphere, landmark, layout);
        return obj;
    }
    
    static SceneDetails fromJson(const QJsonObject& obj)
    {
        SceneDetails det;
        det.description = obj["description"].toString();
        det.type = obj["type"].toString();
        det.setting = obj["setting"].toString();
        det.timeOfDay = obj["timeOfDay"].toString();
        det.weather = obj["weather"].toString();
        
        if (obj.contains("visualCharacteristics") || obj.contains("spatialLayout")) {
            SceneJsonParser::ParsedVisualData visual = 
                SceneJsonParser::parseVisualCharacteristics(obj);
            det.building = visual.building;
            det.color = visual.color;
            det.atmosphere = visual.atmosphere;
            det.landmark = visual.landmark;
            det.layout = visual.layout;
        } else {
            det.building = obj["building"].toString();
            det.color = obj["color"].toString();
            det.atmosphere = obj["atmosphere"].toString();
            det.landmark = obj["landmark"].toString();
            det.layout = obj["layout"].toString();
        }
        
        det.details = JsonUtils::jsonArrayToStringList(obj["details"].toArray());
        
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
        
        addLine(QString::fromUtf8("\u573a\u666f\u63cf\u8ff0"), description);
        addLine(QString::fromUtf8("\u5efa\u7b51"), building);
        addLine(QString::fromUtf8("\u8272\u8c03"), color);
        addLine(QString::fromUtf8("\u5730\u6807"), landmark);
        addLine(QString::fromUtf8("\u5e03\u5c40"), layout);
        addLine(QString::fromUtf8("\u6c1b\u56f4"), atmosphere);
        
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
