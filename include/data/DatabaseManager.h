#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMutex>
#include <QRecursiveMutex>
#include <QObject>
#include <QThread>
#include <QThreadStorage>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QList>
#include <functional>
#include "utils/SingletonUtils.h"
#include "utils/Logger.h"

struct DatabaseConfig
{
    QString host = "localhost";
    int port = 3306;
    QString database = "qingning_comic";
    QString username = "qingning";
    QString password;
    QString connectionName = "qingning_comic_connection";
};

class DatabaseManager : public QObject
{
    DECLARE_SINGLETON_MEMBERS(DatabaseManager)

public:
    using TransactionOperation = std::function<bool()>;

    template<typename T>
    using DbOperation = std::function<T()>;

    bool connect(const DatabaseConfig& config);
    bool connect(const QString& host, int port, const QString& database,
                 const QString& username, const QString& password);
    void disconnect();
    bool isConnected() const;
    bool reconnectIfNeeded();
    QSqlDatabase& database();
    const QSqlDatabase& database() const;

    QString lastError() const;
    QSqlQuery createQuery();

    qint64 insert(const QString& table, const QVariantMap& data);
    bool update(const QString& table, const QVariantMap& data,
                const QString& whereClause, const QVariantList& whereValues);
    bool remove(const QString& table, const QString& whereClause, const QVariantList& whereValues);

    QVariantMap selectOne(const QString& table, const QString& whereClause = QString(),
                          const QVariantList& whereValues = QVariantList());
    QList<QVariantMap> selectAll(const QString& table, const QString& whereClause = QString(),
                                  const QVariantList& whereValues = QVariantList(),
                                  const QString& orderBy = QString(), int limit = 0);

    bool executeUpdate(const QString& sql, const QVariantList& values = QVariantList());
    QList<QVariantMap> executeQuery(const QString& sql);
    QList<QVariantMap> executeQuery(const QString& sql, const QVariantList& values);
    bool executeSql(const QString& sql);

    int count(const QString& table, const QString& whereClause = QString(),
              const QVariantList& whereValues = QVariantList());

    bool ensureSoftDeleteColumns(const QString& tableName);
    bool ensureCharacterPortraitVersionsSchema();

    bool beginTransaction();
    bool commit();
    bool rollback();
    bool inTransaction() const;
    bool transaction(TransactionOperation operation);

    void lock();
    void unlock();
    bool tryLock(int timeoutMs = 5000);

private:
    DatabaseManager();
    ~DatabaseManager();

    bool connectInternal(const QString& host, int port, const QString& database,
                         const QString& username, const QString& password);

    // Per-thread connection wrapper — owned by QThreadStorage, auto-cleaned on thread exit.
    struct ThreadConn {
        QSqlDatabase db;
        QString name;
        ~ThreadConn() {
            if (db.isOpen()) db.close();
            db = QSqlDatabase();
            if (!name.isEmpty() && QSqlDatabase::contains(name))
                QSqlDatabase::removeDatabase(name);
        }
    };

    // Returns the connection appropriate for the calling thread:
    //   main thread  → m_database
    //   other thread → a thread-local connection (opened on first use)
    QSqlDatabase& activeDb();

    void bindValues(QSqlQuery& query, const QVariantList& values);
    QVariantMap recordToMap(const QSqlQuery& query) const;

    void setLastError(const QString& error);
    bool fail(const QString& error);
    bool checkConnection(const QString& operation);
    bool executeQueryInternal(QSqlQuery& query, const QString& sql, const QString& operation);

    template<typename T>
    T safeExecute(const QString& operation, DbOperation<T> dbOp, T defaultValue);

    // Executes dbOp on the correct connection for the calling thread.
    // Main thread: acquires m_mutex, uses m_database.
    // Background thread: uses a thread-local connection directly — main thread never blocked.
    template<typename T>
    T safeExecuteWithLock(const QString& operation, DbOperation<T> dbOp, T defaultValue);

    bool ensureConnection(const QString& operation);
    void logError(const QString& operation, const QString& message);
    void logAndSetError(const QString& operation, const QString& message);

    mutable QRecursiveMutex m_mutex;
    QSqlDatabase m_database;
    QString m_lastError;
    QString m_connectionName;
    bool m_inTransaction = false;

    // Snapshot of connection params used to open per-thread connections.
    QString m_host;
    int     m_port = 3306;
    QString m_dbName;
    QString m_user;
    QString m_pass;

    QThreadStorage<ThreadConn*> m_threadConns;
};

template<typename T>
T DatabaseManager::safeExecute(const QString& operation, DbOperation<T> dbOp, T defaultValue)
{
    try {
        return dbOp();
    } catch (const std::exception& e) {
        logAndSetError(operation, QString::fromUtf8(e.what()));
        return defaultValue;
    } catch (...) {
        logAndSetError(operation, QString("Unknown exception in %1").arg(operation));
        return defaultValue;
    }
}

template<typename T>
T DatabaseManager::safeExecuteWithLock(const QString& operation, DbOperation<T> dbOp, T defaultValue)
{
    if (QThread::currentThread() != thread()) {
        // Background thread: activeDb() returns (or opens) a thread-local connection.
        // The main thread is never involved — no BlockingQueuedConnection, no UI freeze.
        QSqlDatabase& bg = activeDb();
        if (!bg.isValid() || !bg.isOpen()) {
            LOG_ERROR("Database", QString("%1: thread-local connection unavailable").arg(operation));
            return defaultValue;
        }
        return safeExecute(operation, dbOp, defaultValue);
    }

    QMutexLocker locker(&m_mutex);
    if (!ensureConnection(operation)) {
        return defaultValue;
    }
    return safeExecute(operation, dbOp, defaultValue);
}

#endif // DATABASEMANAGER_H
