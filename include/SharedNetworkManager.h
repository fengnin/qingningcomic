#ifndef SHAREDNETWORKMANAGER_H
#define SHAREDNETWORKMANAGER_H

#include <QNetworkAccessManager>
#include <QMutex>

class SharedNetworkManager : public QObject
{
    Q_OBJECT

public:
    static SharedNetworkManager* instance();
    QNetworkAccessManager* manager() const { return m_manager; }

private:
    explicit SharedNetworkManager(QObject* parent = nullptr);
    ~SharedNetworkManager();
    
    static SharedNetworkManager* m_instance;
    static QMutex m_mutex;
    
    QNetworkAccessManager* m_manager;
};

#endif // SHAREDNETWORKMANAGER_H
