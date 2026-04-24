#include "models/Task.h"
#include "utils/JsonUtils.h"
#include <QJsonDocument>

namespace TaskHelper {

TaskType typeFromString(const QString& str)
{
    if (str == "GenerateStoryboard" || str == "generate_storyboard") {
        return TaskType::GenerateStoryboard;
    }
    if (str == "GeneratePanels" || str == "generate_panels") {
        return TaskType::GeneratePanels;
    }
    if (str == "GeneratePanelImage" || str == "generate_panel_image") {
        return TaskType::GeneratePanelImage;
    }
    if (str == "ExportPdf" || str == "export_pdf") {
        return TaskType::ExportPdf;
    }
    return TaskType::GenerateStoryboard;
}

TaskStatus statusFromString(const QString& str)
{
    if (str == "Pending" || str == "pending") {
        return TaskStatus::Pending;
    }
    if (str == "Running" || str == "running") {
        return TaskStatus::Running;
    }
    if (str == "Completed" || str == "completed") {
        return TaskStatus::Completed;
    }
    if (str == "Failed" || str == "failed") {
        return TaskStatus::Failed;
    }
    if (str == "Cancelled" || str == "cancelled") {
        return TaskStatus::Cancelled;
    }
    return TaskStatus::Pending;
}

QString typeToString(TaskType type)
{
    switch (type) {
        case TaskType::GenerateStoryboard: return "GenerateStoryboard";
        case TaskType::GeneratePanels: return "GeneratePanels";
        case TaskType::GeneratePanelImage: return "GeneratePanelImage";
        case TaskType::ExportPdf: return "ExportPdf";
    }
    return "GenerateStoryboard";
}

QString statusToString(TaskStatus status)
{
    switch (status) {
        case TaskStatus::Pending: return "Pending";
        case TaskStatus::Running: return "Running";
        case TaskStatus::Completed: return "Completed";
        case TaskStatus::Failed: return "Failed";
        case TaskStatus::Cancelled: return "Cancelled";
    }
    return "Pending";
}

}

namespace {

QString toCompactJson(const QJsonObject& object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QDateTime readDateTime(const QJsonObject& json, const QString& key)
{
    return QDateTime::fromString(json.value(key).toString(), Qt::ISODate);
}

void writeDateTime(QJsonObject& json, const QString& key, const QDateTime& value)
{
    if (value.isValid()) {
        json[key] = value.toString(Qt::ISODate);
    }
}

} // namespace

QString TaskData::typeString() const
{
    return TaskHelper::typeToString(type);
}

QString TaskData::statusString() const
{
    return TaskHelper::statusToString(status);
}

int TaskData::progress() const
{
    if (total <= 0) return 0;
    return qRound(static_cast<double>(completed) / total * 100);
}

bool TaskData::canRetry() const
{
    return retryCount < maxRetries && status != TaskStatus::Cancelled;
}

QJsonObject TaskData::toJson() const
{
    QJsonObject json;
    json["id"] = id;
    json["type"] = typeString();
    json["status"] = statusString();
    json["novelId"] = novelId;
    json["storyboardId"] = storyboardId;
    json["panelId"] = panelId;
    json["chapterNumber"] = chapterNumber;
    json["text"] = text;
    json["params"] = params;
    json["total"] = total;
    json["completed"] = completed;
    json["errorMessage"] = errorMessage;
    json["result"] = result;
    json["retryCount"] = retryCount;
    json["maxRetries"] = maxRetries;
    writeDateTime(json, QStringLiteral("createdAt"), createdAt);
    writeDateTime(json, QStringLiteral("startedAt"), startedAt);
    writeDateTime(json, QStringLiteral("completedAt"), completedAt);
    
    return json;
}

TaskData TaskData::fromJson(const QJsonObject& json)
{
    TaskData task;
    task.id = json["id"].toString();
    task.type = TaskHelper::typeFromString(json["type"].toString());
    task.status = TaskHelper::statusFromString(json["status"].toString());
    task.novelId = json["novelId"].toString();
    task.storyboardId = json["storyboardId"].toString();
    task.panelId = json["panelId"].toString();
    task.chapterNumber = json["chapterNumber"].toInt(1);
    task.text = json["text"].toString();
    task.params = json["params"].toObject();
    task.total = json["total"].toInt();
    task.completed = json["completed"].toInt();
    task.errorMessage = json["errorMessage"].toString();
    task.result = json["result"].toObject();
    task.retryCount = json["retryCount"].toInt();
    task.maxRetries = json["maxRetries"].toInt(3);
    
    task.createdAt = readDateTime(json, QStringLiteral("createdAt"));
    task.startedAt = readDateTime(json, QStringLiteral("startedAt"));
    task.completedAt = readDateTime(json, QStringLiteral("completedAt"));
    
    return task;
}

TaskData TaskData::fromDatabaseRow(const QVariantMap& row)
{
    TaskData task;
    task.id = row["id"].toString();
    task.novelId = row["novel_id"].toString();
    task.storyboardId = row["storyboard_id"].toString();
    task.panelId = row["panel_id"].toString();
    task.total = row["total_tasks"].toInt();
    task.completed = row["completed_tasks"].toInt();
    task.errorMessage = row["error_message"].toString();
    
    if (row.contains("result") && !row["result"].isNull()) {
        QJsonDocument resultDoc = QJsonDocument::fromJson(row["result"].toString().toUtf8());
        if (!resultDoc.isNull() && resultDoc.isObject()) {
            task.result = resultDoc.object();
        }
    }
    
    task.type = TaskHelper::typeFromString(row["type"].toString());
    task.status = TaskHelper::statusFromString(row["status"].toString());
    
    if (row.contains("params") && !row["params"].isNull()) {
        QJsonDocument paramsDoc = QJsonDocument::fromJson(row["params"].toString().toUtf8());
        if (!paramsDoc.isNull() && paramsDoc.isObject()) {
            task.params = paramsDoc.object();
            task.chapterNumber = task.params["chapterNumber"].toInt();
        }
    }
    
    return task;
}

QVariantMap TaskData::toDatabaseRow() const
{
    QVariantMap data;
    data["id"] = id;
    data["type"] = typeString();
    data["status"] = statusString();
    data["novel_id"] = novelId;
    data["storyboard_id"] = storyboardId;
    data["total_tasks"] = total;
    data["completed_tasks"] = completed;
    data["progress"] = progress();
    data["error_message"] = errorMessage;
    
    QJsonObject paramsObj = params;
    paramsObj["chapterNumber"] = chapterNumber;
    data["params"] = toCompactJson(paramsObj);
    
    if (!result.isEmpty()) {
        data["result"] = toCompactJson(result);
    } else {
        data["result"] = QVariant();
    }
    
    return data;
}

void TaskData::resetForRetry()
{
    status = TaskStatus::Pending;
    retryCount++;
    errorMessage.clear();
    startedAt = QDateTime();
    completedAt = QDateTime();
}

void TaskData::markAsStarted()
{
    status = TaskStatus::Running;
    startedAt = QDateTime::currentDateTime();
}

void TaskData::markAsCompleted(const QJsonObject& resultData)
{
    status = TaskStatus::Completed;
    completedAt = QDateTime::currentDateTime();
    result = resultData;
}

void TaskData::markAsFailed(const QString& error)
{
    status = TaskStatus::Failed;
    completedAt = QDateTime::currentDateTime();
    errorMessage = error;
}

void TaskData::markAsCancelled()
{
    status = TaskStatus::Cancelled;
    completedAt = QDateTime::currentDateTime();
}
