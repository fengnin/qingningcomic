#include "services/BaseService.h"
#include "DatabaseManager.h"
#include "Logger.h"
#include "EncodingUtils.h"

BaseService::BaseService(DatabaseManager* db, QObject* parent)
    : QObject(parent)
    , m_db(db ? db : DatabaseManager::instance())
{
}

bool BaseService::checkConnection(const QString& operation) const
{
    if (!m_db || !m_db->isConnected()) {
        LOG_ERROR("Service", QString("%1: %2").arg(operation, TR("数据库未连接")));
        return false;
    }
    return true;
}

void BaseService::emitError(const QString& operation, const QString& message)
{
    LOG_ERROR("Service", QString("%1: %2").arg(operation, message));
    emit errorOccurred(operation, message);
}

QString BaseService::serializeJson(const QJsonObject& json) const
{
    return QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

QString BaseService::serializeJson(const QVariantMap& data) const
{
    return QString::fromUtf8(QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
}

QJsonObject BaseService::parseJsonObject(const QString& str) const
{
    if (str.isEmpty()) {
        return QJsonObject();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());
    return doc.isNull() ? QJsonObject() : doc.object();
}

QVariantMap BaseService::parseJsonToMap(const QString& str) const
{
    if (str.isEmpty()) {
        return QVariantMap();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());
    return doc.isNull() ? QVariantMap() : doc.toVariant().toMap();
}

bool BaseService::safeDbBoolOperation(const QString& operation, std::function<bool()> dbOp)
{
    return safeDbOperation<bool>(operation, dbOp, false);
}
