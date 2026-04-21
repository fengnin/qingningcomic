#include "data/DatabaseWorker.h"
#include "data/SqlBuilder.h"
#include "utils/DatabaseUtils.h"
#include "utils/Logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QUuid>
#include <QThread>

DatabaseWorker::DatabaseWorker(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
    m_connectionName = QString("worker_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

DatabaseWorker::~DatabaseWorker()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_database.isOpen()) {
        m_database.close();
    }
    
    QString connName = m_connectionName;
    m_database = QSqlDatabase();
    
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase::removeDatabase(connName);
    }
}

bool DatabaseWorker::initialize(const QString& host, int port, const QString& database,
                                 const QString& username, const QString& password)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_database.isOpen()) {
        return true;
    }
    
    bool result = connectInternal(host, port, database, username, password);
    if (result) {
        m_initialized = true;
        LOG_INFO("DatabaseWorker", QString("Worker initialized in thread: %1").arg((qintptr)QThread::currentThreadId()));
    }
    return result;
}

bool DatabaseWorker::isConnected() const
{
    QMutexLocker locker(&m_mutex);
    return m_database.isOpen();
}

bool DatabaseWorker::connectInternal(const QString& host, int port, const QString& database,
                                      const QString& username, const QString& password)
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
    
    m_database = QSqlDatabase::addDatabase("QMYSQL", m_connectionName);
    m_database.setHostName(host);
    m_database.setPort(port);
    m_database.setDatabaseName(database);
    m_database.setUserName(username);
    m_database.setPassword(password);
    m_database.setConnectOptions(DatabaseUtils::simpleConnectionOptions());
    
    if (!m_database.open()) {
        setLastError(m_database.lastError().text());
        LOG_ERROR("DatabaseWorker", QString("Connection failed: %1").arg(m_lastError));
        return false;
    }
    
    if (!DatabaseUtils::initConnectionCharset(m_database)) {
        LOG_WARNING("DatabaseWorker", "Failed to set charset, continuing anyway");
    }

    if (!DatabaseUtils::initConnectionSessionTimeouts(m_database)) {
        LOG_WARNING("DatabaseWorker", "Failed to set session timeouts, continuing anyway");
    }

    LOG_INFO("DatabaseWorker", QString("Connected to %1 (thread: %2)")
        .arg(database).arg((qintptr)QThread::currentThreadId()));
    return true;
}

void DatabaseWorker::setLastError(const QString& error)
{
    m_lastError = error;
}

void DatabaseWorker::executeInsert(const QString& requestId, const QString& table, const QVariantMap& data)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_database.isOpen()) {
        emit insertCompleted(requestId, -1, false, "Database not connected");
        return;
    }
    
    QStringList columns;
    QVariantList values;
    QString sql = SqlBuilder::buildInsert(table, data, columns, values);
    
    QSqlQuery query(m_database);
    if (!query.prepare(sql)) {
        emit insertCompleted(requestId, -1, false, query.lastError().text());
        return;
    }
    
    DatabaseUtils::bindValues(query, values);
    
    if (!query.exec()) {
        emit insertCompleted(requestId, -1, false, query.lastError().text());
        return;
    }
    
    qint64 id = query.lastInsertId().toLongLong();
    emit insertCompleted(requestId, id, true, QString());
}

void DatabaseWorker::executeUpdate(const QString& requestId, const QString& table, const QVariantMap& data,
                                    const QString& whereClause, const QVariantList& whereValues)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_database.isOpen()) {
        emit updateCompleted(requestId, false, "Database not connected");
        return;
    }
    
    QVariantList values;
    QString sql = SqlBuilder::buildUpdate(table, data, whereClause, values);
    values << whereValues;
    
    QSqlQuery query(m_database);
    if (!query.prepare(sql)) {
        emit updateCompleted(requestId, false, query.lastError().text());
        return;
    }
    
    DatabaseUtils::bindValues(query, values);
    
    if (!query.exec()) {
        emit updateCompleted(requestId, false, query.lastError().text());
        return;
    }
    
    emit updateCompleted(requestId, true, QString());
}

void DatabaseWorker::executeRemove(const QString& requestId, const QString& table,
                                    const QString& whereClause, const QVariantList& whereValues)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_database.isOpen()) {
        emit removeCompleted(requestId, false, "Database not connected");
        return;
    }
    
    QString sql = SqlBuilder::buildDelete(table, whereClause);
    
    QSqlQuery query(m_database);
    if (!query.prepare(sql)) {
        emit removeCompleted(requestId, false, query.lastError().text());
        return;
    }
    
    DatabaseUtils::bindValues(query, whereValues);
    
    if (!query.exec()) {
        emit removeCompleted(requestId, false, query.lastError().text());
        return;
    }
    
    emit removeCompleted(requestId, true, QString());
}

void DatabaseWorker::executeSelectOne(const QString& requestId, const QString& table,
                                       const QString& whereClause, const QVariantList& whereValues)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_database.isOpen()) {
        emit selectOneCompleted(requestId, QVariantMap(), false, "Database not connected");
        return;
    }
    
    QString sql = SqlBuilder::buildSelect(table, whereClause, QString(), 1);
    
    QSqlQuery query(m_database);
    if (!query.prepare(sql)) {
        emit selectOneCompleted(requestId, QVariantMap(), false, query.lastError().text());
        return;
    }
    
    DatabaseUtils::bindValues(query, whereValues);
    
    if (!query.exec()) {
        emit selectOneCompleted(requestId, QVariantMap(), false, query.lastError().text());
        return;
    }
    
    QVariantMap result;
    if (query.next()) {
        result = DatabaseUtils::recordToMap(query);
    }
    
    emit selectOneCompleted(requestId, result, true, QString());
}

void DatabaseWorker::executeSelectAll(const QString& requestId, const QString& table,
                                       const QString& whereClause, const QVariantList& whereValues,
                                       const QString& orderBy, int limit)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_database.isOpen()) {
        emit selectAllCompleted(requestId, QList<QVariantMap>(), false, "Database not connected");
        return;
    }
    
    QString sql = SqlBuilder::buildSelect(table, whereClause, orderBy, limit);
    
    QSqlQuery query(m_database);
    if (!query.prepare(sql)) {
        emit selectAllCompleted(requestId, QList<QVariantMap>(), false, query.lastError().text());
        return;
    }
    
    DatabaseUtils::bindValues(query, whereValues);
    
    if (!query.exec()) {
        emit selectAllCompleted(requestId, QList<QVariantMap>(), false, query.lastError().text());
        return;
    }
    
    QList<QVariantMap> results;
    while (query.next()) {
        results.append(DatabaseUtils::recordToMap(query));
    }
    
    emit selectAllCompleted(requestId, results, true, QString());
}
