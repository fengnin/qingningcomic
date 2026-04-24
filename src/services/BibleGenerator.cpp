#include "services/BibleGenerator.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "utils/AnalysisFieldUtils.h"
#include "utils/Logger.h"
#include "utils/JsonUtils.h"
#include "utils/BibleUtils.h"
#include "utils/BibleCache.h"
#include <QJsonDocument>

using namespace BibleUtils::BibleUpdateStrategy;

namespace {
using AnalysisFieldUtils::describeFieldChange;

template<typename Item, typename FetchFn, typename ConvertFn>
QJsonArray collectExistingItems(const QString& novelId, FetchFn fetchFn, ConvertFn convertFn)
{
    QJsonArray result;
    const QList<Item> items = fetchFn(novelId);
    for (const Item& item : items) {
        result.append(convertFn(item));
    }
    return result;
}

template<typename ExistingT, typename ResolveFn, typename ShouldUpdateFn, typename ApplyFn, typename SkipFn>
int updateExistingItems(const QJsonArray& items,
                        ResolveFn resolveExisting,
                        ShouldUpdateFn shouldUpdate,
                        ApplyFn applyUpdate,
                        SkipFn logSkip)
{
    int updatedCount = 0;

    for (const QJsonValue& value : items) {
        QJsonObject itemObj = value.toObject();
        const QString itemId = itemObj["id"].toString().trimmed();
        const QString itemName = itemObj["name"].toString().trimmed();

        if (itemId.isEmpty() && itemName.isEmpty()) {
            continue;
        }

        ExistingT existing = resolveExisting(itemId, itemName, itemObj);
        if (existing.id().isEmpty()) {
            continue;
        }

        if (shouldUpdate(existing, itemObj)) {
            if (applyUpdate(existing, itemObj, itemName)) {
                ++updatedCount;
            }
        } else {
            logSkip(existing, itemObj, itemName);
        }
    }

    return updatedCount;
}

inline bool scalarFieldDiffers(const QJsonObject& object, const QString& key, const QString& existingValue)
{
    const QString incoming = object.value(key).toString().trimmed();
    return !incoming.isEmpty() && incoming != existingValue.trimmed();
}

inline bool intFieldDiffers(const QJsonObject& object, const QString& key, int existingValue)
{
    const int incoming = object.value(key).toInt();
    return incoming > 0 && incoming != existingValue;
}

inline bool listFieldDiffers(const QJsonObject& object, const QString& key, const QStringList& existingValues)
{
    const QStringList incoming = JsonUtils::jsonArrayToStringList(object.value(key).toArray());
    for (const QString& value : incoming) {
        if (!existingValues.contains(value)) {
            return true;
        }
    }
    return false;
}

QStringList buildCharacterSkipReasons(const Character& existing, const QJsonObject& charObj)
{
    const QJsonObject appObj = resolveAppearanceObject(charObj);
    const QJsonObject incomingSources = appObj.value("fieldSources").toObject();
    const CharacterAppearance app = existing.appearance();
    const QJsonObject existingSources = app.fieldSources;

    QStringList reasons;
    reasons << describeFieldChange("gender", app.gender, appObj.value("gender").toString().trimmed(),
                                   existingSources.value("gender").toString(), incomingSources.value("gender").toString());
    reasons << describeFieldChange("age", QString::number(app.age), QString::number(appObj.value("age").toInt()),
                                   existingSources.value("age").toString(), incomingSources.value("age").toString());
    reasons << describeFieldChange("hairColor", app.hairColor, appObj.value("hairColor").toString().trimmed(),
                                   existingSources.value("hairColor").toString(), incomingSources.value("hairColor").toString());
    reasons << describeFieldChange("hairStyle", app.hairStyle, appObj.value("hairStyle").toString().trimmed(),
                                   existingSources.value("hairStyle").toString(), incomingSources.value("hairStyle").toString());
    reasons << describeFieldChange("eyeColor", app.eyeColor, appObj.value("eyeColor").toString().trimmed(),
                                   existingSources.value("eyeColor").toString(), incomingSources.value("eyeColor").toString());
    reasons << describeFieldChange("build", app.build, appObj.value("build").toString().trimmed(),
                                   existingSources.value("build").toString(), incomingSources.value("build").toString());
    reasons << describeFieldChange("height", app.height, appObj.value("height").toString().trimmed(),
                                   existingSources.value("height").toString(), incomingSources.value("height").toString());
    return reasons;
}

QStringList buildSceneSkipReasons(const Scene& existing, const QJsonObject& sceneObj)
{
    const QJsonObject detailsObj = resolveSceneDetailsObject(sceneObj);
    const SceneDetails det = existing.details();
    const QJsonObject incomingSources = detailsObj.value("fieldSources").toObject();
    const QJsonObject existingSources = det.fieldSources;

    QStringList reasons;
    reasons << describeFieldChange("description", det.description, detailsObj.value("description").toString().trimmed(),
                                   existingSources.value("description").toString(), incomingSources.value("description").toString());
    reasons << describeFieldChange("building", det.building, detailsObj.value("building").toString().trimmed(),
                                   existingSources.value("building").toString(), incomingSources.value("building").toString());
    reasons << describeFieldChange("layout", det.layout, detailsObj.value("layout").toString().trimmed(),
                                   existingSources.value("layout").toString(), incomingSources.value("layout").toString());
    reasons << describeFieldChange("atmosphere", det.atmosphere, detailsObj.value("atmosphere").toString().trimmed(),
                                   existingSources.value("atmosphere").toString(), incomingSources.value("atmosphere").toString());
    reasons << describeFieldChange("type", det.type, detailsObj.value("type").toString().trimmed(),
                                   existingSources.value("type").toString(), incomingSources.value("type").toString());
    reasons << describeFieldChange("typeZh", det.typeZh, detailsObj.value("typeZh").toString().trimmed(),
                                   existingSources.value("typeZh").toString(), incomingSources.value("typeZh").toString());
    reasons << describeFieldChange("setting", det.setting, detailsObj.value("setting").toString().trimmed(),
                                   existingSources.value("setting").toString(), incomingSources.value("setting").toString());
    reasons << describeFieldChange("timeOfDay", det.timeOfDay, detailsObj.value("timeOfDay").toString().trimmed(),
                                   existingSources.value("timeOfDay").toString(), incomingSources.value("timeOfDay").toString());
    reasons << describeFieldChange("weather", det.weather, detailsObj.value("weather").toString().trimmed(),
                                   existingSources.value("weather").toString(), incomingSources.value("weather").toString());
    reasons << describeFieldChange("spaceSize", det.spaceSize, detailsObj.value("spaceSize").toString().trimmed(),
                                   existingSources.value("spaceSize").toString(), incomingSources.value("spaceSize").toString());
    reasons << describeFieldChange("narrativeRole", det.narrativeRole, detailsObj.value("narrativeRole").toString().trimmed(),
                                   existingSources.value("narrativeRole").toString(), incomingSources.value("narrativeRole").toString());
    reasons << describeFieldChange("narrativeRoleZh", det.narrativeRoleZh, detailsObj.value("narrativeRoleZh").toString().trimmed(),
                                   existingSources.value("narrativeRoleZh").toString(), incomingSources.value("narrativeRoleZh").toString());
    return reasons;
}
}

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
    const QJsonArray result = collectExistingItems<Character>(
        novelId,
        [](const QString& id) { return CharacterExtractor::instance()->getCharactersByNovel(id); },
        [](const Character& character) { return JsonUtils::toJson(character); });
    const int count = result.size();
    
    LOG_INFO("BibleGenerator", QString("Collected %1 existing characters for novel %2")
        .arg(count).arg(novelId));
    
    emit charactersCollected(count);
    
    return result;
}

QJsonArray BibleGenerator::collectExistingScenes(const QString& novelId)
{
    const QJsonArray result = collectExistingItems<Scene>(
        novelId,
        [](const QString& id) { return SceneExtractor::instance()->getScenesByNovel(id); },
        [](const Scene& scene) { return scene.toJson(); });
    const int count = result.size();
    
    LOG_INFO("BibleGenerator", QString("Collected %1 existing scenes for novel %2")
        .arg(count).arg(novelId));
    
    emit scenesCollected(count);
    
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
    auto resolveExisting = [novelId](const QString& charId, const QString& name, const QJsonObject&) -> Character {
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
        return existing;
    };

    auto shouldUpdate = [this](const Character& existing, const QJsonObject& charObj) {
        return shouldUpdateCharacter(existing, charObj);
    };

    auto applyUpdate = [this](Character& existing, const QJsonObject& charObj, const QString& name) {
        QJsonObject appObj = resolveAppearanceObject(charObj);
        CharacterAppearance app = existing.appearance();
        const bool personalityLocked = BibleUtils::BibleUpdateStrategy::isManualSource(
            app.fieldSources.value("personality").toString());
        const QString incomingRole = charObj["role"].toString().trimmed();

        updateCharacterAppearance(app, appObj);
        existing.setAppearance(app);

        if (!incomingRole.isEmpty() && existing.role().trimmed() != incomingRole) {
            existing.setRole(incomingRole);
        }

        if (!personalityLocked) {
            QJsonArray personalityArray = charObj["personality"].toArray();
            if (hasCharacterPersonalityChanges(existing.personality(), personalityArray)) {
                existing.setPersonality(JsonUtils::jsonArrayToStringList(personalityArray));
            }
        }

        if (CharacterExtractor::instance()->updateCharacter(existing)) {
            LOG_INFO("BibleGenerator", QString("Updated character: %1").arg(name.isEmpty() ? existing.name() : name));
            emit characterUpdated(name.isEmpty() ? existing.name() : name);
            return true;
        }

        return false;
    };

    auto logSkip = [](const Character& existing, const QJsonObject& charObj, const QString& name) {
        QStringList reasons = buildCharacterSkipReasons(existing, charObj);
        LOG_DEBUG("BibleGenerator", QString("Skip character update: %1 | %2")
            .arg(name.isEmpty() ? existing.name() : name, reasons.join(QStringLiteral(" || "))));
    };

    const int updatedCount = updateExistingItems<Character>(characters, resolveExisting, shouldUpdate, applyUpdate, logSkip);

    LOG_INFO("BibleGenerator", QString("Updated %1 characters for novel %2")
        .arg(updatedCount).arg(novelId));
    
    if (updatedCount > 0) {
        BibleCache::instance()->invalidateCharacters(novelId);
    }
}

void BibleGenerator::updateExistingScenes(const QString& novelId, const QJsonArray& scenes)
{
    auto resolveExisting = [novelId](const QString& sceneId, const QString& name, const QJsonObject&) -> Scene {
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
        return existing;
    };

    auto shouldUpdate = [this](const Scene& existing, const QJsonObject& sceneObj) {
        return shouldUpdateScene(existing, sceneObj);
    };

    auto applyUpdate = [this](Scene& existing, const QJsonObject& sceneObj, const QString& name) {
        QJsonObject detailsObj = resolveSceneDetailsObject(sceneObj);
        SceneDetails det = existing.details();

        updateSceneDetails(det, detailsObj);
        existing.setDetails(det);

        if (SceneExtractor::instance()->updateScene(existing)) {
            LOG_INFO("BibleGenerator", QString("Updated scene: %1").arg(name.isEmpty() ? existing.name() : name));
            emit sceneUpdated(name.isEmpty() ? existing.name() : name);
            return true;
        }

        return false;
    };

    auto logSkip = [](const Scene& existing, const QJsonObject& sceneObj, const QString& name) {
        QStringList reasons = buildSceneSkipReasons(existing, sceneObj);
        LOG_DEBUG("BibleGenerator", QString("Skip scene update: %1 | %2")
            .arg(name.isEmpty() ? existing.name() : name, reasons.join(QStringLiteral(" || "))));
    };

    const int updatedCount = updateExistingItems<Scene>(scenes, resolveExisting, shouldUpdate, applyUpdate, logSkip);

    LOG_INFO("BibleGenerator", QString("Updated %1 scenes for novel %2")
        .arg(updatedCount).arg(novelId));
    
    if (updatedCount > 0) {
        BibleCache::instance()->invalidateScenes(novelId);
    }
}

bool BibleGenerator::shouldUpdateCharacter(const Character& existing, const QJsonObject& newData) const
{
    QJsonObject appObj = resolveAppearanceObject(newData);
    const QJsonObject incomingSources = appObj.value("fieldSources").toObject();
    const QJsonObject existingSources = existing.appearance().fieldSources;
    const CharacterAppearance app = existing.appearance();
    const auto canAdopt = [&](const QString& key) {
        return shouldUpdateFromSource(
            existingSources.value(key).toString(),
            incomingSources.value(key).toString());
    };

    if ((canAdopt("gender") && scalarFieldDiffers(appObj, "gender", app.gender)) ||
        (canAdopt("age") && intFieldDiffers(appObj, "age", app.age)) ||
        (canAdopt("hairColor") && scalarFieldDiffers(appObj, "hairColor", app.hairColor)) ||
        (canAdopt("hairStyle") && scalarFieldDiffers(appObj, "hairStyle", app.hairStyle)) ||
        (canAdopt("eyeColor") && scalarFieldDiffers(appObj, "eyeColor", app.eyeColor)) ||
        (canAdopt("build") && scalarFieldDiffers(appObj, "build", app.build)) ||
        (canAdopt("height") && scalarFieldDiffers(appObj, "height", app.height)) ||
        (canAdopt("clothing") && listFieldDiffers(appObj, "clothing", app.clothing)) ||
        (canAdopt("accessories") && listFieldDiffers(appObj, "accessories", app.accessories)) ||
        (canAdopt("distinctiveFeatures") && listFieldDiffers(appObj, "distinctiveFeatures", app.distinctiveFeatures)) ||
        (canAdopt("aliases") && listFieldDiffers(appObj, "aliases", app.aliases))) {
        return true;
    }

    const QString incomingRole = newData["role"].toString().trimmed();
    if (!incomingRole.isEmpty() && existing.role().trimmed() != incomingRole) {
        return true;
    }

    if (BibleUtils::BibleUpdateStrategy::isManualSource(existingSources.value("personality").toString())) {
        return false;
    }
    return hasCharacterPersonalityChanges(existing.personality(), newData["personality"].toArray());
}

bool BibleGenerator::shouldUpdateScene(const Scene& existing, const QJsonObject& newData) const
{
    QJsonObject detailsObj = resolveSceneDetailsObject(newData);
    SceneDetails det = existing.details();
    const QJsonObject incomingSources = detailsObj.value("fieldSources").toObject();
    const QJsonObject existingSources = det.fieldSources;
    const auto canAdopt = [&](const QString& key) {
        return shouldUpdateFromSource(
            existingSources.value(key).toString(),
            incomingSources.value(key).toString());
    };

    auto scalarChanged = [&](const QString& key, const QString& existingValue) {
        return canAdopt(key) && scalarFieldDiffers(detailsObj, key, existingValue);
    };
    auto listChanged = [&](const QString& key, const QStringList& existingValues) {
        return canAdopt(key) && listFieldDiffers(detailsObj, key, existingValues);
    };

    return scalarChanged("description", det.description) ||
           scalarChanged("building", det.building) ||
           scalarChanged("color", det.color) ||
           scalarChanged("landmark", det.landmark) ||
           scalarChanged("layout", det.layout) ||
           scalarChanged("atmosphere", det.atmosphere) ||
           scalarChanged("type", det.type) ||
           scalarChanged("typeZh", det.typeZh) ||
           scalarChanged("setting", det.setting) ||
           scalarChanged("timeOfDay", det.timeOfDay) ||
           scalarChanged("weather", det.weather) ||
           scalarChanged("spaceSize", det.spaceSize) ||
           scalarChanged("currentInterpretation", det.currentInterpretation) ||
           scalarChanged("confidence", det.confidence) ||
           scalarChanged("status", det.status) ||
           scalarChanged("narrativeRole", det.narrativeRole) ||
           scalarChanged("narrativeRoleZh", det.narrativeRoleZh) ||
           listChanged("anchorPoints", det.anchorPoints) ||
           listChanged("signatureObjects", det.signatureObjects) ||
           listChanged("fixedColorBlocks", det.fixedColorBlocks) ||
           listChanged("consistencyRules", det.consistencyRules) ||
           listChanged("evidence", det.evidence) ||
           listChanged("aliases", det.aliases) ||
           listChanged("history", det.history);
}
