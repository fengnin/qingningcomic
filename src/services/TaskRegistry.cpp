#include "services/TaskRegistry.h"
#include "services/ServiceContainer.h"
#include "services/ImageService.h"
#include "services/StoryboardService.h"
#include "services/AnalysisService.h"
#include "utils/Logger.h"
#include <QEventLoop>
#include <QJsonDocument>
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

    ImageService::BatchPresetMode batchPresetModeFromMode(const QString& modeStr)
    {
        if (modeStr == "3x2" || modeStr == "3:2") {
            return ImageService::BatchPresetMode::Standard_3x2;
        }
        if (modeStr == "16x9" || modeStr == "16:9") {
            return ImageService::BatchPresetMode::Widescreen_16x9;
        }
        return ImageService::BatchPresetMode::Square_1x1;
    }

    ImageService::GenerateMode generateModeFromMode(const QString& modeStr)
    {
        return (modeStr == "hd") ? ImageService::GenerateMode::HD : ImageService::GenerateMode::Preview;
    }

    void emitBatchProgress(const QString& taskId, int total, int current, const QString& info)
    {
        if (TaskQueue::instance()) {
            const int progress = total > 0 ? current * 100 / total : 0;
            emit TaskQueue::instance()->taskProgress(taskId, progress, info);
        }
    }

    void connectBatchSignals(ImageService* imageService,
                             QEventLoop* loop,
                             TaskData* task,
                             int total,
                             bool* batchFinished,
                             bool* batchFailed,
                             QString* batchError)
    {
        QObject::connect(
            imageService, &ImageService::batchProgressChanged,
            loop,
            [task, total](int current, int totalPanel, const QString& info) {
                Q_UNUSED(totalPanel)
                task->completed = current;
                emitBatchProgress(task->id, total, current, info);
            },
            Qt::QueuedConnection);

        QObject::connect(
            imageService, &ImageService::imageBatchCompleted,
            loop,
            [loop, task, batchFinished, batchFailed, batchError](const ImageService::BatchResult& result) {
                *batchFinished = true;
                task->result = buildBatchResult(result.successCount, result.failedCount, result.totalCount);
                task->completed = result.totalCount;
                if ((result.totalCount > 0 && result.successCount == 0) || !batchError->isEmpty()) {
                    *batchFailed = true;
                    if (batchError->isEmpty()) {
                        *batchError = QStringLiteral("Batch generation failed");
                    }
                }
                loop->quit();
            },
            Qt::QueuedConnection);

        QObject::connect(
            imageService, &ImageService::errorOccurred,
            loop,
            [loop, batchFailed, batchError, task](const QString& operation, const QString& message) {
                if (operation == "startBatchGeneration") {
                    *batchFailed = true;
                    *batchError = message;
                    task->result = buildBatchResult(0, 0, 0);
                    loop->quit();
                }
            },
            Qt::QueuedConnection);
    }

    void executePanelBatch(TaskData& task, ImageService* imageService)
    {
        if (!imageService) {
            throw std::runtime_error("ImageService is null");
        }

        const QJsonArray panelIds = task.params["panelIds"].toArray();
        const QString modeStr = task.params["mode"].toString("1x1");
        const int total = panelIds.size();
        task.total = total;
        task.completed = 0;

        if (total == 0) {
            task.result = buildBatchResult(0, 0, 0);
            return;
        }

        const QStringList panelIdList = toPanelIdList(panelIds);

        bool batchFinished = false;
        bool batchFailed = false;
        QString batchError;

        QEventLoop loop;
        connectBatchSignals(imageService, &loop, &task, total, &batchFinished, &batchFailed, &batchError);

        if (modeStr == "1x1" || modeStr == "1:1" || modeStr == "3x2" || modeStr == "3:2"
            || modeStr == "16x9" || modeStr == "16:9") {
            imageService->generatePanelImages(panelIdList, batchPresetModeFromMode(modeStr));
        } else {
            imageService->generatePanelImages(panelIdList, generateModeFromMode(modeStr));
        }

        loop.exec();

        if (batchFailed) {
            throw std::runtime_error(batchError.toUtf8().constData());
        }

        if (!batchFinished) {
            throw std::runtime_error("Batch generation finished without a completion signal");
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
