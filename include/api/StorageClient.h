#ifndef STORAGECLIENT_H
#define STORAGECLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QByteArray>
#include <QString>
#include <QMap>
#include <QMutex>

/**
 * @brief S3 兼容存储客户端
 * 
 * 支持 Backblaze B2、AWS S3 等 S3 兼容存储服务。
 * 
 * 两种模式：
 * 1. 直接模式：客户端持有 AccessKey/SecretKey（不推荐用于 WebAssembly）
 * 2. 预签名模式：通过后端 API 获取预签名 URL（推荐，更安全）
 */
class StorageClient : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 存储配置
     */
    struct Config {
        QString endpoint;           // 服务端点，如 https://s3.example.com
        QString bucket;             // 存储桶名称
        QString accessKey;          // Access Key ID（可选，预签名模式不需要）
        QString secretKey;          // Secret Access Key（可选，预签名模式不需要）
        QString region;             // 区域，默认 us-east-1
        int presignExpiresIn = 900; // 预签名 URL 过期时间（秒），默认 15 分钟
        
        // 预签名 API 配置（推荐，更安全）
        QString presignApiEndpoint; // 预签名 API 端点
        QString authToken;          // API 认证 Token
    };

    /**
     * @brief 预签名 URL 响应
     */
    struct PresignedUrlResponse {
        bool success = false;
        QString putUrl;             // 上传 URL
        QString getUrl;             // 下载 URL
        QString key;                // 对象键
        int expiresIn = 0;          // 过期时间（秒）
        QString errorMessage;
    };

    /**
     * @brief 上传结果
     */
    struct UploadResult {
        bool success = false;
        QString uri;                // S3 URI: s3://bucket/key
        QString url;                // HTTP URL
        QString errorMessage;
    };

    /**
     * @brief 下载结果
     */
    struct DownloadResult {
        bool success = false;
        QByteArray data;
        QString contentType;
        QString errorMessage;
    };

    static StorageClient* instance();
    static void cleanup();
    
    StorageClient();
    ~StorageClient();

    /**
     * @brief 初始化客户端
     */
    bool init(const Config& config);
    bool isInitialized() const { return m_initialized; }
    QString lastError() const { return m_lastError; }

    /**
     * @brief 上传文件
     * @param key 对象键（路径）
     * @param data 文件数据
     * @param contentType 内容类型，默认 image/png
     * @param metadata 自定义元数据
     * @return 上传结果
     */
    UploadResult upload(const QString& key, const QByteArray& data,
                        const QString& contentType = "image/png",
                        const QMap<QString, QString>& metadata = QMap<QString, QString>());

    /**
     * @brief 下载文件
     * @param key 对象键
     * @return 下载结果
     */
    DownloadResult download(const QString& key);

    /**
     * @brief 删除文件
     * @param key 对象键
     * @return 成功返回 true
     */
    bool remove(const QString& key);

    /**
     * @brief 获取预签名 URL（临时访问链接）
     * @param key 对象键
     * @param expiresIn 过期时间（秒），默认使用配置值
     * @return 预签名 URL
     */
    QString getPresignedUrl(const QString& key, int expiresIn = 0);

    /**
     * @brief 从后端 API 获取预签名 URL（推荐）
     * @param key 对象键
     * @param options 选项
     * @return 预签名 URL 响应
     */
    PresignedUrlResponse fetchPresignedUrlFromApi(const QString& key, 
                                                   const QString& method = "GET",
                                                   int expiresIn = 3600,
                                                   const QString& contentType = "application/octet-stream",
                                                   qint64 contentLength = 0);

    /**
     * @brief 使用预签名 URL 上传文件（推荐）
     * @param presignedUrl 预签名 URL
     * @param data 文件数据
     * @param contentType 内容类型
     * @return 上传结果
     */
    UploadResult uploadWithPresignedUrl(const QString& presignedUrl, 
                                         const QByteArray& data,
                                         const QString& contentType = "image/png");

    /**
     * @brief 使用预签名 URL 下载文件（推荐）
     * @param presignedUrl 预签名 URL
     * @return 下载结果
     */
    DownloadResult downloadWithPresignedUrl(const QString& presignedUrl);

    /**
     * @brief 检查文件是否存在
     * @param key 对象键
     * @return 存在返回 true
     */
    bool exists(const QString& key);

    /**
     * @brief 复制文件
     * @param sourceKey 源对象键
     * @param destKey 目标对象键
     * @return 成功返回 true
     */
    bool copy(const QString& sourceKey, const QString& destKey);

    /**
     * @brief 获取 S3 URI
     * @param key 对象键
     * @return s3://bucket/key 格式的 URI
     */
    QString getS3Uri(const QString& key) const;

    /**
     * @brief 解析 S3 URI
     * @param uri S3 URI (s3://bucket/key)
     * @param bucket 输出：存储桶名称
     * @param key 输出：对象键
     * @return 解析成功返回 true
     */
    static bool parseS3Uri(const QString& uri, QString& bucket, QString& key);

signals:
    void uploadProgress(const QString& key, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(const QString& key, qint64 bytesReceived, qint64 bytesTotal);
    void errorOccurred(const QString& operation, const QString& message);

private:
    StorageClient(const StorageClient&) = delete;
    StorageClient& operator=(const StorageClient&) = delete;

    // HTTP 请求辅助方法
    QByteArray buildAuthorizationHeader(const QString& method, const QString& key,
                                         const QString& contentType,
                                         const QMap<QString, QString>& headers,
                                         const QString& date);
    QString formatDateHeader();
    QString generatePresignedUrl(const QString& key, int expiresIn, const QString& method = "GET");
    QByteArray hmacSha256(const QByteArray& key, const QByteArray& data);
    QString sha256Hash(const QByteArray& data);
    QString urlEncode(const QString& str, bool encodeSlash = true);

    static StorageClient* m_instance;
    static QMutex m_instanceMutex;
    
    Config m_config;
    QNetworkAccessManager* m_networkManager;
    QString m_lastError;
    bool m_initialized;
};

#endif // STORAGECLIENT_H
