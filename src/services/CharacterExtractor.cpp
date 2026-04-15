#include "CharacterExtractor.h"
#include "BibleCache.h"
#include "DatabaseManager.h"
#include "Logger.h"
#include "utils/JsonUtils.h"
#include "utils/BibleUtils.h"
#include <QDateTime>
#include <QUuid>

CharacterExtractor* CharacterExtractor::m_instance = nullptr;
std::once_flag CharacterExtractor::m_instanceOnceFlag;

QJsonObject ExtractedCharacter::toJson() const
{
    QJsonObject json;
    json["name"] = name;
    json["gender"] = gender;
    json["age"] = age;
    json["hairColor"] = hairColor;
    json["hairStyle"] = hairStyle;
    json["eyeColor"] = eyeColor;
    json["bodyType"] = bodyType;
    json["clothing"] = JsonUtils::stringListToJsonArray(clothing);
    json["accessories"] = JsonUtils::stringListToJsonArray(accessories);
    json["distinctiveFeatures"] = JsonUtils::stringListToJsonArray(distinctiveFeatures);
    json["personality"] = JsonUtils::stringListToJsonArray(personality);
    json["tags"] = JsonUtils::stringListToJsonArray(tags);
    return json;
}

ExtractedCharacter ExtractedCharacter::fromJson(const QJsonObject& json)
{
    ExtractedCharacter c;
    c.name = json["name"].toString();
    c.gender = json["gender"].toString();
    c.age = json["age"].toInt();
    c.hairColor = json["hairColor"].toString();
    c.hairStyle = json["hairStyle"].toString();
    c.eyeColor = json["eyeColor"].toString();
    c.bodyType = json["bodyType"].toString();
    c.clothing = JsonUtils::jsonArrayToStringList(json["clothing"].toArray());
    c.accessories = JsonUtils::jsonArrayToStringList(json["accessories"].toArray());
    c.distinctiveFeatures = JsonUtils::jsonArrayToStringList(json["distinctiveFeatures"].toArray());
    c.personality = JsonUtils::jsonArrayToStringList(json["personality"].toArray());
    c.tags = JsonUtils::jsonArrayToStringList(json["tags"].toArray());
    return c;
}

CharacterExtractor::CharacterExtractor(QObject* parent)
    : QObject(parent)
{
}

CharacterExtractor* CharacterExtractor::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new CharacterExtractor();
    });
    return m_instance;
}

QList<ExtractedCharacter> CharacterExtractor::extractFromPanels(const QJsonArray& panels)
{
    QList<ExtractedCharacter> characters;
    QSet<QString> seenNames;
    
    for (const QJsonValue& panelVal : panels) {
        QJsonObject panel = panelVal.toObject();
        QJsonArray charArray = panel["characters"].toArray();
        
        for (const QJsonValue& charVal : charArray) {
            ExtractedCharacter extracted = parsePanelCharacter(charVal);
            
            if (!extracted.name.isEmpty() && !seenNames.contains(extracted.name)) {
                seenNames.insert(extracted.name);
                characters.append(extracted);
                emit characterExtracted(extracted);
            }
        }
    }
    
    emit extractionCompleted(characters.size());
    LOG_INFO("CharacterExtractor", QString("Extracted %1 unique characters from %2 panels")
        .arg(characters.size()).arg(panels.size()));
    
    return characters;
}

ExtractedCharacter CharacterExtractor::parsePanelCharacter(const QJsonValue& charVal)
{
    ExtractedCharacter extracted;
    
    if (charVal.isString()) {
        QString charStr = charVal.toString();
        int parenPos = charStr.indexOf('(');
        if (parenPos > 0) {
            extracted.name = charStr.left(parenPos).trimmed();
            int endParen = charStr.indexOf(')', parenPos);
            if (endParen > parenPos) {
                QString featureStr = charStr.mid(parenPos + 1, endParen - parenPos - 1);
                if (!featureStr.isEmpty()) {
                    extracted.distinctiveFeatures << featureStr;
                }
            }
        } else {
            extracted.name = charStr.trimmed();
        }
    } else if (charVal.isObject()) {
        QJsonObject charObj = charVal.toObject();
        extracted.name = charObj["name"].toString();
        QString expression = charObj["expression"].toString();
        QString pose = charObj["pose"].toString();
        QString featureStr;
        if (!expression.isEmpty()) {
            featureStr = expression;
        }
        if (!pose.isEmpty()) {
            if (!featureStr.isEmpty()) {
                featureStr += " ";
            }
            featureStr += pose;
        }
        if (!featureStr.isEmpty()) {
            extracted.distinctiveFeatures << featureStr;
        }
    }
    
    return extracted;
}

QList<ExtractedCharacter> CharacterExtractor::extractFromCharacters(const QJsonArray& characters)
{
    QList<ExtractedCharacter> result;
    
    for (const QJsonValue& charVal : characters) {
        QJsonObject charObj = charVal.toObject();
        ExtractedCharacter extracted = parseAICharacter(charObj);
        
        if (!extracted.name.isEmpty() && containsChinese(extracted.name)) {
            result.append(extracted);
            emit characterExtracted(extracted);
        } else if (!extracted.name.isEmpty()) {
            LOG_DEBUG("CharacterExtractor", QString("Skipping non-Chinese character: %1").arg(extracted.name));
        }
    }
    
    emit extractionCompleted(result.size());
    LOG_INFO("CharacterExtractor", QString("Extracted %1 characters from AI response")
        .arg(result.size()));
    
    return result;
}

ExtractedCharacter CharacterExtractor::parseAICharacter(const QJsonObject& charObj)
{
    ExtractedCharacter extracted;
    extracted.name = charObj["name"].toString();
    
    if (extracted.name.isEmpty()) {
        return extracted;
    }
    
    QJsonObject appObj = charObj["appearance"].toObject();
    extracted.gender = appObj["gender"].toString();
    extracted.age = appObj["age"].toInt();
    extracted.hairColor = appObj["hairColor"].toString();
    extracted.hairStyle = appObj["hairStyle"].toString();
    extracted.eyeColor = appObj["eyeColor"].toString();
    extracted.bodyType = appObj["build"].toString();
    extracted.clothing = JsonUtils::jsonArrayToStringList(appObj["clothing"].toArray());
    extracted.accessories = JsonUtils::jsonArrayToStringList(appObj["accessories"].toArray());
    extracted.distinctiveFeatures = JsonUtils::jsonArrayToStringList(appObj["distinctiveFeatures"].toArray());
    extracted.personality = JsonUtils::jsonArrayToStringList(charObj["personality"].toArray());
    extracted.tags = JsonUtils::jsonArrayToStringList(charObj["tags"].toArray());
    
    return extracted;
}

Character CharacterExtractor::toCharacter(const ExtractedCharacter& extracted, const QString& novelId)
{
    Character character;
    character.setId(generateId());
    character.setNovelId(novelId);
    character.setName(extracted.name);
    
    CharacterAppearance app;
    app.gender = extracted.gender;
    app.age = extracted.age;
    app.hairColor = extracted.hairColor;
    app.hairStyle = extracted.hairStyle;
    app.eyeColor = extracted.eyeColor;
    app.build = extracted.bodyType;
    app.height = "";
    app.clothing = extracted.clothing;
    app.accessories = extracted.accessories;
    app.distinctiveFeatures = extracted.distinctiveFeatures;
    
    character.setAppearance(app);
    character.setPersonality(extracted.personality);
    
    return character;
}

bool CharacterExtractor::containsChinese(const QString& text)
{
    return text.contains(QRegularExpression("\\p{Han}"));
}

QString CharacterExtractor::normalizeCharacterName(const QString& name)
{
    if (name.isEmpty()) return name;
    
    QString normalized = name.trimmed();
    
    // 清理 AI 常见的角色后缀，避免同一角色被拆成多个名字
    static const QStringList suffixes = {
        "(true form)",
        "(clone)",
        "(past)",
        "(future)",
        "(young)",
        "(old)",
        "（真身）",
        "（分身）",
        "（少年）",
        "（老年）"
    };
    
    for (const QString& suffix : suffixes) {
        if (normalized.endsWith(suffix, Qt::CaseInsensitive)) {
            normalized.chop(suffix.length());
            normalized = normalized.trimmed();
            
            LOG_DEBUG("CharacterExtractor", QString("Normalized character name: '%1' -> '%2'")
                .arg(name).arg(normalized));
            break;
        }
    }
    
    return normalized;
}

QString CharacterExtractor::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool CharacterExtractor::saveCharacter(const QString& novelId, const ExtractedCharacter& extracted)
{
    Character character = toCharacter(extracted, novelId);
    
    // 优先按常见角色标签合并，避免重复保存同一人物
    QString normalizedName = normalizeCharacterName(character.name());
    Character existing = getCharacterByName(novelId, normalizedName);
    
    if (!existing.id().isEmpty()) {
        LOG_INFO("CharacterExtractor", QString("Character '%1' (normalized from '%2') already exists, merging...")
            .arg(normalizedName).arg(character.name()));
        Character merged = mergeCharacters(existing, extracted);
        return updateCharacter(merged);
    }
    
    // 再尝试按原始名称匹配一次
    existing = getCharacterByName(novelId, character.name());
    if (!existing.id().isEmpty()) {
        LOG_DEBUG("CharacterExtractor", QString("Character '%1' already exists (exact match), merging...")
            .arg(character.name()));
        Character merged = mergeCharacters(existing, extracted);
        return updateCharacter(merged);
    }
    
    // Normalize the character name before saving.
    if (!normalizedName.isEmpty() && normalizedName != character.name()) {
        LOG_INFO("CharacterExtractor", QString("Saving new character with normalized name: '%1' -> '%2'")
            .arg(character.name()).arg(normalizedName));
        character.setName(normalizedName);
    }
    
    QVariantMap data = characterToData(character);
    
    qint64 result = DatabaseManager::instance()->insert("characters", data);
    
    if (result > 0) {
        LOG_INFO("CharacterExtractor", QString("Saved new character: %1").arg(character.name()));
        BibleCache::instance()->invalidateCharacters(novelId);
        emit characterSaved(character.id());
    } else {
        LOG_ERROR("CharacterExtractor", QString("Failed to save character: %1").arg(character.name()));
    }
    
    return result > 0;
}
// Merge extracted character data with an existing character record.
Character CharacterExtractor::mergeCharacters(const Character& existing, const ExtractedCharacter& incoming) const
{
    Character merged = existing;
    CharacterAppearance app = merged.appearance();

    if (merged.role().isEmpty() && !incoming.tags.isEmpty()) {
        static const QStringList roleOrder = {
            "protagonist",
            "antagonist",
            "supporting",
            "background",
            "mentor",
            "love-interest"
        };

        for (const QString& role : roleOrder) {
            if (incoming.tags.contains(role, Qt::CaseInsensitive)) {
                merged.setRole(role);
                break;
            }
        }

        if (merged.role().isEmpty()) {
            merged.setRole(incoming.tags.first());
        }
    }

    if (app.gender.isEmpty() && !incoming.gender.isEmpty()) {
        app.gender = incoming.gender;
    }
    if (app.age == 0 && incoming.age > 0) {
        app.age = incoming.age;
    }
    if (app.hairColor.isEmpty() && !incoming.hairColor.isEmpty()) {
        app.hairColor = incoming.hairColor;
    }
    if (app.hairStyle.isEmpty() && !incoming.hairStyle.isEmpty()) {
        app.hairStyle = incoming.hairStyle;
    }
    if (app.eyeColor.isEmpty() && !incoming.eyeColor.isEmpty()) {
        app.eyeColor = incoming.eyeColor;
    }
    if (app.build.isEmpty() && !incoming.bodyType.isEmpty()) {
        app.build = incoming.bodyType;
    }

    if (!incoming.clothing.isEmpty()) {
        app.clothing = BibleUtils::mergeStringLists(app.clothing, incoming.clothing);
    }
    if (!incoming.accessories.isEmpty()) {
        app.accessories = BibleUtils::mergeStringLists(app.accessories, incoming.accessories);
    }
    if (!incoming.distinctiveFeatures.isEmpty()) {
        app.distinctiveFeatures = BibleUtils::mergeStringLists(app.distinctiveFeatures, incoming.distinctiveFeatures);
    }

    merged.setAppearance(app);

    if (!incoming.personality.isEmpty()) {
        merged.setPersonality(BibleUtils::mergeStringLists(merged.personality(), incoming.personality));
    }

    return merged;
}

QVariantMap CharacterExtractor::characterToData(const Character& character) const
{
    return character.toVariantMap();
}

int CharacterExtractor::saveCharacters(const QString& novelId, const QList<ExtractedCharacter>& characters)
{
    int savedCount = 0;
    
    for (const ExtractedCharacter& extracted : characters) {
        if (saveCharacter(novelId, extracted)) {
            savedCount++;
        }
    }
    
    LOG_INFO("CharacterExtractor", QString("Saved %1/%2 characters for novel %3")
        .arg(savedCount).arg(characters.size()).arg(novelId));
    
    return savedCount;
}

Character CharacterExtractor::characterFromRow(const QVariantMap& row)
{
    return Character::fromVariantMap(row);
}

QList<Character> CharacterExtractor::getCharactersByNovel(const QString& novelId)
{
    if (!DatabaseManager::instance()->ensureSoftDeleteColumns("characters")) {
        LOG_WARNING("CharacterExtractor", "Failed to ensure character soft-delete columns");
    }

    if (BibleCache::instance()->hasCharacters(novelId)) {
        return BibleCache::instance()->getCharacters(novelId);
    }
    
    QList<Character> characters;
    QList<QVariantMap> results = DatabaseManager::instance()->selectAll(
        "characters", "novel_id = ? AND is_deleted = 0", QVariantList() << novelId);
    
    for (const QVariantMap& row : results) {
        characters.append(characterFromRow(row));
    }
    
    BibleCache::instance()->setCharacters(novelId, characters);
    
    return characters;
}

Character CharacterExtractor::getCharacterById(const QString& characterId)
{
    if (!DatabaseManager::instance()->ensureSoftDeleteColumns("characters")) {
        LOG_WARNING("CharacterExtractor", "Failed to ensure character soft-delete columns");
    }
    
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "characters", "id = ? AND is_deleted = 0", QVariantList() << characterId);
    
    return row.isEmpty() ? Character() : characterFromRow(row);
}

Character CharacterExtractor::getCharacterByName(const QString& novelId, const QString& name)
{
    if (!DatabaseManager::instance()->ensureSoftDeleteColumns("characters")) {
        LOG_WARNING("CharacterExtractor", "Failed to ensure character soft-delete columns");
    }
    
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "characters", "novel_id = ? AND name = ? AND is_deleted = 0", QVariantList() << novelId << name);
    
    return row.isEmpty() ? Character() : characterFromRow(row);
}

bool CharacterExtractor::updateCharacter(const Character& character)
{
    QVariantMap data = character.toVariantMap();
    data.remove("id");
    data.remove("novel_id");
    
    bool success = DatabaseManager::instance()->update(
        "characters", data, "id = ?", QVariantList() << character.id());
    
    if (success) {
        LOG_INFO("CharacterExtractor", QString("Updated character: %1").arg(character.name()));
        BibleCache::instance()->invalidateCharacters(character.novelId());
        emit characterUpdated(character.id(), character.portraitPath());
    }
    
    return success;
}

bool CharacterExtractor::deleteCharacter(const QString& characterId)
{
    if (!DatabaseManager::instance()->ensureSoftDeleteColumns("characters")) {
        LOG_WARNING("CharacterExtractor", "Failed to ensure character soft-delete columns");
    }
    
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "characters", "id = ?", QVariantList() << characterId);
    if (row.isEmpty()) {
        return false;
    }
    
    QVariantMap data;
    data["is_deleted"] = 1;
    data["deleted_at"] = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss");
    
    bool success = DatabaseManager::instance()->update(
        "characters", data, "id = ?", QVariantList() << characterId);
    
    if (success) {
        LOG_INFO("CharacterExtractor", QString("Soft-deleted character: %1").arg(characterId));
        BibleCache::instance()->invalidateCharacters(row["novel_id"].toString());
    }
    
    return success;
}
