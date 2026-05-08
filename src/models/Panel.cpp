
#include "models/Panel.h"
#include "utils/DialogueTextUtils.h"
#include "utils/Logger.h"

namespace {
    template<typename T>
    QList<T> parseJsonArray(const QJsonValue& value, std::function<T(const QJsonObject&)> parser, const QString& fieldName)
    {
        QList<T> result;
        if (!value.isArray()) {
            return result;
        }
        
        QJsonArray arr = value.toArray();
        for (int i = 0; i < arr.size(); ++i) {
            QJsonValue v = arr[i];
            if (!v.isObject()) {
                LOG_WARNING("Panel", QString("%1[%2] is not an object, skipping").arg(fieldName).arg(i));
                continue;
            }
            result.append(parser(v.toObject()));
        }
        return result;
    }

    QString formatDialogueLineText(const DialogueLine& line)
    {
        if (line.text.isEmpty()) {
            return line.speaker;
        }
        return QString::fromUtf8(u8"%1\uff1a%2").arg(line.speaker, line.text);
    }

    template<typename T, typename Selector>
    QStringList collectStrings(const QList<T>& items, Selector selector)
    {
        QStringList result;
        result.reserve(items.size());
        for (const T& item : items) {
            result << selector(item);
        }
        return result;
    }

    void copyIfPresent(QJsonObject& target, const QJsonObject& source, const QString& key)
    {
        if (source.contains(key)) {
            target[key] = source.value(key);
        }
    }

    void copyStableContentFields(QJsonObject& target, const QJsonObject& source)
    {
        copyIfPresent(target, source, "background");
        copyIfPresent(target, source, "sceneId");
    }
}

Panel::Panel()
    : m_page(0)
    , m_index(0)
{
}

QJsonObject Panel::content() const
{
    QJsonObject obj = m_rawContent;
    obj["scene"] = m_scene;
    obj["shotType"] = m_shotType;
    obj["cameraAngle"] = m_cameraAngle;
    obj["visualPrompt"] = m_visualPrompt;
    obj["visualPromptEn"] = m_visualPromptEn;
    copyStableContentFields(obj, m_rawContent);
    obj["characters"] = charactersToJsonArray();
    obj["dialogue"] = dialogueToJsonArray();
    return obj;
}

QJsonObject Panel::applyUpdatesKeepingStableFields(const QJsonObject& updates) const
{
    QJsonObject merged = m_rawContent;
    for (auto it = updates.begin(); it != updates.end(); ++it) {
        merged[it.key()] = it.value();
    }
    copyStableContentFields(merged, m_rawContent);
    return merged;
}

void Panel::setContent(const QJsonObject& content)
{
    m_rawContent = content;
    parseSceneFields(content);
    parseCharacters(content);
    parseDialogue(content);
}

void Panel::mergeContent(const QJsonObject& extraContent)
{
    for (auto it = extraContent.begin(); it != extraContent.end(); ++it) {
        if (!m_rawContent.contains(it.key())) {
            m_rawContent[it.key()] = it.value();
        }
    }
}

QString Panel::positionLabel() const
{
    return QString("P%1-%2").arg(m_page).arg(m_index);
}

QString Panel::charactersText() const
{
    if (m_characters.isEmpty()) {
        return QString::fromUtf8(u8"\u65e0\u89d2\u8272");
    }
    return collectStrings(m_characters, [](const CharacterPose& pose) { return pose.name; })
        .join(QString::fromUtf8(u8"\u3001"));
}

QString Panel::dialogueText() const
{
    if (m_dialogue.isEmpty()) {
        return QString::fromUtf8(u8"\u65e0\u5bf9\u767d");
    }
    return collectStrings(m_dialogue, formatDialogueLineText).join(QString::fromUtf8(u8"\uff1b"));
}

QJsonArray Panel::charactersToJsonArray() const
{
    QJsonArray arr;
    for (const auto &c : m_characters) {
        arr.append(QJsonObject{
            {"charId", c.charId},
            {"name", c.name},
            {"pose", c.pose},
            {"expression", c.expression}
        });
    }
    return arr;
}

QJsonArray Panel::dialogueToJsonArray() const
{
    QJsonArray arr;
    for (const auto &d : m_dialogue) {
        arr.append(QJsonObject{
            {"speaker", d.speaker},
            {"text", d.text},
            {"bubbleType", d.bubbleType},
            {"speakerSide", d.speakerSide}
        });
    }
    return arr;
}

void Panel::parseSceneFields(const QJsonObject& content)
{
    if (content.contains("scene")) {
        m_scene = content["scene"].toString();
    }
    if (content.contains("shotType")) {
        m_shotType = content["shotType"].toString();
    }
    if (content.contains("cameraAngle")) {
        m_cameraAngle = content["cameraAngle"].toString();
    }
    if (content.contains("visualPrompt")) {
        m_visualPrompt = content["visualPrompt"].toString();
    } else if (content.contains("visual_prompt")) {
        m_visualPrompt = content["visual_prompt"].toString();
    }
    if (content.contains("visualPromptEn")) {
        m_visualPromptEn = content["visualPromptEn"].toString();
    } else if (content.contains("visual_prompt_en")) {
        m_visualPromptEn = content["visual_prompt_en"].toString();
    }
}

void Panel::parseCharacters(const QJsonObject& content)
{
    auto parser = [](const QJsonObject& obj) -> CharacterPose {
        return {
            obj["charId"].toString(),
            obj["name"].toString(),
            obj["pose"].toString(),
            obj["expression"].toString()
        };
    };
    m_characters = parseJsonArray<CharacterPose>(content["characters"], parser, "characters");
}

void Panel::parseDialogue(const QJsonObject& content)
{
    m_dialogue.clear();

    const QJsonArray dialogueArray = DialogueTextUtils::normalizeDialogueArray(content["dialogue"].toArray());
    m_dialogue.reserve(dialogueArray.size());
    for (const QJsonValue& value : dialogueArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject obj = value.toObject();
        m_dialogue.append(DialogueLine(
            obj.value("speaker").toString(),
            obj.value("text").toString(),
            obj.value("bubbleType").toString("speech")));
    }
}

void Panel::clear()
{
    m_id.clear();
    m_storyboardId.clear();
    m_page = 0;
    m_index = 0;
    m_scene.clear();
    m_shotType.clear();
    m_cameraAngle.clear();
    m_visualPrompt.clear();
    m_visualPromptEn.clear();
    m_status = "pending";
    m_previewS3Key.clear();
    m_hdS3Key.clear();
    m_previewUrl.clear();
    m_hdUrl.clear();
    m_rawContent = QJsonObject();
    m_characters.clear();
    m_dialogue.clear();
}

QStringList Panel::characterNames() const
{
    return collectStrings(m_characters, [](const CharacterPose& pose) { return pose.name; });
}

QStringList Panel::dialogueTexts() const
{
    return collectStrings(m_dialogue, [](const DialogueLine& line) { return line.text; });
}
