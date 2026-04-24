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
    QJsonObject fieldSources; // 字段来源：explicit / inferred / manual
    
    QJsonObject toJson() const;
    static SceneDetails fromJson(const QJsonObject& json);
    
    QStringList toDisplayStrings() const;
    
    // 判断场景详情是否有空字段
    bool hasEmptyFields() const;
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
    
    QJsonObject toJson() const;
    static Scene fromJson(const QJsonObject& json);
    
    QVariantMap toVariantMap() const;
    static Scene fromVariantMap(const QVariantMap& map);
    
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
