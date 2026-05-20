#include "models/CharacterPortraitVersion.h"
#include "utils/JsonUtils.h"

#include <QJsonDocument>

static const int s_cpvMetaType = qRegisterMetaType<CharacterPortraitVersion>();

QVariantMap CharacterPortraitVersion::toVariantMap() const
{
    QVariantMap map;
    map["id"] = m_id;
    map["character_id"] = m_characterId;
    map["version_no"] = m_versionNo;
    map["portrait_path"] = m_portraitPath;
    if (!m_baseVersionId.isEmpty()) {
        map["base_version_id"] = m_baseVersionId;
    }
    if (!m_editPrompt.isEmpty()) {
        map["edit_prompt"] = m_editPrompt;
    }
    if (!m_fieldDiff.isEmpty()) {
        map["field_diff"] = JsonUtils::jsonToString(m_fieldDiff);
    }
    if (!m_appearanceSnapshot.isEmpty()) {
        map["appearance_snapshot"] = JsonUtils::jsonToString(m_appearanceSnapshot);
    }
    if (m_sourceChapter > 0) {
        map["source_chapter"] = m_sourceChapter;
    }
    if (m_createdAt.isValid()) {
        map["created_at"] = m_createdAt;
    }
    return map;
}

CharacterPortraitVersion CharacterPortraitVersion::fromVariantMap(const QVariantMap& map)
{
    CharacterPortraitVersion v;
    v.m_id = map.value("id").toString();
    v.m_characterId = map.value("character_id").toString();
    v.m_versionNo = map.value("version_no").toInt();
    v.m_portraitPath = map.value("portrait_path").toString();
    v.m_baseVersionId = map.value("base_version_id").toString();
    v.m_editPrompt = map.value("edit_prompt").toString();

    const QVariant diffVar = map.value("field_diff");
    if (diffVar.isValid() && !diffVar.isNull()) {
        v.m_fieldDiff = JsonUtils::variantToJson(diffVar);
    }

    const QVariant snapshotVar = map.value("appearance_snapshot");
    if (snapshotVar.isValid() && !snapshotVar.isNull()) {
        v.m_appearanceSnapshot = JsonUtils::variantToJson(snapshotVar);
    }

    v.m_sourceChapter = map.value("source_chapter").toInt();
    v.m_createdAt = map.value("created_at").toDateTime();
    return v;
}

QJsonObject CharacterPortraitVersion::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["characterId"] = m_characterId;
    json["versionNo"] = m_versionNo;
    json["portraitPath"] = m_portraitPath;
    json["baseVersionId"] = m_baseVersionId;
    json["editPrompt"] = m_editPrompt;
    if (!m_fieldDiff.isEmpty()) {
        json["fieldDiff"] = m_fieldDiff;
    }
    if (!m_appearanceSnapshot.isEmpty()) {
        json["appearanceSnapshot"] = m_appearanceSnapshot;
    }
    json["sourceChapter"] = m_sourceChapter;
    if (m_createdAt.isValid()) {
        json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    }
    return json;
}

CharacterPortraitVersion CharacterPortraitVersion::fromJson(const QJsonObject& json)
{
    CharacterPortraitVersion v;
    v.m_id = json.value("id").toString();
    v.m_characterId = json.value("characterId").toString();
    v.m_versionNo = json.value("versionNo").toInt();
    v.m_portraitPath = json.value("portraitPath").toString();
    v.m_baseVersionId = json.value("baseVersionId").toString();
    v.m_editPrompt = json.value("editPrompt").toString();
    v.m_fieldDiff = json.value("fieldDiff").toObject();
    v.m_appearanceSnapshot = json.value("appearanceSnapshot").toObject();
    v.m_sourceChapter = json.value("sourceChapter").toInt();
    v.m_createdAt = QDateTime::fromString(json.value("createdAt").toString(), Qt::ISODate);
    return v;
}
