#ifndef QWEN_PROMPT_BUILDER_H
#define QWEN_PROMPT_BUILDER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class QwenPromptBuilder
{
public:
    static QString buildSystemPrompt(const QString& schemaPath = "schemas/storyboard.json");
    static QString buildChangeRequestPrompt();
    static QString buildDialogueRewritePrompt();
    static QString buildUserMessageWithBible(
        const QString& text,
        const QJsonArray& existingCharacters,
        const QJsonArray& existingScenes,
        int chapterNumber);
    
    static QJsonObject buildExamplePanel();
    static QJsonObject buildExampleCharacter();
    static QJsonObject buildExampleScene();
    static QJsonObject buildCustomExample();

private:
    QwenPromptBuilder() = delete;
};

#endif // QWEN_PROMPT_BUILDER_H
