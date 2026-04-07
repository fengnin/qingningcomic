#include "api/QwenJsonRepair.h"
#include "JsonRepairAdapter.h"
#include "Logger.h"
#include <QRegularExpression>

QJsonObject QwenJsonRepair::parseWithRepair(const QString& rawContent)
{
    QString content = rawContent;
    
    if (content.isEmpty()) {
        return QJsonObject();
    }
    
    content.replace("\u201c", "\"").replace("\u201d", "\"");
    content.replace("\u2018", "'").replace("\u2019", "'");
    
    content = stripMarkdownCodeBlock(content);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError) {
        return doc.object();
    }
    
    JsonRepair::RepairResult result = JsonRepair::repair(content);
    if (result.success && result.value.isObject()) {
        return result.value.toObject();
    }
    
    QString fixedContent = tryFixMissingColon(content, parseError.offset);
    if (!fixedContent.isEmpty()) {
        doc = QJsonDocument::fromJson(fixedContent.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            return doc.object();
        }
    }
    
    QString repairedJson = tryRepairTruncatedJson(content);
    if (!repairedJson.isEmpty()) {
        doc = QJsonDocument::fromJson(repairedJson.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            return doc.object();
        }
    }
    
    int lastCompleteBrace = findLastCompleteBrace(content);
    if (lastCompleteBrace > 0) {
        QString partialJson = content.left(lastCompleteBrace + 1);
        doc = QJsonDocument::fromJson(partialJson.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            return doc.object();
        }
    }
    
    QJsonArray panelsArray = extractCompleteArray(content, "panels", parseError.offset);
    if (!panelsArray.isEmpty()) {
        QJsonObject result;
        result["panels"] = panelsArray;
        result["characters"] = QJsonArray();
        result["scenes"] = QJsonArray();
        return result;
    }
    
    return QJsonObject();
}

QString QwenJsonRepair::stripMarkdownCodeBlock(const QString& content)
{
    QRegularExpression codeBlockRegex("```json\\s*([\\s\\S]*?)\\s*```");
    QRegularExpressionMatch match = codeBlockRegex.match(content);
    return match.hasMatch() ? match.captured(1) : content;
}

QString QwenJsonRepair::tryRepairTruncatedJson(const QString& content)
{
    QString trimmed = content.trimmed();
    
    if (trimmed.isEmpty() || !trimmed.startsWith("{")) {
        return QString();
    }
    
    QJsonParseError parseError;
    QJsonDocument::fromJson(trimmed.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError) {
        return trimmed;
    }
    
    QString errorMsg = parseError.errorString();
    bool isTruncationError = errorMsg.contains("unterminated") || 
                              errorMsg.contains("unexpected end") ||
                              errorMsg.contains("missing value separator") ||
                              errorMsg.contains("missing colon") ||
                              errorMsg.contains("illegal value") ||
                              errorMsg.contains("unexpected token");
    
    if (!isTruncationError) {
        return QString();
    }
    
    QJsonArray panels = extractCompleteArray(trimmed, "panels", parseError.offset);
    QJsonArray characters = extractCompleteArray(trimmed, "characters", parseError.offset);
    QJsonArray scenes = extractCompleteArray(trimmed, "scenes", parseError.offset);
    
    if (panels.isEmpty() && characters.isEmpty() && scenes.isEmpty()) {
        return QString();
    }
    
    QJsonObject result;
    result["panels"] = panels;
    result["characters"] = characters;
    result["scenes"] = scenes;
    
    return QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Indented));
}

QString QwenJsonRepair::tryFixMissingColon(const QString& content, int errorOffset)
{
    QString fixed = content;
    
    int pos = errorOffset;
    while (pos > 0 && pos < fixed.length()) {
        QChar c = fixed[pos];
        
        if (c == '"' || c == '\'') {
            int quoteStart = pos;
            pos--;
            while (pos > 0 && fixed[pos] != '"' && fixed[pos] != '\'') {
                pos--;
            }
            if (pos > 0) {
                QString between = fixed.mid(pos + 1, quoteStart - pos - 1).trimmed();
                if (!between.contains(':') && !between.isEmpty()) {
                    QString key = fixed.mid(pos, quoteStart - pos + 1);
                    QString value = fixed.mid(quoteStart, fixed.indexOf(',', quoteStart) - quoteStart);
                    if (!key.isEmpty() && !value.isEmpty()) {
                        QString before = fixed.left(pos);
                        QString after = fixed.mid(quoteStart);
                        fixed = before + key + ":" + after;
                        return fixed;
                    }
                }
            }
            break;
        }
        pos--;
    }
    
    return QString();
}

int QwenJsonRepair::findLastCompleteBrace(const QString& json)
{
    int braceCount = 0;
    bool inString = false;
    bool escape = false;
    int lastCompleteBrace = -1;
    
    for (int i = 0; i < json.length(); ++i) {
        QChar c = json[i];
        
        if (escape) { escape = false; continue; }
        if (c == '\\' && inString) { escape = true; continue; }
        if (c == '"') { inString = !inString; continue; }
        if (inString) continue;
        
        if (c == '{') braceCount++;
        else if (c == '}') {
            braceCount--;
            if (braceCount == 0) {
                lastCompleteBrace = i;
            }
        }
    }
    
    return lastCompleteBrace;
}

QString QwenJsonRepair::closeOpenBrackets(const QString& json)
{
    QString result = json;
    int openBraces = result.count('{') - result.count('}');
    int openBrackets = result.count('[') - result.count(']');
    
    for (int i = 0; i < openBrackets; ++i) {
        result += ']';
    }
    for (int i = 0; i < openBraces; ++i) {
        result += '}';
    }
    
    return result;
}

QJsonArray QwenJsonRepair::extractCompleteArray(const QString& json, const QString& arrayKey, int errorOffset)
{
    int arrayStartPos = json.indexOf(QString("\"%1\"").arg(arrayKey));
    if (arrayStartPos < 0) {
        return QJsonArray();
    }
    
    int maxPos = qMin(errorOffset, json.length());
    
    int arrayStart = json.indexOf("[", arrayStartPos);
    if (arrayStart < 0 || arrayStart >= maxPos) {
        return QJsonArray();
    }
    
    int bracketCount = 0;
    int lastCompleteEnd = -1;
    bool inString = false;
    bool escape = false;
    
    for (int i = arrayStart + 1; i < json.length(); ++i) {
        QChar c = json[i];
        
        if (escape) { escape = false; continue; }
        if (c == '\\' && inString) { escape = true; continue; }
        if (c == '"') { inString = !inString; continue; }
        if (inString) continue;
        
        if (c == '{') bracketCount++;
        else if (c == '}') {
            bracketCount--;
            if (bracketCount == 0) {
                lastCompleteEnd = i;
            }
        }
        else if (c == ']' && bracketCount == 0) {
            break;
        }
    }
    
    if (lastCompleteEnd < 0) {
        return QJsonArray();
    }
    
    QString arrayStr = json.mid(arrayStart, lastCompleteEnd - arrayStart + 1) + "]";
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(arrayStr.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError) {
        return doc.array();
    }
    
    return QJsonArray();
}
