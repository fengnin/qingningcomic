#include "api/SharedNetworkManager.h"
#include "utils/Logger.h"

SharedNetworkManager* SharedNetworkManager::m_instance = nullptr;
QMutex SharedNetworkManager::m_mutex;

SharedNetworkManager::SharedNetworkManager(QObject* parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
    LOG_INFO("SharedNetworkManager", "Shared QNetworkAccessManager created");
}

SharedNetworkManager::~SharedNetworkManager()
{
    LOG_INFO("SharedNetworkManager", "Shared QNetworkAccessManager destroyed");
}

SharedNetworkManager* SharedNetworkManager::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_mutex);
        if (!m_instance) {
            m_instance = new SharedNetworkManager();
        }
    }
    return m_instance;
}
