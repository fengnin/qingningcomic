#ifndef DATABASEWORKER_H
#define DATABASEWORKER_H

#include <QObject>
#include <QSqlDatabase>
#include <QVariantMap>
#include <QVariantList>
#include <QMutex>

class DatabaseWorker : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseWorker(QObject* parent = nullptr);
    ~DatabaseWorker();

    bool initialize(const QString& host, int port, const QString& database,
                    const QString& username, const QString& password);
    bool isConnected() const;

public slots:
    void executeInsert(const QString& requestId, const QString& table, const QVariantMap& data);
    void executeUpdate(const QString& requestId, const QString& table, const QVariantMap& data,
                       const QString& whereClause, const QVariantList& whereValues);
    void executeRemove(const QString& requestId, const QString& table,
                       const QString& whereClause, const QVariantList& whereValues);
    void executeSelectOne(const QString& requestId, const QString& table,
                          const QString& whereClause, const QVariantList& whereValues);
    void executeSelectAll(const QString& requestId, const QString& table,
                          const QString& whereClause, const QVariantList& whereValues,
                          const QString& orderBy, int limit);

signals:
    void insertCompleted(const QString& requestId, qint64 id, bool success, const QString& error);
    void updateCompleted(const QString& requestId, bool success, const QString& error);
    void removeCompleted(const QString& requestId, bool success, const QString& error);
    void selectOneCompleted(const QString& requestId, const QVariantMap& result, bool success, const QString& error);
    void selectAllCompleted(const QString& requestId, const QList<QVariantMap>& results, bool success, const QString& error);

private:
    bool connectInternal(const QString& host, int port, const QString& database,
                         const QString& username, const QString& password);
    void setLastError(const QString& error);

    QSqlDatabase m_database;
    QString m_connectionName;
    QString m_lastError;
    mutable QMutex m_mutex;
    bool m_initialized;
};

#endif // DATABASEWORKER_H
