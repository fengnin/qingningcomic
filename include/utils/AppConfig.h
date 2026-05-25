#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QList>
#include <QPair>
#include <QSettings>
#include <mutex>

/**
 * @brief 应用程序配置管理类
 * 
 * 单例模式，管理所有配置项的读取和存储。
 */
class AppConfig
{
public:
    static const QString DEFAULT_USER_ID;
    
    struct DatabaseConfig {
        QString host;
        int port = 3306;
        QString database;
        QString username;
        QString password;
    };

    struct QwenConfig {
        QString apiKey;
        QString model = "qwen-plus";
        QString endpoint = "https://dashscope.aliyuncs.com/compatible-mode/v1";
        int maxRetries = 3;
        int maxChunkLength = 8000;
        int requestTimeout = 120000;
    };

    struct StorageConfig {
        QString type = "local";         // local 或 s3
        QString dataDir = "/data/comic";
        QString endpoint;               // S3 端点
        QString bucket;                 // S3 存储桶
        QString accessKey;              // S3 Access Key
        QString secretKey;              // S3 Secret Key
        QString region = "us-east-1";   // S3 区域
        int presignExpiresIn = 900;     // 预签名 URL 过期时间（秒）
        
        // 预签名 API 配置（推荐，更安全）
        QString presignApiEndpoint;     // 预签名 API 端点
        QString authToken;              // API 认证 Token
    };

    struct QwenImageConfig {
        QString apiKey;                           // DashScope API Key（与 Qwen 共用）
        QString baseUrl = "https://dashscope.aliyuncs.com";
        QString generateModel = "wanx2.1-t2i-plus"; // 图像生成模型
        QString editModel = "qwen-image-edit";     // 图像编辑模型
        int requestTimeout = 120000;               // 请求超时时间（毫秒）
        int maxRetries = 3;                        // 最大重试次数
        bool forceMock = false;                    // 强制使用占位图
    };

    struct VolcEngineConfig {
        QString accessKey;                         // Access Key ID
        QString secretKey;                         // Secret Access Key
        QString baseUrl = "https://visual.volcengineapi.com";
        QString region = "cn-north-1";
        QString service = "cv";
        QString reqKey = "high_aes_general_v30l_zt2i";   // 文生图模型
        QString img2imgReqKey = "seed3l_multi_ip";       // 图生图多图版（最多5张参考图）
        QString seedEditReqKey = "seededit_v3.0";        // SeedEdit 3.0 指令编辑
        int requestTimeout = 120000;
        bool forceMock = false;
    };

    struct ImageServiceConfig {
        QString provider = "qwen";  // qwen 或 volcengine
    };

    struct DemoConfig {
        bool enabled = false;
        QList<QPair<QString, QString>> users; // username, password
    };

    static AppConfig* instance();

    bool load(const QString& configPath = QString());
    bool isLoaded() const { return m_loaded; }

    DatabaseConfig database() const { return m_database; }
    QwenConfig qwen() const { return m_qwen; }
    StorageConfig storage() const { return m_storage; }
    QwenImageConfig qwenImage() const { return m_qwenImage; }
    VolcEngineConfig volcEngine() const { return m_volcEngine; }
    ImageServiceConfig imageService() const { return m_imageService; }
    DemoConfig demo() const { return m_demo; }
    QString configPath() const { return m_configPath; }
    QString lastError() const { return m_lastError; }

private:
    AppConfig();
    ~AppConfig();
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    void readDatabaseConfig(QSettings& settings);
    void readQwenConfig(QSettings& settings);
    void readStorageConfig(QSettings& settings);
    void readQwenImageConfig(QSettings& settings);
    void readVolcEngineConfig(QSettings& settings);
    void readImageServiceConfig(QSettings& settings);
    void readDemoConfig(QSettings& settings);
    bool validateConfig();
    bool validateQwenConfig();
    bool validateS3Config();
    bool setValidationError(const QString& message);

    static AppConfig* m_instance;
    static std::once_flag m_instanceOnceFlag;
    
    DatabaseConfig m_database;
    QwenConfig m_qwen;
    StorageConfig m_storage;
    QwenImageConfig m_qwenImage;
    VolcEngineConfig m_volcEngine;
    ImageServiceConfig m_imageService;
    DemoConfig m_demo;
    
    QString m_configPath;
    QString m_lastError;
    bool m_loaded;
};

#endif // APPCONFIG_H
