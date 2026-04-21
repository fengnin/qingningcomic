#include "services/ServiceContainer.h"
#include "data/DatabaseManager.h"
#include "api/QwenClient.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "api/StorageClient.h"
#include "data/FileStorage.h"
#include "services/StoryboardService.h"
#include "services/NovelService.h"
#include "services/ImageService.h"
#include "services/BibleImageService.h"
#include "services/TaskQueue.h"
#include "utils/AppConfig.h"
#include "services/ExportService.h"
#include "services/ChangeRequestService.h"

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

void ServiceContainer::cleanup()
{
    QMutexLocker locker(&m_instanceMutex);
    if (m_instance) {
        m_instance->clear();
        delete m_instance;
        m_instance = nullptr;
    }
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
