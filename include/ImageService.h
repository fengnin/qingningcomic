#ifndef IMAGESERVICE_H
#define IMAGESERVICE_H

#include "BatchImageProcessor.h"
#include "Panel.h"
#include "RetryPolicy.h"
#include "utils/SingletonUtils.h"
#include <QJsonObject>
#include <QByteArray>
#include <QMutex>
#include <QTimer>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

class Character;  // 前向声明
class Scene;     // 前向声明

class ImageService : public BatchImageProcessor
{
    Q_OBJECT
    DECLARE_SINGLETON_MEMBERS(ImageService)

public:
    enum class GenerateMode {
        Preview,
        HD
    };

    struct GenerateResult {
        bool success = false;
        QString panelId;
        QString imageUrl;
        QString s3Key;
        QString errorMessage;
        int width = 0;
        int height = 0;
        qint64 timestamp = 0;
    };

    struct BatchResult {
        int totalCount = 0;
        int successCount = 0;
        int failedCount = 0;
        QList<GenerateResult> results;
    };

    GenerateResult generatePanelImage(const QString& panelId, GenerateMode mode = GenerateMode::Preview);
    void generatePanelImages(const QStringList& panelIds, GenerateMode mode = GenerateMode::Preview);
    void generateStoryboardImages(const QString& storyboardId, GenerateMode mode = GenerateMode::Preview);
    GenerateResult regeneratePanelImage(const QString& panelId, GenerateMode mode = GenerateMode::Preview);

    QString enqueuePanelImageGeneration(const QString& panelId, GenerateMode mode = GenerateMode::Preview);
    QString enqueueBatchPanelImageGeneration(const QStringList& panelIds, GenerateMode mode = GenerateMode::Preview);
    QStringList enqueuePanelImageGenerations(const QStringList& panelIds, GenerateMode mode = GenerateMode::Preview);
    QJsonObject handleGeneratePanelImageTask(const QJsonObject& task);

    void cancelCurrentBatch();

signals:
    void panelGenerated(const ImageService::GenerateResult& result);
    void imageBatchCompleted(const ImageService::BatchResult& result);
    void batchGenerationCancelled();
    void progressChanged(const QString& stage, int progress);
    void batchProgressChanged(int current, int total, const QString& message);

public:
    explicit ImageService(QObject* parent = nullptr);
    ~ImageService();

private:
    struct GenerationContext {
        QString panelId;
        GenerateMode mode;
        Panel panel;
        QString prompt;
        QString negativePrompt;
        QByteArray imageData;
        QString s3Key;
        QStringList referenceImages;  // 角色参考图片路径列表
    };

    bool safeFetchPanel(GenerationContext& ctx);
    bool safeUpdateDatabase(GenerationContext& ctx);
    bool buildPromptForPanel(GenerationContext& ctx);
    bool generateImage(GenerationContext& ctx);
    bool generateWithReference(GenerationContext& ctx);  // 使用参考图片生成
    bool storeImage(GenerationContext& ctx);
    GenerateResult finalizeResult(GenerationContext& ctx);

    QJsonObject buildPanelJson(const Panel& panel);
    QString fetchNovelIdByStoryboardId(const QString& storyboardId);
    QString generateS3Key(const QString& panelId, GenerateMode mode);
    GenerateResult createErrorResult(const QString& panelId, const QString& message);
    
    // 重构：提取的辅助方法
    QJsonObject buildCharacterRef(const Character& ch, QStringList& outPortraitPaths);
    QMap<QString, QJsonObject> fetchCharacterRefs(const QString& novelId, QStringList& outReferenceImages);
    QJsonObject buildSceneRef(const Scene& scene, QStringList& outReferenceImages);
    QMap<QString, QJsonObject> fetchSceneRefs(const QString& novelId, QStringList& outReferenceImages);
    bool executeImageGeneration(GenerationContext& ctx, const QByteArray& refImageData);
    
    void runBatchGeneration();

    mutable QMutex m_mutex;
    QString m_currentPanelId;
    GenerateMode m_currentMode;
    QStringList m_pendingPanelIds;
    BatchResult m_batchResult;
    int m_currentProcessIndex = 0;
    bool m_batchCancelled = false;
    QFuture<void> m_batchFuture;
    
    RetryPolicy* m_dbRetryPolicy;
    RetryPolicy* m_apiRetryPolicy;
};

Q_DECLARE_METATYPE(ImageService::GenerateResult)
Q_DECLARE_METATYPE(ImageService::BatchResult)

#endif // IMAGESERVICE_H
