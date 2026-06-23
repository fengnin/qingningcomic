#ifndef CHARACTEREXTRACTOR_H
#define CHARACTEREXTRACTOR_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVariantMap>
#include <QMap>
#include <QMetaType>
#include <mutex>
#include "models/Character.h"
#include "models/CharacterPortraitVersion.h"

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
    QJsonObject fieldSources;

    QJsonObject toJson() const;
    static ExtractedCharacter fromJson(const QJsonObject& json);
};

// 角色视觉字段变更：用于触发圣经图增量编辑（半自动草稿）
struct CharacterFieldDiff {
    QString field;       // hairColor / hairStyle / eyeColor / height / build / clothing / accessories / distinctiveFeatures
    QString oldValue;    // 列表字段以 "、" 拼接
    QString newValue;
    QString sourceFrom;  // 旧来源：explicit / inferred / manual / ""
    QString sourceTo;    // 新来源
};
Q_DECLARE_METATYPE(CharacterFieldDiff)
Q_DECLARE_METATYPE(QList<CharacterFieldDiff>)

class CharacterExtractor : public QObject
{
    Q_OBJECT

public:
    static CharacterExtractor* instance();

    QList<ExtractedCharacter> extractFromPanels(const QJsonArray& panels);
    QList<ExtractedCharacter> extractFromCharacters(const QJsonArray& characters);
    QList<ExtractedCharacter> extractFromCharacters(const QJsonArray& characters, const QString& sourceText);

    bool saveCharacter(const QString& novelId, const ExtractedCharacter& character);
    bool saveCharacter(const QString& novelId, const ExtractedCharacter& character, int sourceChapter);
    int saveCharacters(const QString& novelId, const QList<ExtractedCharacter>& characters);
    int saveCharacters(const QString& novelId, const QList<ExtractedCharacter>& characters, int sourceChapter);

    QList<Character> getCharactersByNovel(const QString& novelId);
    Character getCharacterById(const QString& characterId);
    Character getCharacterByName(const QString& novelId, const QString& name);
    bool updateCharacter(const Character& character);
    bool updateCharacterSilent(const Character& character);
    bool deleteCharacter(const QString& characterId);

    // 角色肖像版本 CRUD
    QList<CharacterPortraitVersion> loadPortraitVersions(const QString& characterId);
    // 批量加载：一次查询返回 novelId 下所有角色的版本，key = character_id
    QMap<QString, QList<CharacterPortraitVersion>> loadPortraitVersionsByNovel(const QString& novelId);
    CharacterPortraitVersion getPortraitVersion(const QString& versionId);
    int nextVersionNo(const QString& characterId);
    bool addPortraitVersion(const CharacterPortraitVersion& version);
    bool setCurrentPortraitVersion(const QString& characterId, const QString& versionId);
    bool deletePortraitVersion(const QString& versionId);
    bool updateCurrentVersionSnapshot(const QString& characterId, const QJsonObject& snapshot);
    // 懒迁移：旧角色已有 portrait_path 但无版本行时，补一条 v1
    CharacterPortraitVersion ensureFirstPortraitVersion(const Character& character);

signals:
    void characterExtracted(const ExtractedCharacter& character);
    void characterSaved(const QString& characterId);
    void extractionCompleted(int count);
    void characterUpdated(const QString& characterId, const QString& portraitPath);
    // 视觉字段发生变化：UI 据此弹出半自动编辑草稿
    void characterVisualFieldsChanged(const QString& characterId,
                                     const QList<CharacterFieldDiff>& diff,
                                     int sourceChapter);

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
    bool persistCharacterRecord(const Character& character, bool emitSignals);

    static QString normalizeCharacterName(const QString& name);

    Character mergeCharacters(const Character& existing, const ExtractedCharacter& incoming) const;
    // 重载：同时输出视觉字段 diff（白名单字段）。outDiff 可为 nullptr。
    Character mergeCharacters(const Character& existing,
                              const ExtractedCharacter& incoming,
                              QList<CharacterFieldDiff>* outDiff) const;

    static CharacterExtractor* m_instance;
    static std::once_flag m_instanceOnceFlag;
};

#endif // CHARACTEREXTRACTOR_H
