#ifndef SERVICECONTAINER_H
#define SERVICECONTAINER_H

#include <QObject>
#include <QMutex>
#include <QMutexLocker>

class DatabaseManager;
class QwenClient;
class QwenImageClient;
class VolcEngineImageClient;
class StorageClient;
class FileStorage;
class StoryboardService;
class NovelService;
class ImageService;
class BibleImageService;
class TaskQueue;
class AppConfig;
class ExportService;
class ChangeRequestService;

#define SERVICE_CONTAINER_METHODS(Type, member, getterName) \
    void set##Type(Type* service) { m_##member = service; } \
    Type* getterName() const { return m_##member; } \
    bool has##Type() const { return m_##member != nullptr; }

class ServiceContainer : public QObject
{public:
    static ServiceContainer* instance();
    
    SERVICE_CONTAINER_METHODS(DatabaseManager, databaseManager, databaseManager)
    SERVICE_CONTAINER_METHODS(QwenClient, qwenClient, qwenClient)
    SERVICE_CONTAINER_METHODS(QwenImageClient, qwenImageClient, qwenImageClient)
    SERVICE_CONTAINER_METHODS(VolcEngineImageClient, volcEngineImageClient, volcEngineImageClient)
    SERVICE_CONTAINER_METHODS(StorageClient, storageClient, storageClient)
    SERVICE_CONTAINER_METHODS(FileStorage, fileStorage, fileStorage)
    SERVICE_CONTAINER_METHODS(StoryboardService, storyboardService, storyboardService)
    SERVICE_CONTAINER_METHODS(NovelService, novelService, novelService)
    SERVICE_CONTAINER_METHODS(ImageService, imageService, imageService)
    SERVICE_CONTAINER_METHODS(BibleImageService, bibleImageService, bibleImageService)
    SERVICE_CONTAINER_METHODS(TaskQueue, taskQueue, taskQueue)
    SERVICE_CONTAINER_METHODS(AppConfig, appConfig, appConfig)
    SERVICE_CONTAINER_METHODS(ExportService, exportService, exportService)
    SERVICE_CONTAINER_METHODS(ChangeRequestService, changeRequestService, changeRequestService)
    
    void clear();

private:
    ServiceContainer();
    ~ServiceContainer();
    ServiceContainer(const ServiceContainer&) = delete;
    ServiceContainer& operator=(const ServiceContainer&) = delete;
    
    static ServiceContainer* m_instance;
    static QMutex m_instanceMutex;
    
    DatabaseManager* m_databaseManager = nullptr;
    QwenClient* m_qwenClient = nullptr;
    QwenImageClient* m_qwenImageClient = nullptr;
    VolcEngineImageClient* m_volcEngineImageClient = nullptr;
    StorageClient* m_storageClient = nullptr;
    FileStorage* m_fileStorage = nullptr;
    StoryboardService* m_storyboardService = nullptr;
    NovelService* m_novelService = nullptr;
    ImageService* m_imageService = nullptr;
    BibleImageService* m_bibleImageService = nullptr;
    TaskQueue* m_taskQueue = nullptr;
    AppConfig* m_appConfig = nullptr;
    ExportService* m_exportService = nullptr;
    ChangeRequestService* m_changeRequestService = nullptr;
};

#undef SERVICE_CONTAINER_METHODS

#endif // SERVICECONTAINER_H
