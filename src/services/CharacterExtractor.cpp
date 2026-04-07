#include "CharacterExtractor.h"
#include "BibleCache.h"
#include "DatabaseManager.h"
#include "Logger.h"
#include "utils/JsonUtils.h"
#include <QUuid>

CharacterExtractor* CharacterExtractor::m_instance = nullptr;

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
    json["clothing"] = clothing;
    json["features"] = features;
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
    c.clothing = json["clothing"].toString();
    c.features = json["features"].toString();
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
    if (!m_instance) {
        m_instance = new CharacterExtractor();
    }
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
                extracted.features = charStr.mid(parenPos + 1, endParen - parenPos - 1);
            }
        } else {
            extracted.name = charStr.trimmed();
        }
    } else if (charVal.isObject()) {
        QJsonObject charObj = charVal.toObject();
        extracted.name = charObj["name"].toString();
        extracted.features = charObj["expression"].toString();
        QString pose = charObj["pose"].toString();
        if (!pose.isEmpty()) {
            if (!extracted.features.isEmpty()) {
                extracted.features += " ";
            }
            extracted.features += pose;
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
    extracted.clothing = JsonUtils::jsonArrayToStringList(appObj["clothing"].toArray()).join(", ");
    extracted.features = JsonUtils::jsonArrayToStringList(appObj["distinctiveFeatures"].toArray()).join(", ");
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
    app.clothing = extracted.clothing.isEmpty() ? QStringList() : extracted.clothing.split(", ");
    app.distinctiveFeatures = extracted.features.isEmpty() ? QStringList() : extracted.features.split(", ");
    
    character.setAppearance(app);
    character.setPersonality(extracted.personality);
    
    return character;
}

bool CharacterExtractor::containsChinese(const QString& text)
{
    return text.contains(QRegularExpression("\\p{Han}"));
}

QString CharacterExtractor::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool CharacterExtractor::saveCharacter(const QString& novelId, const ExtractedCharacter& extracted)
{
    Character character = toCharacter(extracted, novelId);
    
    // 只检查精确匹配（和原仓库保持一致）
    Character existing = getCharacterByName(novelId, character.name());
    if (!existing.id().isEmpty()) {
        LOG_DEBUG("CharacterExtractor", QString("Character '%1' already exists, merging...").arg(character.name()));
        Character merged = mergeCharacters(existing, extracted);
        return updateCharacter(merged);
    }
    
    // 新角色，直接插入
    QVariantMap data = characterToData(character);
    
    qint64 result = DatabaseManager::instance()->insert("characters", data);
    
    if (result > 0) {
        LOG_INFO("CharacterExtractor", QString("Saved new character: %1").arg(character.name()));
        emit characterSaved(character.id());
    } else {
        LOG_ERROR("CharacterExtractor", QString("Failed to save character: %1").arg(character.name()));
    }
    
    return result > 0;
}

// 合并两个角色的属性（完全按照原仓库 bible-manager.js 的 mergeCharacters 逻辑）
Character CharacterExtractor::mergeCharacters(const Character& existing, const ExtractedCharacter& incoming) const
{
    Character merged = existing;
    
    // 1. role: 只填充空值
    if (merged.role().isEmpty() && !incoming.tags.isEmpty()) {
        // 从 tags 中推断 role
        if (incoming.tags.contains("protagonist") || incoming.tags.contains("主角")) {
            merged.setRole("protagonist");
        } else if (incoming.tags.contains("antagonist") || incoming.tags.contains("反派")) {
            merged.setRole("antagonist");
        } else if (!incoming.tags.isEmpty()) {
            merged.setRole(incoming.tags.first());
        }
    }
    
    // 2. appearance: 对象深度合并（只填充空值，数组合并去重）
    CharacterAppearance app = merged.appearance();
    
    // 只填充空值
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
    
    // 数组字段：合并去重
    if (!incoming.clothing.isEmpty()) {
        QStringList clothingList = incoming.clothing.split(", ");
        app.clothing = mergeStringLists(app.clothing, clothingList);
    }
    if (!incoming.features.isEmpty()) {
        QStringList featureList = incoming.features.split(", ");
        app.distinctiveFeatures = mergeStringLists(app.distinctiveFeatures, featureList);
    }
    
    merged.setAppearance(app);
    
    // 3. personality: 数组合并去重
    QStringList mergedPersonality = mergeStringLists(merged.personality(), incoming.personality);
    merged.setPersonality(mergedPersonality);
    
    return merged;
}

// 合并两个字符串列表并去重
QStringList CharacterExtractor::mergeStringLists(const QStringList& a, const QStringList& b) const
{
    QStringList result = a;
    for (const QString& item : b) {
        if (!result.contains(item)) {
            result.append(item);
        }
    }
    return result;
}

QVariantMap CharacterExtractor::characterToData(const Character& character) const
{
    QVariantMap data;
    data["id"] = character.id();
    data["novel_id"] = character.novelId();
    data["name"] = character.name();
    data["role"] = character.role();
    
    CharacterAppearance app = character.appearance();
    data["gender"] = app.gender;
    data["age"] = app.age;
    
    QJsonObject appearanceJson;
    appearanceJson["gender"] = app.gender;
    appearanceJson["age"] = app.age;
    appearanceJson["hairColor"] = app.hairColor;
    appearanceJson["hairStyle"] = app.hairStyle;
    appearanceJson["eyeColor"] = app.eyeColor;
    appearanceJson["height"] = app.height;
    appearanceJson["build"] = app.build;
    appearanceJson["clothing"] = JsonUtils::stringListToJsonArray(app.clothing);
    appearanceJson["distinctiveFeatures"] = JsonUtils::stringListToJsonArray(app.distinctiveFeatures);
    data["appearance"] = JsonUtils::jsonToString(appearanceJson);
    
    data["personalities"] = JsonUtils::jsonToString(JsonUtils::stringListToJsonArray(character.personality()));
    
    // 添加肖像图片路径
    if (!character.portraitPath().isEmpty()) {
        data["portrait_path"] = character.portraitPath();
    }
    
    return data;
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
    Character character;
    character.setId(row["id"].toString());
    character.setNovelId(row["novel_id"].toString());
    character.setName(row["name"].toString());
    character.setRole(row["role"].toString());
    
    CharacterAppearance app;
    app.gender = row["gender"].toString();
    app.age = row["age"].toInt();
    
    QJsonObject appearanceJson = JsonUtils::variantToJson(row["appearance"]);
    if (!appearanceJson.isEmpty()) {
        app.hairColor = appearanceJson["hairColor"].toString();
        app.hairStyle = appearanceJson["hairStyle"].toString();
        app.eyeColor = appearanceJson["eyeColor"].toString();
        app.height = appearanceJson["height"].toString();
        app.build = appearanceJson["build"].toString();
        app.clothing = JsonUtils::jsonArrayToStringList(appearanceJson["clothing"].toArray());
        app.distinctiveFeatures = JsonUtils::jsonArrayToStringList(appearanceJson["distinctiveFeatures"].toArray());
    }
    character.setAppearance(app);
    
    character.setPersonality(JsonUtils::variantToStringList(row["personalities"]));
    
    // 读取肖像图片路径
    character.setPortraitPath(row["portrait_path"].toString());
    
    return character;
}

QList<Character> CharacterExtractor::getCharactersByNovel(const QString& novelId)
{
    if (BibleCache::instance()->hasCharacters(novelId)) {
        return BibleCache::instance()->getCharacters(novelId);
    }
    
    QList<Character> characters;
    QList<QVariantMap> results = DatabaseManager::instance()->selectAll(
        "characters", "novel_id = ?", QVariantList() << novelId);
    
    for (const QVariantMap& row : results) {
        characters.append(characterFromRow(row));
    }
    
    BibleCache::instance()->setCharacters(novelId, characters);
    
    return characters;
}

Character CharacterExtractor::getCharacterById(const QString& characterId)
{
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "characters", "id = ?", QVariantList() << characterId);
    
    return row.isEmpty() ? Character() : characterFromRow(row);
}

Character CharacterExtractor::getCharacterByName(const QString& novelId, const QString& name)
{
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "characters", "novel_id = ? AND name = ?", QVariantList() << novelId << name);
    
    return row.isEmpty() ? Character() : characterFromRow(row);
}

bool CharacterExtractor::updateCharacter(const Character& character)
{
    QVariantMap data;
    data["name"] = character.name();
    data["role"] = character.role();
    
    CharacterAppearance app = character.appearance();
    data["gender"] = app.gender;
    data["age"] = app.age;
    
    QJsonObject appearanceJson;
    appearanceJson["gender"] = app.gender;
    appearanceJson["age"] = app.age;
    appearanceJson["hairColor"] = app.hairColor;
    appearanceJson["hairStyle"] = app.hairStyle;
    appearanceJson["eyeColor"] = app.eyeColor;
    appearanceJson["height"] = app.height;
    appearanceJson["build"] = app.build;
    appearanceJson["clothing"] = JsonUtils::stringListToJsonArray(app.clothing);
    appearanceJson["distinctiveFeatures"] = JsonUtils::stringListToJsonArray(app.distinctiveFeatures);
    data["appearance"] = JsonUtils::jsonToString(appearanceJson);
    
    data["personalities"] = JsonUtils::jsonToString(JsonUtils::stringListToJsonArray(character.personality()));
    
    // 保存肖像图片路径（包括清空的情况）
    data["portrait_path"] = character.portraitPath();
    
    bool success = DatabaseManager::instance()->update(
        "characters", data, "id = ?", QVariantList() << character.id());
    
    if (success) {
        LOG_INFO("CharacterExtractor", QString("Updated character: %1").arg(character.name()));
        
        // 更新缓存
        BibleCache::instance()->invalidateCharacters(character.novelId());
        
        // 发出信号通知 UI 更新
        emit characterUpdated(character.id(), character.portraitPath());
    }
    
    return success;
}

bool CharacterExtractor::deleteCharacter(const QString& characterId)
{
    bool success = DatabaseManager::instance()->remove(
        "characters", "id = ?", QVariantList() << characterId);
    
    if (success) {
        LOG_INFO("CharacterExtractor", QString("Deleted character: %1").arg(characterId));
    }
    
    return success;
}
