#ifndef VOLCENGINEIMAGECLIENT_H
#define VOLCENGINEIMAGECLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QMutex>
#include <QMessageAuthenticationCode>
#include <functional>
#include "api/VolcEngineRequest.h"
#include "api/VolcEngineResponse.h"
#include "api/VolcEngineSignature.h"

/**
 * @brief 火山引擎图像生成客户端（Seedream 3.0）
 * 
 * 通过火山引擎智能视觉API实现图像生成功能。
 * 支持文生图和图生图两种模式。
 * 
 * 文生图 API：CVProcess（同步接口）
 * 图生图 API：CVSync2AsyncSubmitTask + CVSync2AsyncGetResult（异步接口）
 * 
 * API文档：
 * - 文生图：https://www.volcengine.com/docs/85128/1526761
 * - 图生图：https://www.volcengine.com/docs/86081/1804562
 * 
 * 特点：
 * - 文生图：同步接口，直接返回图片URL或Base64
 * - 图生图：异步接口，支持角色特征保持
 * - 支持最高2K分辨率
 * - 中文prompt友好
 * - 500次免费额度
 * 
 * 并发限制：
 * - 免费版：并发限制为1
 * - 付费版：并发限制为2
 */
class VolcEngineImageClient : public QObject
{
    Q_OBJECT

public:
    struct Config {
        QString accessKey;      // Access Key ID
        QString secretKey;      // Secret Access Key
        QString baseUrl = "https://visual.volcengineapi.com";
        QString region = "cn-north-1";
        QString service = "cv";
        QString reqKey = "high_aes_general_v30l_zt2i";  // Seedream 3.0 文生图
        QString img2imgReqKey = "seed3l_multi_ip";      // Seedream 3.0 图生图多图版（最多5张参考图）
        QString seedEditReqKey = "seededit_v3.0";       // SeedEdit 3.0 指令编辑
        int requestTimeout = 120000;
        bool forceMock = false;
    };

    struct GenerateOptions {
        QString prompt;
        QString negativePrompt;
        int width = 1328;       // 默认1.3K分辨率
        int height = 1328;
        float scale = 2.5f;     // 文本影响程度
        int seed = -1;          // -1为随机
        bool returnUrl = true;  // 返回URL而非Base64
        QString requestId;
        bool usePreLlm = false; // 开启文本扩写
        QList<QByteArray> referenceImages; // 参考图片数据列表（用于图生图，最多5张）
        QStringList refTypeList;            // 参考图类型列表（IP/ID/STYLE/AUTO），长度必须等于referenceImages
    };

    struct GenerateResult {
        bool success = false;
        QByteArray imageData;
        QString imageUrl;
        QString mimeType;
        QString requestId;
        QString errorMessage;
        int width = 0;
        int height = 0;
        qint64 timestamp = 0;
    };

    static VolcEngineImageClient* instance();
    static void cleanup();
    
    VolcEngineImageClient();
    ~VolcEngineImageClient();

    bool init(const Config& config);
    bool isInitialized() const { return m_initialized; }
    QString lastError() const { return m_lastError; }

    // 异步 API（推荐）
    void generateAsync(const GenerateOptions& options);

    // 同步 API（用于测试或简单场景）
    GenerateResult generate(const GenerateOptions& options);

    // 图像编辑 API（SeedEdit 3.0）
    GenerateResult edit(const GenerateOptions& options);

    // 下载图片（public 供 ImageService 调用）
    QByteArray downloadImage(const QString& url);

    bool shouldMock() const;
    GenerateResult generatePlaceholder(const QString& prompt = QString());

signals:
    void generateCompleted(const VolcEngineImageClient::GenerateResult& result);
    void progressChanged(const QString& stage, int progress);
    void errorOccurred(const QString& operation, const QString& message);

private slots:
    void onReplyFinished();
    void onImageDownloadFinished();
    void onReplyError(QNetworkReply::NetworkError error);

private:
    VolcEngineImageClient(const VolcEngineImageClient&) = delete;
    VolcEngineImageClient& operator=(const VolcEngineImageClient&) = delete;

    // 请求构建
    QJsonObject buildGenerateRequestBody(const GenerateOptions& options);
    QJsonObject buildReferenceSubmitPayload(const GenerateOptions& options);
    QJsonObject buildReferenceQueryPayload(const QString& taskId) const;
    
    // 图生图 API
    GenerateResult generateWithReference(const GenerateOptions& options);
    GenerateResult pollTaskResult(const QString& taskId, const GenerateOptions& options);
    GenerateResult handleTaskDone(const QJsonObject& data, const GenerateOptions& options);
    GenerateResult buildImageResult(const QString& requestId,
                                    const QByteArray& imageData,
                                    const QString& imageUrl = QString()) const;
    QString extractResponseImageUrl(const QJsonObject& data) const;
    QString extractResponseErrorMessage(const QJsonObject& response) const;
    
    // 网络请求
    QNetworkRequest createNetworkRequest(const QString& url, const QString& body);
    QJsonObject sendSyncRequest(const QString& url, const QJsonObject& payload);

    // 响应解析
    GenerateResult parseResponse(const QJsonObject& response);

    // 辅助方法
    QString buildApiUrl() const;
    void registerRequest(const QString& requestId, QNetworkReply* reply);
    void cleanupRequest(QNetworkReply* reply, const QString& key);
    void cancelAllRequests();
    QJsonObject parseJsonResponse(const QByteArray& data);
    GenerateResult createErrorResult(const QString& requestId, const QString& message) const;
    void setError(const QString& message);
    void applyRequestThrottle();
    void noteRequestStarted();
    void noteRateLimitHit();
    void handleRateLimitResponse(QNetworkReply* reply);
    
    template<typename T>
    T executeSyncRequest(std::function<T(QNetworkAccessManager*)> operation, int timeoutMs);

    static VolcEngineImageClient* m_instance;
    static QMutex m_instanceMutex;

    Config m_config;
    QNetworkAccessManager* m_networkManager;
    QString m_lastError;
    int m_lastErrorCode = 0;
    bool m_initialized;
    QMutex m_requestThrottleMutex;
    qint64 m_nextAllowedRequestAtMs = 0;
    int m_minRequestIntervalMs = 1500;
    int m_rateLimitBackoffMs = 5000;

    QMap<QString, QNetworkReply*> m_activeRequests;
    QMap<QString, GenerateOptions> m_pendingOptions;
};

Q_DECLARE_METATYPE(VolcEngineImageClient::GenerateResult)

#endif // VOLCENGINEIMAGECLIENT_H
