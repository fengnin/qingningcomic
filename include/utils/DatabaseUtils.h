#ifndef DATABASEUTILS_H
#define DATABASEUTILS_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QVariantMap>
#include <QString>

namespace DatabaseUtils {

// 初始化 MySQL 连接的字符集设置（utf8mb4）
inline bool initConnectionCharset(QSqlDatabase& db)
{
    QSqlQuery query(db);
    if (!query.exec("SET NAMES utf8mb4")) return false;
    if (!query.exec("SET CHARACTER SET utf8mb4")) return false;
    if (!query.exec("SET character_set_connection=utf8mb4")) return false;
    if (!query.exec("SET character_set_results=utf8mb4")) return false;
    if (!query.exec("SET character_set_client=utf8mb4")) return false;
    return true;
}

// 初始化 MySQL 连接的字符集设置（包含 autocommit=0）
inline bool initConnectionCharsetNoAutoCommit(QSqlDatabase& db)
{
    if (!initConnectionCharset(db)) return false;
    QSqlQuery query(db);
    if (!query.exec("SET autocommit = 0")) return false;
    return true;
}

// 提升会话超时，降低长时间空闲后被 MySQL 回收连接的概率
inline bool initConnectionSessionTimeouts(QSqlDatabase& db)
{
    QSqlQuery query(db);
    if (!query.exec("SET SESSION wait_timeout = 28800")) return false;
    if (!query.exec("SET SESSION interactive_timeout = 28800")) return false;
    if (!query.exec("SET SESSION net_read_timeout = 30")) return false;
    if (!query.exec("SET SESSION net_write_timeout = 30")) return false;
    return true;
}

// 将 QSqlQuery 当前记录转换为 QVariantMap
inline QVariantMap recordToMap(const QSqlQuery& query)
{
    QVariantMap result;
    QSqlRecord record = query.record();
    for (int i = 0; i < record.count(); ++i) {
        result[record.fieldName(i)] = query.value(i);
    }
    return result;
}

// 绑定值列表到 QSqlQuery
inline void bindValues(QSqlQuery& query, const QVariantList& values)
{
    for (const QVariant& value : values) {
        query.addBindValue(value);
    }
}

// 检查连接是否有效（执行 SELECT 1）
inline bool testConnection(QSqlDatabase& db)
{
    if (!db.isOpen()) return false;
    QSqlQuery query(db);
    return query.exec("SELECT 1");
}

// 获取连接错误信息
inline QString connectionErrorString(QSqlDatabase& db)
{
    return db.lastError().text();
}

// 检查错误是否表示连接丢失
inline bool isConnectionLostError(const QString& errorMsg)
{
    return errorMsg.contains("gone away", Qt::CaseInsensitive) ||
           errorMsg.contains("Lost connection", Qt::CaseInsensitive) ||
           errorMsg.contains("server has gone away", Qt::CaseInsensitive);
}

// MySQL 连接选项（带重连和超时）
inline QString defaultConnectionOptions()
{
    return "MYSQL_OPT_RECONNECT=1;MYSQL_OPT_CONNECT_TIMEOUT=10;MYSQL_OPT_READ_TIMEOUT=30;MYSQL_OPT_WRITE_TIMEOUT=30";
}

// MySQL 连接选项（仅重连）
inline QString simpleConnectionOptions()
{
    return "MYSQL_OPT_RECONNECT=1";
}

}

#endif // DATABASEUTILS_H
