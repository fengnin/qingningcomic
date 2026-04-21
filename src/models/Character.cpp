
#include "models/Character.h"
#include "utils/JsonUtils.h"
#include <QJsonDocument>

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
    
    QVariant appearanceVariant = map["appearance"];
    qDebug() << "[Character] name:" << c.m_name 
             << ", appearance type:" << appearanceVariant.typeName() 
             << ", isNull:" << appearanceVariant.isNull() 
             << ", isValid:" << appearanceVariant.isValid()
             << ", canConvert<QString>:" << appearanceVariant.canConvert<QString>();
    
    if (appearanceVariant.isValid() && !appearanceVariant.isNull()) {
        QString appearanceStr;
        if (appearanceVariant.type() == QVariant::ByteArray) {
            appearanceStr = QString::fromUtf8(appearanceVariant.toByteArray());
            qDebug() << "[Character] appearance is ByteArray, converted to:" << appearanceStr.left(200);
        } else if (appearanceVariant.type() == QVariant::String) {
            appearanceStr = appearanceVariant.toString();
            qDebug() << "[Character] appearance is String:" << appearanceStr.left(200);
        } else {
            appearanceStr = appearanceVariant.toString();
            qDebug() << "[Character] appearance is other type, toString:" << appearanceStr.left(200);
        }
        
        if (!appearanceStr.isEmpty() && appearanceStr != "{}" && appearanceStr != "null") {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(appearanceStr.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject appearanceJson = doc.object();
                qDebug() << "[Character] Parsed appearance JSON:" << appearanceJson;
                c.m_appearance = CharacterAppearance::fromJson(appearanceJson);
            } else {
                qDebug() << "[Character] JSON parse error:" << parseError.errorString();
            }
        }
    }
    
    if (c.m_appearance.gender.isEmpty() && c.m_appearance.age == 0) {
        qDebug() << "[Character] Using fallback for appearance fields";
        c.m_appearance.gender = map["gender"].toString();
        c.m_appearance.age = map["age"].toInt();
    }
    
    QVariant personalitiesVariant = map["personalities"];
    qDebug() << "[Character] personalities type:" << personalitiesVariant.typeName()
             << ", isNull:" << personalitiesVariant.isNull()
             << ", isValid:" << personalitiesVariant.isValid();
    
    if (personalitiesVariant.isValid() && !personalitiesVariant.isNull()) {
        QString personalitiesStr;
        if (personalitiesVariant.type() == QVariant::ByteArray) {
            personalitiesStr = QString::fromUtf8(personalitiesVariant.toByteArray());
        } else {
            personalitiesStr = personalitiesVariant.toString();
        }
        
        if (!personalitiesStr.isEmpty() && personalitiesStr != "null") {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(personalitiesStr.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                c.m_personality = JsonUtils::jsonArrayToStringList(doc.array());
                qDebug() << "[Character] Parsed personalities:" << c.m_personality;
            }
        }
    }
    
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

QStringList Character::toDisplayStrings() const
{
    QStringList lines;
    
    auto addLine = [&lines](const QString& label, const QString& value) {
        if (!value.isEmpty()) {
            lines << QString::fromUtf8("%1: %2").arg(label, value);
        }
    };
    
    QStringList appearanceBits;
    if (!m_appearance.gender.isEmpty()) {
        appearanceBits << QString::fromUtf8("性别: %1").arg(m_appearance.gender);
    }
    if (m_appearance.age > 0) {
        appearanceBits << QString::fromUtf8("年龄: %1岁").arg(m_appearance.age);
    }
    if (!m_appearance.hairColor.isEmpty()) {
        appearanceBits << QString::fromUtf8("发色: %1").arg(m_appearance.hairColor);
    }
    if (!m_appearance.hairStyle.isEmpty()) {
        appearanceBits << QString::fromUtf8("发型: %1").arg(m_appearance.hairStyle);
    }
    if (!m_appearance.eyeColor.isEmpty()) {
        appearanceBits << QString::fromUtf8("瞳色: %1").arg(m_appearance.eyeColor);
    }
    if (!m_appearance.build.isEmpty()) {
        appearanceBits << QString::fromUtf8("体型: %1").arg(m_appearance.build);
    }
    if (!appearanceBits.isEmpty()) {
        lines << QString::fromUtf8("外观: %1").arg(appearanceBits.join(QString::fromUtf8("、")));
    }
    
    if (!m_appearance.clothing.isEmpty()) {
        lines << QString::fromUtf8("服饰: %1").arg(m_appearance.clothing.join(QString::fromUtf8("、")));
    }
    if (!m_appearance.distinctiveFeatures.isEmpty()) {
        lines << QString::fromUtf8("明显特征: %1").arg(m_appearance.distinctiveFeatures.join(QString::fromUtf8("、")));
    }
    if (!m_personality.isEmpty()) {
        lines << QString::fromUtf8("个性: %1").arg(m_personality.join(QString::fromUtf8("、")));
    }
    
    return lines;
}
