#include "AppInitializer.h"
#include "ServiceContainer.h"
#include "DatabaseManager.h"
#include "QwenClient.h"
#include "QwenImageClient.h"
#include "StorageClient.h"
#include "FileStorage.h"
#include "NovelService.h"
#include "StoryboardService.h"
#include "ImageService.h"
#include "BibleImageService.h"
#include "services/ExportService.h"
#include "TaskQueue.h"
#include "TaskRegistry.h"
#include "viewmodels/NovelViewModel.h"
#include "viewmodels/StoryboardViewModel.h"
#include "AppConfig.h"
#include "UserSession.h"
#include "Logger.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>

namespace {
    const QString CONFIG_FILE_NAME = "config.ini";
    const QString ENV_CONFIG_PATH = "QINGNING_CONFIG_PATH";
    const QString PLACEHOLDER_API_KEY = "sk-your-api-key-here";
    
    QStringList getConfigSearchPaths()
    {
        QStringList paths;
        
        QString envPath = qEnvironmentVariable(ENV_CONFIG_PATH.toUtf8().constData());
        if (!envPath.isEmpty()) {
            if (QFileInfo(envPath).isDir()) {
                paths << envPath + "/" + CONFIG_FILE_NAME;
            } else {
                paths << envPath;
            }
        }
        
        QString appDir = QCoreApplication::applicationDirPath();
        paths << appDir + "/" + CONFIG_FILE_NAME;
        paths << appDir + "/../" + CONFIG_FILE_NAME;
        paths << appDir + "/../../" + CONFIG_FILE_NAME;
        paths << appDir + "/../../../" + CONFIG_FILE_NAME;
        paths << appDir + "/../../../../" + CONFIG_FILE_NAME;
        
        QString workDir = QDir::currentPath();
        paths << workDir + "/" + CONFIG_FILE_NAME;
        paths << workDir + "/../" + CONFIG_FILE_NAME;
        paths << workDir + "/../../" + CONFIG_FILE_NAME;
        
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        if (!configDir.isEmpty()) {
            paths << configDir + "/" + CONFIG_FILE_NAME;
        }
        
        return paths;
    }
    
    template<typename T, typename Config>
    bool initService(T*& service, const Config& config, const QString& serviceName)
    {
        service = new T();
        if (!service->init(config)) {
            LOG_ERROR("AppInitializer", QString("Failed to initialize %1").arg(serviceName));
            delete service;
            service = nullptr;
            return false;
        }
        LOG_INFO("AppInitializer", QString("%1 initialized successfully").arg(serviceName));
        return true;
    }
    
    bool isApiKeyValid(const QString& apiKey)
    {
        return !apiKey.isEmpty() && apiKey != PLACEHOLDER_API_KEY;
    }
    
    bool checkMockMode(const QString& apiKey, bool forceMock, const QString& clientName)
    {
        if (!isApiKeyValid(apiKey) && !forceMock) {
            return false;
        }
        
        if (forceMock && !isApiKeyValid(apiKey)) {
            LOG_WARNING("AppInitializer", QString("Running in MOCK mode - %1 will use placeholder responses").arg(clientName));
        }
        
        return true;
    }
}

void AppInitializer::logConfigSearchPaths()
{
    LOG_WARNING("AppInitializer", "config.ini not found in any of these locations:");
    for (const QString& path : getConfigSearchPaths()) {
        LOG_WARNING("AppInitializer", QString("  - %1").arg(path));
    }
}

QString AppInitializer::findConfigFile()
{
    for (const QString& path : getConfigSearchPaths()) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return QString();
}

void AppInitializer::ensureDefaultUserExists()
{
    // 仅在开发模式下创建默认用户
    // 通过环境变量 QINGNING_DEV_MODE=1 启用
    QString devMode = qEnvironmentVariable("QINGNING_DEV_MODE");
    if (devMode != "1" && devMode.toLower() != "true") {
        LOG_INFO("Database", "Skipping default user creation (not in dev mode)");
        return;
    }
    
    DatabaseManager* db = ServiceContainer::instance()->databaseManager();
    if (!db) return;
    
    QList<QVariantMap> results = db->selectAll(
        QStringLiteral("users"),
        QStringLiteral("id = ?"),
        QVariantList() << UserSession::instance()->currentUserId()
    );
    
    if (!results.isEmpty()) {
        return;
    }
    
    QVariantMap userData;
    userData[QStringLiteral("id")] = UserSession::instance()->currentUserId();
    userData[QStringLiteral("username")] = UserSession::instance()->currentUserId();
    userData[QStringLiteral("email")] = UserSession::instance()->currentUserId() + "@local.com";
    userData[QStringLiteral("password_hash")] = QStringLiteral("dev_placeholder_do_not_use_in_production");
    
    qint64 result = db->insert(QStringLiteral("users"), userData);
    if (result > 0) {
        LOG_WARNING("Database", "Default user created in DEV MODE - DO NOT use in production!");
    } else {
        LOG_ERROR("Database", QString("Failed to create default user: %1").arg(db->lastError()));
    }
}

bool AppInitializer::initializeQwenClient(InitResult& result)
{
    QString configPath = findConfigFile();
    
    if (configPath.isEmpty()) {
        logConfigSearchPaths();
        result.errorMessage = "Config file not found";
        return false;
    }
    
    LOG_INFO("AppInitializer", QString("Loading config from: %1").arg(configPath));
    
    AppConfig* appConfig = AppConfig::instance();
    ServiceContainer::instance()->setAppConfig(appConfig);
    
    if (!appConfig->load(configPath)) {
        result.errorMessage = appConfig->lastError();
        LOG_ERROR("AppInitializer", result.errorMessage);
        return false;
    }
    
    AppConfig::QwenConfig qwenConfig = appConfig->qwen();
    bool forceMock = appConfig->qwenImage().forceMock;
    
    if (!checkMockMode(qwenConfig.apiKey, forceMock, "QwenClient")) {
        result.errorMessage = "Qwen API key not configured. Please set a valid API key in config.ini or enable forceMock mode";
        LOG_ERROR("AppInitializer", result.errorMessage);
        return false;
    }
    
    if (forceMock && !isApiKeyValid(qwenConfig.apiKey)) {
        result.qwenSuccess = true;
        return true;
    }
    
    QwenClient::Config config;
    config.apiKey = qwenConfig.apiKey;
    config.model = qwenConfig.model;
    config.endpoint = qwenConfig.endpoint;
    config.maxRetries = qwenConfig.maxRetries;
    config.maxChunkLength = qwenConfig.maxChunkLength;
    config.maxTokens = 32768;
    config.requestTimeout = qwenConfig.requestTimeout;
    
    QwenClient* qwenClient = QwenClient::instance();
    if (!qwenClient->init(config)) {
        LOG_ERROR("AppInitializer", "Failed to initialize QwenClient");
        result.errorMessage = "Failed to initialize QwenClient";
        return false;
    }
    LOG_INFO("AppInitializer", "QwenClient initialized successfully");
    
    ServiceContainer::instance()->setQwenClient(qwenClient);
    result.qwenSuccess = true;
    return true;
}

bool AppInitializer::initializeQwenImageClient(InitResult& result)
{
    AppConfig::QwenImageConfig qwenImageConfig = AppConfig::instance()->qwenImage();
    
    if (!checkMockMode(qwenImageConfig.apiKey, qwenImageConfig.forceMock, "QwenImageClient")) {
        result.errorMessage = "QwenImage API key not configured. Please set a valid API key in config.ini or enable forceMock mode";
        LOG_ERROR("AppInitializer", result.errorMessage);
        return false;
    }
    
    if (qwenImageConfig.forceMock && !isApiKeyValid(qwenImageConfig.apiKey)) {
        result.qwenImageSuccess = true;
        return true;
    }
    
    QwenImageClient::Config config;
    config.apiKey = qwenImageConfig.apiKey;
    config.baseUrl = qwenImageConfig.baseUrl;
    config.generateModel = qwenImageConfig.generateModel;
    config.editModel = qwenImageConfig.editModel;
    config.requestTimeout = qwenImageConfig.requestTimeout;
    config.maxRetries = qwenImageConfig.maxRetries;
    config.forceMock = qwenImageConfig.forceMock;
    
    QwenImageClient* qwenImageClient = QwenImageClient::instance();
    if (!qwenImageClient->init(config)) {
        LOG_ERROR("AppInitializer", "Failed to initialize QwenImageClient");
        result.errorMessage = "Failed to initialize QwenImageClient";
        return false;
    }
    LOG_INFO("AppInitializer", "QwenImageClient initialized successfully");
    
    ServiceContainer::instance()->setQwenImageClient(qwenImageClient);
    result.qwenImageSuccess = true;
    return true;
}

bool AppInitializer::initializeStorageClient(InitResult& result)
{
    AppConfig::StorageConfig storageConfig = AppConfig::instance()->storage();
    
    StorageClient::Config config;
    config.endpoint = storageConfig.endpoint;
    config.bucket = storageConfig.bucket;
    config.accessKey = storageConfig.accessKey;
    config.secretKey = storageConfig.secretKey;
    config.region = storageConfig.region;
    config.presignExpiresIn = storageConfig.presignExpiresIn;
    config.presignApiEndpoint = storageConfig.presignApiEndpoint;
    config.authToken = storageConfig.authToken;
    
    bool usePresignApi = !config.presignApiEndpoint.isEmpty();
    bool useDirectMode = !config.accessKey.isEmpty() && !config.secretKey.isEmpty();
    
    if (!usePresignApi && !useDirectMode) {
        result.errorMessage = "Storage not configured. Please configure StorageClient in config.ini";
        LOG_ERROR("AppInitializer", result.errorMessage);
        return false;
    }
    
    StorageClient* storageClient = nullptr;
    if (!initService(storageClient, config, "StorageClient")) {
        result.errorMessage = "Failed to initialize StorageClient";
        return false;
    }
    
    ServiceContainer::instance()->setStorageClient(storageClient);
    result.storageSuccess = true;
    return true;
}

void AppInitializer::registerTaskHandlers()
{
    TaskRegistry::instance()->registerAllHandlers();
}

AppInitializer::InitResult AppInitializer::initialize()
{
    InitResult result;
    
    ServiceContainer* container = ServiceContainer::instance();
    
    QString configPath = findConfigFile();
    if (configPath.isEmpty()) {
        logConfigSearchPaths();
        result.errorMessage = "Config file not found. Please copy config.ini.example to config.ini and fill in your database password.";
        LOG_ERROR("AppInitializer", result.errorMessage);
        return result;
    }
    
    LOG_INFO("AppInitializer", QString("Loading config from: %1").arg(configPath));
    
    AppConfig* appConfig = AppConfig::instance();
    container->setAppConfig(appConfig);
    
    if (!appConfig->load(configPath)) {
        result.errorMessage = appConfig->lastError();
        LOG_ERROR("AppInitializer", result.errorMessage);
        return result;
    }
    
    AppConfig::DatabaseConfig dbConfig = appConfig->database();
    
    DatabaseManager* db = DatabaseManager::instance();
    container->setDatabaseManager(db);
    
    ::DatabaseConfig dbConfigStruct;
    dbConfigStruct.host = dbConfig.host;
    dbConfigStruct.port = dbConfig.port;
    dbConfigStruct.database = dbConfig.database;
    dbConfigStruct.username = dbConfig.username;
    dbConfigStruct.password = dbConfig.password;
    
    if (!db->connect(dbConfigStruct)) {
        result.errorMessage = QString("Failed to connect to database: %1").arg(db->lastError());
        LOG_ERROR("Database", result.errorMessage);
        return result;
    }
    
    LOG_INFO("Database", "Database connected successfully");
    ensureDefaultUserExists();
    result.databaseSuccess = true;
    
    StoryboardService* storyboardService = new StoryboardService(db);
    container->setStoryboardService(storyboardService);
    
    NovelService* novelService = NovelService::instance();
    container->setNovelService(novelService);
    
    ImageService* imageService = new ImageService();
    container->setImageService(imageService);
    
    ExportService* exportService = new ExportService(db);
    container->setExportService(exportService);
    
    initializeQwenClient(result);
    initializeQwenImageClient(result);
    initializeStorageClient(result);
    
    QString dataDir = AppConfig::instance()->storage().dataDir;
    FileStorage::instance()->init(dataDir);
    LOG_INFO("AppInitializer", QString("FileStorage initialized at: %1").arg(dataDir));
    
    registerTaskHandlers();
    
    NovelViewModel::instance()->initialize();
    StoryboardViewModel::instance()->initialize();
    LOG_INFO("AppInitializer", "ViewModels initialized");
    
    return result;
}
