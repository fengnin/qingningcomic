#ifndef IMAGESERVICE_H
#define IMAGESERVICE_H

#include "services/BatchImageProcessor.h"
#include "models/Panel.h"
#include "utils/RetryPolicy.h"
#include "utils/SingletonUtils.h"
#include <QJsonObject>
#include <QByteArray>
#include <QMutex>
#include <QTimer>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

class Character;
class Scene;

class ImageService : public BatchImageProcessor
{
    Q_OBJECT
    DECLARE_SINGLETON_MEMBERS(ImageService)

public:
    enum class GenerateMode {
        Preview,
        HD
    };

    enum class BatchPresetMode {
        Square_1x1,
        Standard_3x2,
        Widescreen_16x9
    };

    struct ResolutionConfig {
        int width = 1328;
        int height = 1328;
        bool isPreset = false;
        QString presetName;
        
        static ResolutionConfig fromMode(GenerateMode mode);
        static ResolutionConfig fromPreset(BatchPresetMode preset);
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
        QString errorMessage;
        QList<GenerateResult> results;
    };

    GenerateResult generatePanelImage(const QString& panelId, GenerateMode mode = GenerateMode::Preview);
    void generatePanelImages(const QStringList& panelIds, GenerateMode mode = GenerateMode::Preview);
    void generateStoryboardImages(const QString& storyboardId, GenerateMode mode = GenerateMode::Preview);
    GenerateResult regeneratePanelImage(const QString& panelId, GenerateMode mode = GenerateMode::Preview);

    GenerateResult generatePanelImage(const QString& panelId, BatchPresetMode presetMode);
    void generatePanelImages(const QStringList& panelIds, BatchPresetMode presetMode);
    void generateStoryboardImages(const QString& storyboardId, BatchPresetMode presetMode);
    GenerateResult regeneratePanelImage(const QString& panelId, BatchPresetMode presetMode);

    QString enqueuePanelImageGeneration(const QString& panelId, GenerateMode mode = GenerateMode::Preview);
    QString enqueueBatchPanelImageGeneration(const QStringList& panelIds, GenerateMode mode = GenerateMode::Preview);
    QStringList enqueuePanelImageGenerations(const QStringList& panelIds, GenerateMode mode = GenerateMode::Preview);
    QString enqueuePanelImageGeneration(const QString& panelId, BatchPresetMode presetMode);
    QString enqueueBatchPanelImageGeneration(const QStringList& panelIds, BatchPresetMode presetMode);
    QStringList enqueuePanelImageGenerations(const QStringList& panelIds, BatchPresetMode presetMode);
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
        QString presetMode;
        ResolutionConfig resolution;
        Panel panel;
        QString prompt;
        QString negativePrompt;
        QByteArray imageData;
        QString s3Key;
        QStringList referenceImages;
        QByteArray refImageData;
    };

    struct ProviderConfig {
        QString name;
        int maxRefImages;
        bool supportsImg2Img;
        int previewWidth;
        int previewHeight;
        int hdWidth;
        int hdHeight;
    };

    bool safeFetchPanel(GenerationContext& ctx);
    bool safeUpdateDatabase(GenerationContext& ctx);
    bool buildPromptForPanel(GenerationContext& ctx);
    bool generateImage(GenerationContext& ctx);
    bool storeImage(GenerationContext& ctx);
    GenerateResult finalizeResult(GenerationContext& ctx);

    QJsonObject buildPanelJson(const Panel& panel);
    QString fetchNovelIdByStoryboardId(const QString& storyboardId);
    QString generateS3Key(const QString& panelId, GenerateMode mode);
    GenerateResult createErrorResult(const QString& panelId, const QString& message);
    
    QJsonObject buildCharacterRef(const Character& ch, QStringList& outPortraitPaths);
    QMap<QString, QJsonObject> fetchCharacterRefs(const QString& novelId, QStringList& outReferenceImages);
    QJsonObject buildSceneRef(const Scene& scene, QStringList& outReferenceImages);
    QMap<QString, QJsonObject> fetchSceneRefs(const QString& novelId, QStringList& outReferenceImages);
    
    void runBatchGeneration();
    
private slots:
    void processNextPanel();
    void processPanelAsync(const QString& panelId, const ResolutionConfig& resolution, int currentIndex, int total);
    
private:
    GenerateResult generatePanelImageInternal(const QString& panelId, const ResolutionConfig& resolution);
    GenerateResult generatePanelImageCore(const QString& panelId, const ResolutionConfig& resolution, bool checkConcurrency);
    GenerateResult generateWithRetry(const QString& panelId, GenerateMode mode, int maxRetries);
    GenerateResult generateWithRetry(const QString& panelId, const QString& presetMode, int maxRetries);
    GenerateResult generateWithRetryInternal(const QString& panelId, const ResolutionConfig& resolution, int maxRetries);
    void startBatchGeneration(const QStringList& panelIds, const ResolutionConfig& resolution);
    
    ProviderConfig getProviderConfig() const;
    QString presetModeToString(BatchPresetMode presetMode) const;
    BatchPresetMode presetModeFromString(const QString& presetMode) const;
    void getPresetResolution(BatchPresetMode presetMode, int& width, int& height) const;
    void getPresetResolution(const QString& presetMode, int& width, int& height) const;
    bool loadReferenceImage(GenerationContext& ctx);
    bool executeWithVolcEngine(GenerationContext& ctx);
    bool executeWithQwen(GenerationContext& ctx);
    bool executeImageGeneration(GenerationContext& ctx);

    mutable QMutex m_mutex;
    QString m_currentPanelId;
    GenerateMode m_currentMode;
    ResolutionConfig m_currentResolution;
    QStringList m_pendingPanelIds;
    BatchResult m_batchResult;
    int m_currentProcessIndex = 0;
    bool m_batchCancelled = false;
    QString m_currentPresetMode;
    
    RetryPolicy* m_dbRetryPolicy;
    RetryPolicy* m_apiRetryPolicy;
};

Q_DECLARE_METATYPE(ImageService::GenerateResult)
Q_DECLARE_METATYPE(ImageService::BatchResult)

#endif // IMAGESERVICE_H
