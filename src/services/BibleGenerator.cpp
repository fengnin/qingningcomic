#include "BibleGenerator.h"
#include "CharacterExtractor.h"
#include "SceneExtractor.h"
#include "Logger.h"
#include "utils/JsonUtils.h"

BibleGenerator* BibleGenerator::m_instance = nullptr;

BibleGenerator::BibleGenerator(QObject* parent)
    : QObject(parent)
{
}

BibleGenerator* BibleGenerator::instance()
{
    if (!m_instance) {
        m_instance = new BibleGenerator();
    }
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

template<typename T>
bool shouldUpdateField(const T& existing, const T& newValue)
{
    return existing.isEmpty() && !newValue.isEmpty();
}

template<>
bool shouldUpdateField<int>(const int& existing, const int& newValue)
{
    return existing == 0 && newValue > 0;
}

void updateCharacterAppearance(CharacterAppearance& app, const QJsonObject& appObj)
{
    if (shouldUpdateField(app.gender, appObj["gender"].toString())) {
        app.gender = appObj["gender"].toString();
    }
    if (shouldUpdateField(app.age, appObj["age"].toInt())) {
        app.age = appObj["age"].toInt();
    }
    if (shouldUpdateField(app.hairColor, appObj["hairColor"].toString())) {
        app.hairColor = appObj["hairColor"].toString();
    }
    if (shouldUpdateField(app.hairStyle, appObj["hairStyle"].toString())) {
        app.hairStyle = appObj["hairStyle"].toString();
    }
    if (shouldUpdateField(app.eyeColor, appObj["eyeColor"].toString())) {
        app.eyeColor = appObj["eyeColor"].toString();
    }
    if (shouldUpdateField(app.build, appObj["build"].toString())) {
        app.build = appObj["build"].toString();
    }
}

void updateSceneDetails(SceneDetails& det, const QJsonObject& detailsObj)
{
    auto updateField = [&detailsObj, &det](const QString& key, QString& field) {
        QString value = detailsObj[key].toString();
        if (shouldUpdateField(field, value)) {
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
}

bool hasEmptyAppearanceFields(const CharacterAppearance& app)
{
    return app.gender.isEmpty() || app.age == 0 || app.hairColor.isEmpty() ||
           app.hairStyle.isEmpty() || app.eyeColor.isEmpty() || app.build.isEmpty();
}

bool hasEmptyDetailsFields(const SceneDetails& det)
{
    return det.description.isEmpty() || det.building.isEmpty() || det.color.isEmpty() ||
           det.landmark.isEmpty() || det.layout.isEmpty() || det.atmosphere.isEmpty() ||
           det.type.isEmpty() || det.setting.isEmpty() || det.timeOfDay.isEmpty() ||
           det.weather.isEmpty();
}

bool hasNewAppearanceData(const QJsonObject& appObj)
{
    return !appObj["gender"].toString().isEmpty() || appObj["age"].toInt() > 0 ||
           !appObj["hairColor"].toString().isEmpty() || !appObj["hairStyle"].toString().isEmpty() ||
           !appObj["eyeColor"].toString().isEmpty() || !appObj["build"].toString().isEmpty();
}

bool hasNewDetailsData(const QJsonObject& detailsObj)
{
    QStringList keys = {"description", "building", "color", "landmark", "layout", 
                        "atmosphere", "type", "setting", "timeOfDay", "weather"};
    for (const QString& key : keys) {
        if (!detailsObj[key].toString().isEmpty()) {
            return true;
        }
    }
    return false;
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
            if (!personalityArray.isEmpty() && existing.personality().isEmpty()) {
                existing.setPersonality(JsonUtils::jsonArrayToStringList(personalityArray));
            }
            
            CharacterExtractor::instance()->updateCharacter(existing);
            updatedCount++;
            
            LOG_INFO("BibleGenerator", QString("Updated character: %1").arg(name));
            emit characterUpdated(name);
        }
    }
    
    LOG_INFO("BibleGenerator", QString("Updated %1 characters for novel %2")
        .arg(updatedCount).arg(novelId));
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
            SceneDetails det = existing.details();
            
            updateSceneDetails(det, detailsObj);
            existing.setDetails(det);
            
            SceneExtractor::instance()->updateScene(existing);
            updatedCount++;
            
            LOG_INFO("BibleGenerator", QString("Updated scene: %1").arg(name));
            emit sceneUpdated(name);
        }
    }
    
    LOG_INFO("BibleGenerator", QString("Updated %1 scenes for novel %2")
        .arg(updatedCount).arg(novelId));
}

bool BibleGenerator::shouldUpdateCharacter(const Character& existing, const QJsonObject& newData) const
{
    QJsonObject appObj = newData["appearance"].toObject();
    CharacterAppearance app = existing.appearance();
    
    if (hasEmptyAppearanceFields(app) && hasNewAppearanceData(appObj)) {
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
    SceneDetails det = existing.details();
    
    return hasEmptyDetailsFields(det) && hasNewDetailsData(detailsObj);
}
