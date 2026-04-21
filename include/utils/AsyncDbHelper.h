#ifndef ASYNCDBHELPER_H
#define ASYNCDBHELPER_H

#include "data/DatabaseManager.h"
#include "utils/DatabaseUtils.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QString>
#include <QUuid>

namespace AsyncDbHelper {

struct DatabaseConnectionInfo
{
    QString host;
    int port = 3306;
    QString database;
    QString username;
    QString password;
};

inline DatabaseConnectionInfo snapshotConnection()
{
    DatabaseConnectionInfo info;
    const QSqlDatabase db = DatabaseManager::instance()->database();
    info.host = db.hostName();
    info.port = db.port();
    info.database = db.databaseName();
    info.username = db.userName();
    info.password = db.password();
    return info;
}

inline QString makeConnectionName(const QString& prefix)
{
    return QStringLiteral("%1_%2")
        .arg(prefix, QUuid::createUuid().toString(QUuid::WithoutBraces));
}

inline bool openTemporaryConnection(QSqlDatabase& db, const DatabaseConnectionInfo& info,
                                   const QString& connectionName, QString* error)
{
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    db = QSqlDatabase::addDatabase("QMYSQL", connectionName);
    db.setHostName(info.host);
    db.setPort(info.port);
    db.setDatabaseName(info.database);
    db.setUserName(info.username);
    db.setPassword(info.password);
    db.setConnectOptions(DatabaseUtils::defaultConnectionOptions());

    if (!db.open()) {
        if (error) {
            *error = db.lastError().text();
        }
        return false;
    }

    DatabaseUtils::initConnectionCharset(db);
    DatabaseUtils::initConnectionSessionTimeouts(db);
    return true;
}

inline void closeTemporaryConnection(QSqlDatabase& db, const QString& connectionName)
{
    if (db.isOpen()) {
        db.close();
    }

    db = QSqlDatabase();
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

} // namespace AsyncDbHelper

#endif // ASYNCDBHELPER_H
