#include "api/QwenStreamHandler.h"
#include <QJsonDocument>
#include <QRegularExpression>

namespace {

QStringList splitSseEvents(const QString& data)
{
    QStringList events;
    QStringList rawEvents = data.split("\n\n", Qt::SkipEmptyParts);
    
    for (const QString& rawEvent : rawEvents) {
        QString trimmed = rawEvent.trimmed();
        if (!trimmed.isEmpty()) {
            events.append(trimmed);
        }
    }
    
    return events;
}

QString extractDataField(const QString& event)
{
    QStringList lines = event.split('\n');
    QStringList dataLines;
    
    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("data:", Qt::CaseInsensitive)) {
            QString dataContent = trimmedLine.mid(5).trimmed();
            if (!dataContent.isEmpty()) {
                dataLines.append(dataContent);
            }
        }
    }
    
    return dataLines.join('\n');
}

}

QJsonObject QwenStreamHandler::handleStreamResponse(const QByteArray& data)
{
    QString dataStr = QString::fromUtf8(data);
    QStringList events = splitSseEvents(dataStr);
    
    QString accumulated;
    QJsonObject lastResponse;
    
    for (const QString& event : events) {
        QString dataContent = extractDataField(event);
        if (dataContent.isEmpty()) {
            continue;
        }
        
        if (dataContent == "[DONE]") {
            continue;
        }
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(dataContent.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            continue;
        }
        
        QJsonObject response = doc.object();
        lastResponse = response;
        
        QJsonObject delta = extractStreamDelta(dataContent);
        if (!delta.isEmpty()) {
            QString content = delta["content"].toString();
            accumulated += content;
        }
    }
    
    QJsonObject result;
    result["accumulated"] = accumulated;
    result["lastResponse"] = lastResponse;
    return result;
}

QJsonObject QwenStreamHandler::extractStreamDelta(const QString& data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return QJsonObject();
    }
    
    QJsonObject root = doc.object();
    QJsonArray choices = root["choices"].toArray();
    if (choices.isEmpty()) {
        return QJsonObject();
    }
    
    QJsonObject choice = choices[0].toObject();
    return choice["delta"].toObject();
}
