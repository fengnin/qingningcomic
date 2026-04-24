#include "models/Bible.h"
#include "utils/BibleUtils.h"

namespace {

template <typename T>
void fillIfEmpty(T& target, const T& incoming)
{
    if (target.isEmpty() && !incoming.isEmpty()) {
        target = incoming;
    }
}

void fillIfEmpty(int& target, int incoming)
{
    if (target == 0 && incoming > 0) {
        target = incoming;
    }
}

void appendDisplayLine(QStringList& lines, const QString& label, const QString& value)
{
    if (!value.isEmpty()) {
        lines << QString::fromUtf8("%1：%2").arg(label, value);
    }
}

bool hasMeaningfulAppearance(const CharacterAppearance& appearance)
{
    return !appearance.gender.trimmed().isEmpty() ||
           appearance.age > 0 ||
           !appearance.hairColor.trimmed().isEmpty() ||
           !appearance.hairStyle.trimmed().isEmpty() ||
           !appearance.eyeColor.trimmed().isEmpty() ||
           !appearance.height.trimmed().isEmpty() ||
           !appearance.build.trimmed().isEmpty() ||
           !appearance.clothing.isEmpty() ||
           !appearance.accessories.isEmpty() ||
           !appearance.distinctiveFeatures.isEmpty();
}

bool hasMeaningfulSceneDetails(const SceneDetails& details)
{
    return !details.description.trimmed().isEmpty() ||
           !details.building.trimmed().isEmpty() ||
           !details.color.trimmed().isEmpty() ||
           !details.landmark.trimmed().isEmpty() ||
           !details.layout.trimmed().isEmpty() ||
           !details.atmosphere.trimmed().isEmpty() ||
           !details.anchorPoints.isEmpty() ||
           !details.signatureObjects.isEmpty() ||
           !details.fixedColorBlocks.isEmpty() ||
           !details.consistencyRules.isEmpty() ||
           !details.type.trimmed().isEmpty() ||
           !details.typeZh.trimmed().isEmpty() ||
           !details.setting.trimmed().isEmpty() ||
           !details.timeOfDay.trimmed().isEmpty() ||
           !details.weather.trimmed().isEmpty() ||
           !details.spaceSize.trimmed().isEmpty() ||
           !details.currentInterpretation.trimmed().isEmpty() ||
           !details.confidence.trimmed().isEmpty() ||
           !details.status.trimmed().isEmpty() ||
           !details.narrativeRole.trimmed().isEmpty() ||
           !details.narrativeRoleZh.trimmed().isEmpty() ||
           !details.details.isEmpty() ||
           !details.evidence.isEmpty() ||
           !details.aliases.isEmpty() ||
           !details.history.isEmpty() ||
           !details.visualCharacteristics.isEmpty() ||
           !details.spatialLayout.isEmpty() ||
           !details.timeVariations.isEmpty() ||
           !details.weatherVariations.isEmpty();
}

bool isEmptyBibleEntry(const BibleEntry& entry)
{
    return entry.id().isEmpty() &&
           entry.novelId().isEmpty() &&
           entry.name().isEmpty() &&
           entry.tags().isEmpty() &&
           entry.referenceImages().isEmpty() &&
           entry.personality().isEmpty() &&
           entry.updatedBy().isEmpty() &&
           !hasMeaningfulAppearance(entry.characterAppearance()) &&
           !hasMeaningfulSceneDetails(entry.sceneDetails());
}

CharacterAppearance mergeAppearance(const CharacterAppearance& existing, const CharacterAppearance& incoming)
{
    CharacterAppearance merged = existing;
    fillIfEmpty(merged.gender, incoming.gender);
    fillIfEmpty(merged.age, incoming.age);
    fillIfEmpty(merged.hairColor, incoming.hairColor);
    fillIfEmpty(merged.hairStyle, incoming.hairStyle);
    fillIfEmpty(merged.eyeColor, incoming.eyeColor);
    fillIfEmpty(merged.height, incoming.height);
    fillIfEmpty(merged.build, incoming.build);
    merged.clothing = BibleUtils::mergeStringLists(merged.clothing, incoming.clothing);
    merged.accessories = BibleUtils::mergeStringLists(merged.accessories, incoming.accessories);
    merged.distinctiveFeatures = BibleUtils::mergeStringLists(merged.distinctiveFeatures, incoming.distinctiveFeatures);
    return merged;
}

SceneDetails mergeSceneDetails(const SceneDetails& existing, const SceneDetails& incoming)
{
    SceneDetails merged = existing;
    fillIfEmpty(merged.description, incoming.description);
    fillIfEmpty(merged.building, incoming.building);
    fillIfEmpty(merged.color, incoming.color);
    fillIfEmpty(merged.landmark, incoming.landmark);
    fillIfEmpty(merged.layout, incoming.layout);
    fillIfEmpty(merged.atmosphere, incoming.atmosphere);
    fillIfEmpty(merged.type, incoming.type);
    fillIfEmpty(merged.typeZh, incoming.typeZh);
    fillIfEmpty(merged.setting, incoming.setting);
    fillIfEmpty(merged.timeOfDay, incoming.timeOfDay);
    fillIfEmpty(merged.weather, incoming.weather);
    fillIfEmpty(merged.spaceSize, incoming.spaceSize);
    fillIfEmpty(merged.currentInterpretation, incoming.currentInterpretation);
    fillIfEmpty(merged.confidence, incoming.confidence);
    fillIfEmpty(merged.status, incoming.status);
    fillIfEmpty(merged.narrativeRole, incoming.narrativeRole);
    fillIfEmpty(merged.narrativeRoleZh, incoming.narrativeRoleZh);

    merged.anchorPoints = BibleUtils::mergeStringLists(merged.anchorPoints, incoming.anchorPoints);
    merged.signatureObjects = BibleUtils::mergeStringLists(merged.signatureObjects, incoming.signatureObjects);
    merged.fixedColorBlocks = BibleUtils::mergeStringLists(merged.fixedColorBlocks, incoming.fixedColorBlocks);
    merged.consistencyRules = BibleUtils::mergeStringLists(merged.consistencyRules, incoming.consistencyRules);
    merged.details = BibleUtils::mergeStringLists(merged.details, incoming.details);
    merged.evidence = BibleUtils::mergeStringLists(merged.evidence, incoming.evidence);
    merged.aliases = BibleUtils::mergeStringLists(merged.aliases, incoming.aliases);
    merged.history = BibleUtils::mergeStringLists(merged.history, incoming.history);
    merged.visualCharacteristics = BibleUtils::mergeVisualCharacteristics(
        merged.visualCharacteristics, incoming.visualCharacteristics);
    merged.spatialLayout = BibleUtils::mergeSpatialLayout(merged.spatialLayout, incoming.spatialLayout);
    merged.timeVariations = BibleUtils::mergeVariations(merged.timeVariations, incoming.timeVariations, QStringLiteral("timeOfDay"));
    merged.weatherVariations = BibleUtils::mergeVariations(merged.weatherVariations, incoming.weatherVariations, QStringLiteral("weather"));
    return merged;
}

} // namespace

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
        appendDisplayLine(details, QString::fromUtf8("\u6027\u522b"), m_characterAppearance.gender);
        if (m_characterAppearance.age > 0) {
            appendDisplayLine(details, QString::fromUtf8("\u5e74\u9f84"), QString::fromUtf8("%1\u5c81").arg(m_characterAppearance.age));
        }
        appendDisplayLine(details, QString::fromUtf8("\u53d1\u8272"), m_characterAppearance.hairColor);
        appendDisplayLine(details, QString::fromUtf8("\u53d1\u578b"), m_characterAppearance.hairStyle);
        appendDisplayLine(details, QString::fromUtf8("\u773c\u775b"), m_characterAppearance.eyeColor);
        appendDisplayLine(details, QString::fromUtf8("\u8eab\u9ad8"), m_characterAppearance.height);
        appendDisplayLine(details, QString::fromUtf8("\u4f53\u578b"), m_characterAppearance.build);
        appendDisplayLine(details, QString::fromUtf8("\u670d\u9970"), m_characterAppearance.clothing.join(", "));
        appendDisplayLine(details, QString::fromUtf8("\u914d\u9970"), m_characterAppearance.accessories.join(", "));
        appendDisplayLine(details, QString::fromUtf8("\u7279\u5f81"), m_characterAppearance.distinctiveFeatures.join(", "));
        appendDisplayLine(details, QString::fromUtf8("\u6027\u683c"), m_personality.join(", "));
    } else {
        details = m_sceneDetails.toDisplayStrings();
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
        json["appearance"] = m_characterAppearance.toJson();
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
    QJsonArray imagesArray = json["referenceImages"].toArray();
    entry.m_tags = JsonUtils::jsonArrayToStringList(tagsArray);
    entry.m_referenceImages = JsonUtils::jsonArrayToStringList(imagesArray);
    entry.m_createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
    entry.m_updatedAt = QDateTime::fromString(json["updatedAt"].toString(), Qt::ISODate);
    entry.m_updatedBy = json["updatedBy"].toString();
    
    if (entry.m_type == BibleType::Character) {
        entry.m_characterAppearance = CharacterAppearance::fromJson(json["appearance"].toObject());
        entry.m_personality = JsonUtils::jsonArrayToStringList(json["personality"].toArray());
    } else {
        entry.m_sceneDetails = SceneDetails::fromJson(json["sceneDetails"].toObject());
    }
    
    return entry;
}

BibleEntry BibleEntry::merge(const BibleEntry& existing, const BibleEntry& incoming)
{
    BibleEntry merged = existing;

    if (merged.id().isEmpty()) {
        merged.setId(incoming.id());
    }
    if (merged.novelId().isEmpty()) {
        merged.setNovelId(incoming.novelId());
    }
    if (merged.name().isEmpty()) {
        merged.setName(incoming.name());
    }
    if (merged.tags().isEmpty()) {
        merged.setTags(incoming.tags());
    } else {
        merged.setTags(BibleUtils::mergeStringLists(merged.tags(), incoming.tags()));
    }
    if (merged.referenceImages().isEmpty()) {
        merged.setReferenceImages(incoming.referenceImages());
    } else {
        merged.setReferenceImages(BibleUtils::mergeStringLists(merged.referenceImages(), incoming.referenceImages()));
    }
    if (!incoming.updatedBy().isEmpty()) {
        merged.setUpdatedBy(incoming.updatedBy());
    }
    if (!merged.createdAt().isValid() && incoming.createdAt().isValid()) {
        merged.setCreatedAt(incoming.createdAt());
    }
    if (incoming.updatedAt().isValid()) {
        merged.setUpdatedAt(incoming.updatedAt());
    }

    const bool existingEmpty = isEmptyBibleEntry(existing);
    const BibleType resultType = existingEmpty ? incoming.type() : existing.type();
    merged.setType(resultType);

    if (resultType == BibleType::Character) {
        merged.setCharacterAppearance(mergeAppearance(existing.characterAppearance(), incoming.characterAppearance()));
        merged.setPersonality(BibleUtils::mergeStringLists(existing.personality(), incoming.personality()));
        merged.setSceneDetails(SceneDetails());
    } else {
        merged.setSceneDetails(mergeSceneDetails(existing.sceneDetails(), incoming.sceneDetails()));
        merged.setCharacterAppearance(CharacterAppearance());
        merged.setPersonality(QStringList());
    }

    return merged;
}

QVariantMap BibleEntry::toVariantMap() const
{
    return toJson().toVariantMap();
}

BibleEntry BibleEntry::fromVariantMap(const QVariantMap& map)
{
    return fromJson(QJsonObject::fromVariantMap(map));
}
