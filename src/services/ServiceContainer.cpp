#include "ServiceContainer.h"
#include "DatabaseManager.h"
#include "QwenClient.h"
#include "QwenImageClient.h"
#include "VolcEngineImageClient.h"
#include "StorageClient.h"
#include "FileStorage.h"
#include "StoryboardService.h"
#include "NovelService.h"
#include "ImageService.h"
#include "BibleImageService.h"
#include "TaskQueue.h"
#include "AppConfig.h"
#include "services/ExportService.h"
#include "ChangeRequestService.h"

ServiceContainer* ServiceContainer::m_instance = nullptr;
QMutex ServiceContainer::m_instanceMutex;

ServiceContainer::ServiceContainer()
    : QObject(nullptr)
{
}

ServiceContainer::~ServiceContainer()
{
}

ServiceContainer* ServiceContainer::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_instanceMutex);
        if (!m_instance) {
            m_instance = new ServiceContainer();
        }
    }
    return m_instance;
}

void ServiceContainer::clear()
{
    m_databaseManager = nullptr;
    m_qwenClient = nullptr;
    m_qwenImageClient = nullptr;
    m_storageClient = nullptr;
    m_fileStorage = nullptr;
    m_storyboardService = nullptr;
    m_novelService = nullptr;
    m_imageService = nullptr;
    m_bibleImageService = nullptr;
    m_taskQueue = nullptr;
    m_appConfig = nullptr;
    m_exportService = nullptr;
    m_changeRequestService = nullptr;
}
