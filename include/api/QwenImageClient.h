#ifndef QWENIMAGECLIENT_H
#define QWENIMAGECLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QVariant>
#include <QMutex>

/**
 * @brief Qwen 图像生成客户端（异步版本）
 * 
 * 通过阿里云百炼 DashScope API 实现图像生成和编辑功能。
 * 所有网络操作都是异步的，不会阻塞调用线程。
 */
class QwenImageClient : public QObject
{
    Q_OBJECT

public:
    struct Config {
        QString apiKey;
        QString baseUrl = "https://dashscope.aliyuncs.com";
        QString generateModel = "wanx2.1-t2i-plus";
        QString editModel = "qwen-image-edit";
        int requestTimeout = 120000;
        int maxRetries = 3;
        bool forceMock = false;
    };

    enum class ImageSize {
        Square,
        Portrait,
        Landscape,
        Custom
    };

    struct GenerateOptions {
        QString prompt;
        QString negativePrompt;
        ImageSize size = ImageSize::Square;
        int width = 0;
        int height = 0;
        QString style;
        int seed = -1;
        bool textRendering = true;
        QString requestId;
        QStringList referenceImages;  // 参考图片路径列表（用于角色一致性）
    };

    struct EditOptions {
        QString prompt;
        QByteArray sourceImage;
        QByteArray maskImage;
        QString negativePrompt;
        ImageSize size = ImageSize::Square;
        int width = 0;
        int height = 0;
        int seed = -1;
        QString requestId;
        
        static EditOptions fromGenerateOptions(const GenerateOptions& opts) {
            EditOptions e;
            e.prompt = opts.prompt;
            e.negativePrompt = opts.negativePrompt;
            e.size = opts.size;
            e.width = opts.width;
            e.height = opts.height;
            e.seed = opts.seed;
            e.requestId = opts.requestId;
            return e;
        }
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

    static QwenImageClient* instance();
    static void cleanup();
    
    QwenImageClient();
    ~QwenImageClient();

    bool init(const Config& config);
    bool isInitialized() const { return m_initialized; }
    QString lastError() const { return m_lastError; }

    // ==================== 异步 API（推荐）====================
    
    void generateAsync(const GenerateOptions& options);
    void editAsync(const EditOptions& options);

    // ==================== 同步 API（仅用于测试）====================
    
    GenerateResult generate(const GenerateOptions& options);
    GenerateResult edit(const EditOptions& options);

    bool shouldMock() const;
    GenerateResult generatePlaceholder(const QString& prompt = QString());
    static QString sizeToString(ImageSize size, int width = 0, int height = 0);

signals:
    void generateCompleted(const QwenImageClient::GenerateResult& result);
    void editCompleted(const QwenImageClient::GenerateResult& result);
    void progressChanged(const QString& stage, int progress);
    void errorOccurred(const QString& operation, const QString& message);

private slots:
    void onReplyFinished();
    void onTaskStatusReplyFinished();
    void onImageDownloadFinished();
    void onReplyError(QNetworkReply::NetworkError error);

private:
    enum class RequestType {
        Generate = 0,
        Edit = 1,
        TaskStatus = 2,
        Download = 3
    };
    
    QwenImageClient(const QwenImageClient&) = delete;
    QwenImageClient& operator=(const QwenImageClient&) = delete;

    // 验证
    bool validateGenerateOptions(const GenerateOptions& options);
    bool validateEditOptions(const EditOptions& options);

    // 请求构建
    QJsonObject buildCommonParameters(ImageSize size, int seed);
    QJsonObject buildGenerateRequestBody(const GenerateOptions& options);
    QJsonObject buildMultimodalRequestBody(const GenerateOptions& options);
    QJsonObject buildEditRequestBody(const EditOptions& options);
    QJsonObject applyCustomSize(QJsonObject parameters, ImageSize size, int width, int height) const;
    QString resolveRequestModel(RequestType type) const;
    QString resolveRequestService(RequestType type) const;
    QString resolveRequestUrl(RequestType type) const;
    QString buildApiUrl(const QString& service) const;
    QString buildApiUrl(const QString& service, const QString& model) const;

    // 异步请求发送
    void sendAsyncRequest(const GenerateOptions& options, RequestType type);
    void sendAsyncRequest(const EditOptions& options, RequestType type);
    void dispatchAsyncRequest(const QString& requestId, const QNetworkRequest& request,
                              const QJsonObject& payload, RequestType type);
    void storePendingRequest(const QString& requestId, const GenerateOptions& options);
    void storePendingRequest(const QString& requestId, const EditOptions& options);
    void pollTaskStatus(const QString& taskId, const QString& requestId);
    void downloadImageFromUrl(const QString& url, const QString& requestId);
    
    // 同步请求
    QJsonObject sendSyncRequest(const QString& url, const QJsonObject& payload);
    QJsonObject pollTaskSync(const QString& taskId);
    QByteArray downloadImage(const QString& url);
    
    // 响应解析
    GenerateResult extractImageFromResponse(const QJsonObject& response);
    GenerateResult parseSyncResponse(const QJsonObject& response);
    GenerateResult parseAsyncResponse(const QJsonObject& response);
    QString extractImageUrl(const QJsonObject& output) const;
    QString extractInlineImageUrl(const QJsonObject& response) const;
    QString extractTaskId(const QJsonObject& response) const;
    QString extractResponseErrorMessage(const QJsonObject& response) const;

    // 辅助方法
    QNetworkRequest createNetworkRequest(const QString& url, bool async = true) const;
    void registerRequest(const QString& requestId, QNetworkReply* reply, RequestType type);
    void cleanupRequest(QNetworkReply* reply, const QString& key);
    void cancelAllRequests();
    QJsonObject parseJsonResponse(const QByteArray& data);
    void emitResult(RequestType type, const QString& requestId, const GenerateResult& result);
    void emitPendingResult(const QString& requestId, const GenerateResult& result);
    void handleTaskSuccess(const QJsonObject& output, const QString& requestId);
    GenerateResult createErrorResult(const QString& requestId, const QString& message) const;
    void setError(const QString& message);

    static QwenImageClient* m_instance;
    static QMutex m_instanceMutex;

    Config m_config;
    QNetworkAccessManager* m_networkManager;
    QString m_lastError;
    bool m_initialized;

    QMap<QString, QNetworkReply*> m_activeRequests;
    QMap<QString, GenerateOptions> m_pendingGenerateOptions;
    QMap<QString, EditOptions> m_pendingEditOptions;
    QMap<QString, int> m_retryCount;

    static const QByteArray PLACEHOLDER_IMAGE;
};

Q_DECLARE_METATYPE(QwenImageClient::GenerateResult)

#endif // QWENIMAGECLIENT_H
