#ifndef SCENEEXTRACTOR_H
#define SCENEEXTRACTOR_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVariantMap>
#include "Scene.h"

struct ExtractedScene {
    QString id;
    QString name;
    QString description;
    QString building;
    QString color;
    QString landmark;
    QString layout;
    QString atmosphere;
    QString type;
    QString setting;
    QString timeOfDay;
    QString weather;
    QStringList details;
    
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
    Scene getSceneBySceneId(const QString& novelId, const QString& sceneId);
    int getSceneCountByNovel(const QString& novelId);
    
    bool updateScene(const Scene& scene);
    bool deleteScene(const QString& sceneId);
    
signals:
    void sceneExtracted(const ExtractedScene& scene);
    void sceneSaved(const QString& sceneId, const QString& sceneName);
    void sceneUpdated(const QString& sceneId, const QString& sceneName);
    void sceneCountChanged(int count);
    void extractionCompleted(int count);

private:
    explicit SceneExtractor(QObject* parent = nullptr);
    ~SceneExtractor() = default;
    SceneExtractor(const SceneExtractor&) = delete;
    SceneExtractor& operator=(const SceneExtractor&) = delete;
    
    Scene toScene(const ExtractedScene& extracted, const QString& novelId);
    QVariantMap sceneToData(const Scene& scene) const;
    Scene sceneFromRow(const QVariantMap& row);
    QString generateId();
    Scene findExistingScene(const QString& novelId, const QString& sceneId, const QString& name);
    
    static bool containsChinese(const QString& text);
    
    static SceneExtractor* m_instance;
};

#endif // SCENEEXTRACTOR_H
