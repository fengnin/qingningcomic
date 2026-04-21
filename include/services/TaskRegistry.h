#ifndef TASKREGISTRY_H
#define TASKREGISTRY_H

#include "models/Task.h"
#include "services/TaskQueue.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <functional>
#include <mutex>

class TaskRegistry : public QObject
{
    Q_OBJECT

public:
    using TaskHandler = TaskQueue::TaskHandler;
    
    static TaskRegistry* instance();
    
    void registerAllHandlers();
    void registerHandler(TaskType type, TaskHandler handler);
    void unregisterHandler(TaskType type);
    bool hasHandler(TaskType type) const;
    
    void setGenerateStoryboardHandler(TaskHandler handler);
    void setGeneratePanelImageHandler(TaskHandler handler);
    void setGeneratePanelsHandler(TaskHandler handler);

private:
    TaskRegistry();
    ~TaskRegistry();
    TaskRegistry(const TaskRegistry&) = delete;
    TaskRegistry& operator=(const TaskRegistry&) = delete;
    
    void registerGenerateStoryboardHandler();
    void registerGeneratePanelImageHandler();
    void registerGeneratePanelsHandler();
    
    static TaskRegistry* m_instance;
    static std::once_flag m_instanceOnceFlag;
    QMap<TaskType, TaskHandler> m_handlers;
};

#endif // TASKREGISTRY_H
