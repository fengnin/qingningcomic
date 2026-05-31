#include "app/AppInitializer.h"
#include "services/ServiceContainer.h"
#include "data/DatabaseManager.h"
#include "api/QwenClient.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "data/FileStorage.h"
#include "services/NovelService.h"
#include "services/StoryboardService.h"
#include "services/ImageService.h"
#include "services/BibleImageService.h"
#include "services/ExportService.h"
#include "services/ChangeRequestService.h"
#include "services/TaskQueue.h"
#include "services/TaskRegistry.h"
#include "services/CharacterExtractor.h"
#include "viewmodels/NovelViewModel.h"
#include "viewmodels/StoryboardViewModel.h"
#include "utils/AppConfig.h"
#include "utils/UserSession.h"
#include "utils/Logger.h"
#include "utils/CharacterEditPromptBuilder.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QCryptographicHash>
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>

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
    const AppConfig::DemoConfig demo = AppConfig::instance()->demo();
    if (!demo.enabled || demo.users.isEmpty()) {
        LOG_INFO("AppInitializer", "Demo accounts not configured, skipping user creation");
        return;
    }

    DatabaseManager* db = ServiceContainer::instance()->databaseManager();
    if (!db) return;

    for (const auto& user : demo.users) {
        const QString& username = user.first;
        const QString& password = user.second;

        if (username.isEmpty() || password.isEmpty()) continue;

        if (!db->selectAll("users", "username = ?", QVariantList{username}).isEmpty())
            continue;

        const QString passwordHash = QString(
            QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());

        QVariantMap userData;
        userData["id"]            = QUuid::createUuid().toString(QUuid::WithoutBraces);
        userData["username"]      = username;
        userData["email"]         = username + "@demo.local";
        userData["password_hash"] = passwordHash;

        if (db->insert("users", userData) > 0) {
            LOG_INFO("AppInitializer", QString("Demo account created: %1").arg(username));
        } else {
            LOG_ERROR("AppInitializer", QString("Failed to create demo account: %1").arg(db->lastError()));
        }
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

bool AppInitializer::initializeVolcEngineImageClient(InitResult& result)
{
    AppConfig::VolcEngineConfig volcConfig = AppConfig::instance()->volcEngine();
    
    if (volcConfig.accessKey.isEmpty() || volcConfig.accessKey == "your-volcengine-access-key") {
        LOG_WARNING("AppInitializer", "VolcEngine accessKey not configured, skipping VolcEngineImageClient initialization");
        result.volcEngineImageSuccess = false;
        return true;  // 不是致命错误，继续初始化
    }
    
    if (volcConfig.forceMock) {
        LOG_WARNING("AppInitializer", "VolcEngineImageClient running in MOCK mode");
    }
    
    VolcEngineImageClient::Config config;
    config.accessKey = volcConfig.accessKey;
    config.secretKey = volcConfig.secretKey;
    config.baseUrl = volcConfig.baseUrl;
    config.region = volcConfig.region;
    config.service = volcConfig.service;
    config.reqKey = volcConfig.reqKey;
    config.img2imgReqKey = volcConfig.img2imgReqKey;
    config.seedEditReqKey = volcConfig.seedEditReqKey;
    config.requestTimeout = volcConfig.requestTimeout;
    config.forceMock = volcConfig.forceMock;
    
    VolcEngineImageClient* volcEngineClient = VolcEngineImageClient::instance();
    if (!volcEngineClient->init(config)) {
        LOG_ERROR("AppInitializer", "Failed to initialize VolcEngineImageClient");
        result.errorMessage = "Failed to initialize VolcEngineImageClient";
        return false;
    }
    LOG_INFO("AppInitializer", "VolcEngineImageClient initialized successfully");
    
    ServiceContainer::instance()->setVolcEngineImageClient(volcEngineClient);
    result.volcEngineImageSuccess = true;
    return true;
}

void AppInitializer::registerTaskHandlers()
{
    TaskRegistry::instance()->registerAllHandlers();
}

void AppInitializer::connectAutoPortraitEdit()
{
    QObject::connect(CharacterExtractor::instance(),
                     &CharacterExtractor::characterVisualFieldsChanged,
                     BibleImageService::instance(),
                     [](const QString& characterId,
                        const QList<CharacterFieldDiff>& diff,
                        int sourceChapter) {
        if (characterId.isEmpty() || diff.isEmpty()) {
            return;
        }
        Character character = CharacterExtractor::instance()->getCharacterById(characterId);
        if (character.id().isEmpty()) {
            LOG_WARNING("AppInitializer",
                QString("Auto-edit skipped: character %1 not found").arg(characterId));
            return;
        }
        if (character.portraitPath().isEmpty() && character.currentPortraitVersionId().isEmpty()) {
            LOG_INFO("AppInitializer",
                QString("Auto-edit skipped: character %1 has no base portrait yet").arg(character.name()));
            return;
        }
        const QString prompt = CharacterEditPromptBuilder::buildFromDiff(diff);
        if (prompt.isEmpty()) {
            return;
        }
        LOG_INFO("AppInitializer",
            QString("Auto-editing portrait for %1 (chapter %2): %3")
                .arg(character.name()).arg(sourceChapter).arg(prompt));
        BibleImageService::instance()->editCharacterPortraitAsync(
            character, prompt, QString(), diff, sourceChapter);
    });
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
    // Run schema migrations in the background so the main thread is never blocked
    // by information_schema queries or DDL on a remote MySQL server.
    QtConcurrent::run([]() {
        DatabaseManager::instance()->ensureCharacterPortraitVersionsSchema();
    });
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
    
    ChangeRequestService* changeRequestService = new ChangeRequestService(db);
    container->setChangeRequestService(changeRequestService);

    initializeQwenClient(result);
    initializeQwenImageClient(result);
    initializeVolcEngineImageClient(result);

    QString dataDir = AppConfig::instance()->storage().dataDir;
    FileStorage::instance()->init(dataDir);
    LOG_INFO("AppInitializer", QString("FileStorage initialized at: %1")
        .arg(FileStorage::instance()->getFullPath(QString())));
    
    registerTaskHandlers();
    
    NovelViewModel::instance()->initialize();
    StoryboardViewModel::instance()->initialize();
    LOG_INFO("AppInitializer", "ViewModels initialized");

    connectAutoPortraitEdit();

    return result;
}
