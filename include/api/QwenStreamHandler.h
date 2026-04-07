#ifndef QWEN_STREAM_handler_H
#define QWEN_STREAM_HANDLER_H

#include <QJsonObject>
#include <QJsonArray>
#include <functional>

class QwenStreamHandler
{
public:
    static QJsonObject handleStreamResponse(const QByteArray& data);
    
private:
    QString parseSseContent(const QString& data);
    static QJsonObject extractStreamDelta(const QString& data);
    
    QwenStreamHandler() = delete;
};

#endif // QWEN_STREAM_HANDLER_H
