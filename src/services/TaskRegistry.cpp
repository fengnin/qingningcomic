#include "TaskRegistry.h"
#include "ServiceContainer.h"
#include "ImageService.h"
#include "StoryboardService.h"
#include "AnalysisService.h"
#include "Logger.h"
#include <QThread>
#include <QEventLoop>
#include <QJsonDocument>
#include <stdexcept>

namespace {
    ImageService* getImageService()
    {
        ImageService* service = ServiceContainer::instance()->imageService();
        return service ? service : ImageService::instance();
    }

    bool parseBatchPresetMode(const QString& modeStr, ImageService::BatchPresetMode& outMode)
    {
        if (modeStr == "1x1") {
            outMode = ImageService::BatchPresetMode::Square_1x1;
            return true;
        }
        if (modeStr == "3x2" || modeStr == "3:2") {
            outMode = ImageService::BatchPresetMode::Standard_3x2;
            return true;
        }
        if (modeStr == "16x9" || modeStr == "16:9") {
            outMode = ImageService::BatchPresetMode::Widescreen_16x9;
            return true;
        }
        return false;
    }

    ImageService::GenerateMode parseMode(const QString& modeStr)
    {
        return (modeStr == "hd") ? ImageService::GenerateMode::HD : ImageService::GenerateMode::Preview;
    }

    QJsonObject buildBatchResult(int success, int failed, int total)
    {
        QJsonObject result;
        result["success"] = success;
        result["failed"] = failed;
        result["total"] = total;
        return result;
    }

    void logHandlerRegistered(const QString& name)
    {
        LOG_INFO("TaskRegistry", QString("%1 handler registered").arg(name));
    }
}

TaskRegistry* TaskRegistry::m_instance = nullptr;

TaskRegistry::TaskRegistry()
    : QObject(nullptr)
{
}

TaskRegistry::~TaskRegistry()
{
}

TaskRegistry* TaskRegistry::instance()
{
    if (!m_instance) {
        m_instance = new TaskRegistry();
    }
    return m_instance;
}

void TaskRegistry::registerAllHandlers()
{
    registerGenerateStoryboardHandler();
    registerGeneratePanelImageHandler();
    registerGeneratePanelsHandler();
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

void TaskRegistry::registerGenerateStoryboardHandler()
{
    auto handler = [](TaskData& task) {
        if (AnalysisService* service = AnalysisService::instance()) {
            service->processTask(task);
        }
    };

    registerHandler(TaskType::GenerateStoryboard, handler);
    logHandlerRegistered("GenerateStoryboard");
}

void TaskRegistry::registerGeneratePanelImageHandler()
{
    auto handler = [](TaskData& task) -> QJsonObject {
        return getImageService()->handleGeneratePanelImageTask(task.toJson());
    };

    registerHandler(TaskType::GeneratePanelImage, handler);
    logHandlerRegistered("GeneratePanelImage");
}

void TaskRegistry::registerGeneratePanelsHandler()
{
    auto handler = [](TaskData& task) {
        ImageService* imageService = getImageService();
        if (!imageService) {
            throw std::runtime_error("ImageService is null");
        }

        QJsonArray panelIds = task.params["panelIds"].toArray();
        QString modeStr = task.params["mode"].toString("1x1");

        const int total = panelIds.size();
        if (total == 0) {
            task.result = buildBatchResult(0, 0, 0);
            return;
        }

        QStringList panelIdList;
        panelIdList.reserve(total);
        for (const QJsonValue& v : panelIds) {
            panelIdList << v.toString();
        }

        bool batchFinished = false;
        bool batchFailed = false;
        QString batchError;

        QEventLoop loop;
        QMetaObject::Connection progressConn = QObject::connect(
            imageService, &ImageService::batchProgressChanged,
            &loop,
            [&task, total](int current, int totalPanel, const QString& info) {
                Q_UNUSED(totalPanel)
                task.completed = current;
                if (TaskQueue::instance()) {
                    emit TaskQueue::instance()->taskProgress(task.id, current * 100 / total, info);
                }
            },
            Qt::QueuedConnection);

        QMetaObject::Connection completedConn = QObject::connect(
            imageService, &ImageService::imageBatchCompleted,
            &loop,
            [&loop, &task, &batchFinished, &batchFailed, &batchError](const ImageService::BatchResult& result) {
                batchFinished = true;
                task.result = buildBatchResult(result.successCount, result.failedCount, result.totalCount);
                task.completed = result.totalCount;
                if ((result.totalCount > 0 && result.successCount == 0) || !batchError.isEmpty()) {
                    batchFailed = true;
                    if (batchError.isEmpty()) {
                        batchError = QStringLiteral("Batch generation failed");
                    }
                }
                loop.quit();
            },
            Qt::QueuedConnection);

        QMetaObject::Connection errorConn = QObject::connect(
            imageService, &ImageService::errorOccurred,
            &loop,
            [&loop, &batchFailed, &batchError, &task](const QString& operation, const QString& message) {
                if (operation == "startBatchGeneration") {
                    batchFailed = true;
                    batchError = message;
                    task.result = buildBatchResult(0, 0, 0);
                    loop.quit();
                }
            },
            Qt::QueuedConnection);

        ImageService::BatchPresetMode presetMode;
        if (parseBatchPresetMode(modeStr, presetMode)) {
            imageService->generatePanelImages(panelIdList, presetMode);
        } else {
            ImageService::GenerateMode mode = parseMode(modeStr);
            imageService->generatePanelImages(panelIdList, mode);
        }

        loop.exec();

        QObject::disconnect(progressConn);
        QObject::disconnect(completedConn);
        QObject::disconnect(errorConn);

        if (batchFailed) {
            throw std::runtime_error(batchError.toUtf8().constData());
        }

        if (!batchFinished) {
            throw std::runtime_error("Batch generation finished without a completion signal");
        }

        LOG_INFO("TaskRegistry", QString("Batch generation task completed: %1")
            .arg(QString::fromUtf8(QJsonDocument(task.result).toJson())));
    };

    registerHandler(TaskType::GeneratePanels, handler);
    logHandlerRegistered("GeneratePanels");
}
