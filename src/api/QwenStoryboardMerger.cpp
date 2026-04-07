#include "api/QwenStoryboardMerger.h"
#include <QMap>
#include <QSet>

static QString getStringValue(const QJsonObject& obj, const QString& key)
{
    return obj.value(key).toString();
}

static QJsonArray getArrayValue(const QJsonObject& obj, const QString& key)
{
    return obj.value(key).toArray();
}

QJsonObject QwenStoryboardMerger::normalizePanel(const QJsonObject& panel, int index)
{
    QJsonObject normalized;
    
    int page = index / PANELS_PER_PAGE + 1;
    int pageIndex = index % PANELS_PER_PAGE;
    normalized["page"] = page;
    normalized["index"] = pageIndex;
    
    normalized["scene"] = getStringValue(panel, "scene");
    if (normalized["scene"].toString().isEmpty()) {
        normalized["scene"] = getStringValue(panel, "description");
    }
    
    QJsonObject background;
    QJsonObject bgObj = panel.value("background").toObject();
    background["sceneId"] = getStringValue(bgObj, "sceneId");
    background["setting"] = getStringValue(bgObj, "setting");
    background["timeOfDay"] = getStringValue(bgObj, "timeOfDay");
    background["weather"] = getStringValue(bgObj, "weather");
    background["lighting"] = getStringValue(bgObj, "lighting");
    normalized["background"] = background;
    
    normalized["atmosphere"] = getStringValue(panel, "atmosphere");
    if (normalized["atmosphere"].toString().isEmpty()) {
        normalized["atmosphere"] = getStringValue(bgObj, "atmosphere");
    }
    
    normalized["shotType"] = getStringValue(panel, "shotType");
    normalized["cameraAngle"] = getStringValue(panel, "cameraAngle");
    
    QJsonArray chars = getArrayValue(panel, "characters");
    normalized["characters"] = chars;
    
    QJsonArray dialogue = getArrayValue(panel, "dialogue");
    normalized["dialogue"] = dialogue;
    
    normalized["visualPrompt"] = getStringValue(panel, "visualPrompt");
    normalized["visualPromptEn"] = getStringValue(panel, "visualPromptEn");
    
    return normalized;
}

QJsonObject QwenStoryboardMerger::mergeCharacter(const QJsonObject& existing, const QJsonObject& newChar)
{
    QJsonObject merged = existing;
    
    QString newName = newChar["name"].toString();
    if (!newName.isEmpty() && merged["name"].toString().isEmpty()) {
        merged["name"] = newName;
    }
    
    if (newChar.contains("role") && !merged.contains("role")) {
        merged["role"] = newChar["role"];
    }
    
    if (newChar.contains("appearance") && merged.contains("appearance")) {
        QJsonObject newApp = newChar["appearance"].toObject();
        QJsonObject existingApp = merged["appearance"].toObject();
        QJsonObject mergedApp = existingApp;
        
        for (auto it = newApp.begin(); it != newApp.end(); ++it) {
            if (!existingApp.contains(it.key())) {
                mergedApp[it.key()] = it.value();
            }
        }
        merged["appearance"] = mergedApp;
    } else if (newChar.contains("appearance")) {
        merged["appearance"] = newChar["appearance"];
    }
    
    if (newChar.contains("personality") && !merged.contains("personality")) {
        merged["personality"] = newChar["personality"];
    }
    
    return merged;
}

QJsonObject QwenStoryboardMerger::merge(const QList<QJsonObject>& storyboards)
{
    if (storyboards.isEmpty()) {
        return QJsonObject();
    }
    
    QJsonArray mergedPanels;
    QMap<QString, QJsonObject> charMap;
    QMap<QString, QJsonObject> sceneMap;
    
    int panelIndex = 0;
    for (const QJsonObject& storyboard : storyboards) {
        QJsonArray panels = storyboard["panels"].toArray();
        
        for (const QJsonValue& panelVal : panels) {
            QJsonObject panel = panelVal.toObject();
            QJsonObject normalized = normalizePanel(panel, panelIndex);
            mergedPanels.append(normalized);
            panelIndex++;
            
            QJsonArray chars = panel["characters"].toArray();
            for (const QJsonValue& charVal : chars) {
                QJsonObject charObj = charVal.toObject();
                QString name = charObj["name"].toString();
                if (name.isEmpty()) {
                    continue;
                }
                
                if (!charMap.contains(name)) {
                    charMap[name] = charObj;
                } else {
                    charMap[name] = mergeCharacter(charMap[name], charObj);
                }
            }
            
            QJsonArray scenes = panel["scenes"].toArray();
            for (const QJsonValue& sceneVal : scenes) {
                QJsonObject scene = sceneVal.toObject();
                QString id = scene["id"].toString();
                if (id.isEmpty()) {
                    continue;
                }
                
                if (!sceneMap.contains(id)) {
                    sceneMap[id] = scene;
                }
            }
        }
    }
    
    QJsonObject result;
    result["panels"] = mergedPanels;
    
    QJsonArray charactersArray;
    for (const QJsonObject& c : charMap) {
        charactersArray.append(c);
    }
    result["characters"] = charactersArray;
    
    QJsonArray scenesArray;
    for (const QJsonObject& s : sceneMap) {
        scenesArray.append(s);
    }
    result["scenes"] = scenesArray;
    result["totalPages"] = (mergedPanels.size() + PANELS_PER_PAGE - 1) / PANELS_PER_PAGE;
    
    return result;
}

