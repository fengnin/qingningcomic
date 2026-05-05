
#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaType>
#include <QVariant>

/**
 * @brief 角色姿态信息
 */
struct CharacterPose {
    QString charId;
    QString name;
    QString pose;
    QString expression;
    
    CharacterPose() = default;
    CharacterPose(const QString &cid, const QString &n, 
                  const QString &p = QString(), const QString &e = QString())
        : charId(cid), name(n), pose(p), expression(e) {}
};
Q_DECLARE_METATYPE(CharacterPose)

/**
 * @brief 对白信息
 */
struct DialogueLine {
    QString speaker;
    QString text;
    QString bubbleType;
    QString speakerSide;
    
    DialogueLine() : bubbleType(QStringLiteral("speech")) {}
    DialogueLine(const QString &s, const QString &t, const QString &bt = QStringLiteral("speech"))
        : speaker(s), text(t), bubbleType(bt) {}
    DialogueLine(const QString &s, const QString &t, const QString &bt, const QString &side)
        : speaker(s), text(t), bubbleType(bt), speakerSide(side) {}
};
Q_DECLARE_METATYPE(DialogueLine)

/**
 * @brief 面板数据模型
 * 
 * 数据管理策略：
 * - scene/shotType/cameraAngle 可通过 setScene/setShotType/setCameraAngle 单独设置
 * - 也可通过 setContent() 批量设置（仅更新 content 中存在的字段）
 * - setContent() 不会清空已设置但 content 中不存在的字段
 */
class Panel
{
    Q_GADGET
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString storyboardId READ storyboardId WRITE setStoryboardId)
    Q_PROPERTY(int page READ page WRITE setPage)
    Q_PROPERTY(int index READ index WRITE setIndex)
    Q_PROPERTY(QString scene READ scene WRITE setScene)
    Q_PROPERTY(QString shotType READ shotType WRITE setShotType)
    Q_PROPERTY(QString cameraAngle READ cameraAngle WRITE setCameraAngle)
    Q_PROPERTY(QString visualPrompt READ visualPrompt WRITE setVisualPrompt)
    Q_PROPERTY(QString visualPromptEn READ visualPromptEn WRITE setVisualPromptEn)
    Q_PROPERTY(QString status READ status WRITE setStatus)
    Q_PROPERTY(QString previewS3Key READ previewS3Key WRITE setPreviewS3Key)
    Q_PROPERTY(QString hdS3Key READ hdS3Key WRITE setHdS3Key)
    Q_PROPERTY(QString previewUrl READ previewUrl WRITE setPreviewUrl)
    Q_PROPERTY(QString hdUrl READ hdUrl WRITE setHdUrl)

public:
    Panel();
    ~Panel() = default;

    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString storyboardId() const { return m_storyboardId; }
    void setStoryboardId(const QString& storyboardId) { m_storyboardId = storyboardId; }

    int page() const { return m_page; }
    void setPage(int page) { m_page = page; }

    int index() const { return m_index; }
    void setIndex(int index) { m_index = index; }

    QString scene() const { return m_scene; }
    void setScene(const QString& scene) { m_scene = scene; }

    QString shotType() const { return m_shotType; }
    void setShotType(const QString& shotType) { m_shotType = shotType; }

    QString cameraAngle() const { return m_cameraAngle; }
    void setCameraAngle(const QString& cameraAngle) { m_cameraAngle = cameraAngle; }

    QString visualPrompt() const { return m_visualPrompt; }
    void setVisualPrompt(const QString& prompt) { m_visualPrompt = prompt; }

    QString visualPromptEn() const { return m_visualPromptEn; }
    void setVisualPromptEn(const QString& prompt) { m_visualPromptEn = prompt; }

    QString status() const { return m_status; }
    void setStatus(const QString& status) { m_status = status; }

    QString previewS3Key() const { return m_previewS3Key; }
    void setPreviewS3Key(const QString& key) { m_previewS3Key = key; }

    QString hdS3Key() const { return m_hdS3Key; }
    void setHdS3Key(const QString& key) { m_hdS3Key = key; }

    QString previewUrl() const { return m_previewUrl; }
    void setPreviewUrl(const QString& url) { m_previewUrl = url; }

    QString hdUrl() const { return m_hdUrl; }
    void setHdUrl(const QString& url) { m_hdUrl = url; }

    QList<CharacterPose> characters() const { return m_characters; }
    void setCharacters(const QList<CharacterPose>& characters) { m_characters = characters; }
    void addCharacter(const CharacterPose& character) { m_characters.append(character); }

    QList<DialogueLine> dialogue() const { return m_dialogue; }
    void setDialogue(const QList<DialogueLine>& dialogue) { m_dialogue = dialogue; }
    void addDialogue(const DialogueLine& line) { m_dialogue.append(line); }

    QJsonObject content() const;
    QJsonObject applyUpdatesKeepingStableFields(const QJsonObject& updates) const;
    void setContent(const QJsonObject& content);
    
    void mergeContent(const QJsonObject& extraContent);
    
    QJsonObject rawContent() const { return m_rawContent; }
    void setRawContent(const QJsonObject& content) { m_rawContent = content; }

    QString positionLabel() const;
    QString charactersText() const;
    QString dialogueText() const;
    
    bool isValid() const { return !m_id.isEmpty(); }
    bool hasPreview() const { return !m_previewUrl.isEmpty() || !m_previewS3Key.isEmpty(); }
    bool hasHdImage() const { return !m_hdUrl.isEmpty() || !m_hdS3Key.isEmpty(); }
    bool isPending() const { return m_status == "pending"; }
    bool isCompleted() const { return m_status == "completed"; }
    
    void clear();
    QStringList characterNames() const;
    QStringList dialogueTexts() const;

private:
    QJsonArray charactersToJsonArray() const;
    QJsonArray dialogueToJsonArray() const;
    void parseSceneFields(const QJsonObject& content);
    void parseCharacters(const QJsonObject& content);
    void parseDialogue(const QJsonObject& content);

    QString m_id;
    QString m_storyboardId;
    int m_page = 0;
    int m_index = 0;
    QString m_scene;
    QString m_shotType;
    QString m_cameraAngle;
    QString m_visualPrompt;
    QString m_visualPromptEn;
    QString m_status = QStringLiteral("pending");
    QString m_previewS3Key;
    QString m_hdS3Key;
    QString m_previewUrl;
    QString m_hdUrl;
    QJsonObject m_rawContent;
    QList<CharacterPose> m_characters;
    QList<DialogueLine> m_dialogue;
};

Q_DECLARE_METATYPE(Panel)
