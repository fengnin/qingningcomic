#include "data/DatabaseManager.h"
#include "services/ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "utils/DatabaseUtils.h"
#include "data/SqlBuilder.h"
#include "utils/Logger.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QSet>
#include <QThread>
#include <atomic>

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

    if (!DatabaseUtils::initConnectionCharset(m_database)) {
        LOG_WARNING("Database", "Failed to set charset, continuing anyway");
    }
    if (!DatabaseUtils::initConnectionSessionTimeouts(m_database)) {
        LOG_WARNING("Database", "Failed to set session timeouts, continuing anyway");
    }

    // Snapshot params so background threads can open their own connections.
    m_host   = host;
    m_port   = port;
    m_dbName = database;
    m_user   = username;
    m_pass   = password;

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
    if (QThread::currentThread() != thread()) {
        // For background threads, check the thread-local connection if it exists.
        if (m_threadConns.hasLocalData()) {
            return m_threadConns.localData()->db.isOpen();
        }
        // No thread-local connection yet — report based on main connection params being set.
        return !m_host.isEmpty();
    }

    QMutexLocker locker(&m_mutex);
    return m_database.isOpen();
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

static void initThreadConn(QSqlDatabase& db)
{
    DatabaseUtils::initConnectionCharset(db);
    DatabaseUtils::initConnectionSessionTimeouts(db);
}

QSqlDatabase& DatabaseManager::activeDb()
{
    if (QThread::currentThread() == thread()) {
        return m_database;
    }

    // Background thread: open a thread-local connection on first use.
    if (!m_threadConns.hasLocalData()) {
        ThreadConn* tc = new ThreadConn();
        tc->name = QStringLiteral("dm_thread_%1")
                       .arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16);

        if (QSqlDatabase::contains(tc->name)) {
            QSqlDatabase::removeDatabase(tc->name);
        }
        tc->db = QSqlDatabase::addDatabase("QMYSQL", tc->name);
        tc->db.setHostName(m_host);
        tc->db.setPort(m_port);
        tc->db.setDatabaseName(m_dbName);
        tc->db.setUserName(m_user);
        tc->db.setPassword(m_pass);
        tc->db.setConnectOptions(DatabaseUtils::defaultConnectionOptions());

        if (!tc->db.open()) {
            LOG_ERROR("Database", QString("Thread-local connection failed: %1").arg(tc->db.lastError().text()));
        } else {
            initThreadConn(tc->db);
        }
        m_threadConns.setLocalData(tc);
    }

    ThreadConn* tc = m_threadConns.localData();
    if (!tc->db.isOpen()) {
        if (tc->db.open()) {
            initThreadConn(tc->db);
        }
    }
    return tc->db;
}

QSqlQuery DatabaseManager::createQuery()
{
    try {
        return QSqlQuery(activeDb());
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
        
        QSqlQuery query(activeDb());
        query.prepare(sql);
        bindValues(query, values);

        bool result = executeQueryInternal(query, QString(), "update");

        if (result) {
            QSqlQuery commitQuery(activeDb());
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

    if (!ensureConnection("beginTransaction")) {
        return false;
    }

    return safeExecute<bool>("beginTransaction", [&]() -> bool {
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
    if (QThread::currentThread() != thread()) {
        // Reconnect the thread-local connection for this background thread.
        QSqlDatabase& bg = activeDb();
        if (bg.isOpen()) {
            QSqlQuery test(bg);
            if (test.exec("SELECT 1")) return true;
            bg.close();
        }
        if (bg.open()) {
            initThreadConn(bg);
            return true;
        }
        LOG_ERROR("Database", QString("Thread-local reconnect failed: %1").arg(bg.lastError().text()));
        return false;
    }

    // Main thread path (caller must hold m_mutex or be in a safe context).
    try {
        if (m_database.isOpen()) {
            QSqlQuery testQuery(m_database);
            if (testQuery.exec("SELECT 1")) {
                return true;
            }
            LOG_WARNING("Database", "Connection check failed, reconnecting...");
        }
    } catch (...) {
        LOG_WARNING("Database", "Connection check exception, will reconnect");
    }

    LOG_INFO("Database", "Reconnecting to database...");

    QString connectionName = m_database.connectionName();
    if (m_database.isOpen()) m_database.close();
    m_database = QSqlDatabase();
    if (QSqlDatabase::contains(connectionName)) QSqlDatabase::removeDatabase(connectionName);

    bool result = connectInternal(m_host, m_port, m_dbName, m_user, m_pass);
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
    QMutexLocker locker(&m_mutex);
    return ensureConnection(operation);
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

static bool runTransactionOnDb(QSqlDatabase& db, const DatabaseManager::TransactionOperation& operation,
                               QString& outError)
{
    QSqlQuery txQuery(db);
    if (!txQuery.exec("START TRANSACTION")) {
        outError = QString("Failed to start transaction: %1").arg(txQuery.lastError().text());
        return false;
    }
    bool success = false;
    try {
        success = operation();
    } catch (...) {
        success = false;
    }
    if (success) {
        QSqlQuery commitQuery(db);
        if (!commitQuery.exec("COMMIT")) {
            outError = QString("Failed to commit transaction: %1").arg(commitQuery.lastError().text());
            QSqlQuery(db).exec("ROLLBACK");
            return false;
        }
        return true;
    }
    QSqlQuery(db).exec("ROLLBACK");
    return false;
}

bool DatabaseManager::transaction(TransactionOperation operation)
{
    if (QThread::currentThread() != thread()) {
        QSqlDatabase& bg = activeDb();
        if (!bg.isValid() || !bg.isOpen()) {
            setLastError("transaction: thread-local connection unavailable");
            return false;
        }
        QString err;
        bool ok = runTransactionOnDb(bg, operation, err);
        if (!err.isEmpty()) setLastError(err);
        return ok;
    }

    QMutexLocker locker(&m_mutex);

    if (m_inTransaction) {
        setLastError("Already in a transaction");
        return false;
    }

    if (!ensureConnection("transaction")) {
        return false;
    }

    m_inTransaction = true;
    QString err;
    bool success = runTransactionOnDb(m_database, operation, err);
    m_inTransaction = false;
    if (!err.isEmpty()) setLastError(err);
    if (success) {
        LOG_DEBUG("Database", "Transaction completed successfully");
    } else {
        LOG_DEBUG("Database", "Transaction rolled back due to operation failure");
    }
    return success;
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

bool DatabaseManager::ensureCharacterPortraitVersionsSchema()
{
    static std::atomic<bool> s_ensured{false};
    if (s_ensured) {
        return true;
    }

    // This function may be called from a background thread. Use a dedicated
    // connection so we never touch m_database from a non-owning thread.
    const QString bgConn = QStringLiteral("schema_migration_connection");
    const QString host     = m_host;
    const int     port     = m_port;
    const QString dbName   = m_dbName;
    const QString user     = m_user;
    const QString password = m_pass;
    const QString connOpts = DatabaseUtils::defaultConnectionOptions();

    // Scope the QSqlDatabase object so it's destroyed before removeDatabase.
    bool ok = false;
    {
        QSqlDatabase bg = QSqlDatabase::addDatabase("QMYSQL", bgConn);
        bg.setHostName(host);
        bg.setPort(port);
        bg.setDatabaseName(dbName);
        bg.setUserName(user);
        bg.setPassword(password);
        bg.setConnectOptions(connOpts);

        if (!bg.open()) {
            LOG_ERROR("Database", QString("ensureCharacterPortraitVersionsSchema: failed to open bg connection: %1")
                .arg(bg.lastError().text()));
            QSqlDatabase::removeDatabase(bgConn);
            return false;
        }

        auto execSql = [&bg](const QString& sql) -> bool {
            QSqlQuery q(bg);
            if (q.exec(sql)) return true;
            // Retry once on error (connection may have timed out)
            if (!bg.open()) return false;
            QSqlQuery q2(bg);
            return q2.exec(sql);
        };

        auto hasColumn = [&bg](const QString& tableName, const QString& columnName) -> bool {
            const QString sql = QString(
                "SELECT COUNT(*) AS cnt FROM information_schema.columns "
                "WHERE table_schema = DATABASE() AND table_name = '%1' AND column_name = '%2'")
                .arg(tableName, columnName);
            QSqlQuery q(bg);
            if (!q.exec(sql) || !q.next()) return false;
            return q.value("cnt").toInt() > 0;
        };

        auto hasTable = [&bg](const QString& tableName) -> bool {
            const QString sql = QString(
                "SELECT COUNT(*) AS cnt FROM information_schema.tables "
                "WHERE table_schema = DATABASE() AND table_name = '%1'").arg(tableName);
            QSqlQuery q(bg);
            if (!q.exec(sql) || !q.next()) return false;
            return q.value("cnt").toInt() > 0;
        };

        if (!hasTable("character_portrait_versions")) {
            const QString createSql =
                "CREATE TABLE character_portrait_versions ("
                "id VARCHAR(36) PRIMARY KEY,"
                "character_id VARCHAR(36) NOT NULL,"
                "version_no INT NOT NULL,"
                "portrait_path VARCHAR(255) NOT NULL,"
                "base_version_id VARCHAR(36) NULL,"
                "edit_prompt TEXT NULL,"
                "field_diff JSON NULL,"
                "appearance_snapshot JSON NULL,"
                "source_chapter INT NULL,"
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                "INDEX idx_character_id (character_id),"
                "INDEX idx_character_version (character_id, version_no)"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
            if (!execSql(createSql)) {
                LOG_ERROR("Database", "ensureCharacterPortraitVersionsSchema: CREATE TABLE failed");
                bg.close();
                QSqlDatabase::removeDatabase(bgConn);
                return false;
            }
        }

        if (!hasColumn("characters", "current_portrait_version_id")) {
            if (!execSql("ALTER TABLE characters ADD COLUMN current_portrait_version_id VARCHAR(36) NULL")) {
                LOG_ERROR("Database", "ensureCharacterPortraitVersionsSchema: ALTER TABLE characters failed");
                bg.close();
                QSqlDatabase::removeDatabase(bgConn);
                return false;
            }
        }

        // appearance_snapshot: nullable, fallback exists — ignore duplicate-column error (1060)
        if (!hasColumn("character_portrait_versions", "appearance_snapshot")) {
            QSqlQuery q(bg);
            if (!q.exec("ALTER TABLE character_portrait_versions ADD COLUMN appearance_snapshot JSON NULL")) {
                const int errCode = q.lastError().nativeErrorCode().toInt();
                if (errCode != 1060) {
                    LOG_WARNING("Database", QString("ensureCharacterPortraitVersionsSchema: appearance_snapshot ALTER failed: %1")
                        .arg(q.lastError().text()));
                    // Non-fatal: column is nullable with fallback
                }
            } else {
                LOG_INFO("Database", "appearance_snapshot column added");
            }
        }

        bg.close();
        ok = true;
    }
    QSqlDatabase::removeDatabase(bgConn);

    if (ok) {
        s_ensured = true;
        LOG_INFO("Database", "character_portrait_versions schema ensured");
    }
    return ok;
}
