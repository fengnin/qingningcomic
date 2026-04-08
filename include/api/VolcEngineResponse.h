#ifndef VOLCENGINERESPONSE_H
#define VOLCENGINERESPONSE_H

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

/**
 * @brief 火山引擎图像生成响应封装类
 * 
 * 解析 API 响应，提取图片 URL 或 Base64 数据。
 */
class VolcEngineResponse
{
public:
    /**
     * @brief 响应状态
     */
    enum class Status {
        Success,        // 成功
        ApiError,       // API 错误
        ParseError,     // 解析错误
        NetworkError,   // 网络错误
        Timeout         // 超时
    };

    VolcEngineResponse();

    // ========== 解析响应 ==========

    /**
     * @brief 从 JSON 数据解析响应
     * @param jsonData JSON 响应数据
     * @return 解析后的响应对象
     */
    static VolcEngineResponse fromJson(const QByteArray& jsonData);

    /**
     * @brief 从 JSON 对象解析响应
     * @param json JSON 对象
     * @return 解析后的响应对象
     */
    static VolcEngineResponse fromJson(const QJsonObject& json);

    // ========== 创建错误响应 ==========

    /**
     * @brief 创建错误响应
     * @param status 错误状态
     * @param message 错误信息
     * @param requestId 请求 ID
     */
    static VolcEngineResponse createError(Status status, 
                                          const QString& message,
                                          const QString& requestId = QString());

    // ========== 获取结果 ==========

    bool isSuccess() const { return m_status == Status::Success; }
    Status status() const { return m_status; }
    QString errorMessage() const { return m_errorMessage; }
    int errorCode() const { return m_errorCode; }

    QString imageUrl() const { return m_imageUrl; }
    QByteArray imageData() const { return m_imageData; }
    QString mimeType() const { return m_mimeType; }

    QString requestId() const { return m_requestId; }
    qint64 timestamp() const { return m_timestamp; }

    // ========== 设置数据 ==========

    void setImageData(const QByteArray& data) { m_imageData = data; }
    void setMimeType(const QString& mime) { m_mimeType = mime; }

private:
    Status m_status = Status::Success;
    QString m_errorMessage;
    int m_errorCode = 0;

    QString m_imageUrl;
    QByteArray m_imageData;
    QString m_mimeType = "image/jpeg";

    QString m_requestId;
    qint64 m_timestamp = 0;
};

#endif // VOLCENGINERESPONSE_H
