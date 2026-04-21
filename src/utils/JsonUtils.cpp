#include "utils/JsonUtils.h"
#include "models/Character.h"
#include "models/Storyboard.h"
#include "models/Panel.h"
#include "models/Job.h"

namespace JsonUtils {

QJsonObject toJson(const Character& character)
{
    QJsonObject obj;
    obj["id"] = character.id();
    obj["novelId"] = character.novelId();
    obj["name"] = character.name();
    obj["role"] = character.role();
    obj["appearance"] = toJson(character.appearance());
    obj["personality"] = toJsonArray(character.personality());
    return obj;
}

Character toCharacter(const QJsonObject& obj)
{
    Character character;
    character.setId(get<QString>(obj, "id"));
    character.setNovelId(get<QString>(obj, "novelId"));
    character.setName(get<QString>(obj, "name"));
    character.setRole(get<QString>(obj, "role"));
    character.setAppearance(toAppearance(getObject(obj, "appearance")));
    character.setPersonality(toStringList(getArray(obj, "personality")));
    return character;
}

QJsonObject toJson(const CharacterAppearance& appearance)
{
    QJsonObject obj;
    obj["gender"] = appearance.gender;
    obj["age"] = appearance.age;
    obj["hairColor"] = appearance.hairColor;
    obj["hairStyle"] = appearance.hairStyle;
    obj["eyeColor"] = appearance.eyeColor;
    obj["height"] = appearance.height;
    obj["build"] = appearance.build;
    obj["clothing"] = toJsonArray(appearance.clothing);
    obj["accessories"] = toJsonArray(appearance.accessories);
    obj["distinctiveFeatures"] = toJsonArray(appearance.distinctiveFeatures);
    return obj;
}

CharacterAppearance toAppearance(const QJsonObject& obj)
{
    CharacterAppearance appearance;
    appearance.gender = get<QString>(obj, "gender");
    appearance.age = get<int>(obj, "age");
    appearance.hairColor = get<QString>(obj, "hairColor");
    appearance.hairStyle = get<QString>(obj, "hairStyle");
    appearance.eyeColor = get<QString>(obj, "eyeColor");
    appearance.height = get<QString>(obj, "height");
    appearance.build = get<QString>(obj, "build");
    appearance.clothing = toStringList(getArray(obj, "clothing"));
    appearance.accessories = toStringList(getArray(obj, "accessories"));
    appearance.distinctiveFeatures = toStringList(getArray(obj, "distinctiveFeatures"));
    return appearance;
}

QJsonObject toJson(const Storyboard& storyboard)
{
    QJsonObject obj;
    obj["id"] = storyboard.id();
    obj["novelId"] = storyboard.novelId();
    obj["totalPages"] = storyboard.totalPages();
    obj["panelCount"] = storyboard.panelCount();
    obj["version"] = storyboard.version();
    return obj;
}

Storyboard toStoryboard(const QJsonObject& obj)
{
    Storyboard storyboard;
    storyboard.setId(get<QString>(obj, "id"));
    storyboard.setNovelId(get<QString>(obj, "novelId"));
    storyboard.setTotalPages(get<int>(obj, "totalPages"));
    storyboard.setPanelCount(get<int>(obj, "panelCount"));
    storyboard.setVersion(get<int>(obj, "version"));
    return storyboard;
}

QJsonObject toJson(const Panel& panel)
{
    QJsonObject obj;
    obj["id"] = panel.id();
    obj["storyboardId"] = panel.storyboardId();
    obj["page"] = panel.page();
    obj["index"] = panel.index();
    obj["scene"] = panel.scene();
    obj["visualPrompt"] = panel.visualPrompt();
    obj["previewS3Key"] = panel.previewS3Key();
    obj["hdS3Key"] = panel.hdS3Key();
    obj["rawContent"] = panel.rawContent();
    return obj;
}

Panel toPanel(const QJsonObject& obj)
{
    Panel panel;
    panel.setId(get<QString>(obj, "id"));
    panel.setStoryboardId(get<QString>(obj, "storyboardId"));
    panel.setPage(get<int>(obj, "page"));
    panel.setIndex(get<int>(obj, "index"));
    
    if (obj.contains("rawContent")) {
        panel.setContent(getObject(obj, "rawContent"));
    } else {
        panel.setContent(getObject(obj, "content"));
    }
    
    panel.setScene(get<QString>(obj, "scene"));
    panel.setVisualPrompt(get<QString>(obj, "visualPrompt"));
    panel.setPreviewS3Key(get<QString>(obj, "previewS3Key"));
    panel.setHdS3Key(get<QString>(obj, "hdS3Key"));
    
    return panel;
}

QJsonObject toJson(const Job& job)
{
    QJsonObject obj;
    obj["id"] = job.id();
    obj["type"] = job.type();
    obj["status"] = Job::statusToString(job.status());
    obj["novelId"] = job.novelId();
    obj["storyboardId"] = job.storyboardId();
    obj["total"] = job.total();
    obj["completed"] = job.completed();
    obj["failed"] = job.failed();
    set(obj, "createdAt", job.createdAt());
    set(obj, "updatedAt", job.updatedAt());
    return obj;
}

Job toJob(const QJsonObject& obj)
{
    Job job;
    job.setId(get<QString>(obj, "id"));
    job.setType(get<QString>(obj, "type"));
    job.setStatus(Job::stringToStatus(get<QString>(obj, "status")));
    job.setNovelId(get<QString>(obj, "novelId"));
    job.setStoryboardId(get<QString>(obj, "storyboardId"));
    job.setTotal(get<int>(obj, "total"));
    job.setCompleted(get<int>(obj, "completed"));
    job.setFailed(get<int>(obj, "failed"));
    job.setCreatedAt(getDateTime(obj, "createdAt"));
    job.setUpdatedAt(getDateTime(obj, "updatedAt"));
    return job;
}

}
