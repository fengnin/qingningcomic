#include "api/QwenJsonRepair.h"
#include "utils/JsonRepairAdapter.h"
#include "utils/Logger.h"
#include <QRegularExpression>

namespace {
QString normalizeSmartQuotes(QString content)
{
    content.replace("\u201c", "\"").replace("\u201d", "\"");
    content.replace("\u2018", "'").replace("\u2019", "'");
    return content;
}

bool tryParseObjectCandidate(const QString& content, QJsonObject* object, QJsonParseError* parseError = nullptr)
{
    QJsonParseError localParseError;
    QJsonParseError* errorPtr = parseError ? parseError : &localParseError;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), errorPtr);
    if (errorPtr->error == QJsonParseError::NoError && doc.isObject()) {
        if (object) {
            *object = doc.object();
        }
        return true;
    }
    if (object) {
        *object = QJsonObject();
    }
    return false;
}

bool isTruncationError(const QString& errorMessage)
{
    return errorMessage.contains("unterminated") ||
           errorMessage.contains("unexpected end") ||
           errorMessage.contains("missing value separator") ||
           errorMessage.contains("missing colon") ||
           errorMessage.contains("illegal value") ||
           errorMessage.contains("unexpected token");
}

QJsonObject buildPanelsFallback(const QJsonArray& panels)
{
    QJsonObject result;
    result["panels"] = panels;
    result["characters"] = QJsonArray();
    result["scenes"] = QJsonArray();
    return result;
}
}

QJsonObject QwenJsonRepair::parseWithRepair(const QString& rawContent)
{
    QString content = normalizeSmartQuotes(rawContent);
    if (content.isEmpty()) {
        return QJsonObject();
    }

    content = stripMarkdownCodeBlock(content);

    QJsonParseError parseError;
    QJsonObject parsed;
    if (tryParseObjectCandidate(content, &parsed, &parseError)) {
        return parsed;
    }

    JsonRepair::RepairResult result = JsonRepair::repair(content);
    if (result.success && result.value.isObject()) {
        return result.value.toObject();
    }

    const QString fixedContent = tryFixMissingColon(content, parseError.offset);
    if (!fixedContent.isEmpty()) {
        if (tryParseObjectCandidate(fixedContent, &parsed, &parseError)) {
            return parsed;
        }
    }

    const QString repairedJson = tryRepairTruncatedJson(content);
    if (!repairedJson.isEmpty()) {
        if (tryParseObjectCandidate(repairedJson, &parsed, &parseError)) {
            return parsed;
        }
    }

    const int lastCompleteBrace = findLastCompleteBrace(content);
    if (lastCompleteBrace > 0) {
        const QString partialJson = content.left(lastCompleteBrace + 1);
        if (tryParseObjectCandidate(partialJson, &parsed, &parseError)) {
            return parsed;
        }
    }

    const QJsonArray panelsArray = extractCompleteArray(content, "panels", parseError.offset);
    if (!panelsArray.isEmpty()) {
        return buildPanelsFallback(panelsArray);
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
    QJsonObject parsed;
    if (tryParseObjectCandidate(trimmed, &parsed, &parseError) && parseError.error == QJsonParseError::NoError) {
        return trimmed;
    }

    if (!isTruncationError(parseError.errorString())) {
        return QString();
    }

    const QJsonArray panels = extractCompleteArray(trimmed, "panels", parseError.offset);
    const QJsonArray characters = extractCompleteArray(trimmed, "characters", parseError.offset);
    const QJsonArray scenes = extractCompleteArray(trimmed, "scenes", parseError.offset);

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
                    const int commaPos = fixed.indexOf(',', quoteStart);
                    const QString value = commaPos >= 0
                        ? fixed.mid(quoteStart, commaPos - quoteStart)
                        : fixed.mid(quoteStart);
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

QJsonArray QwenJsonRepair::extractCompleteArray(const QString& json, const QString& arrayKey, int errorOffset)
{
    int arrayStartPos = json.indexOf(QString("\"%1\"").arg(arrayKey));
    if (arrayStartPos < 0) {
        return QJsonArray();
    }
    
    const int maxPos = qMin(errorOffset, json.length());

    const int arrayStart = json.indexOf("[", arrayStartPos);
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

    const QString arrayStr = json.mid(arrayStart, lastCompleteEnd - arrayStart + 1) + "]";
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(arrayStr.toUtf8(), &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        return doc.array();
    }
    
    return QJsonArray();
}
