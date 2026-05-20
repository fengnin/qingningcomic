#include "models/Scene.h"
#include "utils/BibleUtils.h"

namespace {

void setIfNotEmpty(QJsonObject& json, const QString& key, const QString& value)
{
    if (!value.isEmpty()) {
        json[key] = value;
    }
}

void setIfNotEmpty(QJsonObject& json, const QString& key, const QStringList& values)
{
    if (!values.isEmpty()) {
        json[key] = QJsonArray::fromStringList(values);
    }
}

void setIfNotEmpty(QJsonObject& json, const QString& key, const QJsonObject& value)
{
    if (!value.isEmpty()) {
        json[key] = value;
    }
}

void setIfNotEmpty(QJsonObject& json, const QString& key, const QJsonArray& value)
{
    if (!value.isEmpty()) {
        json[key] = value;
    }
}

QStringList readStringList(const QJsonObject& json, const QString& key)
{
    return JsonUtils::jsonArrayToStringList(json.value(key).toArray());
}

QString translateSceneStatus(const QString& status)
{
    static const QMap<QString, QString> map = {
        {"unknown", QString::fromUtf8("未知")},
        {"inferred", QString::fromUtf8("推断中")},
        {"confirmed", QString::fromUtf8("已确认")},
        {"merged", QString::fromUtf8("已合并")}
    };
    return map.value(status, status);
}

QString translateConfidence(const QString& confidence)
{
    static const QMap<QString, QString> map = {
        {"low", QString::fromUtf8("低")},
        {"medium", QString::fromUtf8("中")},
        {"high", QString::fromUtf8("高")}
    };
    return map.value(confidence, confidence);
}

void appendDisplayLine(QStringList& lines, const QString& label, const QString& value)
{
    if (!value.isEmpty()) {
        lines << QString::fromUtf8("%1: %2").arg(label, value);
    }
}

} // namespace

QJsonObject SceneDetails::toJson() const
{
    QJsonObject json;
    setIfNotEmpty(json, QStringLiteral("description"), description);
    setIfNotEmpty(json, QStringLiteral("building"), building);
    setIfNotEmpty(json, QStringLiteral("color"), color);
    setIfNotEmpty(json, QStringLiteral("landmark"), landmark);
    setIfNotEmpty(json, QStringLiteral("layout"), layout);
    setIfNotEmpty(json, QStringLiteral("atmosphere"), atmosphere);
    setIfNotEmpty(json, QStringLiteral("anchorPoints"), anchorPoints);
    setIfNotEmpty(json, QStringLiteral("signatureObjects"), signatureObjects);
    setIfNotEmpty(json, QStringLiteral("fixedColorBlocks"), fixedColorBlocks);
    setIfNotEmpty(json, QStringLiteral("consistencyRules"), consistencyRules);
    setIfNotEmpty(json, QStringLiteral("type"), type);
    setIfNotEmpty(json, QStringLiteral("typeZh"), typeZh);
    setIfNotEmpty(json, QStringLiteral("setting"), setting);
    setIfNotEmpty(json, QStringLiteral("timeOfDay"), timeOfDay);
    setIfNotEmpty(json, QStringLiteral("weather"), weather);
    setIfNotEmpty(json, QStringLiteral("spaceSize"), spaceSize);
    setIfNotEmpty(json, QStringLiteral("currentInterpretation"), currentInterpretation);
    setIfNotEmpty(json, QStringLiteral("confidence"), confidence);
    setIfNotEmpty(json, QStringLiteral("status"), status);
    setIfNotEmpty(json, QStringLiteral("narrativeRole"), narrativeRole);
    setIfNotEmpty(json, QStringLiteral("narrativeRoleZh"), narrativeRoleZh);
    setIfNotEmpty(json, QStringLiteral("details"), details);
    setIfNotEmpty(json, QStringLiteral("evidence"), evidence);
    setIfNotEmpty(json, QStringLiteral("aliases"), aliases);
    setIfNotEmpty(json, QStringLiteral("history"), history);
    setIfNotEmpty(json, QStringLiteral("visualCharacteristics"), visualCharacteristics);
    setIfNotEmpty(json, QStringLiteral("spatialLayout"), spatialLayout);
    setIfNotEmpty(json, QStringLiteral("timeVariations"), timeVariations);
    setIfNotEmpty(json, QStringLiteral("weatherVariations"), weatherVariations);
    setIfNotEmpty(json, QStringLiteral("fieldSources"), fieldSources);
    json["fieldSourcePolicyVersion"] = BibleUtils::BibleUpdateStrategy::fieldSourcePolicyVersion();
    return json;
}

SceneDetails SceneDetails::fromJson(const QJsonObject& json)
{
    SceneDetails det;
    det.description = json.value(QStringLiteral("description")).toString();
    det.building = json.value(QStringLiteral("building")).toString();
    det.color = json.value(QStringLiteral("color")).toString();
    det.landmark = json.value(QStringLiteral("landmark")).toString();
    det.layout = json.value(QStringLiteral("layout")).toString();
    det.atmosphere = json.value(QStringLiteral("atmosphere")).toString();
    det.anchorPoints = readStringList(json, QStringLiteral("anchorPoints"));
    det.signatureObjects = readStringList(json, QStringLiteral("signatureObjects"));
    det.fixedColorBlocks = readStringList(json, QStringLiteral("fixedColorBlocks"));
    det.consistencyRules = readStringList(json, QStringLiteral("consistencyRules"));
    det.type = json.value(QStringLiteral("type")).toString();
    det.typeZh = json.value(QStringLiteral("typeZh")).toString();
    det.setting = json.value(QStringLiteral("setting")).toString();
    det.timeOfDay = json.value(QStringLiteral("timeOfDay")).toString();
    det.weather = json.value(QStringLiteral("weather")).toString();
    det.spaceSize = json.value(QStringLiteral("spaceSize")).toString();
    det.currentInterpretation = json.value(QStringLiteral("currentInterpretation")).toString();
    det.confidence = json.value(QStringLiteral("confidence")).toString();
    det.status = json.value(QStringLiteral("status")).toString();
    det.narrativeRole = json.value(QStringLiteral("narrativeRole")).toString();
    det.narrativeRoleZh = json.value(QStringLiteral("narrativeRoleZh")).toString();
    det.evidence = readStringList(json, QStringLiteral("evidence"));
    det.aliases = readStringList(json, QStringLiteral("aliases"));
    det.history = readStringList(json, QStringLiteral("history"));
    if (isIndoorSceneType(det.type)) {
        det.weather.clear();
        det.weatherVariations = QJsonArray();
    }
    det.details = readStringList(json, QStringLiteral("details"));
    det.visualCharacteristics = json.value(QStringLiteral("visualCharacteristics")).toObject();
    det.spatialLayout = json.value(QStringLiteral("spatialLayout")).toObject();
    det.timeVariations = json.value(QStringLiteral("timeVariations")).toArray();
    det.weatherVariations = json.value(QStringLiteral("weatherVariations")).toArray();
    det.fieldSources = json.value(QStringLiteral("fieldSources")).toObject();
    const int sourcePolicyVersion = json.value(QStringLiteral("fieldSourcePolicyVersion")).toInt(0);
    if (sourcePolicyVersion < BibleUtils::BibleUpdateStrategy::fieldSourcePolicyVersion()) {
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("description"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("building"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("color"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("landmark"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("layout"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("atmosphere"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("anchorPoints"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("signatureObjects"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("fixedColorBlocks"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("consistencyRules"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("type"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("typeZh"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("setting"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("timeOfDay"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("weather"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("spaceSize"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("currentInterpretation"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("confidence"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("status"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("narrativeRole"));
        BibleUtils::BibleUpdateStrategy::downgradeLegacyFieldSource(det.fieldSources, QStringLiteral("narrativeRoleZh"));
    }
    return det;
}

bool SceneDetails::hasEmptyFields() const
{
    const bool indoorScene = isIndoorSceneType(type);
    return description.isEmpty() || building.isEmpty() || color.isEmpty() ||
           landmark.isEmpty() || layout.isEmpty() || atmosphere.isEmpty() ||
           anchorPoints.isEmpty() || signatureObjects.isEmpty() || fixedColorBlocks.isEmpty() ||
           consistencyRules.isEmpty() ||
           spaceSize.isEmpty() || type.isEmpty() || setting.isEmpty() || timeOfDay.isEmpty() ||
           currentInterpretation.isEmpty() || confidence.isEmpty() || status.isEmpty() ||
           evidence.isEmpty() || aliases.isEmpty() || history.isEmpty() ||
           narrativeRole.isEmpty() ||
           (!indoorScene && weather.isEmpty());
}

QStringList SceneDetails::toDisplayStrings() const
{
    using namespace BibleUtils::Inference;

    QStringList lines;

    // 基础信息
    appendDisplayLine(lines, QString::fromUtf8("类型"), preferZh(typeZh, type, translateSceneType));
    appendDisplayLine(lines, QString::fromUtf8("描述"), description);

    // 视觉概览（聚合展示）
    QStringList visualBits;
    if (!building.isEmpty()) visualBits << building;
    if (!landmark.isEmpty()) visualBits << landmark;
    if (!color.isEmpty()) visualBits << color;
    if (!layout.isEmpty()) visualBits << layout;
    if (!atmosphere.isEmpty()) visualBits << atmosphere;
    if (!visualBits.isEmpty()) {
        appendDisplayLine(lines, QString::fromUtf8("视觉概览"), visualBits.join(QString::fromUtf8("；")));
    }

    // 视觉要素（逐项展示）
    appendDisplayLine(lines, QString::fromUtf8("建筑"), building);
    appendDisplayLine(lines, QString::fromUtf8("色调"), color);
    appendDisplayLine(lines, QString::fromUtf8("地标"), landmark);
    appendDisplayLine(lines, QString::fromUtf8("布局"), layout);
    appendDisplayLine(lines, QString::fromUtf8("氛围"), atmosphere);

    // 环境参数
    appendDisplayLine(lines, QString::fromUtf8("背景设定"), setting);
    if (!isIndoorSceneType(type)) {
        appendDisplayLine(lines, QString::fromUtf8("天气"), preferZh(QString(), weather, translateWeather));
    }
    appendDisplayLine(lines, QString::fromUtf8("时间段"), preferZh(QString(), timeOfDay, translateTimeOfDay));
    appendDisplayLine(lines, QString::fromUtf8("叙事角色"), preferZh(narrativeRoleZh, narrativeRole, translateNarrativeRole));

    // AI元数据（测试用，显示所有字段）
    if (!currentInterpretation.isEmpty()) {
        appendDisplayLine(lines, QString::fromUtf8("当前解释"), currentInterpretation);
    }

    QStringList iterationBits;
    if (!status.isEmpty()) iterationBits << QString::fromUtf8("状态: %1").arg(translateSceneStatus(status));
    if (!confidence.isEmpty()) iterationBits << QString::fromUtf8("置信度: %1").arg(translateConfidence(confidence));
    if (!iterationBits.isEmpty()) {
        appendDisplayLine(lines, QString::fromUtf8("迭代"), iterationBits.join(QString::fromUtf8("；")));
    }

    if (!evidence.isEmpty()) {
        appendDisplayLine(lines, QString::fromUtf8("证据"), evidence.join(QString::fromUtf8("；")));
    }

    if (!aliases.isEmpty()) {
        appendDisplayLine(lines, QString::fromUtf8("别名"), aliases.join(QString::fromUtf8("；")));
    }

    QStringList anchorBits;
    if (!anchorPoints.isEmpty()) anchorBits << anchorPoints.join(QString::fromUtf8("；"));
    if (!signatureObjects.isEmpty()) anchorBits << signatureObjects.join(QString::fromUtf8("；"));
    if (!fixedColorBlocks.isEmpty()) anchorBits << fixedColorBlocks.join(QString::fromUtf8("；"));
    if (!anchorBits.isEmpty()) {
        appendDisplayLine(lines, QString::fromUtf8("锚点"), anchorBits.join(QString::fromUtf8("；")));
    }

    if (!consistencyRules.isEmpty()) {
        appendDisplayLine(lines, QString::fromUtf8("一致性"), consistencyRules.join(QString::fromUtf8("；")));
    }

    if (!history.isEmpty()) {
        appendDisplayLine(lines, QString::fromUtf8("历史"), history.join(QString::fromUtf8("；")));
    }

    return lines;
}

QJsonObject Scene::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["novelId"] = m_novelId;
    json["name"] = m_name;
    json["sceneId"] = m_sceneId;
    json["details"] = m_details.toJson();
    json["tags"] = QJsonArray::fromStringList(m_tags);
    json["referenceImagePath"] = m_referenceImagePath;
    return json;
}

Scene Scene::fromJson(const QJsonObject& json)
{
    Scene scene;
    scene.m_id = json["id"].toString();
    scene.m_novelId = json["novelId"].toString();
    scene.m_name = json["name"].toString();
    scene.m_sceneId = json["sceneId"].toString();
    scene.m_details = SceneDetails::fromJson(json["details"].toObject());
    scene.m_tags = JsonUtils::jsonArrayToStringList(json["tags"].toArray());
    scene.m_referenceImagePath = json["referenceImagePath"].toString();
    return scene;
}

QVariantMap Scene::toVariantMap() const
{
    QVariantMap map;
    map["id"] = m_id;
    map["novel_id"] = m_novelId;
    map["name"] = m_name;
    map["scene_id"] = m_sceneId;
    map["details"] = JsonUtils::jsonToString(m_details.toJson());
    map["tags"] = JsonUtils::jsonToString(QJsonArray::fromStringList(m_tags));
    map["reference_image_path"] = m_referenceImagePath;
    return map;
}

Scene Scene::fromVariantMap(const QVariantMap& map)
{
    Scene scene;
    scene.m_id = map["id"].toString();
    scene.m_novelId = map["novel_id"].toString();
    scene.m_name = map["name"].toString();
    scene.m_sceneId = map["scene_id"].toString();

    const QJsonObject detailsJson = JsonUtils::variantToJson(map["details"]);
    if (!detailsJson.isEmpty()) {
        scene.m_details = SceneDetails::fromJson(detailsJson);
    }

    scene.m_tags = JsonUtils::variantToStringList(map["tags"]);
    scene.m_referenceImagePath = map["reference_image_path"].toString();
    return scene;
}
