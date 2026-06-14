#include "services/TaskQueue.h"
#include "data/DatabaseManager.h"
#include "utils/Logger.h"
#include <QUuid>
#include <mutex>

namespace {
constexpr const char* kJobsTable = "jobs";

bool isTransientDatabaseError(const QString& error)
{
    return error.contains("gone away") || error.contains("server has gone away");
}

bool upsertTaskRow(DatabaseManager* db, const TaskData& task)
{
    if (!db) {
        return false;
    }

    const QString where = QStringLiteral("id = ?");
    const QVariantList values{task.id};
    const QVariantMap data = task.toDatabaseRow();
    const QVariantMap existing = db->selectOne(kJobsTable, where, values);

    if (!existing.isEmpty()) {
        return db->update(kJobsTable, data, where, values);
    }

    return db->insert(kJobsTable, data);
}

bool writeTaskRow(DatabaseManager* db, const TaskData& task)
{
    return upsertTaskRow(db, task);
}

bool updateTaskProgressRow(DatabaseManager* db, const QString& taskId, int progress)
{
    if (!db) {
        return false;
    }

    QVariantMap data;
    data["progress"] = progress;

    const QString where = QStringLiteral("id = ?");
    const QVariantList values{taskId};
    return db->update(kJobsTable, data, where, values);
}

void markInterruptedTaskAsFailed(const QString& taskId)
{
    DatabaseManager* db = DatabaseManager::instance();
    if (!db) {
        return;
    }

    QVariantMap updateData;
    updateData["status"] = "failed";
    updateData["error_message"] = "Task interrupted by application restart";
    db->update(kJobsTable, updateData, QStringLiteral("id = ?"), QVariantList{taskId});
}

bool isFailedTaskResult(const QJsonObject& result)
{
    return result.value(QStringLiteral("status")).toString().trimmed().toLower() == QStringLiteral("failed");
}

QString taskResultErrorMessage(const QJsonObject& result)
{
    QString message = result.value(QStringLiteral("errorMessage")).toString().trimmed();
    if (message.isEmpty()) {
        message = result.value(QStringLiteral("message")).toString().trimmed();
    }
    return message.isEmpty() ? QStringLiteral("Task handler reported failure") : message;
}

} // namespace

TaskQueue* TaskQueue::m_instance = nullptr;
std::once_flag TaskQueue::m_instanceOnceFlag;

TaskQueue::TaskQueue()
{
}

TaskQueue::~TaskQueue()
{
    stop();
}

TaskQueue* TaskQueue::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new TaskQueue();
    });
    return m_instance;
}

void TaskQueue::start()
{
    QMutexLocker locker(&m_mutex);
    if (m_running) return;

    m_running = true;
    loadTasksFromDatabase();
    
    disconnect(this, &TaskQueue::taskProgress, this, &TaskQueue::onTaskProgress);
    connect(this, &TaskQueue::taskProgress, this, &TaskQueue::onTaskProgress, Qt::QueuedConnection);
    
    for (int i = 0; i < m_maxConcurrent; ++i) {
        TaskWorker* worker = createWorker();
        m_workers.append(worker);
        worker->start();
    }
    
    LOG_INFO("TaskQueue", QString("Started with %1 workers (signal-driven)").arg(m_maxConcurrent));
}

void TaskQueue::stop()
{
    QMutexLocker locker(&m_mutex);
    if (!m_running) return;

    m_running = false;

    m_condition.wakeAll();

    for (TaskWorker* worker : m_workers) {
        stopWorker(worker);
    }
    m_workers.clear();
    
    LOG_INFO("TaskQueue", "Stopped");
}

QString TaskQueue::enqueue(const TaskData& task)
{
    QMutexLocker locker(&m_mutex);

    TaskData newTask = task;
    if (newTask.id.isEmpty()) {
        newTask.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    newTask.status = TaskStatus::Pending;
    newTask.createdAt = QDateTime::currentDateTime();
    
    m_tasks[newTask.id] = newTask;
    m_queue.enqueue(newTask.id);
    
    saveTaskToDatabase(newTask);
    
    LOG_INFO("TaskQueue", QString("Task enqueued: %1 (%2)").arg(newTask.id, newTask.typeString()));
    
    m_condition.wakeOne();
    
    emit taskEnqueued(newTask.id);

    return newTask.id;
}

void TaskQueue::cancel(const QString& taskId)
{
    QMutexLocker locker(&m_mutex);

    TaskData* task = getTaskRef(taskId);
    if (!task) return;
    
    if (task->status == TaskStatus::Running) {
        LOG_WARNING("TaskQueue", QString("Cannot cancel running task: %1").arg(taskId));
        return;
    }
    
    task->markAsCancelled();
    m_queue.removeAll(taskId);
    saveTaskToDatabase(*task);
    
    LOG_INFO("TaskQueue", QString("Task cancelled: %1").arg(taskId));
    emit taskCancelled(taskId);
}

void TaskQueue::retry(const QString& taskId)
{
    QMutexLocker locker(&m_mutex);

    TaskData* task = getTaskRef(taskId);
    if (!task) return;
    
    if (!task->canRetry()) {
        LOG_WARNING("TaskQueue", QString("Task cannot be retried: %1").arg(taskId));
        return;
    }
    
    task->resetForRetry();
    m_queue.enqueue(taskId);
    saveTaskToDatabase(*task);
    
    m_condition.wakeOne();

    LOG_INFO("TaskQueue", QString("Task retry #%1: %2").arg(task->retryCount).arg(taskId));
}

TaskData TaskQueue::getTask(const QString& taskId) const
{
    QMutexLocker locker(&m_mutex);
    return m_tasks.value(taskId);
}

QList<TaskData> TaskQueue::getPendingTasks() const
{
    QMutexLocker locker(&m_mutex);
    QList<TaskData> tasks;
    for (const TaskData& task : m_tasks) {
        if (task.status == TaskStatus::Pending) {
            tasks.append(task);
        }
    }
    return tasks;
}

QList<TaskData> TaskQueue::getAllTasks() const
{
    QMutexLocker locker(&m_mutex);
    return m_tasks.values();
}

bool TaskQueue::hasRunningTasks() const
{
    QMutexLocker locker(&m_mutex);
    for (const TaskData& task : m_tasks) {
        if (task.status == TaskStatus::Running) {
            return true;
        }
    }
    return false;
}

void TaskQueue::setMaxConcurrent(int max)
{
    QMutexLocker locker(&m_mutex);
    m_maxConcurrent = qMax(1, max);
}

int TaskQueue::maxConcurrent() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxConcurrent;
}

void TaskQueue::registerHandler(TaskType type, TaskHandler handler)
{
    QMutexLocker locker(&m_mutex);
    m_handlers[type] = handler;
}

void TaskQueue::unregisterHandler(TaskType type)
{
    QMutexLocker locker(&m_mutex);
    m_handlers.remove(type);
}

void TaskQueue::onTaskProgress(const QString& taskId, int progress, const QString& message)
{
    Q_UNUSED(message);

    DatabaseManager *db = DatabaseManager::instance();
    if (!db) {
        return;
    }

    if (!updateTaskProgressRow(db, taskId, progress)) {
        const QString error = db->lastError();
        if (!db->isConnected() || isTransientDatabaseError(error)) {
            if (db->reconnectIfNeeded()) {
                updateTaskProgressRow(db, taskId, progress);
            }
        }
    }
}

void TaskQueue::saveTaskToDatabase(const TaskData& task)
{
    DatabaseManager *db = DatabaseManager::instance();
    if (!db) {
        return;
    }

    if (!db->isConnected()) {
        db->reconnectIfNeeded();
    }

    if (writeTaskRow(db, task)) {
        return;
    }

    const QString error = db->lastError();
    if (isTransientDatabaseError(error) && db->reconnectIfNeeded()) {
        writeTaskRow(db, task);
    }
}

void TaskQueue::loadTasksFromDatabase()
{
    DatabaseManager* db = DatabaseManager::instance();
    if (!db) {
        return;
    }

    m_tasks.clear();
    m_queue.clear();

    const QList<QVariantMap> rows = db->selectAll(kJobsTable, "1=1", QVariantList(), "created_at DESC");

    for (const QVariantMap& row : rows) {
        TaskData task = TaskData::fromDatabaseRow(row);
        m_tasks[task.id] = task;

        if (task.status == TaskStatus::Pending) {
            m_queue.enqueue(task.id);
        } else if (task.status == TaskStatus::Running) {
            LOG_WARNING("TaskQueue", QString("Found running task %1 from previous session, marking as failed").arg(task.id));
            markInterruptedTaskAsFailed(task.id);
            m_tasks[task.id].status = TaskStatus::Failed;
        }
    }

    LOG_INFO("TaskQueue", QString("Loaded %1 tasks, %2 pending").arg(m_tasks.size()).arg(m_queue.size()));
}

TaskData* TaskQueue::getTaskRef(const QString& taskId)
{
    return m_tasks.contains(taskId) ? &m_tasks[taskId] : nullptr;
}

void TaskQueue::attachWorkerSignals(TaskWorker* worker)
{
    if (!worker) {
        return;
    }

    connect(worker, &TaskWorker::taskStarted, this, &TaskQueue::taskStarted);
    connect(worker, &TaskWorker::taskProgress, this, &TaskQueue::taskProgress);
    connect(worker, &TaskWorker::taskCompleted, this, &TaskQueue::taskCompleted);
    connect(worker, &TaskWorker::taskFailed, this, &TaskQueue::taskFailed);
}

TaskWorker* TaskQueue::createWorker()
{
    TaskWorker* worker = new TaskWorker(this);
    attachWorkerSignals(worker);
    return worker;
}

void TaskQueue::stopWorker(TaskWorker* worker)
{
    if (!worker) {
        return;
    }

    worker->stop();
    worker->wait();
    worker->deleteLater();
}

// ==================== TaskWorker 实现 ====================

TaskWorker::TaskWorker(TaskQueue* queue, QObject* parent)
    : QThread(parent)
    , m_queue(queue)
{
}

void TaskWorker::stop()
{
    QMutexLocker locker(&m_workerMutex);
    m_running = false;
}

TaskWorker::DequeueStatus TaskWorker::tryDequeueTask(QString& outTaskId, TaskData& outTask)
{
    QMutexLocker queueLock(&m_queue->m_mutex);

    while (m_queue->m_queue.isEmpty() && m_queue->m_running) {
        m_queue->m_condition.wait(&m_queue->m_mutex, 1000);

        QMutexLocker workerLock(&m_workerMutex);
        if (!m_running) {
            return DequeueStatus::Stopped;
        }
    }

    {
        QMutexLocker workerLock(&m_workerMutex);
        if (!m_running) {
            return DequeueStatus::Stopped;
        }
    }

    if (!m_queue->m_running) {
        return DequeueStatus::Stopped;
    }

    if (m_queue->m_queue.isEmpty()) {
        return DequeueStatus::NoTaskAvailable;
    }

    outTaskId = m_queue->m_queue.dequeue();
    TaskData* taskRef = m_queue->getTaskRef(outTaskId);
    if (!taskRef) {
        LOG_WARNING("TaskWorker", QString("Task not found: %1").arg(outTaskId));
        return DequeueStatus::NoTaskAvailable;
    }

    taskRef->markAsStarted();
    outTask = *taskRef;
    m_queue->saveTaskToDatabase(*taskRef);
    return DequeueStatus::Dequeued;
}

void TaskWorker::run()
{
    {
        QMutexLocker locker(&m_workerMutex);
        m_running = true;
    }
    
    LOG_INFO("TaskWorker", "Worker started (signal-driven)");

    while (true) {
        QString taskId;
        TaskData task;

        const DequeueStatus dequeueStatus = tryDequeueTask(taskId, task);
        if (dequeueStatus == DequeueStatus::Stopped) {
            LOG_INFO("TaskWorker", "Worker stopped by signal");
            return;
        }
        if (dequeueStatus == DequeueStatus::NoTaskAvailable) {
            continue;
        }

        emit taskStarted(taskId);
        LOG_INFO("TaskWorker", QString("Processing task: %1").arg(taskId));
        
        TaskResult result = executeTask(task);
        finalizeTask(taskId, result);
    }
    
    LOG_INFO("TaskWorker", "Worker thread stopped");
}

TaskWorker::TaskResult TaskWorker::executeTask(TaskData& task)
{
    TaskResult result;
    
    TaskQueue::TaskHandler handler;
    {
        QMutexLocker locker(&m_queue->m_mutex);
        handler = m_queue->m_handlers.value(task.type);
    }
    
    if (!handler) {
        result.errorMessage = QString("No handler registered for task type: %1").arg(task.typeString());
        LOG_ERROR("TaskWorker", result.errorMessage);
        return result;
    }
    
    try {
        handler(task);
        result.result = task.result;
        if (isFailedTaskResult(task.result)) {
            result.success = false;
            result.errorMessage = taskResultErrorMessage(task.result);
        } else {
            result.success = true;
        }
    } catch (const std::exception& e) {
        result.errorMessage = QString::fromUtf8(e.what());
        LOG_ERROR("TaskWorker", QString("Task exception: %1 - %2").arg(task.id, result.errorMessage));
    }
    
    return result;
}

void TaskWorker::finalizeTask(const QString& taskId, const TaskResult& result)
{
    bool success = result.success;
    QJsonObject taskResult = result.result;
    QString errorMessage = result.errorMessage;
    
    {
        QMutexLocker locker(&m_queue->m_mutex);
        TaskData* task = m_queue->getTaskRef(taskId);
        if (!task) return;
        
        if (success) {
            task->markAsCompleted(taskResult);
        } else {
            task->markAsFailed(errorMessage);
        }
        
        m_queue->saveTaskToDatabase(*task);
    }
    
    if (success) {
        emit taskCompleted(taskId, taskResult);
        LOG_INFO("TaskWorker", QString("Task completed: %1").arg(taskId));
    } else {
        emit taskFailed(taskId, errorMessage);
        LOG_ERROR("TaskWorker", QString("Task failed: %1 - %2").arg(taskId, errorMessage));
    }
}
