#include "services/TaskRegistry.h"
#include "services/ServiceContainer.h"
#include "services/ImageService.h"
#include "services/StoryboardService.h"
#include "services/AnalysisService.h"
#include "services/NovelService.h"
#include "utils/AnalysisJobUtils.h"
#include "utils/BatchGenerationUtils.h"
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

void updateTaskNovelStatus(const QString& novelId, NovelStatus status)
{
    if (!novelId.isEmpty()) {
        NovelService::instance()->updateStatus(novelId, status);
    }
}

void setPanelBatchResult(TaskData& task, const ImageService::BatchResult& result, const QString& errorMessage)
{
    task.result = buildBatchResult(result.successCount, result.failedCount, result.totalCount);
    const bool success = BatchGenerationUtils::isBatchGenerationSuccessful(result.failedCount, errorMessage);
    if (success) {
        task.result["status"] = "completed";
        task.result["message"] = "Batch generation completed";
        if (!task.novelId.isEmpty()) {
            NovelService::instance()->updateStatusAfterTask(task.novelId);
        }
        return;
    }

    const QString failureMessage = BatchGenerationUtils::buildBatchFailureMessage(
        result.successCount,
        result.failedCount,
        result.totalCount,
        errorMessage);
    task.result["errorMessage"] = failureMessage;
    task.result["status"] = "failed";
    task.result["message"] = failureMessage;
    updateTaskNovelStatus(task.novelId, NovelStatus::Error);
}

void setInterruptedPanelBatchResult(TaskData& task)
{
    task.result = buildBatchResult(0, 0, task.total);
    task.result["status"] = "failed";
    task.result["message"] = "Batch generation interrupted";
    updateTaskNovelStatus(task.novelId, NovelStatus::Error);
}

void executePanelBatch(TaskData& task, ImageService* imageService)
{
    if (!imageService) {
        throw std::runtime_error("ImageService is null");
    }

    updateTaskNovelStatus(task.novelId, NovelStatus::Analyzing);

    const QJsonArray panelIds = task.params["panelIds"].toArray();
    const QString modeStr = task.params["mode"].toString("1x1");
    task.total = panelIds.size();
    task.completed = 0;

    if (task.total == 0) {
        task.result = buildBatchResult(0, 0, 0);
        if (!task.novelId.isEmpty()) {
            NovelService::instance()->updateStatusAfterTask(task.novelId);
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
                emitBatchProgress(task.id, task.total, current, info);
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
        setInterruptedPanelBatchResult(task);
        return;
    }

    setPanelBatchResult(task, finalResult, errorMessage);
}

void registerHandler(TaskType type, const TaskQueue::TaskHandler& handler, const char* name)
{
    TaskQueue::instance()->registerHandler(type, handler);
    LOG_INFO("TaskRegistry", QString("%1 handler registered").arg(name));
}

TaskQueue::TaskHandler makeStoryboardHandler()
{
    return [](TaskData& task) {
        if (AnalysisService* service = AnalysisService::instance()) {
            service->processTask(task);
        }
    };
}

TaskQueue::TaskHandler makePanelImageHandler()
{
    return [](TaskData& task) {
        task.result = getImageService()->handleGeneratePanelImageTask(task.toJson());
    };
}

TaskQueue::TaskHandler makePanelBatchHandler()
{
    return [](TaskData& task) {
        executePanelBatch(task, getImageService());
        LOG_INFO("TaskRegistry", QString("Batch generation task completed: %1")
            .arg(QString::fromUtf8(QJsonDocument(task.result).toJson())));
    };
}

void registerBuiltInHandlers()
{
    const HandlerSpec specs[] = {
        {TaskType::GenerateStoryboard, makeStoryboardHandler(), "GenerateStoryboard"},
        {TaskType::GeneratePanelImage, makePanelImageHandler(), "GeneratePanelImage"},
        {TaskType::GeneratePanels, makePanelBatchHandler(), "GeneratePanels"},
    };

    for (const HandlerSpec& spec : specs) {
        registerHandler(spec.type, spec.handler, spec.name);
    }
}

} // namespace

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
    registerBuiltInHandlers();
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
