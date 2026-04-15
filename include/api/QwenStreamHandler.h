#ifndef QWEN_STREAM_HANDLER_H
#define QWEN_STREAM_HANDLER_H

#include <QByteArray>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class QwenStreamHandler
{
public:
    static QJsonObject handleStreamResponse(const QByteArray& data);
    
private:
    static QJsonObject extractStreamDelta(const QString& data);
    
    QwenStreamHandler() = delete;
};

#endif // QWEN_STREAM_HANDLER_H
