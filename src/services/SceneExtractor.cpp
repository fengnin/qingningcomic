#include "services/SceneExtractor.h"
#include "models/Scene.h"
#include "data/DatabaseManager.h"
#include "utils/AnalysisFieldUtils.h"
#include "utils/Logger.h"
#include "utils/BibleCache.h"
#include "utils/BibleUtils.h"
#include "utils/SceneKeyUtils.h"
#include "utils/JsonUtils.h"
#include <QDateTime>
#include <QSet>
#include <QRegularExpression>
#include <QUuid>

namespace {
using namespace SceneKeyUtils;

QStringList splitSceneTokens(const QString& text);
QStringList collectSceneTokens(const QString& name, const QString& desc);
int calculateSceneSimilarity(const QString& name1, const QString& desc1,
                             const QString& name2, const QString& desc2);
bool shouldMergeScenes(const QString& name1, const QString& desc1,
                       const QString& name2, const QString& desc2, int threshold = 50);
QString inferSceneType(const QString& description, const QString& name);
QString inferTimeOfDay(const QString& text);
QString inferWeather(const QString& text);
template<typename KeywordMatcher>
QStringList buildConcreteSceneList(const ExtractedScene& scene,
                                   const QStringList& baseTokens,
                                   const QStringList& keywords,
                                   int limit,
                                   KeywordMatcher matcher)
{
    QStringList items = baseTokens;
    for (const QString& keyword : keywords) {
        if (matcher(scene, keyword)) {
            items << keyword;
        }
    }
    return deduplicateList(items).mid(0, limit);
}

const int SCENE_MERGE_THRESHOLD = 65;

bool isDrawableSceneToken(const QString& text)
{
    static const QStringList drawableTokens = {
        QString::fromUtf8("门"),
        QString::fromUtf8("窗"),
        QString::fromUtf8("墙"),
        QString::fromUtf8("地面"),
        QString::fromUtf8("地板"),
        QString::fromUtf8("屋顶"),
        QString::fromUtf8("瓦片"),
        QString::fromUtf8("栏杆"),
        QString::fromUtf8("桌"),
        QString::fromUtf8("椅"),
        QString::fromUtf8("床"),
        QString::fromUtf8("柜"),
        QString::fromUtf8("书架"),
        QString::fromUtf8("灯"),
        QString::fromUtf8("灯盏"),
        QString::fromUtf8("蜡烛"),
        QString::fromUtf8("煤油灯"),
        QString::fromUtf8("水桶"),
        QString::fromUtf8("灶台"),
        QString::fromUtf8("水缸"),
        QString::fromUtf8("石阶"),
        QString::fromUtf8("石板"),
        QString::fromUtf8("石块"),
        QString::fromUtf8("招牌"),
        QString::fromUtf8("旗帜"),
        QString::fromUtf8("牌位"),
        QString::fromUtf8("神像"),
        QString::fromUtf8("药瓶"),
        QString::fromUtf8("全家福"),
        QString::fromUtf8("帷幕"),
        QString::fromUtf8("射灯"),
        QString::fromUtf8("幕布"),
        QString::fromUtf8("雕塑"),
        QString::fromUtf8("石碑"),
        QString::fromUtf8("墓碑"),
        QString::fromUtf8("门框"),
        QString::fromUtf8("窗框"),
        QString::fromUtf8("屋檐")
    };

    return containsAnyToken(text, drawableTokens);
}

Scene findMatchingScene(const QList<Scene>& candidates, const Scene& incoming)
{
    for (const Scene& candidate : candidates) {
        if (!incoming.id().isEmpty() && !candidate.id().isEmpty() && incoming.id() == candidate.id()) {
            return candidate;
        }

        const QStringList existingKeys = SceneKeyUtils::buildSceneIdentityKeys(candidate);
        const QStringList incomingKeys = SceneKeyUtils::buildSceneIdentityKeys(incoming);
        for (const QString& key : incomingKeys) {
            if (existingKeys.contains(key, Qt::CaseInsensitive)) {
                return candidate;
            }
        }

        if (shouldMergeScenes(
                incoming.name(), incoming.details().description,
                candidate.name(), candidate.details().description, SCENE_MERGE_THRESHOLD)) {
            return candidate;
        }
    }

    return Scene();
}

bool appendExtractedScene(QList<ExtractedScene>& result, const ExtractedScene& scene)
{
    if (scene.name.isEmpty()) {
        return false;
    }
    result << scene;
    return true;
}

QStringList collectSceneTokens(const QString& name, const QString& desc)
{
    QStringList tokens = splitSceneTokens(name + QStringLiteral(" ") + desc);
    QStringList filtered;
    for (const QString& token : tokens) {
        const QString key = buildSceneCoreKey(token);
        if (key.isEmpty()) {
            continue;
        }
        if (!filtered.contains(key)) {
            filtered << key;
        }
    }
    return filtered;
}

int calculateSceneSimilarity(const QString& name1, const QString& desc1,
                             const QString& name2, const QString& desc2)
{
    const QString key1 = buildSceneCoreKey(name1 + QStringLiteral(" ") + desc1);
    const QString key2 = buildSceneCoreKey(name2 + QStringLiteral(" ") + desc2);

    if (!key1.isEmpty() && key1 == key2) {
        return 100;
    }

    int score = 0;
    if (!key1.isEmpty() && !key2.isEmpty()) {
        if (key1.contains(key2) || key2.contains(key1)) {
            score += 45;
        }
    }

    const QStringList tokens1 = collectSceneTokens(name1, desc1);
    const QStringList tokens2 = collectSceneTokens(name2, desc2);

    int shared = 0;
    for (const QString& token : tokens1) {
        if (tokens2.contains(token)) {
            shared++;
        }
    }

    score += qMin(shared * 20, 40);
    return qMin(score, 100);
}

bool shouldMergeScenes(const QString& name1, const QString& desc1,
                       const QString& name2, const QString& desc2, int threshold)
{
    return calculateSceneSimilarity(name1, desc1, name2, desc2) >= threshold;
}

QStringList splitSceneTokens(const QString& text)
{
    QStringList items = text.split(QRegularExpression("[,，;；、/\\n\\r。！？!?]+"), Qt::SkipEmptyParts);

    static const QStringList genericTokens = {
        QString::fromUtf8("氛围"),
        QString::fromUtf8("配色"),
        QString::fromUtf8("布局"),
        QString::fromUtf8("环境"),
        QString::fromUtf8("场景"),
        QString::fromUtf8("画面"),
        QString::fromUtf8("空间"),
        QString::fromUtf8("整体"),
        QString::fromUtf8("感觉"),
        QString::fromUtf8("风格")
    };

    static const QStringList actionTokens = {
        QString::fromUtf8("走"),
        QString::fromUtf8("跑"),
        QString::fromUtf8("哭"),
        QString::fromUtf8("喊"),
        QString::fromUtf8("叫"),
        QString::fromUtf8("听"),
        QString::fromUtf8("看"),
        QString::fromUtf8("说"),
        QString::fromUtf8("吞"),
        QString::fromUtf8("喝"),
        QString::fromUtf8("摔"),
        QString::fromUtf8("撞"),
        QString::fromUtf8("追"),
        QString::fromUtf8("惊"),
        QString::fromUtf8("僵硬"),
        QString::fromUtf8("沉默"),
        QString::fromUtf8("回忆"),
        QString::fromUtf8("痛"),
        QString::fromUtf8("恐惧")
    };

    QStringList filtered;
    for (QString item : items) {
        item = item.trimmed();
        if (item.isEmpty() || item.size() < 2) {
            continue;
        }
        if (BibleUtils::SceneTextFilter::isDynamicSceneText(item)) {
            continue;
        }
        bool generic = BibleUtils::Inference::containsAny(item, genericTokens);
        if (generic) {
            continue;
        }
        bool actionHeavy = BibleUtils::Inference::containsAny(item, actionTokens);
        if (actionHeavy) {
            continue;
        }
        if (!filtered.contains(item)) {
            filtered << item;
        }
    }

    return filtered;
}

QStringList extractStaticSceneTokens(const QString& text)
{
    QStringList tokens = splitSceneTokens(text);
    QStringList filtered = BibleUtils::SceneTextFilter::filterStaticSceneList(tokens);
    QStringList concrete;
    for (const QString& token : filtered) {
        const QString coreKey = buildSceneCoreKey(token);
        if (!coreKey.isEmpty()) {
            if (!concrete.contains(coreKey)) {
                concrete << coreKey;
            }
        } else if (isDrawableSceneToken(token) && !concrete.contains(token)) {
            concrete << token;
        }
    }
    return concrete;
}

QStringList buildConcreteAnchors(const ExtractedScene& scene)
{
    QStringList anchors;
    anchors << extractStaticSceneTokens(scene.landmark);
    anchors << extractStaticSceneTokens(scene.layout);
    anchors << extractStaticSceneTokens(scene.building);
    anchors << extractStaticSceneTokens(scene.description);
    anchors = deduplicateList(anchors);

    if (anchors.size() < 3) {
        if (!scene.name.isEmpty()) {
            anchors << QString::fromUtf8("场景名称锚点: %1").arg(scene.name);
        }
        if (!scene.layout.isEmpty()) {
            anchors << QString::fromUtf8("布局记忆: %1").arg(scene.layout);
        }
        if (!scene.landmark.isEmpty()) {
            anchors << QString::fromUtf8("可识别地标: %1").arg(scene.landmark);
        }
    }

    return anchors.mid(0, 5);
}

QStringList buildConcreteSignatureObjects(const ExtractedScene& scene)
{
    QStringList baseTokens = extractStaticSceneTokens(scene.landmark)
        + extractStaticSceneTokens(scene.description);

    static const QStringList objectKeywords = {
        QString::fromUtf8("门牌"),
        QString::fromUtf8("窗"),
        QString::fromUtf8("窗型"),
        QString::fromUtf8("桌"),
        QString::fromUtf8("椅"),
        QString::fromUtf8("床"),
        QString::fromUtf8("柜"),
        QString::fromUtf8("书架"),
        QString::fromUtf8("灯"),
        QString::fromUtf8("灯盏"),
        QString::fromUtf8("煤油灯"),
        QString::fromUtf8("蜡烛"),
        QString::fromUtf8("香炉"),
        QString::fromUtf8("神像"),
        QString::fromUtf8("牌位"),
        QString::fromUtf8("灶台"),
        QString::fromUtf8("水缸"),
        QString::fromUtf8("井"),
        QString::fromUtf8("石磨"),
        QString::fromUtf8("树"),
        QString::fromUtf8("老树"),
        QString::fromUtf8("石碑"),
        QString::fromUtf8("墓碑"),
        QString::fromUtf8("石像"),
        QString::fromUtf8("雕塑"),
        QString::fromUtf8("招牌"),
        QString::fromUtf8("旗帜"),
        QString::fromUtf8("幡")
    };

    return buildConcreteSceneList(
        scene,
        baseTokens,
        objectKeywords,
        5,
        [](const ExtractedScene& current, const QString& keyword) {
            return current.description.contains(keyword) || current.landmark.contains(keyword);
        });
}

QStringList buildConcreteColorBlocks(const ExtractedScene& scene)
{
    QStringList baseTokens = extractStaticSceneTokens(scene.color);

    static const QStringList colorKeywords = {
        QString::fromUtf8("红"),
        QString::fromUtf8("橙"),
        QString::fromUtf8("黄"),
        QString::fromUtf8("绿"),
        QString::fromUtf8("青"),
        QString::fromUtf8("蓝"),
        QString::fromUtf8("紫"),
        QString::fromUtf8("黑"),
        QString::fromUtf8("白"),
        QString::fromUtf8("灰"),
        QString::fromUtf8("金"),
        QString::fromUtf8("银"),
        QString::fromUtf8("褐"),
        QString::fromUtf8("棕"),
        QString::fromUtf8("暗红"),
        QString::fromUtf8("深蓝"),
        QString::fromUtf8("浅绿"),
        QString::fromUtf8("米白"),
        QString::fromUtf8("土黄")
    };

    return buildConcreteSceneList(
        scene,
        baseTokens,
        colorKeywords,
        4,
        [](const ExtractedScene& current, const QString& color) {
            return current.description.contains(color) || current.atmosphere.contains(color);
        });
}

QStringList buildConcreteConsistencyRules(const ExtractedScene& scene)
{
    QStringList rules;

    if (!scene.building.isEmpty()) {
        rules << QString::fromUtf8("建筑风格: %1").arg(scene.building);
    }
    if (!scene.layout.isEmpty()) {
        rules << QString::fromUtf8("空间布局: %1").arg(scene.layout);
    }
    if (!scene.atmosphere.isEmpty()) {
        rules << QString::fromUtf8("氛围基调: %1").arg(scene.atmosphere);
    }

    static const QStringList ruleKeywords = {
        QString::fromUtf8("古朴"),
        QString::fromUtf8("现代"),
        QString::fromUtf8("破旧"),
        QString::fromUtf8("整洁"),
        QString::fromUtf8("昏暗"),
        QString::fromUtf8("明亮"),
        QString::fromUtf8("阴森"),
        QString::fromUtf8("温馨"),
        QString::fromUtf8("冷清"),
        QString::fromUtf8("热闹"),
        QString::fromUtf8("神秘"),
        QString::fromUtf8("庄严")
    };

    QStringList keywordRules = buildConcreteSceneList(
        scene,
        QStringList(),
        ruleKeywords,
        4,
        [](const ExtractedScene& current, const QString& keyword) {
            return current.description.contains(keyword) || current.atmosphere.contains(keyword);
        });
    for (const QString& keyword : keywordRules) {
        rules << QString::fromUtf8("整体感觉: %1").arg(keyword);
    }

    return deduplicateList(rules).mid(0, 4);
}

void assignSceneStringField(ExtractedScene& scene, const QString& key, QString& field, const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    field = trimmed;
    scene.fieldSources[key] = QStringLiteral("inferred");
}

void assignSceneListField(ExtractedScene& scene, const QString& key, QStringList& field, const QStringList& values)
{
    const QStringList cleaned = deduplicateList(values);
    if (cleaned.isEmpty()) {
        return;
    }

    field = cleaned;
    scene.fieldSources[key] = QStringLiteral("inferred");
}

void assignSceneArrayField(ExtractedScene& scene, const QString& key, QJsonArray& field, const QJsonArray& values)
{
    if (values.isEmpty()) {
        return;
    }

    field = values;
    scene.fieldSources[key] = QStringLiteral("inferred");
}

void populateDerivedSceneDetails(SceneDetails& details, const Scene& scene, const ExtractedScene& extracted)
{
    details.description = extracted.description;
    details.building = extracted.building;
    details.landmark = extracted.landmark;
    details.layout = extracted.layout;
    details.atmosphere = extracted.atmosphere;
    details.color = extracted.color;
    details.type = extracted.type.isEmpty() ? inferSceneType(extracted.description, extracted.name) : extracted.type;
    details.typeZh = extracted.typeZh;
    details.setting = extracted.setting;
    details.timeOfDay = extracted.timeOfDay.isEmpty() ? inferTimeOfDay(extracted.description) : extracted.timeOfDay;
    details.weather = extracted.weather.isEmpty() ? inferWeather(extracted.description) : extracted.weather;
    details.spaceSize = extracted.spaceSize;
    details.currentInterpretation = extracted.currentInterpretation;
    details.confidence = extracted.confidence;
    details.status = extracted.status;
    details.narrativeRole = extracted.narrativeRole;
    details.narrativeRoleZh = extracted.narrativeRoleZh;
    details.aliases = extracted.aliases.isEmpty()
        ? buildSceneAliasKeys(scene.name(), details)
        : deduplicateList(BibleUtils::mergeStringLists(extracted.aliases, buildSceneAliasKeys(scene.name(), details)));
    details.anchorPoints = extracted.anchorPoints.isEmpty() ? buildConcreteAnchors(extracted) : extracted.anchorPoints;
    details.signatureObjects = extracted.signatureObjects.isEmpty() ? buildConcreteSignatureObjects(extracted) : extracted.signatureObjects;
    details.fixedColorBlocks = extracted.fixedColorBlocks.isEmpty() ? buildConcreteColorBlocks(extracted) : extracted.fixedColorBlocks;
    details.consistencyRules = extracted.consistencyRules.isEmpty() ? buildConcreteConsistencyRules(extracted) : extracted.consistencyRules;
    details.details = extracted.details;
    details.evidence = extracted.evidence;
    details.history = extracted.history;
    details.timeVariations = extracted.timeVariations;
    details.weatherVariations = extracted.weatherVariations;
    details.fieldSources = extracted.fieldSources;

    if (extracted.type.isEmpty() && !details.type.isEmpty()) {
        details.fieldSources["type"] = QStringLiteral("inferred");
    }
    if (extracted.timeOfDay.isEmpty() && !details.timeOfDay.isEmpty()) {
        details.fieldSources["timeOfDay"] = QStringLiteral("inferred");
    }
    if (extracted.weather.isEmpty() && !details.weather.isEmpty()) {
        details.fieldSources["weather"] = QStringLiteral("inferred");
    }

    AnalysisFieldUtils::markFieldSources(details.fieldSources, {
        {"description", !details.description.isEmpty()},
        {"building", !details.building.isEmpty()},
        {"landmark", !details.landmark.isEmpty()},
        {"layout", !details.layout.isEmpty()},
        {"atmosphere", !details.atmosphere.isEmpty()},
        {"color", !details.color.isEmpty()},
        {"type", !details.type.isEmpty()},
        {"typeZh", !details.typeZh.isEmpty()},
        {"setting", !details.setting.isEmpty()},
        {"timeOfDay", !details.timeOfDay.isEmpty()},
        {"weather", !details.weather.isEmpty()},
        {"spaceSize", !details.spaceSize.isEmpty()},
        {"currentInterpretation", !details.currentInterpretation.isEmpty()},
        {"confidence", !details.confidence.isEmpty()},
        {"status", !details.status.isEmpty()},
        {"narrativeRole", !details.narrativeRole.isEmpty()},
        {"narrativeRoleZh", !details.narrativeRoleZh.isEmpty()},
        {"anchorPoints", !details.anchorPoints.isEmpty()},
        {"signatureObjects", !details.signatureObjects.isEmpty()},
        {"fixedColorBlocks", !details.fixedColorBlocks.isEmpty()},
        {"consistencyRules", !details.consistencyRules.isEmpty()}
    }, QStringLiteral("inferred"));
}

bool persistSceneRecord(const Scene& scene)
{
    QVariantMap data = scene.toVariantMap();
    bool success = DatabaseManager::instance()->update(
        "scenes", data, "id = ?", QVariantList() << scene.id());

    if (!success) {
        return false;
    }

    BibleCache::instance()->invalidateScenes(scene.novelId());

    return true;
}

QString inferSceneType(const QString& description, const QString& name)
{
    static const QStringList indoorKeywords = {
        QString::fromUtf8("房"),
        QString::fromUtf8("室"),
        QString::fromUtf8("厅"),
        QString::fromUtf8("厨房"),
        QString::fromUtf8("客厅"),
        QString::fromUtf8("卧室"),
        QString::fromUtf8("走廊"),
        QString::fromUtf8("门内"),
        QString::fromUtf8("屋内"),
        QString::fromUtf8("堂屋"),
        QString::fromUtf8("后台"),
        QString::fromUtf8("舞台"),
        QString::fromUtf8("剧场"),
        QString::fromUtf8("教室"),
        QString::fromUtf8("办公室"),
        QString::fromUtf8("病房"),
        QString::fromUtf8("仓库"),
        QString::fromUtf8("店铺")
    };

    static const QStringList outdoorKeywords = {
        QString::fromUtf8("街"),
        QString::fromUtf8("路"),
        QString::fromUtf8("巷"),
        QString::fromUtf8("院"),
        QString::fromUtf8("广场"),
        QString::fromUtf8("山"),
        QString::fromUtf8("河"),
        QString::fromUtf8("湖"),
        QString::fromUtf8("海"),
        QString::fromUtf8("林"),
        QString::fromUtf8("田野"),
        QString::fromUtf8("墓地"),
        QString::fromUtf8("乱葬岗"),
        QString::fromUtf8("门口"),
        QString::fromUtf8("门前"),
        QString::fromUtf8("院门")
    };

    QString combined = description + name;

    for (const QString& keyword : indoorKeywords) {
        if (combined.contains(keyword)) {
            return QString::fromUtf8("室内");
        }
    }

    for (const QString& keyword : outdoorKeywords) {
        if (combined.contains(keyword)) {
            return QString::fromUtf8("室外");
        }
    }

    return QString::fromUtf8("室内");
}

QString inferTimeOfDay(const QString& text)
{
    static const QMap<QString, QString> timeKeywords = {
        {QString::fromUtf8("清晨"), QString::fromUtf8("清晨")},
        {QString::fromUtf8("早晨"), QString::fromUtf8("清晨")},
        {QString::fromUtf8("早上"), QString::fromUtf8("清晨")},
        {QString::fromUtf8("上午"), QString::fromUtf8("上午")},
        {QString::fromUtf8("中午"), QString::fromUtf8("中午")},
        {QString::fromUtf8("正午"), QString::fromUtf8("中午")},
        {QString::fromUtf8("下午"), QString::fromUtf8("下午")},
        {QString::fromUtf8("傍晚"), QString::fromUtf8("傍晚")},
        {QString::fromUtf8("黄昏"), QString::fromUtf8("傍晚")},
        {QString::fromUtf8("日落"), QString::fromUtf8("傍晚")},
        {QString::fromUtf8("夜晚"), QString::fromUtf8("夜晚")},
        {QString::fromUtf8("深夜"), QString::fromUtf8("夜晚")},
        {QString::fromUtf8("午夜"), QString::fromUtf8("夜晚")},
        {QString::fromUtf8("凌晨"), QString::fromUtf8("凌晨")},
        {QString::fromUtf8("黎明"), QString::fromUtf8("凌晨")},
        {QString::fromUtf8("天亮"), QString::fromUtf8("清晨")}
    };

    for (auto it = timeKeywords.constBegin(); it != timeKeywords.constEnd(); ++it) {
        if (text.contains(it.key())) {
            return it.value();
        }
    }

    return QString::fromUtf8("白天");
}

QString inferWeather(const QString& text)
{
    static const QMap<QString, QString> weatherKeywords = {
        {QString::fromUtf8("雨"), QString::fromUtf8("雨天")},
        {QString::fromUtf8("暴雨"), QString::fromUtf8("暴雨")},
        {QString::fromUtf8("大雨"), QString::fromUtf8("大雨")},
        {QString::fromUtf8("小雨"), QString::fromUtf8("小雨")},
        {QString::fromUtf8("雪"), QString::fromUtf8("雪天")},
        {QString::fromUtf8("大雪"), QString::fromUtf8("大雪")},
        {QString::fromUtf8("阴"), QString::fromUtf8("阴天")},
        {QString::fromUtf8("晴"), QString::fromUtf8("晴天")},
        {QString::fromUtf8("雾"), QString::fromUtf8("雾天")},
        {QString::fromUtf8("风"), QString::fromUtf8("大风")},
        {QString::fromUtf8("雷"), QString::fromUtf8("雷雨")},
        {QString::fromUtf8("闪电"), QString::fromUtf8("雷雨")}
    };

    for (auto it = weatherKeywords.constBegin(); it != weatherKeywords.constEnd(); ++it) {
        if (text.contains(it.key())) {
            return it.value();
        }
    }

    return QString();
}

Scene loadUniqueSceneByField(const QString& novelId, const QString& field, const QString& value)
{
    QList<QVariantMap> rows = DatabaseManager::instance()->selectAll(
        "scenes", QString("%1 = ? AND novel_id = ?").arg(field),
        QVariantList() << value << novelId);

    if (rows.isEmpty()) {
        return Scene();
    }

    return Scene::fromVariantMap(rows.first());
}

} // namespace

SceneExtractor* SceneExtractor::m_instance = nullptr;
std::once_flag SceneExtractor::m_instanceOnceFlag;

SceneExtractor::SceneExtractor(QObject* parent)
    : QObject(parent)
{
}

SceneExtractor::~SceneExtractor()
{
}

SceneExtractor* SceneExtractor::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new SceneExtractor();
    });
    return m_instance;
}

QList<ExtractedScene> SceneExtractor::extractFromScenes(const QJsonArray& scenes, const QString& sourceText)
{
    QList<ExtractedScene> result;
    for (const QJsonValue& val : scenes) {
        ExtractedScene scene = ExtractedScene::fromJson(val.toObject(), sourceText);
        if (appendExtractedScene(result, scene)) {
            emit sceneExtracted(scene);
        }
    }
    emit extractionCompleted(result.size());
    return result;
}

ExtractedScene ExtractedScene::fromJson(const QJsonObject& json, const QString& sourceText)
{
    ExtractedScene scene;
    scene.name = json["name"].toString();
    assignSceneStringField(scene, "description", scene.description, json["description"].toString());
    assignSceneStringField(scene, "building", scene.building, json["building"].toString());
    assignSceneStringField(scene, "landmark", scene.landmark, json["landmark"].toString());
    assignSceneStringField(scene, "layout", scene.layout, json["layout"].toString());
    assignSceneStringField(scene, "atmosphere", scene.atmosphere, json["atmosphere"].toString());
    assignSceneStringField(scene, "color", scene.color, json["color"].toString());
    assignSceneStringField(scene, "type", scene.type, json["type"].toString());
    assignSceneStringField(scene, "typeZh", scene.typeZh, json["typeZh"].toString());
    assignSceneStringField(scene, "setting", scene.setting, json["setting"].toString());
    assignSceneStringField(scene, "timeOfDay", scene.timeOfDay, json["timeOfDay"].toString());
    assignSceneStringField(scene, "weather", scene.weather, json["weather"].toString());
    assignSceneStringField(scene, "spaceSize", scene.spaceSize, json["spaceSize"].toString());
    assignSceneStringField(scene, "narrativeRole", scene.narrativeRole, json["narrativeRole"].toString());
    assignSceneStringField(scene, "narrativeRoleZh", scene.narrativeRoleZh, json["narrativeRoleZh"].toString());
    scene.fieldSources = json["fieldSources"].toObject();
    AnalysisFieldUtils::markFieldSources(scene.fieldSources, {
        {"description", !scene.description.isEmpty()},
        {"building", !scene.building.isEmpty()},
        {"landmark", !scene.landmark.isEmpty()},
        {"layout", !scene.layout.isEmpty()},
        {"atmosphere", !scene.atmosphere.isEmpty()},
        {"color", !scene.color.isEmpty()},
        {"type", !scene.type.isEmpty()},
        {"typeZh", !scene.typeZh.isEmpty()},
        {"setting", !scene.setting.isEmpty()},
        {"timeOfDay", !scene.timeOfDay.isEmpty()},
        {"weather", !scene.weather.isEmpty()},
        {"spaceSize", !scene.spaceSize.isEmpty()},
        {"narrativeRole", !scene.narrativeRole.isEmpty()},
        {"narrativeRoleZh", !scene.narrativeRoleZh.isEmpty()}
    }, QStringLiteral("inferred"));

    const bool hasSourceText = !sourceText.trimmed().isEmpty();
    if (hasSourceText) {
        scene.fieldSources["description"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.description);
        scene.fieldSources["building"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.building);
        scene.fieldSources["color"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.color);
        scene.fieldSources["landmark"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.landmark);
        scene.fieldSources["layout"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.layout);
        scene.fieldSources["atmosphere"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.atmosphere);
        scene.fieldSources["anchorPoints"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.anchorPoints);
        scene.fieldSources["signatureObjects"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.signatureObjects);
        scene.fieldSources["fixedColorBlocks"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.fixedColorBlocks);
        scene.fieldSources["consistencyRules"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.consistencyRules);
        scene.fieldSources["type"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.type, inferSceneType(sourceText, scene.name));
        scene.fieldSources["typeZh"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.typeZh);
        scene.fieldSources["setting"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.setting);
        scene.fieldSources["timeOfDay"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.timeOfDay, inferTimeOfDay(sourceText));
        scene.fieldSources["weather"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.weather, inferWeather(sourceText));
        scene.fieldSources["spaceSize"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.spaceSize);
        scene.fieldSources["narrativeRole"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.narrativeRole);
        scene.fieldSources["narrativeRoleZh"] = BibleUtils::BibleUpdateStrategy::sourceFromTextEvidence(sourceText, scene.narrativeRoleZh);
    }
    
    if (json.contains("visualCharacteristics")) {
        QJsonObject visual = json["visualCharacteristics"].toObject();
        if (scene.building.isEmpty()) {
            scene.building = visual["architecture"].toString();
            if (!scene.building.isEmpty()) scene.fieldSources["building"] = QStringLiteral("inferred");
        }
        if (scene.atmosphere.isEmpty()) {
            scene.atmosphere = visual["atmosphere"].toString();
            if (!scene.atmosphere.isEmpty()) scene.fieldSources["atmosphere"] = QStringLiteral("inferred");
        }
        if (scene.color.isEmpty()) {
            scene.color = visual["colorScheme"].toString();
            if (!scene.color.isEmpty()) scene.fieldSources["color"] = QStringLiteral("inferred");
        }
        if (scene.anchorPoints.isEmpty() && visual.contains("keyLandmarks")) {
            scene.anchorPoints = deduplicateList(JsonUtils::jsonArrayToStringList(visual["keyLandmarks"].toArray()));
            if (!scene.anchorPoints.isEmpty()) scene.fieldSources["anchorPoints"] = QStringLiteral("inferred");
        }
        if (scene.landmark.isEmpty() && visual.contains("keyLandmarks")) {
            scene.landmark = JsonUtils::jsonArrayToStringList(visual["keyLandmarks"].toArray()).join("、");
            if (!scene.landmark.isEmpty()) scene.fieldSources["landmark"] = QStringLiteral("inferred");
        }
        if (scene.signatureObjects.isEmpty() && visual.contains("textures")) {
            scene.signatureObjects = JsonUtils::jsonArrayToStringList(visual["textures"].toArray());
            if (!scene.signatureObjects.isEmpty()) scene.fieldSources["signatureObjects"] = QStringLiteral("inferred");
        }
    }
    
    if (json.contains("spatialLayout")) {
        QJsonObject spatial = json["spatialLayout"].toObject();
        if (scene.layout.isEmpty()) {
            scene.layout = spatial["layout"].toString();
            if (!scene.layout.isEmpty()) scene.fieldSources["layout"] = QStringLiteral("inferred");
        }
    }
    
    if (json.contains("anchorPoints")) {
        assignSceneListField(scene, "anchorPoints", scene.anchorPoints,
                             JsonUtils::jsonArrayToStringList(json["anchorPoints"].toArray()));
    }
    if (json.contains("signatureObjects")) {
        assignSceneListField(scene, "signatureObjects", scene.signatureObjects,
                             JsonUtils::jsonArrayToStringList(json["signatureObjects"].toArray()));
    }
    if (json.contains("fixedColorBlocks")) {
        assignSceneListField(scene, "fixedColorBlocks", scene.fixedColorBlocks,
                             JsonUtils::jsonArrayToStringList(json["fixedColorBlocks"].toArray()));
    }
    if (json.contains("consistencyRules")) {
        assignSceneListField(scene, "consistencyRules", scene.consistencyRules,
                             JsonUtils::jsonArrayToStringList(json["consistencyRules"].toArray()));
    }
    if (json.contains("details")) {
        scene.details = JsonUtils::jsonArrayToStringList(json["details"].toArray());
    }
    if (json.contains("timeVariations")) {
        assignSceneArrayField(scene, "timeVariations", scene.timeVariations, json["timeVariations"].toArray());
    }
    if (json.contains("weatherVariations")) {
        assignSceneArrayField(scene, "weatherVariations", scene.weatherVariations, json["weatherVariations"].toArray());
    }
    return scene;
}

QJsonObject ExtractedScene::toJson() const
{
    QJsonObject json;
    json["name"] = name;
    json["description"] = description;
    json["building"] = building;
    json["landmark"] = landmark;
    json["layout"] = layout;
    json["atmosphere"] = atmosphere;
    json["color"] = color;
    json["type"] = type;
    json["typeZh"] = typeZh;
    json["setting"] = setting;
    json["timeOfDay"] = timeOfDay;
    json["weather"] = weather;
    json["spaceSize"] = spaceSize;
    json["narrativeRole"] = narrativeRole;
    json["narrativeRoleZh"] = narrativeRoleZh;
    
    if (!anchorPoints.isEmpty()) {
        json["anchorPoints"] = QJsonArray::fromStringList(anchorPoints);
    }
    if (!signatureObjects.isEmpty()) {
        json["signatureObjects"] = QJsonArray::fromStringList(signatureObjects);
    }
    if (!fixedColorBlocks.isEmpty()) {
        json["fixedColorBlocks"] = QJsonArray::fromStringList(fixedColorBlocks);
    }
    if (!consistencyRules.isEmpty()) {
        json["consistencyRules"] = QJsonArray::fromStringList(consistencyRules);
    }
    if (!details.isEmpty()) {
        json["details"] = QJsonArray::fromStringList(details);
    }
    if (!timeVariations.isEmpty()) {
        json["timeVariations"] = timeVariations;
    }
    if (!weatherVariations.isEmpty()) {
        json["weatherVariations"] = weatherVariations;
    }
    if (!fieldSources.isEmpty()) {
        json["fieldSources"] = fieldSources;
    }

    return json;
}

Scene SceneExtractor::toScene(const ExtractedScene& extracted, const QString& novelId)
{
    Scene scene;
    const QString stableId = SceneKeyUtils::resolveSceneStableKey(
        extracted.name,
        extracted.description,
        extracted.setting,
        extracted.aliases,
        QStringList{extracted.building, extracted.landmark, extracted.layout, extracted.setting}
    );
    QString fallbackStableId = stableId;
    if (fallbackStableId.isEmpty()) {
        fallbackStableId = SceneKeyUtils::normalizeSceneLabel(extracted.name);
    }
    if (fallbackStableId.isEmpty()) {
        fallbackStableId = SceneKeyUtils::normalizeSceneLabel(extracted.setting);
    }
    if (fallbackStableId.isEmpty()) {
        fallbackStableId = SceneKeyUtils::normalizeSceneLabel(extracted.description);
    }

    if (fallbackStableId.isEmpty()) {
        return Scene();
    }

    const QString displayName = SceneKeyUtils::resolveSceneDisplayName(QStringList{
        extracted.setting,
        extracted.name,
        extracted.description,
        extracted.landmark,
        extracted.building,
        extracted.layout
    });

    scene.setId(BibleUtils::Utils::generateId());
    scene.setNovelId(novelId);
    scene.setName(displayName.isEmpty() ? fallbackStableId : displayName);
    scene.setSceneId(fallbackStableId);
    
    SceneDetails details;
    populateDerivedSceneDetails(details, scene, extracted);

    scene.setDetails(details);
    return scene;
}

QList<Scene> SceneExtractor::getScenesByNovel(const QString& novelId)
{
    if (BibleCache::instance()->hasScenes(novelId)) {
        return BibleCache::instance()->getScenes(novelId);
    }

    QList<Scene> scenes;
    QList<QVariantMap> rows = DatabaseManager::instance()->selectAll(
        "scenes", "novel_id = ?", QVariantList() << novelId);

    for (const QVariantMap& row : rows) {
        scenes << Scene::fromVariantMap(row);
    }

    BibleCache::instance()->setScenes(novelId, scenes);

    return scenes;
}

int SceneExtractor::getSceneCountByNovel(const QString& novelId)
{
    return getScenesByNovel(novelId).size();
}

Scene SceneExtractor::getSceneById(const QString& sceneId)
{
    return loadUniqueSceneByField(QString(), "id", sceneId);
}

Scene SceneExtractor::getSceneByName(const QString& novelId, const QString& name)
{
    QList<Scene> scenes = getScenesByNovel(novelId);
    Scene probe;
    probe.setNovelId(novelId);
    probe.setName(name);
    return findMatchingScene(scenes, probe);
}

Scene SceneExtractor::getSceneBySceneId(const QString& novelId, const QString& sceneId)
{
    if (sceneId.isEmpty()) {
        return Scene();
    }
    Scene matched = findExistingScene(novelId, sceneId, sceneId);
    if (!matched.id().isEmpty()) {
        return matched;
    }

    return loadUniqueSceneByField(novelId, "id", sceneId);
}

bool SceneExtractor::saveScene(const QString& novelId, const ExtractedScene& extracted)
{
    Scene scene = toScene(extracted, novelId);
    if (scene.id().isEmpty()) {
        LOG_WARNING("SceneExtractor", QString("Skipping unstable scene: %1").arg(extracted.name));
        return false;
    }

    QList<Scene> existingScenes = getScenesByNovel(novelId);
    Scene existing = findMatchingScene(existingScenes, scene);
    if (!existing.id().isEmpty()) {
        LOG_DEBUG("SceneExtractor", QString("Scene '%1' already exists, updating...").arg(scene.name()));
        Scene merged = mergeScenes(existing, extracted);
        return updateScene(merged);
    }

    for (const Scene& existingScene : existingScenes) {
        if (shouldMergeScenes(scene.name(), scene.details().description,
                              existingScene.name(), existingScene.details().description, SCENE_MERGE_THRESHOLD)) {
            LOG_INFO("SceneExtractor", QString("Merging similar scene '%1' into '%2' (similarity detected)")
                .arg(scene.name()).arg(existingScene.name()));
            Scene merged = mergeScenes(existingScene, extracted);
            return updateScene(merged);
        }
    }
    
    QVariantMap data = sceneToData(scene);
    
    qint64 result = DatabaseManager::instance()->insert("scenes", data);
    
    if (result > 0) {
        LOG_INFO("SceneExtractor", QString("Saved scene: %1").arg(scene.name()));
        BibleCache::instance()->invalidateScenes(novelId);
        emit sceneSaved(scene.id(), scene.name());
    } else {
        LOG_ERROR("SceneExtractor", QString("Failed to save scene: %1").arg(scene.name()));
    }
    
    return result > 0;
}

int SceneExtractor::saveScenes(const QString& novelId, const QList<ExtractedScene>& scenes)
{
    int count = 0;
    for (const ExtractedScene& scene : scenes) {
        if (saveScene(novelId, scene)) {
            count++;
        }
    }
    return count;
}

Scene SceneExtractor::findExistingScene(const QString& novelId, const QString& sceneId, const QString& name)
{
    QList<Scene> candidates = getScenesByNovel(novelId);

    for (const Scene& candidate : candidates) {
        if (!sceneId.isEmpty() && (candidate.id() == sceneId || candidate.sceneId() == sceneId)) {
            return candidate;
        }
    }

    if (!name.isEmpty()) {
        Scene probe;
        probe.setNovelId(novelId);
        probe.setName(name);
        Scene matched = findMatchingScene(candidates, probe);
        if (!matched.id().isEmpty()) {
            return matched;
        }
    }

    return Scene();
}

Scene SceneExtractor::sceneFromRow(const QVariantMap& row)
{
    return Scene::fromVariantMap(row);
}

Scene SceneExtractor::mergeScenes(const Scene& existing, const ExtractedScene& incoming)
{
    Scene merged = existing;
    SceneDetails details = merged.details();
    const QJsonObject incomingJson = incoming.toJson();

    BibleUtils::BibleUpdateStrategy::updateSceneDetails(details, incomingJson);
    details.aliases = BibleUtils::mergeStringLists(details.aliases, buildSceneAliasKeys(merged.name(), details));
    
    details.anchorPoints = deduplicateList(details.anchorPoints, 5);
    details.signatureObjects = deduplicateList(details.signatureObjects, 5);
    details.fixedColorBlocks = deduplicateList(details.fixedColorBlocks, 4);
    details.consistencyRules = deduplicateList(details.consistencyRules, 4);
    details.aliases = deduplicateList(details.aliases, 8);
    
    merged.setDetails(details);
    return merged;
}

QVariantMap SceneExtractor::sceneToData(const Scene& scene) const
{
    return scene.toVariantMap();
}

bool SceneExtractor::updateScene(const Scene& scene)
{
    bool success = persistSceneRecord(scene);

    if (success) {
        LOG_INFO("SceneExtractor", QString("Updated scene: %1").arg(scene.name()));
        emit sceneUpdated(scene.id(), scene.name());
    } else {
        LOG_ERROR("SceneExtractor", QString("Failed to update scene: %1").arg(scene.name()));
    }

    return success;
}

bool SceneExtractor::updateSceneSilent(const Scene& scene)
{
    bool success = persistSceneRecord(scene);

    if (success) {
        LOG_INFO("SceneExtractor", QString("Updated scene (silent): %1").arg(scene.name()));
    }

    return success;
}

bool SceneExtractor::deleteScene(const QString& sceneId)
{
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "scenes", "id = ?", QVariantList() << sceneId);
    
    QString novelId;
    if (!row.isEmpty()) {
        novelId = row["novel_id"].toString();
    }
    
    bool success = DatabaseManager::instance()->remove(
        "scenes", "id = ?", QVariantList() << sceneId);
    
    if (success) {
        LOG_INFO("SceneExtractor", QString("Deleted scene: %1").arg(sceneId));
        if (!novelId.isEmpty()) {
            BibleCache::instance()->invalidateScenes(novelId);
        }
    }
    
    return success;
}
