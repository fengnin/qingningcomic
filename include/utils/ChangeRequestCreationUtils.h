#ifndef CHANGEREQUESTCREATIONUTILS_H
#define CHANGEREQUESTCREATIONUTILS_H

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>
#include "models/ChangeRequest.h"

namespace ChangeRequestCreationUtils {

inline QString currentUtcTimestamp()
{
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss");
}

inline QString serializeJsonObject(const QJsonObject& object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

inline ChangeRequest buildDraft(
    const QString& requestId,
    const QString& novelId,
    const QString& userId,
    const QString& naturalLanguage,
    const QJsonObject& context,
    const QString& timestamp)
{
    ChangeRequest cr;
    cr.setId(requestId);
    cr.setNovelId(novelId);
    cr.setUserId(userId);
    cr.setNaturalLanguage(naturalLanguage);
    cr.setContext(context);
    cr.setStatus("queued");
    cr.setCreatedAt(timestamp);
    cr.setUpdatedAt(timestamp);
    return cr;
}

inline QVariantMap buildInsertData(const ChangeRequest& cr)
{
    QVariantMap data;
    data["id"] = cr.id();
    data["novel_id"] = cr.novelId();
    data["storyboard_id"] = cr.storyboardId();
    data["natural_language"] = cr.naturalLanguage();
    data["status"] = cr.status();
    data["user_id"] = cr.userId();
    data["job_id"] = cr.jobId();
    data["context"] = serializeJsonObject(cr.context());
    data["dsl"] = QVariant();
    data["error_message"] = QVariant();
    data["created_at"] = cr.createdAt();
    data["updated_at"] = cr.updatedAt();
    return data;
}

}

#endif // CHANGEREQUESTCREATIONUTILS_H
