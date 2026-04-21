#include "models/ChangeRequest.h"
#include <QDateTime>

ChangeRequest::ChangeRequest()
{
}

QJsonObject ChangeRequest::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["novelId"] = m_novelId;
    json["storyboardId"] = m_storyboardId;
    json["naturalLanguage"] = m_naturalLanguage;
    json["status"] = m_status;
    json["userId"] = m_userId;
    json["jobId"] = m_jobId;
    json["createdAt"] = m_createdAt;
    json["updatedAt"] = m_updatedAt;
    
    if (!m_errorMessage.isEmpty()) {
        json["error"] = m_errorMessage;
    }
    
    if (m_dsl.isValid()) {
        json["dsl"] = m_dsl.toJson();
    }
    
    if (!m_context.isEmpty()) {
        json["context"] = m_context;
    }
    
    if (!m_results.isEmpty()) {
        QJsonArray resultsArray;
        for (const auto& result : m_results) {
            resultsArray.append(result.toJson());
        }
        json["results"] = resultsArray;
    }
    
    return json;
}

ChangeRequest ChangeRequest::fromJson(const QJsonObject& json)
{
    ChangeRequest cr;
    cr.m_id = json["id"].toString();
    cr.m_novelId = json["novelId"].toString();
    cr.m_storyboardId = json["storyboardId"].toString();
    cr.m_naturalLanguage = json["naturalLanguage"].toString();
    cr.m_status = json["status"].toString("queued");
    cr.m_userId = json["userId"].toString();
    cr.m_jobId = json["jobId"].toString();
    cr.m_createdAt = json["createdAt"].toString();
    cr.m_updatedAt = json["updatedAt"].toString();
    cr.m_errorMessage = json["error"].toString();
    cr.m_context = json["context"].toObject();
    
    if (json.contains("dsl")) {
        cr.m_dsl = ChangeRequestDsl::fromJson(json["dsl"].toObject());
    }
    
    QJsonArray resultsArray = json["results"].toArray();
    for (const auto& result : resultsArray) {
        ChangeRequestOpResult opResult;
        opResult.action = result.toObject()["action"].toString();
        opResult.status = result.toObject()["status"].toString();
        opResult.output = result.toObject()["output"].toObject();
        opResult.errorMessage = result.toObject()["error"].toString();
        cr.m_results.append(opResult);
    }
    
    return cr;
}

QString ChangeRequest::statusToString(ChangeRequestStatus status)
{
    switch (status) {
        case ChangeRequestStatus::Queued: return "queued";
        case ChangeRequestStatus::Parsing: return "parsing";
        case ChangeRequestStatus::Pending: return "pending";
        case ChangeRequestStatus::InProgress: return "in_progress";
        case ChangeRequestStatus::Completed: return "completed";
        case ChangeRequestStatus::Failed: return "failed";
        default: return "queued";
    }
}

ChangeRequestStatus ChangeRequest::stringToStatus(const QString& status)
{
    if (status == "queued") return ChangeRequestStatus::Queued;
    if (status == "parsing") return ChangeRequestStatus::Parsing;
    if (status == "pending") return ChangeRequestStatus::Pending;
    if (status == "in_progress") return ChangeRequestStatus::InProgress;
    if (status == "completed") return ChangeRequestStatus::Completed;
    if (status == "failed") return ChangeRequestStatus::Failed;
    return ChangeRequestStatus::Queued;
}
