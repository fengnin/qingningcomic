#include "api/QwenStreamHandler.h"
#include <QJsonDocument>
#include <QRegularExpression>

QJsonObject QwenStreamHandler::handleStreamResponse(const QByteArray& data)
{
    QString dataStr = QString::fromUtf8(data);
    QStringList lines = dataStr.split("\n", Qt::SkipEmptyParts);
    
    QString accumulated;
    QJsonObject lastResponse;
    
    for (const QString& line : lines) {
        if (!line.startsWith("data: ")) {
            continue;
        }
        
        QString jsonStr = line.mid(6).trimmed();
        if (jsonStr == "[DONE]") {
            continue;
        }
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            continue;
        }
        
        QJsonObject response = doc.object();
        lastResponse = response;
        
        QJsonObject delta = extractStreamDelta(jsonStr);
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
