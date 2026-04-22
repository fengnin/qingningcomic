#ifndef CHARACTEREXTRACTOR_H
#define CHARACTEREXTRACTOR_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVariantMap>
#include <QMap>
#include <mutex>
#include "models/Character.h"

struct ExtractedCharacter {
    QString name;
    QString role;
    QString gender;
    int age = 0;
    QString hairColor;
    QString hairStyle;
    QString eyeColor;
    QString bodyType;
    QString height;
    QStringList clothing;
    QStringList accessories;
    QStringList distinctiveFeatures;
    QStringList aliases;
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
    QList<ExtractedCharacter> extractFromCharacters(const QJsonArray& characters, const QString& sourceText);
    
    bool saveCharacter(const QString& novelId, const ExtractedCharacter& character);
    int saveCharacters(const QString& novelId, const QList<ExtractedCharacter>& characters);
    
    QList<Character> getCharactersByNovel(const QString& novelId);
    Character getCharacterById(const QString& characterId);
    Character getCharacterByName(const QString& novelId, const QString& name);
    bool updateCharacter(const Character& character);
    bool updateCharacterSilent(const Character& character);
    bool deleteCharacter(const QString& characterId);
    
signals:
    void characterExtracted(const ExtractedCharacter& character);
    void characterSaved(const QString& characterId);
    void extractionCompleted(int count);
    void characterUpdated(const QString& characterId, const QString& portraitPath);

public:
    static QStringList buildIdentityKeys(const Character& character);
    static QStringList buildIdentityKeys(const ExtractedCharacter& character);
    static bool isLikelySameCharacter(const Character& existing, const ExtractedCharacter& incoming);

private:
    CharacterExtractor(QObject* parent = nullptr);
    ~CharacterExtractor();
    CharacterExtractor(const CharacterExtractor&) = delete;
    CharacterExtractor& operator=(const CharacterExtractor&) = delete;
    
    ExtractedCharacter parsePanelCharacter(const QJsonValue& charVal);
    ExtractedCharacter parseAICharacter(const QJsonObject& charObj);
    void enrichCharacterFromText(ExtractedCharacter& extracted, const QString& sourceText);
    Character toCharacter(const ExtractedCharacter& extracted, const QString& novelId);
    QVariantMap characterToData(const Character& character) const;
    Character characterFromRow(const QVariantMap& row);
    
    // 名称标准化：去除AI生成的常见后缀，如"（本体）"、"（真身）"等
    static QString normalizeCharacterName(const QString& name);
    
    // 合并相关方法
    Character mergeCharacters(const Character& existing, const ExtractedCharacter& incoming) const;
    
    static CharacterExtractor* m_instance;
    static std::once_flag m_instanceOnceFlag;
};

#endif // CHARACTEREXTRACTOR_H
