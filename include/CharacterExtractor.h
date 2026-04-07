#ifndef CHARACTEREXTRACTOR_H
#define CHARACTEREXTRACTOR_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVariantMap>
#include <QMap>
#include "Character.h"

struct ExtractedCharacter {
    QString name;
    QString gender;
    int age = 0;
    QString hairColor;
    QString hairStyle;
    QString eyeColor;
    QString bodyType;
    QString clothing;
    QString features;
    QStringList personality;
    QStringList tags;
    
    QJsonObject toJson() const;
    static ExtractedCharacter fromJson(const QJsonObject& json);
};

class CharacterExtractor : public QObject
{
    Q_OBJECT

public:
    static CharacterExtractor* instance();
    
    QList<ExtractedCharacter> extractFromPanels(const QJsonArray& panels);
    QList<ExtractedCharacter> extractFromCharacters(const QJsonArray& characters);
    
    bool saveCharacter(const QString& novelId, const ExtractedCharacter& character);
    int saveCharacters(const QString& novelId, const QList<ExtractedCharacter>& characters);
    
    QList<Character> getCharactersByNovel(const QString& novelId);
    Character getCharacterById(const QString& characterId);
    Character getCharacterByName(const QString& novelId, const QString& name);
    bool updateCharacter(const Character& character);
    bool deleteCharacter(const QString& characterId);
    
signals:
    void characterExtracted(const ExtractedCharacter& character);
    void characterSaved(const QString& characterId);
    void extractionCompleted(int count);
    void characterUpdated(const QString& characterId, const QString& portraitPath);

private:
    CharacterExtractor(QObject* parent = nullptr);
    ~CharacterExtractor() = default;
    CharacterExtractor(const CharacterExtractor&) = delete;
    CharacterExtractor& operator=(const CharacterExtractor&) = delete;
    
    ExtractedCharacter parsePanelCharacter(const QJsonValue& charVal);
    ExtractedCharacter parseAICharacter(const QJsonObject& charObj);
    Character toCharacter(const ExtractedCharacter& extracted, const QString& novelId);
    QVariantMap characterToData(const Character& character) const;
    Character characterFromRow(const QVariantMap& row);
    QString generateId();
    
    static bool containsChinese(const QString& text);
    
    // 合并相关方法
    Character mergeCharacters(const Character& existing, const ExtractedCharacter& incoming) const;
    QStringList mergeStringLists(const QStringList& a, const QStringList& b) const;
    
    static CharacterExtractor* m_instance;
};

#endif // CHARACTEREXTRACTOR_H
