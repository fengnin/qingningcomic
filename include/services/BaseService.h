#ifndef BASESERVICE_H
#define BASESERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>
#include <QString>
#include <functional>

class DatabaseManager;

class BaseService : public QObject
{
    Q_OBJECT

public:
    explicit BaseService(DatabaseManager* db, QObject* parent = nullptr);
    virtual ~BaseService() = default;

signals:
    void errorOccurred(const QString& operation, const QString& message);

protected:
    bool checkConnection(const QString& operation) const;
    void emitError(const QString& operation, const QString& message);
    
    QString serializeJson(const QJsonObject& json) const;
    QString serializeJson(const QVariantMap& data) const;
    QJsonObject parseJsonObject(const QString& str) const;
    QVariantMap parseJsonToMap(const QString& str) const;
    
    template<typename T>
    T safeDbOperation(const QString& operation, std::function<T()> dbOp, T defaultValue);
    
    bool safeDbBoolOperation(const QString& operation, std::function<bool()> dbOp);
    
    DatabaseManager* m_db;
};

template<typename T>
T BaseService::safeDbOperation(const QString& operation, std::function<T()> dbOp, T defaultValue)
{
    if (!checkConnection(operation)) {
        return defaultValue;
    }
    
    try {
        return dbOp();
    } catch (const std::exception& e) {
        emitError(operation, QString::fromUtf8(e.what()));
        return defaultValue;
    } catch (...) {
        emitError(operation, QString("Unknown exception in %1").arg(operation));
        return defaultValue;
    }
}

#endif // BASESERVICE_H
