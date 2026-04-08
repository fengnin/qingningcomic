#include "TaskRegistry.h"
#include "ServiceContainer.h"
#include "ImageService.h"
#include "StoryboardService.h"
#include "AnalysisService.h"
#include "Logger.h"
#include <QThread>

namespace {
    ImageService* getImageService()
    {
        ImageService* service = ServiceContainer::instance()->imageService();
        return service ? service : ImageService::instance();
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
            LOG_ERROR("TaskRegistry", "ImageService is null");
            return;
        }
        
        QJsonArray panelIds = task.params["panelIds"].toArray();
        ImageService::GenerateMode mode = parseMode(task.params["mode"].toString("preview"));
        
        int total = panelIds.size();
        int successCount = 0;
        int failedCount = 0;
        
        // 初始化批量生成状态，使 shouldContinue() 返回 true
        imageService->startBatch(total);
        
        try {
            for (int i = 0; i < panelIds.size(); ++i) {
                // 检查是否被取消
                if (!imageService->isGenerating()) {
                    LOG_INFO("TaskRegistry", "Batch generation cancelled by user");
                    break;
                }
                
                QString panelId = panelIds[i].toString();
                
                // 捕获单个面板生成的异常，防止一个面板失败导致整个批次终止
                try {
                    ImageService::GenerateResult result = imageService->generatePanelImage(panelId, mode);
                    
                    if (result.success) {
                        ++successCount;
                        LOG_INFO("TaskRegistry", QString("Panel %1 generated").arg(panelId));
                    } else {
                        ++failedCount;
                        LOG_WARNING("TaskRegistry", QString("Panel %1 failed: %2").arg(panelId, result.errorMessage));
                    }
                } catch (const std::exception& e) {
                    ++failedCount;
                    LOG_ERROR("TaskRegistry", QString("Exception generating panel %1: %2")
                        .arg(panelId).arg(QString::fromUtf8(e.what())));
                } catch (...) {
                    ++failedCount;
                    LOG_ERROR("TaskRegistry", QString("Unknown exception generating panel %1").arg(panelId));
                }
                
                task.completed = i + 1;
                
                // 安全地发射进度信号（检查 TaskQueue 是否仍然有效）
                if (TaskQueue::instance()) {
                    emit TaskQueue::instance()->taskProgress(task.id, 
                        (i + 1) * 100 / total, 
                        QString("正在生成面板图像 %1/%2").arg(i + 1).arg(total));
                }
                
                // 在请求之间添加延迟，避免 API 限流（火山引擎免费版并发限制为1）
                // 最后一个面板不需要延迟
                if (i < panelIds.size() - 1) {
                    LOG_DEBUG("TaskRegistry", "Waiting 3 seconds before next request to avoid rate limiting");
                    QThread::sleep(3);
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("TaskRegistry", QString("Batch generation exception: %1").arg(QString::fromUtf8(e.what())));
        } catch (...) {
            LOG_ERROR("TaskRegistry", "Unknown exception during batch generation");
        }
        
        // 确保总是完成批量生成状态
        try {
            imageService->finishBatch();
        } catch (...) {
            LOG_ERROR("TaskRegistry", "Exception calling finishBatch");
        }
        
        task.result = buildBatchResult(successCount, failedCount, total);
        LOG_INFO("TaskRegistry", QString("Batch completed: %1 success, %2 failed")
            .arg(successCount).arg(failedCount));
    };
    
    registerHandler(TaskType::GeneratePanels, handler);
    logHandlerRegistered("GeneratePanels");
}
