#ifndef BIBLEGENERATOR_H
#define BIBLEGENERATOR_H

#include <QObject>
#include <QJsonArray>
#include <QString>
#include <mutex>
#include "Character.h"
#include "Scene.h"

class BibleGenerator : public QObject
{
    Q_OBJECT

public:
    static BibleGenerator* instance();
    
    QJsonArray collectExistingCharacters(const QString& novelId);
    QJsonArray collectExistingScenes(const QString& novelId);
    
    QJsonObject characterToJson(const Character& character) const;
    QJsonObject sceneToJson(const Scene& scene) const;
    
    void updateExistingCharacters(const QString& novelId, const QJsonArray& characters);
    void updateExistingScenes(const QString& novelId, const QJsonArray& scenes);
    
    bool shouldUpdateCharacter(const Character& existing, const QJsonObject& newData) const;
    bool shouldUpdateScene(const Scene& existing, const QJsonObject& newData) const;

signals:
    void charactersCollected(int count);
    void scenesCollected(int count);
    void characterUpdated(const QString& name);
    void sceneUpdated(const QString& name);

private:
    explicit BibleGenerator(QObject* parent = nullptr);
    ~BibleGenerator() = default;
    BibleGenerator(const BibleGenerator&) = delete;
    BibleGenerator& operator=(const BibleGenerator&) = delete;
    
    static BibleGenerator* m_instance;
    static std::once_flag m_instanceOnceFlag;
};

#endif // BIBLEGENERATOR_H
