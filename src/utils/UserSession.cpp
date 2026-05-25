#include "utils/UserSession.h"
#include "utils/AppConfig.h"
#include "utils/Logger.h"
#include "data/DatabaseManager.h"

UserSession* UserSession::m_instance = nullptr;
std::once_flag UserSession::m_instanceOnceFlag;

UserSession::UserSession()
    : m_currentUserId(AppConfig::DEFAULT_USER_ID)
    , m_currentUsername(AppConfig::DEFAULT_USER_ID)
{
    LOG_INFO("UserSession", QString("Initialized with default user: %1").arg(m_currentUserId));
}

UserSession::~UserSession()
{
}

UserSession* UserSession::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new UserSession();
    });
    return m_instance;
}

void UserSession::login(const QString& userId, const QString& username)
{
    if (userId.isEmpty()) {
        LOG_WARNING("UserSession", "Attempted to login with empty userId");
        return;
    }
    
    m_currentUserId = userId;
    m_currentUsername = username.isEmpty() ? userId : username;
    
    LOG_INFO("UserSession", QString("User logged in: %1 (%2)").arg(m_currentUserId, m_currentUsername));
    
    emit userChanged(m_currentUserId);
    emit loginSuccess(m_currentUserId);
}

void UserSession::logout()
{
    QString oldUser = m_currentUserId;

    if (!oldUser.isEmpty()) {
        DatabaseManager::instance()->executeUpdate(
            "UPDATE users SET is_online = 0 WHERE id = ?",
            QVariantList() << oldUser
        );
    }

    m_currentUserId.clear();
    m_currentUsername.clear();

    LOG_INFO("UserSession", QString("User logged out: %1").arg(oldUser));

    emit userChanged(QString());
    emit logoutSuccess();
}
