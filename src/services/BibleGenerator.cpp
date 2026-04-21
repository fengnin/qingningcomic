#include "services/BibleGenerator.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "utils/Logger.h"
#include "utils/JsonUtils.h"
#include "utils/BibleUtils.h"
#include <QJsonDocument>

using namespace BibleUtils::BibleUpdateStrategy;

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

void BibleGenerator::updateExistingCharacters(const QString& novelId, const QJsonArray& characters)
{
    int updatedCount = 0;
    
    for (const QJsonValue& charVal : characters) {
        QJsonObject charObj = charVal.toObject();
        const QString charId = charObj["id"].toString().trimmed();
        QString name = charObj["name"].toString();
        
        if (name.isEmpty() && charId.isEmpty()) {
            continue;
        }
        
        Character existing;
        if (!charId.isEmpty()) {
            existing = CharacterExtractor::instance()->getCharacterById(charId);
            if (!existing.id().isEmpty() && existing.novelId() != novelId) {
                existing = Character();
            }
        }
        if (existing.id().isEmpty() && !name.isEmpty()) {
            existing = CharacterExtractor::instance()->getCharacterByName(novelId, name);
        }
        if (existing.id().isEmpty()) {
            continue;
        }
        
        if (shouldUpdateCharacter(existing, charObj)) {
            QJsonObject appObj = resolveAppearanceObject(charObj);
            CharacterAppearance app = existing.appearance();
            
            updateCharacterAppearance(app, appObj);
            existing.setAppearance(app);
            
            QJsonArray personalityArray = charObj["personality"].toArray();
            if (!personalityArray.isEmpty() && existing.personality().isEmpty()) {
                existing.setPersonality(JsonUtils::jsonArrayToStringList(personalityArray));
            }
            
            if (CharacterExtractor::instance()->updateCharacter(existing)) {
                updatedCount++;
                LOG_INFO("BibleGenerator", QString("Updated character: %1").arg(name.isEmpty() ? existing.name() : name));
                emit characterUpdated(name.isEmpty() ? existing.name() : name);
            }
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
        const QString sceneId = sceneObj["id"].toString().trimmed();
        QString name = sceneObj["name"].toString();
        
        if (name.isEmpty() && sceneId.isEmpty()) {
            continue;
        }
        
        Scene existing;
        if (!sceneId.isEmpty()) {
            existing = SceneExtractor::instance()->getSceneById(sceneId);
            if (!existing.id().isEmpty() && existing.novelId() != novelId) {
                existing = Scene();
            }
        }
        if (existing.id().isEmpty()) {
            existing = SceneExtractor::instance()->getSceneBySceneId(novelId,
                sceneId.isEmpty() ? name : sceneId);
        }
        if (existing.id().isEmpty() && !name.isEmpty()) {
            existing = SceneExtractor::instance()->getSceneByName(novelId, name);
        }
        if (existing.id().isEmpty()) {
            continue;
        }
        
        if (shouldUpdateScene(existing, sceneObj)) {
            QJsonObject detailsObj = resolveSceneDetailsObject(sceneObj);
            SceneDetails det = existing.details();
            
            updateSceneDetails(det, detailsObj);
            existing.setDetails(det);
            
            if (SceneExtractor::instance()->updateScene(existing)) {
                updatedCount++;
                LOG_INFO("BibleGenerator", QString("Updated scene: %1").arg(name.isEmpty() ? existing.name() : name));
                emit sceneUpdated(name.isEmpty() ? existing.name() : name);
            }
        }
    }

    LOG_INFO("BibleGenerator", QString("Updated %1 scenes for novel %2")
        .arg(updatedCount).arg(novelId));
}

bool BibleGenerator::shouldUpdateCharacter(const Character& existing, const QJsonObject& newData) const
{
    QJsonObject appObj = resolveAppearanceObject(newData);
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
    QJsonObject detailsObj = resolveSceneDetailsObject(newData);
    SceneDetails det = existing.details();
    
    return (hasEmptyDetailsFields(det) && hasNewDetailsData(detailsObj)) ||
           hasIterationUpdates(det, detailsObj);
}
