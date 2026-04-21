#ifndef SCENEEXTRACTOR_H
#define SCENEEXTRACTOR_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVariantMap>
#include <mutex>
#include "models/Scene.h"

struct ExtractedScene {
    QString id;
    QString name;
    QString description;
    QString building;
    QString color;
    QString landmark;
    QString layout;
    QString atmosphere;
    QStringList anchorPoints;
    QStringList signatureObjects;
    QStringList fixedColorBlocks;
    QStringList consistencyRules;
    QString type;
    QString typeZh;
    QString setting;
    QString timeOfDay;
    QString weather;
    QString spaceSize;
    QString currentInterpretation;
    QString confidence;
    QString status;
    QString narrativeRole;
    QString narrativeRoleZh;
    QStringList details;
    QStringList evidence;
    QStringList aliases;
    QStringList history;
    QJsonArray timeVariations;
    QJsonArray weatherVariations;
    
    QJsonObject toJson() const;
    static ExtractedScene fromJson(const QJsonObject& json);
};

class SceneExtractor : public QObject
{
    Q_OBJECT

public:
    static SceneExtractor* instance();
    
    QList<ExtractedScene> extractFromScenes(const QJsonArray& scenes);
    ExtractedScene parseAIScene(const QJsonObject& sceneObj);
    
    bool saveScene(const QString& novelId, const ExtractedScene& scene);
    int saveScenes(const QString& novelId, const QList<ExtractedScene>& scenes);
    
    QList<Scene> getScenesByNovel(const QString& novelId);
    Scene getSceneById(const QString& sceneId);
    Scene getSceneByName(const QString& novelId, const QString& name);
    Scene getSceneBySceneId(const QString& novelId, const QString& sceneId);
    int getSceneCountByNovel(const QString& novelId);
    
    bool updateScene(const Scene& scene);
    bool updateSceneSilent(const Scene& scene);
    bool deleteScene(const QString& sceneId);
    
signals:
    void sceneExtracted(const ExtractedScene& scene);
    void sceneSaved(const QString& sceneId, const QString& sceneName);
    void sceneUpdated(const QString& sceneId, const QString& sceneName);
    void sceneCountChanged(int count);
    void extractionCompleted(int count);

private:
    explicit SceneExtractor(QObject* parent = nullptr);
    ~SceneExtractor();
    SceneExtractor(const SceneExtractor&) = delete;
    SceneExtractor& operator=(const SceneExtractor&) = delete;
    
    Scene toScene(const ExtractedScene& extracted, const QString& novelId);
    Scene mergeScenes(const Scene& existing, const ExtractedScene& incoming);
    QVariantMap sceneToData(const Scene& scene) const;
    Scene sceneFromRow(const QVariantMap& row);
    QString generateId();
    Scene findExistingScene(const QString& novelId, const QString& sceneId, const QString& name);
    void inferSceneDetails(ExtractedScene& scene);
    Scene preserveReferenceImagePath(Scene scene);
    
    static bool containsChinese(const QString& text);
    
    static SceneExtractor* m_instance;
    static std::once_flag m_instanceOnceFlag;
};

#endif // SCENEEXTRACTOR_H
