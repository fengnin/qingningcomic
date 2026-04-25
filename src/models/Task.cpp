#include "models/Task.h"
#include "utils/JsonUtils.h"
#include <QJsonDocument>
#include <initializer_list>

namespace {

struct TaskTypeEntry {
    TaskType type;
    const char* canonicalName;
    std::initializer_list<const char*> aliases;
};

const TaskTypeEntry kTaskTypeEntries[] = {
    {TaskType::GenerateStoryboard, "GenerateStoryboard", {"generatestoryboard", "generate_storyboard"}},
    {TaskType::GeneratePanels, "GeneratePanels", {"generatepanels", "generate_panels"}},
    {TaskType::GeneratePanelImage, "GeneratePanelImage", {"generatepanelimage", "generate_panel_image"}},
    {TaskType::ExportPdf, "ExportPdf", {"exportpdf", "export_pdf"}}
};

QString normalizeTaskToken(const QString& value)
{
    return value.trimmed().toLower();
}

bool matchesAny(const QString& value, std::initializer_list<const char*> candidates)
{
    const QString normalized = normalizeTaskToken(value);
    for (const char* candidate : candidates) {
        if (normalized == QString::fromLatin1(candidate)) {
            return true;
        }
    }
    return false;
}

bool matchesTaskTypeEntry(const QString& normalized, const TaskTypeEntry& entry)
{
    if (normalized == QString::fromLatin1(entry.canonicalName).toLower()) {
        return true;
    }
    for (const char* alias : entry.aliases) {
        if (normalized == QString::fromLatin1(alias)) {
            return true;
        }
    }
    return false;
}

QJsonObject parseJsonObject(const QVariant& value)
{
    return JsonUtils::toObject(value);
}

void loadTaskCommonFromJson(TaskData& task, const QJsonObject& json)
{
    task.id = JsonUtils::get<QString>(json, "id");
    task.type = TaskHelper::typeFromString(JsonUtils::get<QString>(json, "type"));
    task.status = TaskHelper::statusFromString(JsonUtils::get<QString>(json, "status"));
    task.novelId = JsonUtils::get<QString>(json, "novelId");
    task.storyboardId = JsonUtils::get<QString>(json, "storyboardId");
    task.panelId = JsonUtils::get<QString>(json, "panelId");
    task.chapterNumber = JsonUtils::get<int>(json, "chapterNumber", 1);
    task.text = JsonUtils::get<QString>(json, "text");
    task.params = JsonUtils::getObject(json, "params");
    task.total = JsonUtils::get<int>(json, "total");
    task.completed = JsonUtils::get<int>(json, "completed");
    task.errorMessage = JsonUtils::get<QString>(json, "errorMessage");
    task.result = JsonUtils::getObject(json, "result");
    task.retryCount = JsonUtils::get<int>(json, "retryCount");
    task.maxRetries = JsonUtils::get<int>(json, "maxRetries", 3);
    task.createdAt = JsonUtils::getDateTime(json, "createdAt");
    task.startedAt = JsonUtils::getDateTime(json, "startedAt");
    task.completedAt = JsonUtils::getDateTime(json, "completedAt");
}

void loadTaskCommonFromDatabase(TaskData& task, const QVariantMap& row)
{
    task.id = row.value("id").toString();
    task.novelId = row.value("novel_id").toString();
    task.storyboardId = row.value("storyboard_id").toString();
    task.panelId = row.value("panel_id").toString();
    task.total = row.value("total_tasks").toInt();
    task.completed = row.value("completed_tasks").toInt();
    task.errorMessage = row.value("error_message").toString();
    task.result = parseJsonObject(row.value("result"));
    task.type = TaskHelper::typeFromString(row.value("type").toString());
    task.status = TaskHelper::statusFromString(row.value("status").toString());
    task.params = parseJsonObject(row.value("params"));
    if (task.params.contains("chapterNumber")) {
        task.chapterNumber = task.params.value("chapterNumber").toInt(1);
    }
}

void writeTaskDates(QJsonObject& json, const TaskData& task)
{
    JsonUtils::set(json, QStringLiteral("createdAt"), task.createdAt);
    JsonUtils::set(json, QStringLiteral("startedAt"), task.startedAt);
    JsonUtils::set(json, QStringLiteral("completedAt"), task.completedAt);
}

} // namespace

namespace TaskHelper {

TaskType typeFromString(const QString& str)
{
    const QString normalized = normalizeTaskToken(str);
    for (const TaskTypeEntry& entry : kTaskTypeEntries) {
        if (matchesTaskTypeEntry(normalized, entry)) {
            return entry.type;
        }
    }
    return TaskType::GenerateStoryboard;
}

TaskStatus statusFromString(const QString& str)
{
    if (matchesAny(str, {"pending"})) {
        return TaskStatus::Pending;
    }
    if (matchesAny(str, {"running"})) {
        return TaskStatus::Running;
    }
    if (matchesAny(str, {"completed"})) {
        return TaskStatus::Completed;
    }
    if (matchesAny(str, {"failed"})) {
        return TaskStatus::Failed;
    }
    if (matchesAny(str, {"cancelled"})) {
        return TaskStatus::Cancelled;
    }
    return TaskStatus::Pending;
}

QString typeToString(TaskType type)
{
    for (const TaskTypeEntry& entry : kTaskTypeEntries) {
        if (entry.type == type) {
            return QString::fromLatin1(entry.canonicalName);
        }
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

} // namespace TaskHelper

namespace {

QString toCompactJson(const QJsonObject& object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
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
    writeTaskDates(json, *this);
    
    return json;
}

TaskData TaskData::fromJson(const QJsonObject& json)
{
    TaskData task;
    loadTaskCommonFromJson(task, json);
    
    return task;
}

TaskData TaskData::fromDatabaseRow(const QVariantMap& row)
{
    TaskData task;
    loadTaskCommonFromDatabase(task, row);
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
