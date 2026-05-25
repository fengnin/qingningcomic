#include "utils/AppConfig.h"
#include "utils/EncodingUtils.h"
#include <QCoreApplication>
#include <QFile>

AppConfig* AppConfig::m_instance = nullptr;
std::once_flag AppConfig::m_instanceOnceFlag;
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
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new AppConfig();
    });
    return m_instance;
}

bool AppConfig::load(const QString& configPath)
{
    m_configPath = configPath.isEmpty()
        ? QCoreApplication::applicationDirPath() + "/config.ini"
        : configPath;

    if (!QFile::exists(m_configPath)) {
        m_lastError = QStringLiteral("Config file not found: %1").arg(m_configPath);
        return false;
    }

    QSettings settings(m_configPath, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");

    readDatabaseConfig(settings);
    readQwenConfig(settings);
    readStorageConfig(settings);
    readQwenImageConfig(settings);
    readVolcEngineConfig(settings);
    readImageServiceConfig(settings);
    readDemoConfig(settings);

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
    m_storage.dataDir = settings.value("dataDir", "/data/comic").toString();

    m_storage.endpoint = settings.value("endpoint").toString();
    m_storage.bucket = settings.value("bucket").toString();
    m_storage.accessKey = settings.value("accessKey").toString();
    m_storage.secretKey = settings.value("secretKey").toString();
    m_storage.region = settings.value("region", "us-east-1").toString();
    m_storage.presignExpiresIn = settings.value("presignExpiresIn", 900).toInt();

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

    if (m_qwenImage.apiKey.isEmpty() || m_qwenImage.apiKey == "sk-your-api-key-here") {
        m_qwenImage.apiKey = m_qwen.apiKey;
    }
}

void AppConfig::readVolcEngineConfig(QSettings& settings)
{
    settings.beginGroup("volcEngine");
    m_volcEngine.accessKey = settings.value("accessKey").toString();
    m_volcEngine.secretKey = settings.value("secretKey").toString();
    m_volcEngine.baseUrl = settings.value("baseUrl", "https://visual.volcengineapi.com").toString();
    m_volcEngine.region = settings.value("region", "cn-north-1").toString();
    m_volcEngine.service = settings.value("service", "cv").toString();
    m_volcEngine.reqKey = settings.value("reqKey", "high_aes_general_v30l_zt2i").toString();
    m_volcEngine.img2imgReqKey = settings.value("img2imgReqKey", "seed3l_multi_ip").toString();
    m_volcEngine.seedEditReqKey = settings.value("seedEditReqKey", "seededit_v3.0").toString();
    m_volcEngine.requestTimeout = settings.value("requestTimeout", 120000).toInt();
    m_volcEngine.forceMock = settings.value("forceMock", false).toBool();
    settings.endGroup();
}

void AppConfig::readImageServiceConfig(QSettings& settings)
{
    settings.beginGroup("imageService");
    m_imageService.provider = settings.value("provider", "qwen").toString();
    settings.endGroup();
}

void AppConfig::readDemoConfig(QSettings& settings)
{
    settings.beginGroup("demo");
    m_demo.enabled = settings.value("enabled", false).toBool();
    settings.endGroup();

    if (!m_demo.enabled) return;

    settings.beginGroup("demo.users");
    const QStringList keys = settings.childKeys();
    QMap<int, QPair<QString,QString>> indexed;
    for (const QString& key : keys) {
        // key 格式: "1_username", "1_password", ...
        const int sep = key.indexOf('_');
        if (sep < 1) continue;
        bool ok = false;
        const int idx = key.left(sep).toInt(&ok);
        if (!ok) continue;
        const QString field = key.mid(sep + 1);
        if (field == "username")
            indexed[idx].first  = settings.value(key).toString();
        else if (field == "password")
            indexed[idx].second = settings.value(key).toString();
    }
    for (auto it = indexed.cbegin(); it != indexed.cend(); ++it) {
        if (!it->first.isEmpty() && !it->second.isEmpty())
            m_demo.users.append(*it);
    }
    settings.endGroup();
}

bool AppConfig::setValidationError(const QString& message)
{
    m_lastError = message;
    return false;
}

bool AppConfig::validateQwenConfig()
{
    if (m_qwen.apiKey.isEmpty() || m_qwen.apiKey == "sk-your-api-key-here") {
        return setValidationError(QStringLiteral("Qwen API key is missing"));
    }
    return true;
}

bool AppConfig::validateS3Config()
{
    if (m_storage.type != "s3") {
        return true;
    }

    if (m_storage.endpoint.isEmpty()) {
        return setValidationError(QStringLiteral("Storage endpoint is missing"));
    }
    if (m_storage.bucket.isEmpty()) {
        return setValidationError(QStringLiteral("Storage bucket is missing"));
    }
    if (m_storage.accessKey.isEmpty() || m_storage.accessKey == "your-access-key-here") {
        return setValidationError(QStringLiteral("Storage access key is missing"));
    }
    if (m_storage.secretKey.isEmpty() || m_storage.secretKey == "your-secret-key-here") {
        return setValidationError(QStringLiteral("Storage secret key is missing"));
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
