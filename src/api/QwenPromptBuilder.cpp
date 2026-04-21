#include "api/QwenPromptBuilder.h"
#include "services/BibleContextInjector.h"
#include "utils/SchemaToPrompt.h"
#include <QFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>

QString QwenPromptBuilder::buildSystemPrompt(const QString& schemaPath)
{
    QString absolutePath = schemaPath;
    if (QDir::isRelativePath(schemaPath)) {
        absolutePath = QDir(QCoreApplication::applicationDirPath()).filePath(schemaPath);
    }
    
    QFile schemaFile(absolutePath);
    QJsonObject schema;
    
    if (schemaFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(schemaFile.readAll());
        schema = doc.object();
        schemaFile.close();
    }
    
    if (schema.isEmpty()) {
        return QString::fromUtf8("\u65e0\u6cd5\u52a0\u8f7dSchema\u914d\u7f6e\u6587\u4ef6");
    }
    
    SchemaToPrompt::Options options;
    return SchemaToPrompt::buildSystemPrompt(schema, options);
}

QString QwenPromptBuilder::buildUserMessageWithBible(
    const QString& text,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes,
    int chapterNumber)
{
    return BibleContextInjector::instance()->buildContextPrompt(
        text, existingCharacters, existingScenes, chapterNumber);
}

QString QwenPromptBuilder::buildChangeRequestPrompt()
{
    return QStringLiteral(
        "你是漫画分镜编辑助手。\n\n"
        "请将用户的自然语言修改需求转换为结构化 CR-DSL JSON。\n"
        "只输出 JSON，不要添加额外说明。\n"
    );
}

QString QwenPromptBuilder::buildDialogueRewritePrompt()
{
    return QStringLiteral(
        "你是对白改写助手。\n\n"
        "请根据用户的修改要求，重写原对白。\n"
        "要求：\n"
        "1. 保持原意和人物关系不变。\n"
        "2. 语言自然，符合漫画对白风格。\n"
        "3. 不要输出分析过程。\n"
        "4. 只返回改写后的对白文本。\n"
    );
}
