#include "BibleGenerator.h"
#include "CharacterExtractor.h"
#include "SceneExtractor.h"
#include "Logger.h"
#include "utils/JsonUtils.h"
#include "utils/BibleUtils.h"

BibleGenerator* BibleGenerator::m_instance = nullptr;
std::once_flag BibleGenerator::m_instanceOnceFlag;

BibleGenerator::BibleGenerator(QObject* parent)
    : QObject(parent)
{
}

BibleGenerator* BibleGenerator::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new BibleGenerator();
    });
    return m_instance;
}

QJsonArray BibleGenerator::collectExistingCharacters(const QString& novelId)
{
    QJsonArray result;
    
    QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(novelId);
    
    for (const Character& character : characters) {
        result.append(JsonUtils::toJson(character));
    }
    
    LOG_INFO("BibleGenerator", QString("Collected %1 existing characters for novel %2")
        .arg(characters.size()).arg(novelId));
    
    emit charactersCollected(characters.size());
    
    return result;
}

QJsonArray BibleGenerator::collectExistingScenes(const QString& novelId)
{
    QJsonArray result;
    
    QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(novelId);
    
    for (const Scene& scene : scenes) {
        result.append(scene.toJson());
    }
    
    LOG_INFO("BibleGenerator", QString("Collected %1 existing scenes for novel %2")
        .arg(scenes.size()).arg(novelId));
    
    emit scenesCollected(scenes.size());
    
    return result;
}

QJsonObject BibleGenerator::characterToJson(const Character& character) const
{
    return JsonUtils::toJson(character);
}

QJsonObject BibleGenerator::sceneToJson(const Scene& scene) const
{
    return scene.toJson();
}

namespace {

void updateCharacterAppearance(CharacterAppearance& app, const QJsonObject& appObj)
{
    // 标量字段：只填充空值
    if (BibleUtils::shouldUpdateField(app.gender, appObj["gender"].toString())) {
        app.gender = appObj["gender"].toString();
    }
    if (BibleUtils::shouldUpdateField(app.age, appObj["age"].toInt())) {
        app.age = appObj["age"].toInt();
    }
    if (BibleUtils::shouldUpdateField(app.hairColor, appObj["hairColor"].toString())) {
        app.hairColor = appObj["hairColor"].toString();
    }
    if (BibleUtils::shouldUpdateField(app.hairStyle, appObj["hairStyle"].toString())) {
        app.hairStyle = appObj["hairStyle"].toString();
    }
    if (BibleUtils::shouldUpdateField(app.eyeColor, appObj["eyeColor"].toString())) {
        app.eyeColor = appObj["eyeColor"].toString();
    }
    if (BibleUtils::shouldUpdateField(app.build, appObj["build"].toString())) {
        app.build = appObj["build"].toString();
    }
    
    // 数组字段：合并去重（对齐原仓库 mergeAppearance 的 arrayFields 逻辑）
    QJsonArray clothingArray = appObj["clothing"].toArray();
    if (!clothingArray.isEmpty()) {
        QStringList incoming = BibleUtils::jsonArrayToStringList(clothingArray);
        app.clothing = BibleUtils::mergeStringLists(app.clothing, incoming);
    }
    
    QJsonArray accessoriesArray = appObj["accessories"].toArray();
    if (!accessoriesArray.isEmpty()) {
        QStringList incoming = BibleUtils::jsonArrayToStringList(accessoriesArray);
        app.accessories = BibleUtils::mergeStringLists(app.accessories, incoming);
    }
    
    QJsonArray featuresArray = appObj["distinctiveFeatures"].toArray();
    if (!featuresArray.isEmpty()) {
        QStringList incoming = BibleUtils::jsonArrayToStringList(featuresArray);
        app.distinctiveFeatures = BibleUtils::mergeStringLists(app.distinctiveFeatures, incoming);
    }
}

void updateSceneDetails(SceneDetails& det, const QJsonObject& detailsObj)
{
    auto updateField = [&detailsObj, &det](const QString& key, QString& field) {
        QString value = detailsObj[key].toString();
        if (BibleUtils::shouldUpdateField(field, value)) {
            field = value;
        }
    };
    
    updateField("description", det.description);
    updateField("building", det.building);
    updateField("color", det.color);
    updateField("landmark", det.landmark);
    updateField("layout", det.layout);
    updateField("atmosphere", det.atmosphere);
    updateField("type", det.type);
    updateField("setting", det.setting);
    updateField("timeOfDay", det.timeOfDay);
    updateField("weather", det.weather);
    updateField("narrativeRole", det.narrativeRole);
    
    // 深度合并嵌套结构（对齐原仓库 mergeScenes）
    QJsonObject incomingVisual = detailsObj["visualCharacteristics"].toObject();
    if (!incomingVisual.isEmpty()) {
        det.visualCharacteristics = BibleUtils::mergeVisualCharacteristics(
            det.visualCharacteristics, incomingVisual);
    }
    
    QJsonObject incomingSpatial = detailsObj["spatialLayout"].toObject();
    if (!incomingSpatial.isEmpty()) {
        det.spatialLayout = BibleUtils::mergeSpatialLayout(
            det.spatialLayout, incomingSpatial);
    }
    
    QJsonArray incomingTime = detailsObj["timeVariations"].toArray();
    if (!incomingTime.isEmpty()) {
        det.timeVariations = BibleUtils::mergeVariations(
            det.timeVariations, incomingTime, "timeOfDay");
    }
    
    QJsonArray incomingWeather = detailsObj["weatherVariations"].toArray();
    if (!incomingWeather.isEmpty()) {
        det.weatherVariations = BibleUtils::mergeVariations(
            det.weatherVariations, incomingWeather, "weather");
    }
}

}

void BibleGenerator::updateExistingCharacters(const QString& novelId, const QJsonArray& characters)
{
    int updatedCount = 0;
    
    for (const QJsonValue& charVal : characters) {
        QJsonObject charObj = charVal.toObject();
        QString name = charObj["name"].toString();
        
        if (name.isEmpty()) {
            continue;
        }
        
        Character existing = CharacterExtractor::instance()->getCharacterByName(novelId, name);
        if (existing.id().isEmpty()) {
            continue;
        }
        
        if (shouldUpdateCharacter(existing, charObj)) {
            QJsonObject appObj = charObj["appearance"].toObject();
            CharacterAppearance app = existing.appearance();
            
            updateCharacterAppearance(app, appObj);
            existing.setAppearance(app);
            
            QJsonArray personalityArray = charObj["personality"].toArray();
            if (!personalityArray.isEmpty()) {
                QStringList incoming = JsonUtils::jsonArrayToStringList(personalityArray);
                existing.setPersonality(BibleUtils::mergeStringLists(existing.personality(), incoming));
            }
            
            CharacterExtractor::instance()->updateCharacter(existing);
            updatedCount++;
            
            emit characterUpdated(name);
        }
    }
    
    if (updatedCount > 0) {
        LOG_INFO("BibleGenerator", QString("Updated %1 characters for novel %2")
            .arg(updatedCount).arg(novelId));
    }
}

void BibleGenerator::updateExistingScenes(const QString& novelId, const QJsonArray& scenes)
{
    int updatedCount = 0;
    
    for (const QJsonValue& sceneVal : scenes) {
        QJsonObject sceneObj = sceneVal.toObject();
        QString name = sceneObj["name"].toString();
        QString sceneId = sceneObj["id"].toString();
        
        if (name.isEmpty() && sceneId.isEmpty()) {
            continue;
        }
        
        Scene existing = SceneExtractor::instance()->getSceneBySceneId(novelId, 
            sceneId.isEmpty() ? name : sceneId);
        if (existing.id().isEmpty()) {
            continue;
        }
        
        if (shouldUpdateScene(existing, sceneObj)) {
            QJsonObject detailsObj = sceneObj["details"].toObject();
            if (detailsObj.isEmpty()) {
                detailsObj = sceneObj;
            }
            
            SceneDetails det = existing.details();
            
            updateSceneDetails(det, detailsObj);
            existing.setDetails(det);
            
            SceneExtractor::instance()->updateScene(existing);
            updatedCount++;
            
            emit sceneUpdated(name);
        }
    }
    
    if (updatedCount > 0) {
        LOG_INFO("BibleGenerator", QString("Updated %1 scenes for novel %2")
            .arg(updatedCount).arg(novelId));
    }
}

bool BibleGenerator::shouldUpdateCharacter(const Character& existing, const QJsonObject& newData) const
{
    QJsonObject appObj = newData["appearance"].toObject();
    CharacterAppearance app = existing.appearance();
    
    if (BibleUtils::hasEmptyAppearanceFields(app) && BibleUtils::hasNewAppearanceData(appObj)) {
        return true;
    }
    
    if (existing.personality().isEmpty() && !newData["personality"].toArray().isEmpty()) {
        return true;
    }
    
    return false;
}

bool BibleGenerator::shouldUpdateScene(const Scene& existing, const QJsonObject& newData) const
{
    QJsonObject detailsObj = newData["details"].toObject();
    if (detailsObj.isEmpty()) {
        detailsObj = newData;
    }
    SceneDetails det = existing.details();
    
    return BibleUtils::hasEmptyDetailsFields(det) && BibleUtils::hasNewDetailsData(detailsObj);
}
