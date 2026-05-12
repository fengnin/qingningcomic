
#include "models/Character.h"
#include "utils/JsonUtils.h"
#include "utils/BibleUtils.h"

Character::Character()
{
}

// 注册元类型
static const int s_characterMetaType = qRegisterMetaType<Character>();
static const int s_appearanceMetaType = qRegisterMetaType<CharacterAppearance>();
static const int s_configMetaType = qRegisterMetaType<CharacterConfiguration>();

namespace {

QJsonObject readJsonObjectVariant(const QVariant& value)
{
    return JsonUtils::variantToJson(value);
}

QStringList readStringListVariant(const QVariant& value)
{
    return JsonUtils::variantToStringList(value);
}

void applyTopLevelAppearanceFallback(CharacterAppearance& appearance, const QVariantMap& map)
{
    if (appearance.gender.isEmpty()) {
        appearance.gender = map["gender"].toString();
    }

    if (appearance.age <= 0) {
        const int mappedAge = map["age"].toInt();
        if (mappedAge > 0) {
            appearance.age = mappedAge;
        }
    }
}

} // namespace

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
    json["aliases"] = JsonUtils::stringListToJsonArray(aliases);
    if (!fieldSources.isEmpty()) {
        json["fieldSources"] = fieldSources;
    }
    json["fieldSourcePolicyVersion"] = BibleUtils::BibleUpdateStrategy::fieldSourcePolicyVersion();
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
    app.aliases = JsonUtils::jsonArrayToStringList(json["aliases"].toArray());
    app.fieldSources = json["fieldSources"].toObject();
    const int sourcePolicyVersion = json.value(QStringLiteral("fieldSourcePolicyVersion")).toInt(0);
    if (sourcePolicyVersion < BibleUtils::BibleUpdateStrategy::fieldSourcePolicyVersion()) {
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("gender"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("age"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("hairColor"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("hairStyle"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("eyeColor"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("height"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("build"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("clothing"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("accessories"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("distinctiveFeatures"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(app.fieldSources, QStringLiteral("aliases"));
    }
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
    if (!aliases().isEmpty()) {
        map["aliases"] = JsonUtils::jsonToString(JsonUtils::stringListToJsonArray(aliases()));
    }
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

    const QVariant appearanceVariant = map["appearance"];
    if (appearanceVariant.isValid() && !appearanceVariant.isNull()) {
        c.m_appearance = CharacterAppearance::fromJson(readJsonObjectVariant(appearanceVariant));
    }
    applyTopLevelAppearanceFallback(c.m_appearance, map);

    c.m_aliases = readStringListVariant(map["aliases"]);
    if (c.m_aliases.isEmpty()) {
        c.m_aliases = c.m_appearance.aliases;
    }
    c.m_appearance.aliases = c.m_aliases;

    c.m_personality = readStringListVariant(map["personalities"]);
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
    json["aliases"] = JsonUtils::stringListToJsonArray(aliases());
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
    c.m_aliases = JsonUtils::jsonArrayToStringList(json["aliases"].toArray());
    if (c.m_aliases.isEmpty()) {
        c.m_aliases = c.m_appearance.aliases;
    }
    c.m_appearance.aliases = c.m_aliases;
    c.m_personality = JsonUtils::jsonArrayToStringList(json["personality"].toArray());
    c.m_portraitPath = json["portraitPath"].toString();
    return c;
}

QStringList Character::toDisplayStrings() const
{
    QStringList lines;

    auto addLine = [&lines](const QString& label, const QString& value) {
        lines << QString::fromUtf8("%1: %2").arg(label, value);
    };

    addLine(QString::fromUtf8("性别"), m_appearance.gender);
    addLine(QString::fromUtf8("年龄"), m_appearance.age > 0 ? QString::fromUtf8("%1岁").arg(m_appearance.age) : QString());
    addLine(QString::fromUtf8("发色"), m_appearance.hairColor);
    addLine(QString::fromUtf8("发型"), m_appearance.hairStyle);
    addLine(QString::fromUtf8("瞳色"), m_appearance.eyeColor);
    addLine(QString::fromUtf8("体型"), m_appearance.build);
    addLine(QString::fromUtf8("服饰"), m_appearance.clothing.join(QString::fromUtf8("、")));
    addLine(QString::fromUtf8("明显特征"), m_appearance.distinctiveFeatures.join(QString::fromUtf8("、")));
    addLine(QString::fromUtf8("个性"), m_personality.join(QString::fromUtf8("、")));
    
    return lines;
}

#include "moc_Character.cpp"
