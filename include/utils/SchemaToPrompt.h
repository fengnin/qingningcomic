#ifndef SCHEMATOPROMPT_H
#define SCHEMATOPROMPT_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QStringList>

/**
 * @brief Schema转Prompt工具类
 * 
 * 将JSON Schema转换为人类可读的System Prompt文档。
 * 确保Schema描述是唯一的事实来源。
 */
class SchemaToPrompt
{
public:
    struct Options {
        QJsonObject customExample;
        QString additionalInstructions;
    };
    
    static QString buildSystemPrompt(const QJsonObject &schema, const Options &options = Options());
    static QString generateFieldDocs(const QJsonObject &schema, int indentLevel = 0);
    static QJsonObject generateExample(const QJsonObject &schema);
    
private:
    static QString getFieldType(const QJsonObject &fieldSchema);
    static QStringList getFieldConstraints(const QJsonObject &fieldSchema);
    static QJsonValue generateFieldExample(const QJsonObject &fieldSchema);
    static QString sanitizeDescription(const QString &description);
};

#endif
