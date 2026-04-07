#ifndef SCHEMATOPROMPT_H
#define SCHEMATOPROMPT_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

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
    static QJsonValue generateFieldExample(const QJsonObject &fieldSchema);
    static QString getFieldType(const QJsonObject &fieldSchema);
    static QStringList getFieldConstraints(const QJsonObject &fieldSchema);
};

#endif
