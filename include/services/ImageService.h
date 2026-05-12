#ifndef IMAGESERVICE_H
#define IMAGESERVICE_H

#include "services/BatchImageProcessor.h"
#include "models/Panel.h"
#include "api/QwenImageClient.h"
#include "utils/RetryPolicy.h"
#include "utils/SingletonUtils.h"
#include "utils/Logger.h"
#include <QJsonObject>
#include <QVariantMap>
#include <QByteArray>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QMutex>
#include <atomic>
#include <QFuture>

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

    struct EditHint {
        enum class Strategy {
            Default,
            FaceExpression,
            SubjectReplacement
        };

        Strategy strategy;
        bool faceOnly;
        QString expression;
        QString subjectDescription;
        QString replacementDescription;
        QString preserveDescription;
        double regionScale;
        double maskCoreRatio;
        double maskFeatherRatio;
        QString forceProvider;

        EditHint()
            : strategy(Strategy::Default)
            , faceOnly(false)
            , expression()
            , subjectDescription()
            , replacementDescription()
            , preserveDescription()
            , regionScale(1.15)
            , maskCoreRatio(0.90)
            , maskFeatherRatio(0.10)
            , forceProvider()
        {}

        bool isEnabled() const { return faceOnly || strategy != Strategy::Default || !expression.isEmpty(); }
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

    // 面板参考图数据结构
    // 用于存储面板生成所需的角色和场景参考信息
    struct PanelReferenceData {
        QMap<QString, QJsonObject> characterRefs;      // 角色参考信息（key: 角色名或ID）
        QMap<QString, QJsonObject> sceneRefs;          // 场景参考信息（key: 场景名或ID）
        QStringList allAvailableRefImages;             // 所有可用的参考图路径列表
    };

    GenerateResult generatePanelImage(const QString& panelId, GenerateMode mode = GenerateMode::Preview);
    GenerateResult generatePanelImage(const QString& panelId, GenerateMode mode, const EditHint& hint);
    void generatePanelImages(const QStringList& panelIds, GenerateMode mode = GenerateMode::Preview);
    void generateStoryboardImages(const QString& storyboardId, GenerateMode mode = GenerateMode::Preview);
    GenerateResult regeneratePanelImage(const QString& panelId, GenerateMode mode = GenerateMode::Preview);
    GenerateResult regeneratePanelImage(const QString& panelId, GenerateMode mode, const EditHint& hint);

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
    static QString preferredPanelSourceImagePath(const Panel& panel, GenerateMode mode);

    void cancelCurrentBatch();
    
    QString getProviderName() const;

signals:
    void panelGenerated(const ImageService::GenerateResult& result);
    void imageBatchCompleted(const ImageService::BatchResult& result);
    void batchGenerationCancelled();
    void progressChanged(const QString& stage, int progress);
    void batchProgressChanged(int current, int total, const QString& message);

public:
    explicit ImageService(QObject* parent = nullptr);
    virtual ~ImageService();  // 修改为虚函数，确保正确的多态销毁

private:
    struct GenerationContext {
        QString panelId;
        GenerateMode mode;
        QString presetMode;
        bool allowReferenceEdit = true;
        bool faceOnlyEdit = false;
        EditHint::Strategy editStrategy = EditHint::Strategy::Default;
        ResolutionConfig resolution;
        Panel panel;
        QString sourceImagePath;
        QString editExpression;
        QString editSubjectDescription;
        QString editReplacementDescription;
        QString editPreserveDescription;
        double editRegionScale = 1.15;
        double editMaskCoreRatio = 0.90;
        double editMaskFeatherRatio = 0.10;
        QRect editTargetRect;
        QByteArray editSourceImageData;
        QString prompt;
        QString editDirective;  // 编辑操作的原始指令（仅用于千问编辑API，不含角色/场景描述）
        QString negativePrompt;
        QByteArray imageData;
        QByteArray editMaskData;
        QString s3Key;
        QStringList referenceImages;
        QString primaryReferenceImagePath;
        QByteArray refImageData;
        int refImageWidth = 0;
        int refImageHeight = 0;
        QString forceProvider;
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
    PanelReferenceData fetchPanelReferenceData(const QString& novelId);
    
    void runBatchGeneration();
    
private slots:
    void processNextPanel();
    void processPanelAsync(const QString& panelId, const ResolutionConfig& resolution);
    
private:
    GenerateResult generatePanelImageInternal(const QString& panelId, const ResolutionConfig& resolution);
    GenerateResult generatePanelImageCore(const QString& panelId, const ResolutionConfig& resolution, bool checkConcurrency, const EditHint& hint);
    GenerateResult generateWithRetry(const QString& panelId, GenerateMode mode, int maxRetries);
    GenerateResult generateWithRetry(const QString& panelId, const QString& presetMode, int maxRetries);
    GenerateResult generateWithRetryInternal(const QString& panelId, const ResolutionConfig& resolution, int maxRetries);
    void startBatchGeneration(const QStringList& panelIds, const ResolutionConfig& resolution);
    bool takeNextBatchItem(QString& panelId, int& currentIndex, int& total, ResolutionConfig& resolution);
    void completeBatchGeneration();
    bool recordBatchItemResult(const QString& panelId, const GenerateResult& result, int maxRetries);
    
    ProviderConfig getProviderConfig() const;
    QString presetModeToString(BatchPresetMode presetMode) const;
    BatchPresetMode presetModeFromString(const QString& presetMode) const;
    void getPresetResolution(BatchPresetMode presetMode, int& width, int& height) const;
    void getPresetResolution(const QString& presetMode, int& width, int& height) const;
    bool loadReferenceImage(GenerationContext& ctx);
    void prepareEditReferenceImages(GenerationContext& ctx);
    bool prepareFaceOnlyEditContext(GenerationContext& ctx) const;
    QRectF resolveFaceEditRegion(const GenerationContext& ctx) const;
    QByteArray buildFaceEditMask(const GenerationContext& ctx) const;
    QString buildFaceEditPrompt(const GenerationContext& ctx) const;
    static double faceEditMinimumScaleForShotType(const QString& shotType);
    void applyFaceOnlyEditHints(GenerationContext& ctx, QwenImageClient::EditOptions& options) const;
    bool applyFaceOnlyEditPostProcess(GenerationContext& ctx);
    static QRectF expandEditRegion(const QRectF& region, double scale, const QSize& canvasSize);
    bool finalizeGeneratedImage(GenerationContext& ctx, const QByteArray& imageData);
    bool executeWithVolcEngine(GenerationContext& ctx);
    bool executeWithQwen(GenerationContext& ctx);
    bool executeImageGeneration(GenerationContext& ctx);
    QwenImageClient::GenerateOptions buildQwenGenerateOptions(const GenerationContext& ctx,
                                                              const ResolutionConfig& resolution) const;
    QwenImageClient::EditOptions buildQwenEditOptions(GenerationContext& ctx,
                                                      const ResolutionConfig& resolution) const;
    QJsonObject buildPanelPromptOptions(const GenerationContext& ctx, const QJsonObject& panelJson) const;
    void appendUniqueReferenceImages(QStringList& target, const QStringList& source) const;
    QString selectPrimaryReferenceImagePath(const GenerationContext& ctx) const;
    void limitReferenceImagesForProvider(GenerationContext& ctx) const;
    bool shouldAdvanceToNextBatchItem() const;
    void queueNextBatchItemProcessing();

    template<typename Func>
    GenerateResult executeWithErrorHandling(const QString& operationName, Func&& func)
    {
        try {
            return func();
        } catch (const std::exception& e) {
            const QString errorMsg = QString::fromUtf8(e.what());
            LOG_ERROR("ImageService",
                QString("%1 exception: %2").arg(operationName).arg(errorMsg));
            return createErrorResult(QString(), errorMsg);
        } catch (...) {
            LOG_ERROR("ImageService",
                QString("%1 unknown exception").arg(operationName));
            return createErrorResult(QString(),
                tr("%1过程中发生未知错误").arg(operationName));
        }
    }

    template<typename Func>
    bool executeWithErrorHandlingBool(const QString& operationName, Func&& func)
    {
        try {
            return func();
        } catch (const std::exception& e) {
            const QString errorMsg = QString::fromUtf8(e.what());
            LOG_ERROR("ImageService",
                QString("%1 exception: %2").arg(operationName).arg(errorMsg));
            setError(TR_FMT("%1 failed: %2", operationName, errorMsg));
            return false;
        } catch (...) {
            LOG_ERROR("ImageService",
                QString("%1 unknown exception").arg(operationName));
            setError(tr("%1时发生异常").arg(operationName));
            return false;
        }
    }

    mutable QMutex m_mutex;
    QString m_currentPanelId;
    GenerateMode m_currentMode;
    ResolutionConfig m_currentResolution;
    QStringList m_pendingPanelIds;
    BatchResult m_batchResult;
    
    // 🔧 优化：使用原子变量替代部分锁保护的状态
    std::atomic<int> m_currentProcessIndex{0};  // 原子计数器
    std::atomic<bool> m_batchCancelled{false};   // 原子标志
    
    QString m_currentPresetMode;

    // 线程安全和生命周期管理
    std::atomic<bool> m_destroyed{false};  // 原子标志：对象是否已被销毁
    QList<QFuture<void>> m_runningTasks;   // 跟踪运行中的后台任务

    RetryPolicy* m_dbRetryPolicy;
    RetryPolicy* m_apiRetryPolicy;
};

Q_DECLARE_METATYPE(ImageService::GenerateResult)
Q_DECLARE_METATYPE(ImageService::BatchResult)

#endif // IMAGESERVICE_H
