#include "AppConfig.h"
#include "EncodingUtils.h"
#include <QCoreApplication>
#include <QFile>

AppConfig* AppConfig::m_instance = nullptr;
const QString AppConfig::DEFAULT_USER_ID = "qingning";

AppConfig::AppConfig()
    : m_loaded(false)
{
}

AppConfig::~AppConfig()
{
}

AppConfig* AppConfig::instance()
{
    if (!m_instance) {
        m_instance = new AppConfig();
    }
    return m_instance;
}

bool AppConfig::load(const QString& configPath)
{
    m_configPath = configPath.isEmpty() 
        ? QCoreApplication::applicationDirPath() + "/config.ini"
        : configPath;

    if (!QFile::exists(m_configPath)) {
        m_lastError = TR("配置文件不存在: %1\n请复制 config.ini.example 为 config.ini 并填入配置").arg(m_configPath);
        return false;
    }

    QSettings settings(m_configPath, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");

    readDatabaseConfig(settings);
    readQwenConfig(settings);
    readStorageConfig(settings);
    readQwenImageConfig(settings);

    return validateConfig();
}

void AppConfig::readDatabaseConfig(QSettings& settings)
{
    settings.beginGroup("database");
    m_database.host = settings.value("host", "localhost").toString();
    m_database.port = settings.value("port", 3306).toInt();
    m_database.database = settings.value("database", "qingning_comic").toString();
    m_database.username = settings.value("username", "root").toString();
    m_database.password = settings.value("password").toString();
    settings.endGroup();
}

void AppConfig::readQwenConfig(QSettings& settings)
{
    settings.beginGroup("qwen");
    m_qwen.apiKey = settings.value("apiKey").toString();
    m_qwen.model = settings.value("model", "qwen-plus").toString();
    m_qwen.endpoint = settings.value("endpoint", "https://dashscope.aliyuncs.com/compatible-mode/v1").toString();
    m_qwen.maxRetries = settings.value("maxRetries", 3).toInt();
    m_qwen.maxChunkLength = settings.value("maxChunkLength", 8000).toInt();
    m_qwen.requestTimeout = settings.value("requestTimeout", 300000).toInt();
    settings.endGroup();
}

void AppConfig::readStorageConfig(QSettings& settings)
{
    settings.beginGroup("storage");
    m_storage.type = settings.value("type", "local").toString();
    m_storage.dataDir = settings.value("dataDir", "data").toString();
    
    m_storage.endpoint = settings.value("endpoint").toString();
    m_storage.bucket = settings.value("bucket").toString();
    m_storage.accessKey = settings.value("accessKey").toString();
    m_storage.secretKey = settings.value("secretKey").toString();
    m_storage.region = settings.value("region", "us-east-1").toString();
    m_storage.presignExpiresIn = settings.value("presignExpiresIn", 900).toInt();
    
    // 预签名 API 配置
    m_storage.presignApiEndpoint = settings.value("presignApiEndpoint").toString();
    m_storage.authToken = settings.value("authToken").toString();
    settings.endGroup();
}

void AppConfig::readQwenImageConfig(QSettings& settings)
{
    settings.beginGroup("qwenImage");
    m_qwenImage.apiKey = settings.value("apiKey").toString();
    m_qwenImage.baseUrl = settings.value("baseUrl", "https://dashscope.aliyuncs.com").toString();
    m_qwenImage.generateModel = settings.value("generateModel", "wanx2.1-t2i-plus").toString();
    m_qwenImage.editModel = settings.value("editModel", "qwen-image-edit").toString();
    m_qwenImage.requestTimeout = settings.value("requestTimeout", 120000).toInt();
    m_qwenImage.maxRetries = settings.value("maxRetries", 3).toInt();
    m_qwenImage.forceMock = settings.value("forceMock", false).toBool();
    settings.endGroup();
    
    // 如果 qwenImage.apiKey 为空，自动使用 qwen.apiKey（两者共用同一个 API Key）
    if (m_qwenImage.apiKey.isEmpty() || m_qwenImage.apiKey == "sk-your-api-key-here") {
        m_qwenImage.apiKey = m_qwen.apiKey;
    }
}

bool AppConfig::setValidationError(const QString& message)
{
    m_lastError = message;
    return false;
}

bool AppConfig::validateQwenConfig()
{
    if (m_qwen.apiKey.isEmpty() || m_qwen.apiKey == "sk-your-api-key-here") {
        return setValidationError(TR("请在 config.ini 中配置有效的 Qwen API Key"));
    }
    return true;
}

bool AppConfig::validateS3Config()
{
    if (m_storage.type != "s3") {
        return true;
    }
    
    if (m_storage.endpoint.isEmpty()) {
        return setValidationError(TR("使用 S3 存储时，endpoint 不能为空"));
    }
    if (m_storage.bucket.isEmpty()) {
        return setValidationError(TR("使用 S3 存储时，bucket 不能为空"));
    }
    if (m_storage.accessKey.isEmpty() || m_storage.accessKey == "your-access-key-here") {
        return setValidationError(TR("使用 S3 存储时，请配置有效的 accessKey"));
    }
    if (m_storage.secretKey.isEmpty() || m_storage.secretKey == "your-secret-key-here") {
        return setValidationError(TR("使用 S3 存储时，请配置有效的 secretKey"));
    }
    
    return true;
}

bool AppConfig::validateConfig()
{
    if (!validateQwenConfig()) {
        return false;
    }
    
    if (!validateS3Config()) {
        return false;
    }
    
    m_loaded = true;
    return true;
}
