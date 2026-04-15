#include "Bible.h"
#include <QUuid>

BibleEntry::BibleEntry()
    : m_type(BibleType::Character)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

BibleEntry::BibleEntry(const QString& id, const QString& novelId, const QString& name, BibleType type)
    : m_id(id)
    , m_novelId(novelId)
    , m_name(name)
    , m_type(type)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

QStringList BibleEntry::toDisplayDetails() const
{
    QStringList details;
    
    if (m_type == BibleType::Character) {
        if (!m_characterAppearance.gender.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_characterAppearance.gender);
        }
        if (m_characterAppearance.age > 0) {
            details << QString::fromUtf8("Text").arg(m_characterAppearance.age);
        }
        if (!m_characterAppearance.hairColor.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_characterAppearance.hairColor);
        }
        if (!m_characterAppearance.hairStyle.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_characterAppearance.hairStyle);
        }
        if (!m_characterAppearance.eyeColor.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_characterAppearance.eyeColor);
        }
        if (!m_characterAppearance.build.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_characterAppearance.build);
        }
        if (!m_personality.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_personality.join(", "));
        }
    } else {
        if (!m_sceneDetails.description.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_sceneDetails.description);
        }
        if (!m_sceneDetails.type.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_sceneDetails.type);
        }
        if (!m_sceneDetails.atmosphere.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_sceneDetails.atmosphere);
        }
        if (!m_sceneDetails.building.isEmpty()) {
            details << QString::fromUtf8("Text").arg(m_sceneDetails.building);
        }
    }
    
    return details;
}

QJsonObject BibleEntry::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["novelId"] = m_novelId;
    json["name"] = m_name;
    json["type"] = m_type == BibleType::Character ? "character" : "scene";
    json["tags"] = QJsonArray::fromStringList(m_tags);
    json["referenceImages"] = QJsonArray::fromStringList(m_referenceImages);
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    json["updatedAt"] = m_updatedAt.toString(Qt::ISODate);
    
    if (m_type == BibleType::Character) {
        QJsonObject appJson;
        appJson["gender"] = m_characterAppearance.gender;
        appJson["age"] = m_characterAppearance.age;
        appJson["hairColor"] = m_characterAppearance.hairColor;
        appJson["hairStyle"] = m_characterAppearance.hairStyle;
        appJson["eyeColor"] = m_characterAppearance.eyeColor;
        appJson["height"] = m_characterAppearance.height;
        appJson["build"] = m_characterAppearance.build;
        appJson["clothing"] = QJsonArray::fromStringList(m_characterAppearance.clothing);
        appJson["distinctiveFeatures"] = QJsonArray::fromStringList(m_characterAppearance.distinctiveFeatures);
        json["appearance"] = appJson;
        json["personality"] = QJsonArray::fromStringList(m_personality);
    } else {
        json["sceneDetails"] = m_sceneDetails.toJson();
    }
    
    if (!m_updatedBy.isEmpty()) {
        json["updatedBy"] = m_updatedBy;
    }
    
    return json;
}

BibleEntry BibleEntry::fromJson(const QJsonObject& json)
{
    BibleEntry entry;
    entry.m_id = json["id"].toString();
    entry.m_novelId = json["novelId"].toString();
    entry.m_name = json["name"].toString();
    
    QString typeStr = json["type"].toString();
    entry.m_type = (typeStr == "scene") ? BibleType::Scene : BibleType::Character;
    
    QJsonArray tagsArray = json["tags"].toArray();
    for (const QJsonValue& v : tagsArray) {
        entry.m_tags << v.toString();
    }
    
    QJsonArray imagesArray = json["referenceImages"].toArray();
    for (const QJsonValue& v : imagesArray) {
        entry.m_referenceImages << v.toString();
    }
    
    entry.m_createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
    entry.m_updatedAt = QDateTime::fromString(json["updatedAt"].toString(), Qt::ISODate);
    entry.m_updatedBy = json["updatedBy"].toString();
    
    if (entry.m_type == BibleType::Character) {
        QJsonObject appJson = json["appearance"].toObject();
        entry.m_characterAppearance.gender = appJson["gender"].toString();
        entry.m_characterAppearance.age = appJson["age"].toInt();
        entry.m_characterAppearance.hairColor = appJson["hairColor"].toString();
        entry.m_characterAppearance.hairStyle = appJson["hairStyle"].toString();
        entry.m_characterAppearance.eyeColor = appJson["eyeColor"].toString();
        entry.m_characterAppearance.height = appJson["height"].toString();
        entry.m_characterAppearance.build = appJson["build"].toString();
        
        QJsonArray clothingArray = appJson["clothing"].toArray();
        for (const QJsonValue& v : clothingArray) {
            entry.m_characterAppearance.clothing << v.toString();
        }
        
        QJsonArray featuresArray = appJson["distinctiveFeatures"].toArray();
        for (const QJsonValue& v : featuresArray) {
            entry.m_characterAppearance.distinctiveFeatures << v.toString();
        }
        
        QJsonArray personalityArray = json["personality"].toArray();
        for (const QJsonValue& v : personalityArray) {
            entry.m_personality << v.toString();
        }
    } else {
        entry.m_sceneDetails = SceneDetails::fromJson(json["sceneDetails"].toObject());
    }
    
    return entry;
}

QVariantMap BibleEntry::toVariantMap() const
{
    QVariantMap map;
    map["id"] = m_id;
    map["novelId"] = m_novelId;
    map["name"] = m_name;
    map["type"] = m_type == BibleType::Character ? "character" : "scene";
    map["tags"] = m_tags;
    map["referenceImages"] = m_referenceImages;
    map["createdAt"] = m_createdAt;
    map["updatedAt"] = m_updatedAt;
    map["updatedBy"] = m_updatedBy;
    
    if (m_type == BibleType::Character) {
        QVariantMap appMap;
        appMap["gender"] = m_characterAppearance.gender;
        appMap["age"] = m_characterAppearance.age;
        appMap["hairColor"] = m_characterAppearance.hairColor;
        appMap["hairStyle"] = m_characterAppearance.hairStyle;
        appMap["eyeColor"] = m_characterAppearance.eyeColor;
        appMap["height"] = m_characterAppearance.height;
        appMap["build"] = m_characterAppearance.build;
        appMap["clothing"] = m_characterAppearance.clothing;
        appMap["distinctiveFeatures"] = m_characterAppearance.distinctiveFeatures;
        map["appearance"] = appMap;
        map["personality"] = m_personality;
    } else {
        QVariantMap detMap;
        detMap["description"] = m_sceneDetails.description;
        detMap["building"] = m_sceneDetails.building;
        detMap["color"] = m_sceneDetails.color;
        detMap["landmark"] = m_sceneDetails.landmark;
        detMap["layout"] = m_sceneDetails.layout;
        detMap["atmosphere"] = m_sceneDetails.atmosphere;
        detMap["type"] = m_sceneDetails.type;
        map["sceneDetails"] = detMap;
    }
    
    return map;
}

BibleEntry BibleEntry::fromVariantMap(const QVariantMap& map)
{
    BibleEntry entry;
    entry.m_id = map["id"].toString();
    entry.m_novelId = map["novelId"].toString();
    entry.m_name = map["name"].toString();
    
    QString typeStr = map["type"].toString();
    entry.m_type = (typeStr == "scene") ? BibleType::Scene : BibleType::Character;
    
    entry.m_tags = map["tags"].toStringList();
    entry.m_referenceImages = map["referenceImages"].toStringList();
    entry.m_createdAt = map["createdAt"].toDateTime();
    entry.m_updatedAt = map["updatedAt"].toDateTime();
    entry.m_updatedBy = map["updatedBy"].toString();
    
    if (entry.m_type == BibleType::Character) {
        QVariantMap appMap = map["appearance"].toMap();
        entry.m_characterAppearance.gender = appMap["gender"].toString();
        entry.m_characterAppearance.age = appMap["age"].toInt();
        entry.m_characterAppearance.hairColor = appMap["hairColor"].toString();
        entry.m_characterAppearance.hairStyle = appMap["hairStyle"].toString();
        entry.m_characterAppearance.eyeColor = appMap["eyeColor"].toString();
        entry.m_characterAppearance.height = appMap["height"].toString();
        entry.m_characterAppearance.build = appMap["build"].toString();
        entry.m_characterAppearance.clothing = appMap["clothing"].toStringList();
        entry.m_characterAppearance.distinctiveFeatures = appMap["distinctiveFeatures"].toStringList();
        entry.m_personality = map["personality"].toStringList();
    } else {
        QVariantMap detMap = map["sceneDetails"].toMap();
        entry.m_sceneDetails.description = detMap["description"].toString();
        entry.m_sceneDetails.building = detMap["building"].toString();
        entry.m_sceneDetails.color = detMap["color"].toString();
        entry.m_sceneDetails.landmark = detMap["landmark"].toString();
        entry.m_sceneDetails.layout = detMap["layout"].toString();
        entry.m_sceneDetails.atmosphere = detMap["atmosphere"].toString();
        entry.m_sceneDetails.type = detMap["type"].toString();
    }
    
    return entry;
}
