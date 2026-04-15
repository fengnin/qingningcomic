
#include "Character.h"
#include "utils/JsonUtils.h"

Character::Character()
{
}

// 注册元类型
static const int s_characterMetaType = qRegisterMetaType<Character>();
static const int s_appearanceMetaType = qRegisterMetaType<CharacterAppearance>();
static const int s_configMetaType = qRegisterMetaType<CharacterConfiguration>();

QJsonObject CharacterAppearance::toJson() const
{
    QJsonObject json;
    json["gender"] = gender;
    json["age"] = age;
    json["hairColor"] = hairColor;
    json["hairStyle"] = hairStyle;
    json["eyeColor"] = eyeColor;
    json["height"] = height;
    json["build"] = build;
    json["clothing"] = JsonUtils::stringListToJsonArray(clothing);
    json["accessories"] = JsonUtils::stringListToJsonArray(accessories);
    json["distinctiveFeatures"] = JsonUtils::stringListToJsonArray(distinctiveFeatures);
    return json;
}

CharacterAppearance CharacterAppearance::fromJson(const QJsonObject& json)
{
    CharacterAppearance app;
    app.gender = json["gender"].toString();
    app.age = json["age"].toInt();
    app.hairColor = json["hairColor"].toString();
    app.hairStyle = json["hairStyle"].toString();
    app.eyeColor = json["eyeColor"].toString();
    app.height = json["height"].toString();
    app.build = json["build"].toString();
    app.clothing = JsonUtils::jsonArrayToStringList(json["clothing"].toArray());
    app.accessories = JsonUtils::jsonArrayToStringList(json["accessories"].toArray());
    app.distinctiveFeatures = JsonUtils::jsonArrayToStringList(json["distinctiveFeatures"].toArray());
    return app;
}

QVariantMap Character::toVariantMap() const
{
    QVariantMap map;
    map["id"] = m_id;
    map["novel_id"] = m_novelId;
    map["name"] = m_name;
    map["role"] = m_role;
    map["gender"] = m_appearance.gender;
    map["age"] = m_appearance.age;
    map["appearance"] = JsonUtils::jsonToString(m_appearance.toJson());
    map["personalities"] = JsonUtils::jsonToString(JsonUtils::stringListToJsonArray(m_personality));
    if (!m_portraitPath.isEmpty()) {
        map["portrait_path"] = m_portraitPath;
    }
    return map;
}

Character Character::fromVariantMap(const QVariantMap& map)
{
    Character c;
    c.m_id = map["id"].toString();
    c.m_novelId = map["novel_id"].toString();
    c.m_name = map["name"].toString();
    c.m_role = map["role"].toString();
    
    QJsonObject appearanceJson = JsonUtils::variantToJson(map["appearance"]);
    if (!appearanceJson.isEmpty()) {
        c.m_appearance = CharacterAppearance::fromJson(appearanceJson);
    } else {
        c.m_appearance.gender = map["gender"].toString();
        c.m_appearance.age = map["age"].toInt();
    }
    
    c.m_personality = JsonUtils::variantToStringList(map["personalities"]);
    c.m_portraitPath = map["portrait_path"].toString();
    return c;
}

QJsonObject Character::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["novelId"] = m_novelId;
    json["name"] = m_name;
    json["role"] = m_role;
    json["appearance"] = m_appearance.toJson();
    json["personality"] = JsonUtils::stringListToJsonArray(m_personality);
    if (!m_portraitPath.isEmpty()) {
        json["portraitPath"] = m_portraitPath;
    }
    return json;
}

Character Character::fromJson(const QJsonObject& json)
{
    Character c;
    c.m_id = json["id"].toString();
    c.m_novelId = json["novelId"].toString();
    c.m_name = json["name"].toString();
    c.m_role = json["role"].toString();
    c.m_appearance = CharacterAppearance::fromJson(json["appearance"].toObject());
    c.m_personality = JsonUtils::jsonArrayToStringList(json["personality"].toArray());
    c.m_portraitPath = json["portraitPath"].toString();
    return c;
}
