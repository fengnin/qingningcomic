#include "TaskQueue.h"
#include "DatabaseManager.h"
#include "Logger.h"
#include <QUuid>

TaskQueue* TaskQueue::m_instance = nullptr;
QMutex TaskQueue::m_instanceMutex;

TaskQueue::TaskQueue()
{
}

TaskQueue::~TaskQueue()
{
    stop();
}

TaskQueue* TaskQueue::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_instanceMutex);
        if (!m_instance) {
            m_instance = new TaskQueue();
        }
    }
    return m_instance;
}

void TaskQueue::start()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_running) return;
    
    m_running = true;
    loadTasksFromDatabase();
    
    connect(this, &TaskQueue::taskProgress, this, &TaskQueue::onTaskProgress, Qt::QueuedConnection);
    
    for (int i = 0; i < m_maxConcurrent; ++i) {
        TaskWorker* worker = new TaskWorker(this);
        connect(worker, &TaskWorker::taskStarted, this, &TaskQueue::taskStarted);
        connect(worker, &TaskWorker::taskProgress, this, &TaskQueue::taskProgress);
        connect(worker, &TaskWorker::taskCompleted, this, &TaskQueue::taskCompleted);
        connect(worker, &TaskWorker::taskFailed, this, &TaskQueue::taskFailed);
        connect(this, &TaskQueue::workerWakeAll, worker, &TaskWorker::wake);
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
        worker->stop();
        worker->wake();
        worker->wait();
        worker->deleteLater();
    }
    m_workers.clear();
    
    LOG_INFO("TaskQueue", "Stopped");
}

void TaskQueue::wakeWorkers()
{
    m_condition.wakeOne();
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
    return findTasksByStatus(TaskStatus::Pending);
}

QList<TaskData> TaskQueue::getAllTasks() const
{
    QMutexLocker locker(&m_mutex);
    return m_tasks.values();
}

bool TaskQueue::hasRunningTasks() const
{
    return hasTasksByStatus(TaskStatus::Running);
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

void TaskQueue::onTaskProgress(const QString& taskId, int progress, const QString& message)
{
    Q_UNUSED(message);
    
    QVariantMap data;
    data["progress"] = progress;
    
    QString where = "id = ?";
    QVariantList values;
    values << taskId;
    
    DatabaseManager *db = DatabaseManager::instance();
    
    if (!db->update("jobs", data, where, values)) {
        QString error = db->lastError();
        if (error.contains("gone away") || error.contains("server has gone away") || !db->isConnected()) {
            if (db->reconnectIfNeeded()) {
                db->update("jobs", data, where, values);
            }
        }
    }
}

void TaskQueue::saveTaskToDatabase(const TaskData& task)
{
    QVariantMap data = task.toDatabaseRow();
    
    QString where = "id = ?";
    QVariantList values;
    values << task.id;
    
    DatabaseManager *db = DatabaseManager::instance();
    
    if (!db->isConnected()) {
        db->reconnectIfNeeded();
    }
    
    QVariantMap existing = db->selectOne("jobs", where, values);
    
    bool success;
    if (!existing.isEmpty()) {
        success = db->update("jobs", data, where, values);
    } else {
        success = db->insert("jobs", data);
    }
    
    if (!success) {
        QString error = db->lastError();
        if (error.contains("gone away") || error.contains("server has gone away")) {
            if (db->reconnectIfNeeded()) {
                existing = db->selectOne("jobs", where, values);
                if (!existing.isEmpty()) {
                    db->update("jobs", data, where, values);
                } else {
                    db->insert("jobs", data);
                }
            }
        }
    }
}

void TaskQueue::loadTasksFromDatabase()
{
    QList<QVariantMap> rows = DatabaseManager::instance()->selectAll("jobs", "1=1", QVariantList(), "created_at DESC");
    
    for (const QVariantMap& row : rows) {
        TaskData task = TaskData::fromDatabaseRow(row);
        m_tasks[task.id] = task;
        
        if (task.status == TaskStatus::Pending) {
            m_queue.enqueue(task.id);
        }
        
        if (task.status == TaskStatus::Running) {
            LOG_WARNING("TaskQueue", QString("Found running task %1 from previous session, marking as failed").arg(task.id));
            QVariantMap updateData;
            updateData["status"] = "failed";
            updateData["error_message"] = "Task interrupted by application restart";
            DatabaseManager::instance()->update("jobs", updateData, "id = ?", QVariantList{task.id});
            m_tasks[task.id].status = TaskStatus::Failed;
        }
    }
    
    LOG_INFO("TaskQueue", QString("Loaded %1 tasks, %2 pending").arg(m_tasks.size()).arg(m_queue.size()));
}

QString TaskQueue::getNovelText(const QString& novelId) const
{
    if (novelId.isEmpty()) return QString();
    
    QVariantMap novelData = DatabaseManager::instance()->selectOne("novels", "id = ?", {novelId});
    if (novelData.isEmpty()) return QString();
    
    QString text = novelData["original_text"].toString();
    return text.isEmpty() ? novelData["text"].toString() : text;
}

QList<TaskData> TaskQueue::findTasks(TaskPredicate predicate) const
{
    QList<TaskData> result;
    for (const TaskData& task : m_tasks) {
        if (predicate(task)) {
            result.append(task);
        }
    }
    return result;
}

QList<TaskData> TaskQueue::findTasksByStatus(TaskStatus status) const
{
    QMutexLocker locker(&m_mutex);
    return findTasks([status](const TaskData& task) {
        return task.status == status;
    });
}

bool TaskQueue::hasTasks(TaskPredicate predicate) const
{
    for (const TaskData& task : m_tasks) {
        if (predicate(task)) {
            return true;
        }
    }
    return false;
}

bool TaskQueue::hasTasksByStatus(TaskStatus status) const
{
    QMutexLocker locker(&m_mutex);
    return hasTasks([status](const TaskData& task) {
        return task.status == status;
    });
}

bool TaskQueue::updateTaskStatus(const QString& taskId, TaskStatus newStatus)
{
    TaskData* task = getTaskRef(taskId);
    if (!task) return false;
    
    task->status = newStatus;
    return true;
}

TaskData* TaskQueue::getTaskRef(const QString& taskId)
{
    return m_tasks.contains(taskId) ? &m_tasks[taskId] : nullptr;
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

void TaskWorker::wake()
{
    m_workerCondition.wakeAll();
}

void TaskWorker::run()
{
    {
        QMutexLocker locker(&m_workerMutex);
        m_running = true;
    }
    
    LOG_INFO("TaskWorker", "Worker started (signal-driven)");
    
    while (true) {
        {
            QMutexLocker locker(&m_workerMutex);
            if (!m_running) break;
        }
        
        QString taskId;
        TaskData task;
        
        {
            QMutexLocker queueLock(&m_queue->m_mutex);
            
            while (m_queue->m_queue.isEmpty() && m_queue->m_running) {
                m_queue->m_condition.wait(&m_queue->m_mutex, 1000);
            }
            
            {
                QMutexLocker locker(&m_workerMutex);
                if (!m_running) {
                    LOG_INFO("TaskWorker", "Worker stopped by signal");
                    return;
                }
            }
            
            if (!m_queue->m_running) break;
            
            if (m_queue->m_queue.isEmpty()) continue;
            
            taskId = m_queue->m_queue.dequeue();
            
            TaskData* taskRef = m_queue->getTaskRef(taskId);
            if (!taskRef) {
                LOG_WARNING("TaskWorker", QString("Task not found: %1").arg(taskId));
                continue;
            }
            
            taskRef->markAsStarted();
            task = *taskRef;
            m_queue->saveTaskToDatabase(*taskRef);
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
        result.success = true;
        result.result = task.result;
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
