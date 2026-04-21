#ifndef ANALYSISJOBUTILS_H
#define ANALYSISJOBUTILS_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>
#include <QDateTime>
#include <QUuid>
#include "data/DatabaseManager.h"
#include "utils/Logger.h"

namespace AnalysisJobUtils {

inline QString createJob(const QString& novelId, int chapterNumber)
{
    QString jobId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QJsonObject params;
    params["chapterNumber"] = chapterNumber;
    QString paramsJson = QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Compact));
    
    QVariantMap data;
    data["id"] = jobId;
    data["type"] = "generate_storyboard";
    data["status"] = "running";
    data["novel_id"] = novelId;
    data["params"] = paramsJson;
    data["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    data["updated_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (DatabaseManager::instance()->insert("jobs", data) < 0) {
        LOG_ERROR("AnalysisJobUtils", "Failed to create job");
        return QString();
    }
    
    LOG_INFO("AnalysisJobUtils", QString("Created job: %1 for novel %2, chapter %3")
        .arg(jobId, novelId).arg(chapterNumber));
    
    return jobId;
}

inline void updateJobStatus(const QString& jobId, const QString& status, const QString& errorMessage = QString())
{
    if (jobId.isEmpty()) {
        LOG_WARNING("AnalysisJobUtils", "updateJobStatus: jobId is empty");
        return;
    }
    
    QVariantMap data;
    data["status"] = status;
    data["updated_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    if (status == "completed") {
        data["progress"] = 100;
    } else if (status == "failed") {
        data["progress"] = 0;
    }
    
    if (!errorMessage.isEmpty()) {
        data["error_message"] = errorMessage;
    }
    
    QString where = "id = ?";
    QVariantList values;
    values << jobId;
    
    DatabaseManager::instance()->update("jobs", data, where, values);
    
    LOG_INFO("AnalysisJobUtils", QString("Updated job %1 status to: %2, progress: %3")
        .arg(jobId, status).arg(data.value("progress", -1).toInt()));
}

inline void updateJobProgress(const QString& jobId, int progress)
{
    if (jobId.isEmpty()) {
        return;
    }
    
    QVariantMap data;
    data["progress"] = progress;
    DatabaseManager::instance()->update("jobs", data, "id = ?", QVariantList{jobId});
}

}

namespace AnalysisStateUtils {

inline void setResultSuccess(AnalysisResult& result, int chapterNumber, int panelCount, int characterCount, int sceneCount)
{
    result.success = true;
    result.chapterNumber = chapterNumber;
    result.panelCount = panelCount;
    result.characterCount = characterCount;
    result.sceneCount = sceneCount;
}

inline void setResultFailure(AnalysisResult& result, const QString& errorMessage)
{
    result.success = false;
    result.errorMessage = errorMessage;
}

}

#endif // ANALYSISJOBUTILS_H
