#include "api/VolcEngineResponse.h"
#include <QJsonDocument>

VolcEngineResponse::VolcEngineResponse()
    : m_timestamp(QDateTime::currentMSecsSinceEpoch())
{
}

VolcEngineResponse VolcEngineResponse::fromJson(const QByteArray& jsonData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        return createError(Status::ParseError, 
            QString("JSON parse error: %1").arg(parseError.errorString()));
    }
    
    return fromJson(doc.object());
}

VolcEngineResponse VolcEngineResponse::fromJson(const QJsonObject& json)
{
    VolcEngineResponse response;
    
    // 检查 API 错误
    if (json.contains("code")) {
        int code = json["code"].toInt();
        if (code != 10000) {
            response.m_status = Status::ApiError;
            response.m_errorCode = code;
            response.m_errorMessage = json["message"].toString();
            response.m_requestId = json["request_id"].toString();
            return response;
        }
    }
    
    // 解析响应数据
    QJsonObject data = json["data"].toObject();
    
    // 优先从 image_urls 获取
    QJsonArray imageUrls = data["image_urls"].toArray();
    if (!imageUrls.isEmpty()) {
        response.m_imageUrl = imageUrls.first().toString();
        response.m_status = response.m_imageUrl.isEmpty() ? Status::ParseError : Status::Success;
    }
    
    // 如果没有 URL，尝试从 binary_data_base64 获取
    if (!response.isSuccess() || response.m_imageUrl.isEmpty()) {
        QJsonArray binaryData = data["binary_data_base64"].toArray();
        if (!binaryData.isEmpty()) {
            QString base64Data = binaryData.first().toString();
            if (!base64Data.isEmpty()) {
                response.m_imageData = QByteArray::fromBase64(base64Data.toLatin1());
                response.m_mimeType = "image/png";
                response.m_status = response.m_imageData.isEmpty() ? Status::ParseError : Status::Success;
            }
        }
    }
    
    // 提取请求 ID
    response.m_requestId = json["request_id"].toString();
    if (response.m_requestId.isEmpty()) {
        response.m_requestId = data["request_id"].toString();
    }
    
    return response;
}

VolcEngineResponse VolcEngineResponse::createError(Status status, 
                                                    const QString& message,
                                                    const QString& requestId)
{
    VolcEngineResponse response;
    response.m_status = status;
    response.m_errorMessage = message;
    response.m_requestId = requestId;
    return response;
}
