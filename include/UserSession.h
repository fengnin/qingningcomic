#ifndef USERSESSION_H
#define USERSESSION_H

#include <QString>
#include <QObject>

/**
 * @brief 用户会话管理类（单例）
 * 
 * 管理当前登录用户的状态，所有需要用户ID的地方都从这里获取。
 * 
 * 设计思路：
 * 1. 桌面版：自动登录默认用户（开发阶段）
 * 2. 网页版：从登录界面获取用户信息
 * 3. 未来可扩展：支持多用户切换、记住登录状态等
 */
class UserSession : public QObject
{
    Q_OBJECT

public:
    static UserSession* instance();
    
    bool isLoggedIn() const { return !m_currentUserId.isEmpty(); }
    QString currentUserId() const { return m_currentUserId; }
    QString currentUsername() const { return m_currentUsername; }
    
    void login(const QString& userId, const QString& username = QString());
    void logout();
    
    void setUserId(const QString& userId) { m_currentUserId = userId; }

signals:
    void userChanged(const QString& userId);
    void loginSuccess(const QString& userId);
    void logoutSuccess();

private:
    UserSession();
    ~UserSession();
    UserSession(const UserSession&) = delete;
    UserSession& operator=(const UserSession&) = delete;
    
    static UserSession* m_instance;
    
    QString m_currentUserId;
    QString m_currentUsername;
};

#endif // USERSESSION_H
