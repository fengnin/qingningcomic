
#include "Panel.h"
#include "Logger.h"

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
}

Panel::Panel()
    : m_page(0)
    , m_index(0)
{
}

QJsonObject Panel::content() const
{
    QJsonObject obj;
    obj["scene"] = m_scene;
    obj["shotType"] = m_shotType;
    obj["cameraAngle"] = m_cameraAngle;
    obj["characters"] = charactersToJsonArray();
    obj["dialogue"] = dialogueToJsonArray();
    return obj;
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
    QStringList names;
    names.reserve(m_characters.size());
    for (const auto &c : m_characters) {
        names << c.name;
    }
    return names.join(QString::fromUtf8(u8"\u3001"));
}

QString Panel::dialogueText() const
{
    if (m_dialogue.isEmpty()) {
        return QString::fromUtf8(u8"\u65e0\u5bf9\u767d");
    }
    QStringList lines;
    lines.reserve(m_dialogue.size());
    for (const auto &d : m_dialogue) {
        lines << (d.text.isEmpty() ? d.speaker : QString::fromUtf8(u8"%1\uff1a%2").arg(d.speaker, d.text));
    }
    return lines.join(QString::fromUtf8(u8"\uff1b"));
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
            {"bubbleType", d.bubbleType}
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
    auto parser = [](const QJsonObject& obj) -> DialogueLine {
        return {
            obj["speaker"].toString(),
            obj["text"].toString(),
            obj["bubbleType"].toString("speech")
        };
    };
    m_dialogue = parseJsonArray<DialogueLine>(content["dialogue"], parser, "dialogue");
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
    QStringList names;
    names.reserve(m_characters.size());
    for (const auto& c : m_characters) {
        names << c.name;
    }
    return names;
}

QStringList Panel::dialogueTexts() const
{
    QStringList texts;
    texts.reserve(m_dialogue.size());
    for (const auto& d : m_dialogue) {
        texts << d.text;
    }
    return texts;
}
