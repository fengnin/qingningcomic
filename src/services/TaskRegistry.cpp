#include "services/TaskRegistry.h"
#include "services/ServiceContainer.h"
#include "services/ImageService.h"
#include "services/StoryboardService.h"
#include "services/AnalysisService.h"
#include "services/NovelService.h"
#include "utils/AnalysisJobUtils.h"
#include "utils/ImageModeUtils.h"
#include "utils/Logger.h"
#include <QJsonDocument>
#include <QEventLoop>
#include <stdexcept>

namespace {
struct HandlerSpec
{
    TaskType type;
    TaskQueue::TaskHandler handler;
    const char* name;
};

    ImageService* getImageService()
    {
        ImageService* service = ServiceContainer::instance()->imageService();
        return service ? service : ImageService::instance();
    }

    QJsonObject buildBatchResult(int success, int failed, int total)
    {
        QJsonObject result;
        result["success"] = success;
        result["failed"] = failed;
        result["total"] = total;
        return result;
    }

    QStringList toPanelIdList(const QJsonArray& panelIds)
    {
        QStringList panelIdList;
        panelIdList.reserve(panelIds.size());
        for (const QJsonValue& value : panelIds) {
            panelIdList.append(value.toString());
        }
        return panelIdList;
    }

    void emitBatchProgress(const QString& taskId, int total, int current, const QString& info)
    {
        if (TaskQueue::instance()) {
            const int progress = total > 0 ? current * 100 / total : 0;
            emit TaskQueue::instance()->taskProgress(taskId, progress, info);
        }
    }

    void executePanelBatch(TaskData& task, ImageService* imageService)
    {
        if (!imageService) {
            throw std::runtime_error("ImageService is null");
        }

        if (!task.novelId.isEmpty()) {
            NovelService::instance()->updateStatus(task.novelId, NovelStatus::Analyzing);
        }

        const QJsonArray panelIds = task.params["panelIds"].toArray();
        const QString modeStr = task.params["mode"].toString("1x1");
        const int total = panelIds.size();
        task.total = total;
        task.completed = 0;

        if (total == 0) {
            task.result = buildBatchResult(0, 0, 0);
            if (!task.novelId.isEmpty()) {
                NovelService::instance()->updateStatus(task.novelId, NovelStatus::Completed);
            }
            return;
        }

        const QStringList panelIdList = toPanelIdList(panelIds);

        QEventLoop completionLoop;
        ImageService::BatchResult finalResult;
        QString errorMessage;
        bool finished = false;

        QObject::connect(imageService, &ImageService::batchProgressChanged,
                &completionLoop,
                [&](int current, int totalPanel, const QString& info) {
                    Q_UNUSED(totalPanel)
                    task.completed = current;
                    emitBatchProgress(task.id, total, current, info);
                },
                Qt::QueuedConnection);

        QObject::connect(imageService, &ImageService::imageBatchCompleted,
                &completionLoop,
                [&](const ImageService::BatchResult& result) {
                    finalResult = result;
                    finished = true;
                    completionLoop.quit();
                },
                Qt::QueuedConnection);

        QObject::connect(imageService, &ImageService::errorOccurred,
                &completionLoop,
                [&](const QString& operation, const QString& message) {
                    if (operation == "startBatchGeneration") {
                        errorMessage = message;
                        finished = true;
                        completionLoop.quit();
                    }
                },
                Qt::QueuedConnection);

        if (ImageModeUtils::isPresetModeString(modeStr)) {
            imageService->generatePanelImages(panelIdList, ImageModeUtils::presetModeFromString(modeStr));
        } else {
            imageService->generatePanelImages(panelIdList, ImageModeUtils::generateModeFromString(modeStr));
        }

        completionLoop.exec();

        if (!finished) {
            task.result = buildBatchResult(0, 0, total);
            task.result["status"] = "failed";
            task.result["message"] = "Batch generation interrupted";
            if (!task.novelId.isEmpty()) {
                NovelService::instance()->updateStatus(task.novelId, NovelStatus::Error);
            }
            return;
        }

        task.result = buildBatchResult(finalResult.successCount, finalResult.failedCount, finalResult.totalCount);
        if (!errorMessage.isEmpty()) {
            task.result["errorMessage"] = errorMessage;
            task.result["status"] = "failed";
            task.result["message"] = errorMessage;
            if (!task.novelId.isEmpty()) {
                NovelService::instance()->updateStatus(task.novelId, NovelStatus::Error);
            }
        } else {
            task.result["status"] = "completed";
            task.result["message"] = "Batch generation completed";
            if (!task.novelId.isEmpty()) {
                NovelService::instance()->updateStatus(task.novelId, NovelStatus::Completed);
            }
        }
    }

    void bindHandler(TaskType type, TaskQueue::TaskHandler handler, const char* name)
    {
        TaskQueue::instance()->registerHandler(type, handler);
        LOG_INFO("TaskRegistry", QString("%1 handler registered").arg(name));
    }

    void registerBuiltInHandlers(std::initializer_list<HandlerSpec> specs)
    {
        for (const HandlerSpec& spec : specs) {
            bindHandler(spec.type, spec.handler, spec.name);
        }
    }
}

TaskRegistry* TaskRegistry::m_instance = nullptr;
std::once_flag TaskRegistry::m_instanceOnceFlag;

TaskRegistry::TaskRegistry()
    : QObject(nullptr)
{
}

TaskRegistry::~TaskRegistry()
{
}

TaskRegistry* TaskRegistry::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new TaskRegistry();
    });
    return m_instance;
}

void TaskRegistry::registerAllHandlers()
{
    registerBuiltInHandlers({
        {TaskType::GenerateStoryboard, [](TaskData& task) {
            if (AnalysisService* service = AnalysisService::instance()) {
                service->processTask(task);
            }
        }, "GenerateStoryboard"},
        {TaskType::GeneratePanelImage, [](TaskData& task) {
            task.result = getImageService()->handleGeneratePanelImageTask(task.toJson());
        }, "GeneratePanelImage"},
        {TaskType::GeneratePanels, [](TaskData& task) {
            executePanelBatch(task, getImageService());

            LOG_INFO("TaskRegistry", QString("Batch generation task completed: %1")
                .arg(QString::fromUtf8(QJsonDocument(task.result).toJson())));
        }, "GeneratePanels"},
    });
    LOG_INFO("TaskRegistry", "All task handlers registered");
}

void TaskRegistry::registerHandler(TaskType type, TaskHandler handler)
{
    m_handlers[type] = handler;
    TaskQueue::instance()->registerHandler(type, handler);
}

void TaskRegistry::unregisterHandler(TaskType type)
{
    m_handlers.remove(type);
}

bool TaskRegistry::hasHandler(TaskType type) const
{
    return m_handlers.contains(type);
}

void TaskRegistry::setGenerateStoryboardHandler(TaskHandler handler)
{
    registerHandler(TaskType::GenerateStoryboard, handler);
}

void TaskRegistry::setGeneratePanelImageHandler(TaskHandler handler)
{
    registerHandler(TaskType::GeneratePanelImage, handler);
}

void TaskRegistry::setGeneratePanelsHandler(TaskHandler handler)
{
    registerHandler(TaskType::GeneratePanels, handler);
}
