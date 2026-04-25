#pragma once

#include "models/Task.h"
#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <functional>

class TaskWorker;

class TaskQueue : public QObject
{
    Q_OBJECT
    friend class TaskWorker;

public:
    using TaskHandler = std::function<void(TaskData&)>;
    using TaskPredicate = std::function<bool(const TaskData&)>;
    
    static TaskQueue* instance();
    
    void start();
    void stop();
    
    QString enqueue(const TaskData& task);
    void cancel(const QString& taskId);
    void retry(const QString& taskId);
    
    TaskData getTask(const QString& taskId) const;
    QList<TaskData> getPendingTasks() const;
    QList<TaskData> getAllTasks() const;
    bool hasRunningTasks() const;
    
    void setMaxConcurrent(int max);
    int maxConcurrent() const;
    
    void registerHandler(TaskType type, TaskHandler handler);

signals:
    void taskEnqueued(const QString& taskId);
    void taskStarted(const QString& taskId);
    void taskProgress(const QString& taskId, int progress, const QString& message);
    void taskCompleted(const QString& taskId, const QJsonObject& result);
    void taskFailed(const QString& taskId, const QString& error);
    void taskCancelled(const QString& taskId);

private slots:
    void onTaskProgress(const QString& taskId, int progress, const QString& message);

private:
    TaskQueue();
    ~TaskQueue();

    void saveTaskToDatabase(const TaskData& task);
    void loadTasksFromDatabase();
    TaskData* getTaskRef(const QString& taskId);

    static TaskQueue* m_instance;
    static QMutex m_instanceMutex;
    
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    QQueue<QString> m_queue;
    QMap<QString, TaskData> m_tasks;
    QMap<TaskType, TaskHandler> m_handlers;
    QList<TaskWorker*> m_workers;
    int m_maxConcurrent = 1;
    bool m_running = false;
};

class TaskWorker : public QThread
{
    Q_OBJECT

public:
    explicit TaskWorker(TaskQueue* queue, QObject* parent = nullptr);
    ~TaskWorker() = default;

    void stop();

signals:
    void taskStarted(const QString& taskId);
    void taskProgress(const QString& taskId, int progress, const QString& message);
    void taskCompleted(const QString& taskId, const QJsonObject& result);
    void taskFailed(const QString& taskId, const QString& error);

protected:
    void run() override;

private:
    enum class DequeueStatus {
        Dequeued,
        NoTaskAvailable,
        Stopped
    };

    struct TaskResult {
        bool success = false;
        QJsonObject result;
        QString errorMessage;
    };
    
    DequeueStatus tryDequeueTask(QString& outTaskId, TaskData& outTask);
    TaskResult executeTask(TaskData& task);
    void finalizeTask(const QString& taskId, const TaskResult& result);
    
    TaskQueue* m_queue;
    QMutex m_workerMutex;
    bool m_running = false;
};
