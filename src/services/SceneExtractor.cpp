#include "SceneExtractor.h"
#include "Scene.h"
#include "DatabaseManager.h"
#include "Logger.h"
#include "BibleCache.h"
#include "utils/JsonUtils.h"
#include <QUuid>

SceneExtractor* SceneExtractor::m_instance = nullptr;

QJsonObject ExtractedScene::toJson() const
{
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["description"] = description;
    json["building"] = building;
    json["color"] = color;
    json["landmark"] = landmark;
    json["layout"] = layout;
    json["atmosphere"] = atmosphere;
    json["type"] = type;
    json["setting"] = setting;
    json["timeOfDay"] = timeOfDay;
    json["weather"] = weather;
    json["details"] = JsonUtils::stringListToJsonArray(details);
    return json;
}

ExtractedScene ExtractedScene::fromJson(const QJsonObject& json)
{
    ExtractedScene scene;
    scene.id = json["id"].toString();
    scene.name = json["name"].toString();
    scene.description = json["description"].toString();
    scene.type = json["type"].toString();
    scene.setting = json["setting"].toString();
    scene.timeOfDay = json["timeOfDay"].toString();
    scene.weather = json["weather"].toString();
    
    if (json.contains("visualCharacteristics") || json.contains("spatialLayout")) {
        SceneJsonParser::ParsedVisualData visual = 
            SceneJsonParser::parseVisualCharacteristics(json);
        scene.building = visual.building;
        scene.color = visual.color;
        scene.atmosphere = visual.atmosphere;
        scene.landmark = visual.landmark;
        scene.layout = visual.layout;
    } else {
        scene.building = json["building"].toString();
        scene.color = json["color"].toString();
        scene.atmosphere = json["atmosphere"].toString();
        scene.landmark = json["landmark"].toString();
        scene.layout = json["layout"].toString();
    }
    
    scene.details = JsonUtils::jsonArrayToStringList(json["details"].toArray());
    return scene;
}

SceneExtractor::SceneExtractor(QObject* parent)
    : QObject(parent)
{
}

SceneExtractor* SceneExtractor::instance()
{
    if (!m_instance) {
        m_instance = new SceneExtractor();
    }
    return m_instance;
}

QList<ExtractedScene> SceneExtractor::extractFromScenes(const QJsonArray& scenes)
{
    QList<ExtractedScene> result;
    
    for (const QJsonValue& sceneVal : scenes) {
        ExtractedScene extracted = parseAIScene(sceneVal.toObject());
        
        if (!extracted.name.isEmpty() && containsChinese(extracted.name)) {
            result.append(extracted);
            emit sceneExtracted(extracted);
        } else if (!extracted.name.isEmpty()) {
            LOG_DEBUG("SceneExtractor", QString("Skipping non-Chinese scene: %1").arg(extracted.name));
        }
    }
    
    emit extractionCompleted(result.size());
    LOG_INFO("SceneExtractor", QString("Extracted %1 scenes from AI response").arg(result.size()));
    
    return result;
}

ExtractedScene SceneExtractor::parseAIScene(const QJsonObject& sceneObj)
{
    return ExtractedScene::fromJson(sceneObj);
}

Scene SceneExtractor::toScene(const ExtractedScene& extracted, const QString& novelId)
{
    Scene scene;
    scene.setId(generateId());
    scene.setNovelId(novelId);
    scene.setName(extracted.name);
    scene.setSceneId(extracted.id);
    
    SceneDetails det;
    det.description = extracted.description;
    det.building = extracted.building;
    det.color = extracted.color;
    det.landmark = extracted.landmark;
    det.layout = extracted.layout;
    det.atmosphere = extracted.atmosphere;
    det.type = extracted.type;
    det.setting = extracted.setting;
    det.timeOfDay = extracted.timeOfDay;
    det.weather = extracted.weather;
    det.details = extracted.details;
    
    scene.setDetails(det);
    
    return scene;
}

bool SceneExtractor::containsChinese(const QString& text)
{
    return text.contains(QRegularExpression("\\p{Han}"));
}

QString SceneExtractor::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool SceneExtractor::saveScene(const QString& novelId, const ExtractedScene& extracted)
{
    Scene scene = toScene(extracted, novelId);
    
    Scene existing = findExistingScene(novelId, scene.sceneId(), scene.name());
    
    if (!existing.id().isEmpty()) {
        LOG_DEBUG("SceneExtractor", QString("Scene '%1' already exists, updating").arg(scene.name()));
        scene.setId(existing.id());
        return updateScene(scene);
    }
    
    QVariantMap data = sceneToData(scene);
    
    qint64 result = DatabaseManager::instance()->insert("scenes", data);
    
    if (result > 0) {
        LOG_INFO("SceneExtractor", QString("Saved scene: %1").arg(scene.name()));
        emit sceneSaved(scene.id(), scene.name());
        emit sceneCountChanged(getSceneCountByNovel(novelId));
    } else {
        LOG_ERROR("SceneExtractor", QString("Failed to save scene: %1").arg(scene.name()));
    }
    
    return result > 0;
}

bool SceneExtractor::updateScene(const Scene& scene)
{
    QVariantMap data = sceneToData(scene);
    
    bool success = DatabaseManager::instance()->update(
        "scenes", data, "id = ?", QVariantList() << scene.id());
    
    if (success) {
        LOG_INFO("SceneExtractor", QString("Updated scene: %1").arg(scene.name()));
        
        // 更新缓存
        BibleCache::instance()->invalidateScenes(scene.novelId());
        
        emit sceneUpdated(scene.id(), scene.name());
    } else {
        LOG_ERROR("SceneExtractor", QString("Failed to update scene: %1").arg(scene.name()));
    }
    
    return success;
}

int SceneExtractor::saveScenes(const QString& novelId, const QList<ExtractedScene>& scenes)
{
    int savedCount = 0;
    
    for (const ExtractedScene& extracted : scenes) {
        if (saveScene(novelId, extracted)) {
            savedCount++;
        }
    }
    
    LOG_INFO("SceneExtractor", QString("Saved %1/%2 scenes for novel %3")
        .arg(savedCount).arg(scenes.size()).arg(novelId));
    
    return savedCount;
}

QVariantMap SceneExtractor::sceneToData(const Scene& scene) const
{
    QVariantMap data;
    data["id"] = scene.id();
    data["novel_id"] = scene.novelId();
    data["name"] = scene.name();
    data["scene_id"] = scene.sceneId();
    data["details"] = JsonUtils::jsonToString(scene.details().toJson());
    data["tags"] = JsonUtils::jsonToString(JsonUtils::stringListToJsonArray(scene.tags()));
    
    // 保存参考图路径（包括清空的情况）
    data["reference_image_path"] = scene.referenceImagePath();
    
    return data;
}

Scene SceneExtractor::sceneFromRow(const QVariantMap& row)
{
    Scene scene;
    scene.setId(row["id"].toString());
    scene.setNovelId(row["novel_id"].toString());
    scene.setName(row["name"].toString());
    scene.setSceneId(row["scene_id"].toString());
    
    QJsonObject detailsJson = JsonUtils::variantToJson(row["details"]);
    if (!detailsJson.isEmpty()) {
        scene.setDetails(SceneDetails::fromJson(detailsJson));
    }
    
    scene.setTags(JsonUtils::variantToStringList(row["tags"]));
    
    // 读取参考图路径
    scene.setReferenceImagePath(row["reference_image_path"].toString());
    
    return scene;
}

QList<Scene> SceneExtractor::getScenesByNovel(const QString& novelId)
{
    QList<Scene> scenes;
    QList<QVariantMap> results = DatabaseManager::instance()->selectAll(
        "scenes", "novel_id = ?", QVariantList() << novelId);
    
    for (const QVariantMap& row : results) {
        scenes.append(sceneFromRow(row));
    }
    
    return scenes;
}

Scene SceneExtractor::getSceneById(const QString& sceneId)
{
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "scenes", "id = ?", QVariantList() << sceneId);
    
    return row.isEmpty() ? Scene() : sceneFromRow(row);
}

Scene SceneExtractor::getSceneBySceneId(const QString& novelId, const QString& sceneId)
{
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "scenes", "novel_id = ? AND (scene_id = ? OR name = ?)", 
        QVariantList() << novelId << sceneId << sceneId);
    
    return row.isEmpty() ? Scene() : sceneFromRow(row);
}

Scene SceneExtractor::findExistingScene(const QString& novelId, const QString& sceneId, const QString& name)
{
    auto queryByField = [this, &novelId](const QString& field, const QString& value) -> Scene {
        if (value.isEmpty()) {
            return Scene();
        }
        QString whereClause = QString("novel_id = ? AND %1 = ?").arg(field);
        QVariantMap row = DatabaseManager::instance()->selectOne(
            "scenes", whereClause, QVariantList() << novelId << value);
        return row.isEmpty() ? Scene() : sceneFromRow(row);
    };
    
    Scene result = queryByField("scene_id", sceneId);
    if (!result.id().isEmpty()) {
        return result;
    }
    
    return queryByField("name", name);
}

int SceneExtractor::getSceneCountByNovel(const QString& novelId)
{
    QList<QVariantMap> results = DatabaseManager::instance()->selectAll(
        "scenes", "novel_id = ?", QVariantList() << novelId);
    return results.size();
}

bool SceneExtractor::deleteScene(const QString& sceneId)
{
    bool success = DatabaseManager::instance()->remove(
        "scenes", "id = ?", QVariantList() << sceneId);
    
    if (success) {
        LOG_INFO("SceneExtractor", QString("Deleted scene: %1").arg(sceneId));
    }
    
    return success;
}
