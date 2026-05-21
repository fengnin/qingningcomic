#include "utils/BibleUtils.h"

namespace BibleUtils {
namespace BibleUpdateStrategy {

QJsonObject resolveAppearanceObject(const QJsonObject& characterObj)
{
    QJsonObject appObj = characterObj["appearance"].toObject();
    if (appObj.isEmpty() && characterObj["appearance"].isString()) {
        QJsonParseError parseError;
        QJsonDocument appearanceDoc = QJsonDocument::fromJson(characterObj["appearance"].toString().toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && appearanceDoc.isObject()) {
            appObj = appearanceDoc.object();
        }
    }
    if (appObj.isEmpty()) {
        appObj = characterObj;
    }
    return appObj;
}

QJsonObject resolveSceneDetailsObject(const QJsonObject& sceneObj)
{
    QJsonObject detailsObj = sceneObj["details"].toObject();
    if (detailsObj.isEmpty()) {
        detailsObj = sceneObj;
    }
    return detailsObj;
}

bool hasEmptyAppearanceFields(const CharacterAppearance& app)
{
    return app.gender.isEmpty() || app.age == 0 || app.hairColor.isEmpty() ||
           app.hairStyle.isEmpty() || app.eyeColor.isEmpty() || app.build.isEmpty() ||
           app.height.isEmpty() || app.clothing.isEmpty() || app.accessories.isEmpty() ||
           app.distinctiveFeatures.isEmpty() || app.aliases.isEmpty();
}

bool hasNewAppearanceData(const QJsonObject& appObj)
{
    return !appObj["gender"].toString().isEmpty() || appObj["age"].toInt() > 0 ||
           !appObj["hairColor"].toString().isEmpty() || !appObj["hairStyle"].toString().isEmpty() ||
           !appObj["eyeColor"].toString().isEmpty() || !appObj["build"].toString().isEmpty() ||
           !appObj["height"].toString().isEmpty() || !appObj["clothing"].toArray().isEmpty() ||
           !appObj["accessories"].toArray().isEmpty() || !appObj["distinctiveFeatures"].toArray().isEmpty() ||
           !appObj["aliases"].toArray().isEmpty();
}

bool hasCharacterAppearanceChanges(const CharacterAppearance& existing, const QJsonObject& appObj)
{
    auto scalarChanged = [&appObj](const QString& key, const QString& existingValue) {
        const QString incoming = appObj[key].toString().trimmed();
        return !incoming.isEmpty() && incoming != existingValue.trimmed();
    };

    auto intChanged = [&appObj](const QString& key, int existingValue) {
        const int incoming = appObj[key].toInt();
        return incoming > 0 && incoming != existingValue;
    };

    auto listChanged = [&appObj](const QString& key, const QStringList& existingValues) {
        const QStringList incoming = JsonUtils::jsonArrayToStringList(appObj[key].toArray());
        for (const QString& value : incoming) {
            if (!existingValues.contains(value)) {
                return true;
            }
        }
        return false;
    };

    return scalarChanged("gender", existing.gender) ||
           intChanged("age", existing.age) ||
           scalarChanged("hairColor", existing.hairColor) ||
           scalarChanged("hairStyle", existing.hairStyle) ||
           scalarChanged("eyeColor", existing.eyeColor) ||
           scalarChanged("build", existing.build) ||
           scalarChanged("height", existing.height) ||
           listChanged("clothing", existing.clothing) ||
           listChanged("accessories", existing.accessories) ||
           listChanged("distinctiveFeatures", existing.distinctiveFeatures) ||
           listChanged("aliases", existing.aliases);
}

bool hasEmptyDetailsFields(const SceneDetails& det)
{
    const bool indoorScene = Inference::isIndoorSceneType(det.type);
    return det.description.isEmpty() || det.building.isEmpty() || det.color.isEmpty() ||
           det.landmark.isEmpty() || det.layout.isEmpty() || det.atmosphere.isEmpty() ||
           det.anchorPoints.isEmpty() || det.signatureObjects.isEmpty() ||
           det.fixedColorBlocks.isEmpty() || det.consistencyRules.isEmpty() ||
           det.type.isEmpty() || det.typeZh.isEmpty() || det.setting.isEmpty() || det.timeOfDay.isEmpty() ||
           det.currentInterpretation.isEmpty() || det.confidence.isEmpty() ||
           det.status.isEmpty() || det.narrativeRoleZh.isEmpty() || det.evidence.isEmpty() || det.aliases.isEmpty() ||
           det.history.isEmpty() ||
           (!indoorScene && det.weather.isEmpty());
}

bool hasNewDetailsData(const QJsonObject& detailsObj)
{
    QStringList keys = {"description", "building", "color", "landmark", "layout",
                        "atmosphere", "type", "typeZh", "setting", "timeOfDay", "weather", "spaceSize",
                        "currentInterpretation", "confidence", "status", "narrativeRole", "narrativeRoleZh"};
    for (const QString& key : keys) {
        if (!detailsObj[key].toString().isEmpty()) {
            return true;
        }
    }
    QStringList arrayKeys = {"anchorPoints", "signatureObjects", "fixedColorBlocks",
                             "consistencyRules", "evidence", "aliases", "history"};
    for (const QString& key : arrayKeys) {
        if (!detailsObj[key].toArray().isEmpty()) {
            return true;
        }
    }
    return false;
}

bool hasIterationUpdates(const SceneDetails& existing, const QJsonObject& detailsObj)
{
    auto scalarChanged = [&detailsObj](const QString& key, const QString& existingValue) {
        QString incoming = detailsObj[key].toString().trimmed();
        return !incoming.isEmpty() && incoming != existingValue.trimmed();
    };

    auto listChanged = [&detailsObj](const QString& key, const QStringList& existingValues) {
        QStringList incoming = JsonUtils::jsonArrayToStringList(detailsObj[key].toArray());
        for (const QString& value : incoming) {
            if (!existingValues.contains(value)) {
                return true;
            }
        }
        return false;
    };

    return scalarChanged("currentInterpretation", existing.currentInterpretation) ||
           scalarChanged("confidence", existing.confidence) ||
           scalarChanged("status", existing.status) ||
           listChanged("evidence", existing.evidence) ||
           listChanged("aliases", existing.aliases) ||
           listChanged("history", existing.history);
}

void updateCharacterAppearance(CharacterAppearance& app, const QJsonObject& appObj)
{
    const QJsonObject incomingSources = appObj.value("fieldSources").toObject();

    auto updateScalar = [&appObj, &app, &incomingSources](QString& field, const QString& key) {
        const QString incoming = appObj[key].toString().trimmed();
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(app.fieldSources, key);
        if (!incoming.isEmpty() && canAdoptFieldValue(field, existingSource, incomingSource)) {
            field = incoming;
            setFieldSource(app.fieldSources, key, normalizedSource(incomingSource, QStringLiteral("explicit")));
        }
    };
    auto updateInt = [&appObj, &app, &incomingSources](int& field, const QString& key) {
        const int incoming = appObj[key].toInt();
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(app.fieldSources, key);
        if (incoming > 0 && canAdoptFieldValue(field, existingSource, incomingSource)) {
            field = incoming;
            setFieldSource(app.fieldSources, key, normalizedSource(incomingSource, QStringLiteral("explicit")));
        }
    };
    auto updateList = [&appObj, &app, &incomingSources](QStringList& field, const QString& key) {
        const QStringList incoming = JsonUtils::jsonArrayToStringList(appObj[key].toArray());
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(app.fieldSources, key);
        if (incoming.isEmpty()) {
            return;
        }
        if (!canAdoptFieldValue(field, existingSource, incomingSource)) {
            return;
        }
        field = (field.isEmpty() || isExplicitSource(incomingSource) || isManualSource(incomingSource))
            ? incoming
            : mergeStringLists(field, incoming);
        setFieldSource(app.fieldSources, key, normalizedSource(incomingSource, QStringLiteral("inferred")));
    };

    updateScalar(app.gender, "gender");
    updateInt(app.age, "age");
    updateScalar(app.hairColor, "hairColor");
    updateScalar(app.hairStyle, "hairStyle");
    updateScalar(app.eyeColor, "eyeColor");
    updateScalar(app.build, "build");
    updateScalar(app.height, "height");
    updateList(app.clothing, "clothing");
    updateList(app.accessories, "accessories");
    updateList(app.distinctiveFeatures, "distinctiveFeatures");
    updateList(app.aliases, "aliases");
}

void updateSceneDetails(SceneDetails& det, const QJsonObject& detailsObj)
{
    const bool indoorScene = Inference::isIndoorSceneType(detailsObj["type"].toString()) ||
                             Inference::isIndoorSceneType(det.type);
    const QJsonObject incomingSources = detailsObj.value("fieldSources").toObject();

    auto updateField = [&detailsObj, &det, &incomingSources](const QString& key, QString& field) {
        QString value = detailsObj[key].toString().trimmed();
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(det.fieldSources, key);
        if (!value.isEmpty() && canAdoptFieldValue(field, existingSource, incomingSource)) {
            field = value;
            setFieldSource(det.fieldSources, key,
                normalizedSource(incomingSource, QStringLiteral("explicit")));
        }
    };
    auto updateListField = [&detailsObj, &det, &incomingSources](const QString& key, QStringList& field) {
        QStringList values = JsonUtils::jsonArrayToStringList(detailsObj[key].toArray());
        const QString incomingSource = incomingSources.value(key).toString();
        const QString existingSource = fieldSource(det.fieldSources, key);
        if (values.isEmpty()) {
            return;
        }
        if (!canAdoptFieldValue(field, existingSource, incomingSource)) {
            return;
        }
        field = (field.isEmpty() || isExplicitSource(incomingSource) || isManualSource(incomingSource))
            ? values
            : mergeStringLists(field, values);
        setFieldSource(det.fieldSources, key, normalizedSource(incomingSource, QStringLiteral("inferred")));
    };

    updateField("description", det.description);
    updateField("building", det.building);
    updateField("color", det.color);
    updateField("landmark", det.landmark);
    updateField("layout", det.layout);
    updateField("atmosphere", det.atmosphere);
    updateListField("anchorPoints", det.anchorPoints);
    updateListField("signatureObjects", det.signatureObjects);
    updateListField("fixedColorBlocks", det.fixedColorBlocks);
    updateListField("consistencyRules", det.consistencyRules);
    updateField("type", det.type);
    updateField("typeZh", det.typeZh);
    updateField("setting", det.setting);
    updateField("timeOfDay", det.timeOfDay);
    updateField("spaceSize", det.spaceSize);
    if (indoorScene) {
        det.weather.clear();
        det.weatherVariations = QJsonArray();
    } else {
        updateField("weather", det.weather);
    }
    updateField("currentInterpretation", det.currentInterpretation);
    updateField("confidence", det.confidence);
    updateField("status", det.status);
    updateField("narrativeRole", det.narrativeRole);
    updateField("narrativeRoleZh", det.narrativeRoleZh);
    updateListField("evidence", det.evidence);
    updateListField("aliases", det.aliases);
    updateListField("history", det.history);
}

int sourcePriority(const QString& source)
{
    if (isManualSource(source)) {
        return 3;
    }
    if (isExplicitSource(source)) {
        return 2;
    }
    if (isInferredSource(source)) {
        return 1;
    }
    return 0;
}

bool isLockedSource(const QString& source)
{
    return isManualSource(source) || isExplicitSource(source);
}

bool isManualSource(const QString& source)
{
    return source.compare(QStringLiteral("manual"), Qt::CaseInsensitive) == 0;
}

bool isExplicitSource(const QString& source)
{
    return source.compare(QStringLiteral("explicit"), Qt::CaseInsensitive) == 0;
}

bool isInferredSource(const QString& source)
{
    return source.compare(QStringLiteral("inferred"), Qt::CaseInsensitive) == 0;
}

QString fieldSource(const QJsonObject& sources, const QString& field)
{
    return sources.value(field).toString();
}

void setFieldSource(QJsonObject& sources, const QString& field, const QString& source)
{
    sources[field] = source;
}

QString normalizedSource(const QString& source, const QString& fallback)
{
    return source.isEmpty() ? fallback : source;
}

bool canReplaceSource(const QString& existingSource, const QString& incomingSource)
{
    return sourcePriority(existingSource) < sourcePriority(incomingSource);
}

void setUpdatedSource(QJsonObject& sources, const QString& field, const QString& incomingSource, const QString& fallback)
{
    setFieldSource(sources, field, normalizedSource(incomingSource, fallback));
}

void downgradeLegacyFieldSource(QJsonObject& sources, const QString& field)
{
    const QString current = fieldSource(sources, field);
    if (!current.isEmpty() && !isManualSource(current)) {
        setFieldSource(sources, field, QStringLiteral("inferred"));
    }
}

bool shouldUpdateFromSource(const QString& existingSource, const QString& incomingSource)
{
    return !incomingSource.isEmpty() && sourcePriority(incomingSource) > sourcePriority(existingSource);
}

bool canAdoptFieldValue(bool hasExistingValue,
                        const QString& existingSource,
                        const QString& incomingSource)
{
    if (incomingSource.isEmpty() || isLockedSource(existingSource)) {
        return false;
    }

    return !hasExistingValue || shouldUpdateFromSource(existingSource, incomingSource);
}

bool canAdoptFieldValue(const QString& existingValue,
                        const QString& existingSource,
                        const QString& incomingSource)
{
    return canAdoptFieldValue(!existingValue.trimmed().isEmpty(), existingSource, incomingSource);
}

bool canAdoptFieldValue(int existingValue,
                        const QString& existingSource,
                        const QString& incomingSource)
{
    return canAdoptFieldValue(existingValue > 0, existingSource, incomingSource);
}

bool canAdoptFieldValue(const QStringList& existingValues,
                        const QString& existingSource,
                        const QString& incomingSource)
{
    return canAdoptFieldValue(!existingValues.isEmpty(), existingSource, incomingSource);
}

bool hasDirectTextEvidence(const QString& sourceText, const QString& value)
{
    const QString trimmedSource = sourceText.trimmed();
    const QString trimmedValue = value.trimmed();
    return !trimmedSource.isEmpty() && !trimmedValue.isEmpty() &&
           trimmedSource.contains(trimmedValue, Qt::CaseInsensitive);
}

bool hasDirectTextEvidence(const QString& sourceText, const QStringList& values)
{
    for (const QString& value : values) {
        if (hasDirectTextEvidence(sourceText, value)) {
            return true;
        }
    }
    return false;
}

QString sourceFromTextEvidence(const QString& sourceText,
                               const QString& value,
                               const QString& inferredValue)
{
    Q_UNUSED(inferredValue);
    if (hasDirectTextEvidence(sourceText, value)) {
        return QStringLiteral("explicit");
    }
    return QStringLiteral("inferred");
}

QString sourceFromTextEvidence(const QString& sourceText,
                               const QStringList& values,
                               const QStringList& inferredValues)
{
    Q_UNUSED(inferredValues);
    if (hasDirectTextEvidence(sourceText, values)) {
        return QStringLiteral("explicit");
    }
    return QStringLiteral("inferred");
}

bool hasCharacterPersonalityChanges(const QStringList& existing, const QJsonArray& incomingArray)
{
    const QStringList incoming = JsonUtils::jsonArrayToStringList(incomingArray);
    for (const QString& value : incoming) {
        if (!existing.contains(value)) {
            return true;
        }
    }
    return false;
}

int fieldSourcePolicyVersion()
{
    return 3;
}

} // namespace BibleUpdateStrategy
} // namespace BibleUtils
