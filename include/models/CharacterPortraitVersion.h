#ifndef CHARACTERPORTRAITVERSION_H
#define CHARACTERPORTRAITVERSION_H

#include <QString>
#include <QJsonObject>
#include <QVariantMap>
#include <QDateTime>
#include <QMetaType>

class CharacterPortraitVersion
{
public:
    CharacterPortraitVersion() = default;

    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString characterId() const { return m_characterId; }
    void setCharacterId(const QString& id) { m_characterId = id; }

    int versionNo() const { return m_versionNo; }
    void setVersionNo(int v) { m_versionNo = v; }

    QString portraitPath() const { return m_portraitPath; }
    void setPortraitPath(const QString& p) { m_portraitPath = p; }

    QString baseVersionId() const { return m_baseVersionId; }
    void setBaseVersionId(const QString& id) { m_baseVersionId = id; }

    QString editPrompt() const { return m_editPrompt; }
    void setEditPrompt(const QString& p) { m_editPrompt = p; }

    QJsonObject fieldDiff() const { return m_fieldDiff; }
    void setFieldDiff(const QJsonObject& d) { m_fieldDiff = d; }

    QJsonObject appearanceSnapshot() const { return m_appearanceSnapshot; }
    void setAppearanceSnapshot(const QJsonObject& s) { m_appearanceSnapshot = s; }

    int sourceChapter() const { return m_sourceChapter; }
    void setSourceChapter(int c) { m_sourceChapter = c; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& t) { m_createdAt = t; }

    QVariantMap toVariantMap() const;
    static CharacterPortraitVersion fromVariantMap(const QVariantMap& map);

    QJsonObject toJson() const;
    static CharacterPortraitVersion fromJson(const QJsonObject& json);

private:
    QString m_id;
    QString m_characterId;
    int m_versionNo = 0;
    QString m_portraitPath;
    QString m_baseVersionId;
    QString m_editPrompt;
    QJsonObject m_fieldDiff;
    QJsonObject m_appearanceSnapshot;
    int m_sourceChapter = 0;
    QDateTime m_createdAt;
};

Q_DECLARE_METATYPE(CharacterPortraitVersion)

#endif // CHARACTERPORTRAITVERSION_H
