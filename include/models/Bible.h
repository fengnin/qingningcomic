#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QVariantMap>
#include <QMetaType>
#include "models/Character.h"
#include "models/Scene.h"

enum class BibleType {
    Character,
    Scene
};

class BibleEntry
{
public:
    BibleEntry();
    BibleEntry(const QString& id, const QString& novelId, const QString& name, BibleType type);
    
    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }
    
    QString novelId() const { return m_novelId; }
    void setNovelId(const QString& novelId) { m_novelId = novelId; }
    
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }
    
    BibleType type() const { return m_type; }
    void setType(BibleType type) { m_type = type; }
    
    QStringList tags() const { return m_tags; }
    void setTags(const QStringList& tags) { m_tags = tags; }
    
    QStringList referenceImages() const { return m_referenceImages; }
    void setReferenceImages(const QStringList& images) { m_referenceImages = images; }
    
    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }
    
    QDateTime updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QDateTime& dt) { m_updatedAt = dt; }
    
    CharacterAppearance characterAppearance() const { return m_characterAppearance; }
    void setCharacterAppearance(const CharacterAppearance& app) { m_characterAppearance = app; }
    
    SceneDetails sceneDetails() const { return m_sceneDetails; }
    void setSceneDetails(const SceneDetails& det) { m_sceneDetails = det; }
    
    QStringList personality() const { return m_personality; }
    void setPersonality(const QStringList& p) { m_personality = p; }
    
    QString updatedBy() const { return m_updatedBy; }
    void setUpdatedBy(const QString& by) { m_updatedBy = by; }
    
    QStringList toDisplayDetails() const;
    
    QJsonObject toJson() const;
    static BibleEntry fromJson(const QJsonObject& json);
    static BibleEntry merge(const BibleEntry& existing, const BibleEntry& incoming);
    
    QVariantMap toVariantMap() const;
    static BibleEntry fromVariantMap(const QVariantMap& map);

private:
    QString m_id;
    QString m_novelId;
    QString m_name;
    BibleType m_type = BibleType::Character;
    QStringList m_tags;
    QStringList m_referenceImages;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    
    CharacterAppearance m_characterAppearance;
    SceneDetails m_sceneDetails;
    QStringList m_personality;
    QString m_updatedBy;
};

Q_DECLARE_METATYPE(BibleEntry)
Q_DECLARE_METATYPE(BibleType)
