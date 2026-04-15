#include "DatabaseManager.h"
#include "ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "utils/DatabaseUtils.h"
#include "SqlBuilder.h"
#include "Logger.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QSet>

DEFINE_SINGLETON_INSTANCE_SIMPLE(DatabaseManager)

DatabaseManager::DatabaseManager()
    : m_connectionName("qingning_comic_connection")
{
}

DatabaseManager::~DatabaseManager()
{
    disconnect();
}

bool DatabaseManager::connect(const DatabaseConfig& config)
{
    QMutexLocker locker(&m_mutex);
    return connectInternal(config.host, config.port, config.database, config.username, config.password);
}

bool DatabaseManager::connectInternal(const QString& host, int port, const QString& database,
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
    m_database.setConnectOptions(DatabaseUtils::defaultConnectionOptions());
    
    if (!m_database.open()) {
        setLastError(m_database.lastError().text());
        LOG_ERROR("Database", QString("Connection failed: %1").arg(m_lastError));
        return false;
    }
    
    if (!DatabaseUtils::initConnectionCharsetNoAutoCommit(m_database)) {
        LOG_WARNING("Database", "Failed to set charset, continuing anyway");
    }
    
    LOG_INFO("Database", QString("Connected to %1 (utf8mb4)").arg(database));
    return true;
}

bool DatabaseManager::connect(const QString& host, int port, const QString& database,
                               const QString& username, const QString& password)
{
    QMutexLocker locker(&m_mutex);
    return connectInternal(host, port, database, username, password);
}

void DatabaseManager::disconnect()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_database.isOpen()) {
        QString dbName = m_database.databaseName();
        m_database.close();
        LOG_INFO("Database", QString("Disconnected from %1").arg(dbName));
    }
    
    QString connectionName = m_database.connectionName();
    m_database = QSqlDatabase();
    
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool DatabaseManager::isConnected() const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_database.isOpen()) {
        return false;
    }
    
    return true;
}

QString DatabaseManager::lastError() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastError;
}

QSqlDatabase& DatabaseManager::database()
{
    return m_database;
}

const QSqlDatabase& DatabaseManager::database() const
{
    return m_database;
}

void DatabaseManager::bindValues(QSqlQuery& query, const QVariantList& values)
{
    DatabaseUtils::bindValues(query, values);
}

QVariantMap DatabaseManager::recordToMap(const QSqlQuery& query) const
{
    return DatabaseUtils::recordToMap(query);
}

QSqlQuery DatabaseManager::createQuery()
{
    try {
        return QSqlQuery(m_database);
    } catch (const std::exception& e) {
        LOG_ERROR("Database", QString("createQuery exception: %1").arg(e.what()));
        return QSqlQuery();
    } catch (...) {
        LOG_ERROR("Database", "createQuery unknown exception");
        return QSqlQuery();
    }
}

bool DatabaseManager::executeQueryInternal(QSqlQuery& query, const QString& sql, const QString& operation)
{
    try {
        bool success = sql.isEmpty() ? query.exec() : query.exec(sql);
        
        if (!success) {
            setLastError(query.lastError().text());
            QString executedSql = sql.isEmpty() ? query.lastQuery() : sql;
            LOG_ERROR("Database", QString("%1 failed: %2, SQL: %3")
                .arg(operation, m_lastError, executedSql.left(200)));
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        QString errorMsg = QString::fromUtf8(e.what());
        setLastError(errorMsg);
        LOG_ERROR("Database", QString("%1 exception: %2").arg(operation, errorMsg));
        return false;
    } catch (...) {
        setLastError(QString("Unknown exception in %1").arg(operation));
        LOG_ERROR("Database", QString("%1 unknown exception").arg(operation));
        return false;
    }
}

bool DatabaseManager::executeSql(const QString& sql)
{
    return safeExecuteWithLock<bool>("executeSql", [&]() -> bool {
        QSqlQuery query = createQuery();
        return executeQueryInternal(query, sql, "executeSql");
    }, false);
}

QList<QVariantMap> DatabaseManager::executeQuery(const QString& sql)
{
    return safeExecuteWithLock<QList<QVariantMap>>("executeQuery", [&]() -> QList<QVariantMap> {
        QSqlQuery query = createQuery();
        if (!executeQueryInternal(query, sql, "executeQuery")) {
            return QList<QVariantMap>();
        }
        
        QList<QVariantMap> results;
        while (query.next()) {
            results.append(recordToMap(query));
        }
        return results;
    }, QList<QVariantMap>());
}

QList<QVariantMap> DatabaseManager::executeQuery(const QString& sql, const QVariantList& values)
{
    return safeExecuteWithLock<QList<QVariantMap>>("executeQuery", [&]() -> QList<QVariantMap> {
        QSqlQuery query = createQuery();
        query.prepare(sql);
        bindValues(query, values);
        
        if (!executeQueryInternal(query, QString(), "executeQuery")) {
            return QList<QVariantMap>();
        }
        
        QList<QVariantMap> results;
        while (query.next()) {
            results.append(recordToMap(query));
        }
        return results;
    }, QList<QVariantMap>());
}

bool DatabaseManager::executeUpdate(const QString& sql, const QVariantList& values)
{
    return safeExecuteWithLock<bool>("executeUpdate", [&]() -> bool {
        QSqlQuery query = createQuery();
        query.prepare(sql);
        bindValues(query, values);
        
        return executeQueryInternal(query, QString(), "executeUpdate");
    }, false);
}

qint64 DatabaseManager::insert(const QString& table, const QVariantMap& data)
{
    return safeExecuteWithLock<qint64>("insert", [&]() -> qint64 {
        if (data.isEmpty()) {
            setLastError("No data to insert");
            return -1LL;
        }
        
        QStringList columns;
        QVariantList values;
        QString sql = SqlBuilder::buildInsert(table, data, columns, values);
        
        QSqlQuery query = createQuery();
        query.prepare(sql);
        bindValues(query, values);
        
        if (!executeQueryInternal(query, QString(), "insert")) return -1LL;
        
        qint64 lastId = query.lastInsertId().toLongLong();
        return lastId > 0 ? lastId : query.numRowsAffected();
    }, -1LL);
}

bool DatabaseManager::update(const QString& table, const QVariantMap& data,
                              const QString& whereClause, const QVariantList& whereValues)
{
    return safeExecuteWithLock<bool>("update", [&]() -> bool {
        if (data.isEmpty()) {
            setLastError("No data to update");
            return false;
        }
        
        QVariantList values;
        QString sql = SqlBuilder::buildUpdate(table, data, whereClause, values);
        values << whereValues;
        
        QSqlQuery query(m_database);
        query.prepare(sql);
        bindValues(query, values);
        
        bool result = executeQueryInternal(query, QString(), "update");
        
        if (result) {
            QSqlQuery commitQuery(m_database);
            commitQuery.exec("COMMIT");
        }
        
        return result;
    }, false);
}

bool DatabaseManager::remove(const QString& table, const QString& whereClause, const QVariantList& whereValues)
{
    return safeExecuteWithLock<bool>("remove", [&]() -> bool {
        QString sql = SqlBuilder::buildDelete(table, whereClause);
        
        QSqlQuery query = createQuery();
        query.prepare(sql);
        bindValues(query, whereValues);
        
        return executeQueryInternal(query, QString(), "remove");
    }, false);
}

QVariantMap DatabaseManager::selectOne(const QString& table, const QString& whereClause, const QVariantList& whereValues)
{
    return safeExecuteWithLock<QVariantMap>("selectOne", [&]() -> QVariantMap {
        QString sql = SqlBuilder::buildSelect(table, whereClause, QString(), 1);
        QString finalSql = SqlBuilder::buildDirectSql(sql, whereValues);
        
        QSqlQuery query = createQuery();
        if (!query.exec(finalSql)) {
            logAndSetError("selectOne", query.lastError().text());
            return QVariantMap();
        }
        
        if (query.next()) {
            return recordToMap(query);
        }
        return QVariantMap();
    }, QVariantMap());
}

QList<QVariantMap> DatabaseManager::selectAll(const QString& table, const QString& whereClause,
                                               const QVariantList& whereValues, const QString& orderBy, int limit)
{
    return safeExecuteWithLock<QList<QVariantMap>>("selectAll", [&]() -> QList<QVariantMap> {
        QString sql = SqlBuilder::buildSelect(table, whereClause, orderBy, limit);
        QString finalSql = SqlBuilder::buildDirectSql(sql, whereValues);
        
        QSqlQuery query = createQuery();
        if (!query.exec(finalSql)) {
            logAndSetError("selectAll", query.lastError().text());
            return QList<QVariantMap>();
        }
        
        QList<QVariantMap> results;
        while (query.next()) {
            results.append(recordToMap(query));
        }
        return results;
    }, QList<QVariantMap>());
}

int DatabaseManager::count(const QString& table, const QString& whereClause, const QVariantList& whereValues)
{
    return safeExecuteWithLock<int>("count", [&]() -> int {
        QString sql = SqlBuilder::buildCount(table, whereClause);
        QString finalSql = SqlBuilder::buildDirectSql(sql, whereValues);
        
        QSqlQuery query = createQuery();
        if (!query.exec(finalSql)) {
            logAndSetError("count", query.lastError().text());
            return 0;
        }
        
        if (!query.next()) return 0;
        
        return query.value(0).toInt();
    }, 0);
}

bool DatabaseManager::beginTransaction()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_database.isOpen()) {
        logAndSetError("beginTransaction", "Cannot begin transaction: database not open");
        return false;
    }
    
    return safeExecute<bool>("beginTransaction", [&]() -> bool {
        QSqlQuery pingQuery(m_database);
        if (!pingQuery.exec("SELECT 1")) {
            LOG_WARNING("Database", QString("Ping failed, connection may be lost. Error: %1").arg(pingQuery.lastError().text()));
            
            m_database.close();
            if (!m_database.open()) {
                logAndSetError("beginTransaction", QString("Reconnect failed: %1").arg(m_database.lastError().text()));
                return false;
            }
            
            QSqlQuery query(m_database);
            query.exec("SET NAMES utf8mb4");
            query.exec("SET autocommit = 0");
            LOG_INFO("Database", "Reconnected after ping failure");
        }
        
        QSqlQuery txQuery(m_database);
        if (!txQuery.exec("START TRANSACTION")) {
            logAndSetError("beginTransaction", QString("START TRANSACTION failed: %1").arg(txQuery.lastError().text()));
            return false;
        }
        
        m_inTransaction = true;
        return true;
    }, false);
}

bool DatabaseManager::reconnectIfNeeded()
{
    // 注意：此方法假设调用者已持有 m_mutex 锁
    // 不要在此方法内再次获取锁，否则会导致死锁
    
    try {
        // 使用作用域确保 testQuery 在关闭连接前被销毁
        if (m_database.isOpen()) {
            QSqlQuery testQuery(m_database);
            if (testQuery.exec("SELECT 1")) {
                return true;
            }
            
            LOG_WARNING("Database", "Connection check failed, reconnecting...");
        }
        // testQuery 在此处销毁，释放对数据库的引用
    } catch (const std::exception& e) {
        LOG_WARNING("Database", QString("Connection check exception: %1, will reconnect").arg(e.what()));
    } catch (...) {
        LOG_WARNING("Database", "Connection check unknown exception, will reconnect");
    }
    
    LOG_INFO("Database", "Reconnecting to database...");
    
    QString host = m_database.hostName();
    int port = m_database.port();
    QString dbName = m_database.databaseName();
    QString user = m_database.userName();
    QString pass = m_database.password();
    
    // 先完全断开连接（包括删除连接对象）
    QString connectionName = m_database.connectionName();
    if (m_database.isOpen()) {
        m_database.close();
    }
    m_database = QSqlDatabase();
    
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }
    
    bool result = connectInternal(host, port, dbName, user, pass);
    
    if (result) {
        LOG_INFO("Database", "Database reconnected successfully");
    } else {
        LOG_ERROR("Database", QString("Database reconnection failed: %1").arg(m_lastError));
    }
    
    return result;
}

void DatabaseManager::setLastError(const QString& error)
{
    m_lastError = error;
}

bool DatabaseManager::fail(const QString& error)
{
    setLastError(error);
    LOG_ERROR("Database", error);
    return false;
}

void DatabaseManager::logError(const QString& operation, const QString& message)
{
    LOG_ERROR("Database", QString("%1: %2").arg(operation, message));
}

void DatabaseManager::logAndSetError(const QString& operation, const QString& message)
{
    setLastError(message);
    logError(operation, message);
}

bool DatabaseManager::ensureConnection(const QString& operation)
{
    if (!m_database.isOpen()) {
        LOG_WARNING("Database", QString("Database not open for: %1, attempting reconnect...").arg(operation));
        if (!reconnectIfNeeded()) {
            setLastError(QString("Database not connected for: %1").arg(operation));
            return false;
        }
    }
    
    {
        QSqlQuery testQuery(m_database);
        if (testQuery.exec("SELECT 1")) {
            return true;
        }
        
        QString errorMsg = testQuery.lastError().text();
        if (DatabaseUtils::isConnectionLostError(errorMsg)) {
            LOG_WARNING("Database", QString("Connection lost during: %1, reconnecting...").arg(operation));
            if (reconnectIfNeeded()) {
                return true;
            }
        }
        
        setLastError(QString("Database connection invalid: %1").arg(errorMsg));
        return false;
    }
}

bool DatabaseManager::checkConnection(const QString& operation)
{
    try {
        if (!m_database.isOpen()) {
            LOG_WARNING("Database", QString("Database not open for: %1, attempting reconnect...").arg(operation));
            if (reconnectIfNeeded()) {
                return true;
            }
            setLastError(QString("Database not connected for: %1").arg(operation));
            return false;
        }
        
        bool connectionValid = false;
        QString errorMsg;
        {
            QSqlQuery testQuery(m_database);
            connectionValid = testQuery.exec("SELECT 1");
            errorMsg = testQuery.lastError().text();
        }
        
        if (!connectionValid) {
            if (DatabaseUtils::isConnectionLostError(errorMsg)) {
                LOG_WARNING("Database", QString("Connection lost during: %1, reconnecting...").arg(operation));
                if (reconnectIfNeeded()) {
                    return true;
                }
            }
            
            setLastError(QString("Database connection invalid: %1").arg(errorMsg));
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        QString errorMsg = QString::fromUtf8(e.what());
        setLastError(errorMsg);
        LOG_ERROR("Database", QString("checkConnection exception for %1: %2").arg(operation, errorMsg));
        
        LOG_WARNING("Database", QString("Attempting reconnect after exception in: %1").arg(operation));
        if (reconnectIfNeeded()) {
            return true;
        }
        return false;
    } catch (...) {
        setLastError(QString("Unknown exception in checkConnection: %1").arg(operation));
        LOG_ERROR("Database", QString("checkConnection unknown exception for: %1").arg(operation));
        return false;
    }
}

bool DatabaseManager::commit()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_inTransaction) {
        setLastError("No active transaction to commit");
        return false;
    }
    
    try {
        QSqlQuery query(m_database);
        if (!query.exec("COMMIT")) {
            setLastError(QString("Failed to commit: %1").arg(query.lastError().text()));
            rollback();
            return false;
        }
        
        m_inTransaction = false;
        LOG_DEBUG("Database", "Transaction committed successfully");
        return true;
    } catch (const std::exception& e) {
        setLastError(QString("Commit exception: %1").arg(e.what()));
        m_inTransaction = false;
        return false;
    }
}

bool DatabaseManager::rollback()
{
    try {
        QSqlQuery query(m_database);
        if (!query.exec("ROLLBACK")) {
            setLastError(QString("Failed to rollback: %1").arg(query.lastError().text()));
            return false;
        }
        
        m_inTransaction = false;
        LOG_DEBUG("Database", "Transaction rolled back");
        return true;
    } catch (const std::exception& e) {
        setLastError(QString("Rollback exception: %1").arg(e.what()));
        m_inTransaction = false;
        return false;
    }
}

bool DatabaseManager::inTransaction() const
{
    return m_inTransaction;
}

bool DatabaseManager::transaction(TransactionOperation operation)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_inTransaction) {
        setLastError("Already in a transaction");
        return false;
    }
    
    if (!ensureConnection("transaction")) {
        return false;
    }
    
    QSqlQuery txQuery(m_database);
    if (!txQuery.exec("START TRANSACTION")) {
        setLastError(QString("Failed to start transaction: %1").arg(txQuery.lastError().text()));
        return false;
    }
    
    m_inTransaction = true;
    
    bool success = safeExecute<bool>("transaction", [&]() -> bool {
        return operation();
    }, false);
    
    if (success) {
        QSqlQuery commitQuery(m_database);
        if (!commitQuery.exec("COMMIT")) {
            setLastError(QString("Failed to commit transaction: %1").arg(commitQuery.lastError().text()));
            QSqlQuery rollbackQuery(m_database);
            rollbackQuery.exec("ROLLBACK");
            m_inTransaction = false;
            return false;
        }
        m_inTransaction = false;
        LOG_DEBUG("Database", "Transaction completed successfully");
        return true;
    }
    
    QSqlQuery rollbackQuery(m_database);
    rollbackQuery.exec("ROLLBACK");
    m_inTransaction = false;
    LOG_DEBUG("Database", "Transaction rolled back due to operation failure");
    return false;
}

void DatabaseManager::lock()
{
    m_mutex.lock();
}

void DatabaseManager::unlock()
{
    m_mutex.unlock();
}

bool DatabaseManager::tryLock(int timeoutMs)
{
    return m_mutex.tryLock(timeoutMs);
}

bool DatabaseManager::ensureSoftDeleteColumns(const QString& tableName)
{
    static QSet<QString> s_ensuredTables;
    
    if (s_ensuredTables.contains(tableName)) {
        return true;
    }
    
    auto hasColumn = [this, &tableName](const QString& columnName) -> bool {
        const QString sql = QString(
            "SELECT COUNT(*) AS cnt FROM information_schema.columns "
            "WHERE table_schema = DATABASE() AND table_name = '%1' AND column_name = '%2'")
            .arg(tableName, columnName);
        QList<QVariantMap> rows = executeQuery(sql);
        return !rows.isEmpty() && rows.first().value("cnt").toInt() > 0;
    };

    QStringList alters;
    if (!hasColumn("is_deleted")) {
        alters << "ADD COLUMN is_deleted TINYINT(1) NOT NULL DEFAULT 0";
    }
    if (!hasColumn("deleted_at")) {
        alters << "ADD COLUMN deleted_at TIMESTAMP NULL";
    }

    if (alters.isEmpty()) {
        s_ensuredTables.insert(tableName);
        return true;
    }

    bool success = executeSql(QString("ALTER TABLE %1 %2").arg(tableName, alters.join(", ")));
    if (success) {
        s_ensuredTables.insert(tableName);
    }
    return success;
}
