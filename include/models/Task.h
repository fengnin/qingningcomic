#pragma once

#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QVariant>

enum class TaskType {
    GenerateStoryboard,
    GeneratePanels,
    GeneratePanelImage,
    ExportPdf,
    ExportWebtoon,
    ExportResources
};

enum class TaskStatus {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled
};

namespace TaskHelper {
    TaskType typeFromString(const QString& str);
    TaskStatus statusFromString(const QString& str);
    QString typeToString(TaskType type);
    QString statusToString(TaskStatus status);
}

struct TaskData {
    QString id;
    TaskType type = TaskType::GenerateStoryboard;
    TaskStatus status = TaskStatus::Pending;
    
    QString novelId;
    QString storyboardId;
    QString panelId;       // 面板 ID（用于图像生成任务）
    int chapterNumber = 1;
    QString text;
    QJsonObject params;
    
    int total = 0;
    int completed = 0;
    
    QString errorMessage;
    QJsonObject result;
    
    QDateTime createdAt;
    QDateTime startedAt;
    QDateTime completedAt;
    
    int retryCount = 0;
    int maxRetries = 3;
    
    QString typeString() const;
    QString statusString() const;
    int progress() const;
    bool canRetry() const;
    
    QJsonObject toJson() const;
    static TaskData fromJson(const QJsonObject& json);
    static TaskData fromDatabaseRow(const QVariantMap& row);
    QVariantMap toDatabaseRow() const;
    
    void resetForRetry();
    void markAsStarted();
    void markAsCompleted(const QJsonObject& resultData);
    void markAsFailed(const QString& errorMessage);
    void markAsCancelled();
};
