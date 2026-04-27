#ifndef QWEN_JSON_REPAIR_H
#define QWEN_JSON_REPAIR_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

class QwenJsonRepair
{
public:
    static QJsonObject parseWithRepair(const QString& rawContent);
    static QString stripMarkdownCodeBlock(const QString& content);
    static QString tryRepairTruncatedJson(const QString& content);
    static QJsonArray extractCompleteArray(const QString& json, const QString& arrayKey, int errorOffset);
    
private:
    static QString tryFixMissingColon(const QString& content, int errorOffset);
    static int findLastCompleteBrace(const QString& json);
    
    QwenJsonRepair() = delete;
};

#endif // QWEN_JSON_REPAIR_H
