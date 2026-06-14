#include "services/ImageService.h"
#include "services/ServiceContainer.h"
#include "data/DatabaseManager.h"
#include "utils/SingletonUtils.h"
#include "utils/PromptBuilder.h"
#include "utils/ImageModeUtils.h"
#include "utils/ImageBorderTrimmer.h"
#include "utils/FaceEditPromptUtils.h"
#include "utils/LocalEditPromptUtils.h"
#include "utils/PromptTargetUtils.h"
#include "utils/LogSummaryUtils.h"
#include "utils/SceneKeyUtils.h"
#include "utils/FileManager.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "data/FileStorage.h"
#include "services/StoryboardService.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "services/TaskQueue.h"
#include "services/ReferenceImageUploader.h"
#include "models/Task.h"
#include "utils/StatusWriteUtils.h"
#include "utils/ChangeRequestExpressionUtils.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include "utils/AppConfig.h"
#include <QJsonDocument>
#include <QDateTime>
#include <QColor>
#include <QFileInfo>
#include <QBuffer>
#include <QImage>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QPainter>
#include <QRadialGradient>
#include <QSqlQuery>
#include <QIODevice>
#include <QtConcurrent>
#include <QThread>
#include <QTimer>
#include <QCoreApplication>
#include <cmath>
#include <functional>

namespace {
constexpr int kBatchPanelMaxRetries = 3;
constexpr int kBatchNextItemDelayMs = 10000;

constexpr int kRateLimitRetryBaseDelay = 60;
constexpr int kNormalRetryBaseDelay = 5;

bool isRateLimitError(const QString& errorMessage) {
    const QString lower = errorMessage.trimmed().toLower();
    return lower.contains(QStringLiteral("429"))
        || lower.contains(QStringLiteral("too many requests"))
        || lower.contains(QStringLiteral("flowlimitexceeded"));
}

// 轮询超时：服务器队列满，任务接收后长时间未执行。和限流一样需要慢速退避，
// 否则旧任务可能还在队列里，立刻重提只会让队列更拥堵。
bool isQueueTimeoutError(const QString& errorMessage) {
    return errorMessage.trimmed().toLower().contains(QStringLiteral("query task result failed"));
}

bool isJimengV40ReqKey(const QString& reqKey)
{
    const QString normalized = reqKey.trimmed().toLower();
    return normalized == QStringLiteral("jimeng_t2i_v40")
        || normalized.contains(QStringLiteral("jimeng"))
        || normalized.contains(QStringLiteral("v40"));
}

int calculateRetryDelay(const QString& errorMessage, int retryCount) {
    if (isRateLimitError(errorMessage) || isQueueTimeoutError(errorMessage)) {
        // 指数退避: 60s, 120s, 240s...
        return static_cast<int>(kRateLimitRetryBaseDelay * std::pow(2, retryCount - 1));
    } else {
        return static_cast<int>(std::pow(2, retryCount)) * kNormalRetryBaseDelay;
    }
}

QByteArray imageToPngBytes(const QImage& image)
{
    QByteArray bufferData;
    QBuffer buffer(&bufferData);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return QByteArray();
    }
    if (!image.save(&buffer, "PNG")) {
        return QByteArray();
    }
    return bufferData;
}

QJsonObject generateResultToJson(const ImageService::GenerateResult& result)
{
    QJsonObject json;
    json["success"] = result.success;
    json["panelId"] = result.panelId;
    json["imageUrl"] = result.imageUrl;
    json["s3Key"] = result.s3Key;
    json["errorMessage"] = result.errorMessage;
    json["width"] = result.width;
    json["height"] = result.height;
    json["timestamp"] = result.timestamp;
    return json;
}

QString determineRefType(const QString& refPath)
{
    if (refPath.contains(QStringLiteral("characters/"))) {
        return QStringLiteral("IP");
    }
    if (refPath.contains(QStringLiteral("scenes/"))) {
        return QStringLiteral("STYLE");
    }
    return QStringLiteral("AUTO");
}

QStringList determineRefTypes(const QStringList& refPaths,
                              const QMap<QString, QString>& semanticTypes = {})
{
    QStringList types;
    for (const QString& path : refPaths) {
        const QString semantic = semanticTypes.value(path);
        types.append(!semantic.isEmpty() ? semantic : determineRefType(path));
    }
    return types;
}

ImageService::GenerateMode parseTaskMode(const QString& modeStr)
{
    return ImageModeUtils::generateModeFromString(modeStr);
}

bool isPresetTaskMode(const QString& modeStr)
{
    return ImageModeUtils::isPresetModeString(modeStr);
}

QString selectPrimaryReferencePath(const QStringList& referenceImages,
                                   const QString& sourceImagePath,
                                   const QString& primaryReferenceImagePath)
{
    if (!sourceImagePath.isEmpty() && referenceImages.contains(sourceImagePath)) {
        return sourceImagePath;
    }
    if (!primaryReferenceImagePath.isEmpty() && referenceImages.contains(primaryReferenceImagePath)) {
        return primaryReferenceImagePath;
    }
    return referenceImages.isEmpty() ? QString() : referenceImages.first();
}

QRectF faceEditRegionImpl(const Panel& panel, int width, int height)
{
    const QString shotType = panel.shotType().trimmed().toLower();

    double regionWidthRatio = 0.24;
    double regionHeightRatio = 0.24;
    double centerXRatio = 0.50;
    double centerYRatio = 0.22;

    if (shotType.contains("close-up") || shotType.contains("portrait") || shotType.contains("single")
        || shotType.contains(QString::fromUtf8("特写")) || shotType.contains(QString::fromUtf8("近景"))) {
        regionWidthRatio = 0.30;
        regionHeightRatio = 0.32;
        centerYRatio = 0.20;
    } else if (shotType.contains("medium") || shotType.contains(QString::fromUtf8("中景"))) {
        regionWidthRatio = 0.20;
        regionHeightRatio = 0.22;
        centerYRatio = 0.19;
    } else if (shotType.contains("wide") || shotType.contains("full") || shotType.contains("long")
               || shotType.contains(QString::fromUtf8("远景")) || shotType.contains(QString::fromUtf8("全景"))) {
        regionWidthRatio = 0.16;
        regionHeightRatio = 0.18;
        centerYRatio = 0.17;
    }

    const double regionWidth = qMax(72.0, width * regionWidthRatio);
    const double regionHeight = qMax(72.0, height * regionHeightRatio);
    const double centerX = width * centerXRatio;
    const double centerY = height * centerYRatio;

    return QRectF(centerX - regionWidth / 2.0,
                  centerY - regionHeight / 2.0,
                  regionWidth,
                  regionHeight);
}

QRectF subjectReplacementRegionImpl(const Panel& panel, int width, int height)
{
    const QString shotType = panel.shotType().trimmed().toLower();

    double regionWidthRatio = 0.42;
    double regionHeightRatio = 0.56;
    double centerXRatio = 0.50;
    double centerYRatio = 0.38;

    if (shotType.contains("close-up") || shotType.contains("portrait") || shotType.contains("single")
        || shotType.contains(QString::fromUtf8("特写")) || shotType.contains(QString::fromUtf8("近景"))) {
        regionWidthRatio = 0.48;
        regionHeightRatio = 0.62;
        centerYRatio = 0.40;
    } else if (shotType.contains("medium") || shotType.contains(QString::fromUtf8("中景"))) {
        regionWidthRatio = 0.54;
        regionHeightRatio = 0.70;
        centerYRatio = 0.46;
    } else if (shotType.contains("wide") || shotType.contains("full") || shotType.contains("long")
               || shotType.contains(QString::fromUtf8("远景")) || shotType.contains(QString::fromUtf8("全景"))) {
        regionWidthRatio = 0.62;
        regionHeightRatio = 0.78;
        centerYRatio = 0.52;
    }

    const double regionWidth = qMax(110.0, width * regionWidthRatio);
    const double regionHeight = qMax(130.0, height * regionHeightRatio);
    const double centerX = width * centerXRatio;
    const double centerY = height * centerYRatio;

    return QRectF(centerX - regionWidth / 2.0,
                  centerY - regionHeight / 2.0,
                  regionWidth,
                  regionHeight);
}

QRectF expandFaceRegionImpl(const QRectF& region, double scale, const QSize& canvasSize)
{
    if (region.isEmpty() || canvasSize.width() <= 0 || canvasSize.height() <= 0) {
        return QRectF();
    }

    const double clampedScale = qBound(1.0, scale, 1.8);
    const QPointF center = region.center();
    const double width = qMin(static_cast<double>(canvasSize.width()), region.width() * clampedScale);
    const double height = qMin(static_cast<double>(canvasSize.height()), region.height() * clampedScale);
    QRectF expanded(center.x() - width / 2.0,
                    center.y() - height / 2.0,
                    width,
                    height);
    expanded = expanded.intersected(QRectF(0.0, 0.0, canvasSize.width(), canvasSize.height()));
    return expanded;
}

QByteArray buildFaceEditMaskBytesImpl(const QRectF& region,
                                      const QSize& canvasSize,
                                      double coreRatio,
                                      double featherRatio)
{
    if (canvasSize.width() <= 0 || canvasSize.height() <= 0 || region.isEmpty()) {
        return QByteArray();
    }

    QImage mask(canvasSize, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);

    QPainter painter(&mask);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    const QPointF center(region.center());
    const qreal radius = qMax(region.width(), region.height()) * 0.48;
    QRadialGradient gradient(center, radius);
    const qreal clampedCoreRatio = qBound<qreal>(static_cast<qreal>(0.0),
                                                  static_cast<qreal>(coreRatio),
                                                  static_cast<qreal>(0.99));
    const qreal clampedFeatherRatio = qBound<qreal>(static_cast<qreal>(0.0),
                                                    static_cast<qreal>(featherRatio),
                                                    static_cast<qreal>(1.0) - clampedCoreRatio);
    const qreal fadeStart = qBound<qreal>(static_cast<qreal>(0.01), clampedCoreRatio, static_cast<qreal>(0.99));
    const qreal fadeEnd = qBound<qreal>(fadeStart,
                                        fadeStart + clampedFeatherRatio,
                                        static_cast<qreal>(0.995));
    gradient.setColorAt(0.0, QColor(255, 255, 255, 255));
    gradient.setColorAt(fadeStart, QColor(255, 255, 255, 255));
    gradient.setColorAt(fadeEnd, QColor(255, 255, 255, 0));
    gradient.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setBrush(gradient);
    painter.drawEllipse(region);
    painter.end();

    return imageToPngBytes(mask);
}


TaskData createPanelTask(const QString& panelId, TaskType type, const QString& modeValue, int total)
{
    TaskData task;
    task.type = type;
    task.panelId = panelId;
    task.total = total;

    QJsonObject params;
    params["mode"] = modeValue;
    task.params = params;
    return task;
}

struct TaskScope
{
    QString novelId;
    QString storyboardId;
};

TaskScope resolveTaskScope(const QString& panelId, const QStringList& panelIds)
{
    QString resolvedPanelId = panelId;
    if (resolvedPanelId.isEmpty() && !panelIds.isEmpty()) {
        resolvedPanelId = panelIds.first();
    }

    TaskScope scope;
    if (resolvedPanelId.isEmpty()) {
        return scope;
    }

    Panel panel = StoryboardService::instance()->getPanel(resolvedPanelId);
    if (!panel.isValid()) {
        return scope;
    }

    scope.storyboardId = panel.storyboardId();
    if (scope.storyboardId.isEmpty()) {
        return scope;
    }

    DatabaseManager* db = DatabaseManager::instance();
    if (!db) {
        return scope;
    }

    QVariantMap storyboardRow = db->selectOne("storyboards", "id = ?", QVariantList{scope.storyboardId});
    if (!storyboardRow.isEmpty()) {
        scope.novelId = storyboardRow.value("novel_id").toString();
    }

    return scope;
}

QJsonArray panelIdsToJsonArray(const QStringList& panelIds)
{
    QJsonArray panelIdArray;
    for (const QString& id : panelIds) {
        panelIdArray.append(id);
    }
    return panelIdArray;
}

QStringList panelIdsFromPanels(const QList<Panel>& panels)
{
    QStringList panelIds;
    panelIds.reserve(panels.size());
    for (const Panel& panel : panels) {
        panelIds.append(panel.id());
    }
    return panelIds;
}

QList<Panel> fetchStoryboardPanels(const QString& storyboardId, QString& errorMessage, const QString& exceptionMessage, const QString& unknownMessage)
{
    try {
        return StoryboardService::instance()->getPanels(storyboardId);
    } catch (const std::exception& e) {
        LOG_ERROR("ImageService", QString("getPanels exception: %1").arg(e.what()));
        errorMessage = exceptionMessage;
    } catch (...) {
        LOG_ERROR("ImageService", "getPanels unknown exception");
        errorMessage = unknownMessage;
    }
    return QList<Panel>();
}

QString enqueuePanelTask(const QString& panelId,
                         TaskType taskType,
                         const QString& modeValue,
                         const QStringList& panelIds = QStringList())
{
    TaskData task = createPanelTask(panelId, taskType, modeValue, panelIds.isEmpty() ? 1 : panelIds.size());
    const TaskScope scope = resolveTaskScope(panelId, panelIds);
    task.novelId = scope.novelId;
    task.storyboardId = scope.storyboardId;

    if (!panelIds.isEmpty()) {
        task.params["panelIds"] = panelIdsToJsonArray(panelIds);
        task.completed = 0;
    }
    return TaskQueue::instance()->enqueue(task);
}

struct RequestedResolution
{
    int width = 0;
    int height = 0;
    bool isPreset = false;
};

struct ImageDimensions
{
    int width = 0;
    int height = 0;
};

struct EditDimensions
{
    int width = 0;
    int height = 0;
    QwenImageClient::ImageSize size = QwenImageClient::ImageSize::Square;
};

bool resolveFinalImageTargetSize(bool isPreset,
                                 int presetWidth,
                                 int presetHeight,
                                 int referenceWidth,
                                 int referenceHeight,
                                 int& targetWidth,
                                 int& targetHeight)
{
    if (isPreset) {
        targetWidth = presetWidth;
        targetHeight = presetHeight;
        return targetWidth > 0 && targetHeight > 0;
    }

    if (referenceWidth > 0 && referenceHeight > 0) {
        targetWidth = referenceWidth;
        targetHeight = referenceHeight;
        return true;
    }

    return false;
}

const QByteArray& referenceImageForEdit(const QByteArray& faceOnlyImage, const QByteArray& referenceImage, bool faceOnlyEdit)
{
    if (!faceOnlyEdit || faceOnlyImage.isEmpty()) {
        return referenceImage;
    }
    return faceOnlyImage;
}

void applyPresetResolutionForProvider(ImageService::BatchPresetMode presetMode,
                                      const QString& providerName,
                                      int& width,
                                      int& height)
{
    Q_UNUSED(providerName)
    switch (presetMode) {
        case ImageService::BatchPresetMode::Square_1x1:
            width = 2048;
            height = 2048;
            break;
        case ImageService::BatchPresetMode::Standard_3x2:
            width = 2496;
            height = 1664;
            break;
        case ImageService::BatchPresetMode::Widescreen_16x9:
            width = 2560;
            height = 1440;
            break;
    }
}

RequestedResolution requestedResolution(const QString& presetMode,
                                        ImageService::GenerateMode mode,
                                        const QString& providerName,
                                        int previewWidth,
                                        int previewHeight,
                                        int hdWidth,
                                        int hdHeight)
{
    RequestedResolution resolution;
    resolution.isPreset = !presetMode.isEmpty();
    if (resolution.isPreset) {
        const ImageService::BatchPresetMode batchPresetMode = ImageModeUtils::presetModeFromString(presetMode);
        applyPresetResolutionForProvider(batchPresetMode, providerName, resolution.width, resolution.height);
        return resolution;
    }

    resolution.width = (mode == ImageService::GenerateMode::HD) ? hdWidth : previewWidth;
    resolution.height = (mode == ImageService::GenerateMode::HD) ? hdHeight : previewHeight;
    return resolution;
}

ImageDimensions readImageDimensions(const QByteArray& imageData)
{
    ImageDimensions dimensions;
    if (imageData.isEmpty()) {
        return dimensions;
    }

    QImage image;
    if (image.loadFromData(imageData)) {
        dimensions.width = image.width();
        dimensions.height = image.height();
    }
    return dimensions;
}

EditDimensions resolveEditDimensions(int referenceWidth,
                                     int referenceHeight,
                                     int fallbackWidth,
                                     int fallbackHeight,
                                     bool usePresetFallback,
                                     ImageService::GenerateMode mode)
{
    EditDimensions dimensions;
    if (referenceWidth > 0 && referenceHeight > 0) {
        dimensions.width = referenceWidth;
        dimensions.height = referenceHeight;
        dimensions.size = QwenImageClient::ImageSize::Custom;
        return dimensions;
    }

    dimensions.width = fallbackWidth;
    dimensions.height = fallbackHeight;
    dimensions.size = usePresetFallback
        ? QwenImageClient::ImageSize::Custom
        : (mode == ImageService::GenerateMode::HD
            ? QwenImageClient::ImageSize::Square
            : QwenImageClient::ImageSize::Portrait);
    return dimensions;
}

QByteArray normalizeImageSize(const QByteArray& imageData, int targetWidth, int targetHeight)
{
    if (imageData.isEmpty() || targetWidth <= 0 || targetHeight <= 0) {
        return imageData;
    }

    QImage image;
    if (!image.loadFromData(imageData)) {
        return imageData;
    }

    if (image.width() == targetWidth && image.height() == targetHeight) {
        return imageData;
    }

    QImage resized = image.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QByteArray bufferData;
    QBuffer buffer(&bufferData);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return imageData;
    }
    if (!resized.save(&buffer, "PNG")) {
        return imageData;
    }

    LOG_INFO("ImageService", QString("Normalized edited image size from %1x%2 to %3x%4")
        .arg(image.width())
        .arg(image.height())
        .arg(targetWidth)
        .arg(targetHeight));
    return bufferData;
}

}

ImageService::ResolutionConfig ImageService::ResolutionConfig::fromMode(GenerateMode mode)
{
    ResolutionConfig config;
    config.isPreset = false;
    
    QString provider = AppConfig::instance()->imageService().provider;
    if (provider == "volcengine") {
        config.width = (mode == GenerateMode::HD) ? 2048 : 1328;
        config.height = (mode == GenerateMode::HD) ? 2048 : 1328;
    } else {
        config.width = (mode == GenerateMode::HD) ? 1024 : 768;
        config.height = (mode == GenerateMode::HD) ? 1024 : 1024;
    }
    
    return config;
}

ImageService::ResolutionConfig ImageService::ResolutionConfig::fromPreset(BatchPresetMode preset)
{
    ResolutionConfig config;
    config.isPreset = true;
    config.presetName = ImageService::instance()->presetModeToString(preset);
    
    ImageService::instance()->getPresetResolution(preset, config.width, config.height);
    
    return config;
}

DEFINE_SINGLETON_INSTANCE_SIMPLE(ImageService)

ImageService::ImageService(QObject* parent)
    : BatchImageProcessor(parent)
    , m_currentMode(GenerateMode::Preview)
    , m_currentProcessIndex(0)
    , m_batchCancelled(false)
{
    qRegisterMetaType<GenerateResult>("ImageService::GenerateResult");
    qRegisterMetaType<GenerateResult>("GenerateResult");
    qRegisterMetaType<BatchResult>("ImageService::BatchResult");
    qRegisterMetaType<BatchResult>("BatchResult");
    
    RetryConfig dbConfig;
    dbConfig.maxAttempts = 3;
    dbConfig.baseDelayMs = 1000;
    dbConfig.backoffMultiplier = 2.0;
    dbConfig.maxDelayMs = 30000;
    m_dbRetryPolicy = new RetryPolicy(dbConfig, this);
    
    RetryConfig apiConfig;
    apiConfig.maxAttempts = 3;
    apiConfig.baseDelayMs = 2000;
    apiConfig.backoffMultiplier = 2.0;
    apiConfig.maxDelayMs = 60000;
    m_apiRetryPolicy = new RetryPolicy(apiConfig, this);
}

ImageService::~ImageService()
{
    LOG_INFO("ImageService", "ImageService destructor called, cleaning up resources");

    // 设置销毁标志，阻止新的后台任务启动
    m_destroyed.store(true);

    // 取消所有正在进行的批量生成任务
    cancelCurrentBatch();

    // 等待所有后台任务完成，防止访问已销毁的对象
    for (QFuture<void>& task : m_runningTasks) {
        if (!task.isFinished()) {
            task.waitForFinished();
        }
    }
    m_runningTasks.clear();

    LOG_INFO("ImageService", "ImageService cleanup completed");
}

void ImageService::cancelCurrentBatch()
{
    LOG_INFO("ImageService", "Cancelling current batch generation");

    {
        QMutexLocker locker(&m_mutex);
        m_batchCancelled.store(true);  // 🔧 原子操作

        // 调用基类的 cancelGeneration() 方法来同步设置 m_batchState.cancelled
        // 这样可以避免直接访问私有成员
        cancelGeneration();

        m_pendingPanelIds.clear();
        m_currentProcessIndex.store(0);  // 🔧 原子操作
        m_currentPresetMode.clear();

        // 注意：cancelGeneration() 已经调用了 setGenerating(false)，这里不需要重复设置
        // 但我们需要重置 m_batchResult
        m_batchResult = BatchResult();
    }

    emit batchGenerationCancelled();
    LOG_INFO("ImageService", "Batch generation cancelled successfully");
}

ImageService::GenerateResult ImageService::generatePanelImageCore(const QString& panelId,
                                                                 const ResolutionConfig& resolution,
                                                                 bool checkConcurrency,
                                                                 const EditHint& hint)
{
    if (panelId.isEmpty()) {
        return createErrorResult(panelId, tr("图片服务不可用"));
    }

    LOG_INFO("ImageService", QString("generatePanelImageCore: panelId=%1, resolution=%2x%3")
        .arg(panelId).arg(resolution.width).arg(resolution.height));

    if (checkConcurrency && isGenerating()) {
        LOG_WARNING("ImageService", QString("Generation already in progress for panel %1").arg(panelId));
        return createErrorResult(panelId, tr("正在生成中，请等待完成"));
    }

    if (checkConcurrency) {
        setGenerating(true);
    }

    auto resetState = [this, checkConcurrency]() {
        if (checkConcurrency) {
            setGenerating(false);
        }
    };

    GenerationContext ctx;
    ctx.panelId = panelId;
    ctx.resolution = resolution;
    ctx.mode = GenerateMode::Preview;
    ctx.faceOnlyEdit = hint.faceOnly;
    ctx.editStrategy = hint.strategy;
    ctx.editExpression = hint.expression;
    ctx.editSubjectDescription = hint.subjectDescription;
    ctx.editReplacementDescription = hint.replacementDescription;
    ctx.editPreserveDescription = hint.preserveDescription;
    ctx.editRegionScale = hint.regionScale;
    ctx.editMaskCoreRatio = hint.maskCoreRatio;
    ctx.editMaskFeatherRatio = hint.maskFeatherRatio;
    ctx.forceProvider = hint.forceProvider;
    ctx.editMaskData.clear();
    if (resolution.isPreset) {
        ctx.presetMode = resolution.presetName;
    } else {
        ctx.mode = (resolution.width >= 2048) ? GenerateMode::HD : GenerateMode::Preview;
    }

    try {
        if (!safeFetchPanel(ctx)) {
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("获取面板数据失败") : lastError());
        }

        setError(tr("加载参考图失败"));

        if (checkConcurrency) {
            prepareEditReferenceImages(ctx);
        } else {
            ctx.allowReferenceEdit = false;
        }

        if (!buildPromptForPanel(ctx)) {
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("构建提示词失败") : lastError());
        }

        if (!generateImage(ctx)) {
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("图像生成失败") : lastError());
        }

        if (!storeImage(ctx)) {
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("保存图像失败") : lastError());
        }

        if (!safeUpdateDatabase(ctx)) {
            LOG_ERROR("ImageService", QString("Failed to update database for panel: %1").arg(panelId));
            resetState();
            return createErrorResult(panelId, lastError().isEmpty() ? tr("数据库更新失败") : lastError());
        }

        LOG_INFO("ImageService", QString("Panel generated successfully: %1").arg(panelId));

        resetState();
        return finalizeResult(ctx);
    } catch (...) {
        resetState();
        return executeWithErrorHandling(
            QString("generation of panel %1").arg(panelId),
            [&]() -> GenerateResult { throw; });
    }
}

ImageService::GenerateResult ImageService::generatePanelImage(const QString& panelId, GenerateMode mode)
{
    return generatePanelImageCore(panelId, ResolutionConfig::fromMode(mode), true, EditHint());
}

ImageService::GenerateResult ImageService::generatePanelImage(const QString& panelId, GenerateMode mode, const EditHint& hint)
{
    return generatePanelImageCore(panelId, ResolutionConfig::fromMode(mode), true, hint);
}

ImageService::GenerateResult ImageService::generatePanelImage(const QString& panelId, BatchPresetMode presetMode)
{
    return generatePanelImageCore(panelId, ResolutionConfig::fromPreset(presetMode), true, EditHint());
}

ImageService::GenerateResult ImageService::generatePanelImageInternal(const QString& panelId, const ResolutionConfig& resolution)
{
    return generatePanelImageCore(panelId, resolution, false, EditHint());
}

void ImageService::startBatchGeneration(const QStringList& panelIds, const ResolutionConfig& resolution)
{
    if (isGenerating()) {
        emit errorOccurred("startBatchGeneration", tr("正在生成中，请等待完成"));
        return;
    }

    if (panelIds.isEmpty()) {
        BatchResult result;
        result.errorMessage = tr("面板列表为空");
        emit batchProgressChanged(0, 0, result.errorMessage);
        emit imageBatchCompleted(result);
        emit errorOccurred("startBatchGeneration", result.errorMessage);
        return;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_pendingPanelIds = panelIds;
        m_currentResolution = resolution;
        m_currentProcessIndex.store(0);       // 🔧 原子操作
        m_batchCancelled.store(false);        // 🔧 原子操作

        m_batchResult = BatchResult();
        m_batchResult.totalCount = panelIds.size();
    }

    // Snapshot reference data once so all panels in this batch use the same portrait versions.
    {
        const TaskScope scope = resolveTaskScope(QString(), panelIds);
        m_batchReferenceData = scope.novelId.isEmpty() ? PanelReferenceData() : fetchPanelReferenceData(scope.novelId);
    }

    startBatch(panelIds.size());
    LOG_INFO("ImageService", QString("Starting batch generation for %1 panels").arg(panelIds.size()));

    QTimer::singleShot(0, this, &ImageService::processNextPanel);
}

bool ImageService::takeNextBatchItem(QString& panelId, int& currentIndex, int& total, ResolutionConfig& resolution)
{
    if (!shouldAdvanceToNextBatchItem()) {
        return false;
    }

    const int idx = m_currentProcessIndex.load();
    panelId = m_pendingPanelIds[idx];
    currentIndex = idx;
    total = m_pendingPanelIds.size();
    resolution = m_currentResolution;
    return true;
}

void ImageService::completeBatchGeneration()
{
    LOG_INFO("ImageService", "Starting batch finalization...");

    BatchResult finalResult;
    {
        QMutexLocker locker(&m_mutex);
        finalResult = m_batchResult;
    }

    m_batchReferenceData = PanelReferenceData();  // clear snapshot

    finishBatch();

    LOG_INFO("ImageService", QString("Batch completed: %1 success, %2 failed")
        .arg(finalResult.successCount).arg(finalResult.failedCount));

    QCoreApplication::processEvents();

    emit imageBatchCompleted(finalResult);
    LOG_INFO("ImageService", "imageBatchCompleted signal emitted");
}

bool ImageService::recordBatchItemResult(const QString& panelId, const GenerateResult& result, int maxRetries)
{
    m_batchResult.results.append(result);

    if (result.success) {
        m_batchResult.successCount++;
        emit panelGenerated(result);
        return true;
    }

    m_batchResult.failedCount++;
    LOG_ERROR("ImageService", QString("Panel generation failed after %1 attempts: %2 - %3")
        .arg(maxRetries).arg(panelId).arg(result.errorMessage));
    return false;
}

void ImageService::generatePanelImages(const QStringList& panelIds, GenerateMode mode)
{
    startBatchGeneration(panelIds, ResolutionConfig::fromMode(mode));
}

void ImageService::generatePanelImages(const QStringList& panelIds, BatchPresetMode presetMode)
{
    startBatchGeneration(panelIds, ResolutionConfig::fromPreset(presetMode));
}

void ImageService::generateStoryboardImages(const QString& storyboardId, GenerateMode mode)
{
    QString errorMessage;
    const QList<Panel> panels = fetchStoryboardPanels(
        storyboardId,
        errorMessage,
        tr("获取面板数据异常"),
        tr("获取面板数据未知错误"));

    if (!errorMessage.isEmpty()) {
        emit errorOccurred("generateStoryboardImages", errorMessage);
        return;
    }

    if (panels.isEmpty()) {
        emit errorOccurred("generateStoryboardImages", TR_FMT("Storyboard has no panels: %1", storyboardId));
        return;
    }

    generatePanelImages(panelIdsFromPanels(panels), mode);
}

void ImageService::generateStoryboardImages(const QString& storyboardId, BatchPresetMode presetMode)
{
    QString errorMessage;
    const QList<Panel> panels = fetchStoryboardPanels(
        storyboardId,
        errorMessage,
        tr("获取分镜面板异常"),
        tr("获取分镜面板未知错误"));

    if (!errorMessage.isEmpty()) {
        emit errorOccurred("generateStoryboardImages", errorMessage);
        return;
    }

    if (panels.isEmpty()) {
        emit errorOccurred("generateStoryboardImages", TR_FMT("No panels found for storyboard %1", storyboardId));
        return;
    }

    generatePanelImages(panelIdsFromPanels(panels), presetMode);
}

ImageService::GenerateResult ImageService::regeneratePanelImage(const QString& panelId, GenerateMode mode)
{
    return generatePanelImage(panelId, mode);
}

ImageService::GenerateResult ImageService::regeneratePanelImage(const QString& panelId, GenerateMode mode, const EditHint& hint)
{
    LOG_INFO("ImageService", QString("Regenerating panel: %1").arg(panelId));
    return generatePanelImage(panelId, mode, hint);
}

ImageService::GenerateResult ImageService::regeneratePanelImage(const QString& panelId, BatchPresetMode presetMode)
{
    LOG_INFO("ImageService", QString("Regenerating panel preset: %1").arg(panelId));
    return generatePanelImage(panelId, presetMode);
}

QString ImageService::preferredPanelSourceImagePath(const Panel& panel, GenerateMode mode)
{
    const QString previewPath = panel.previewS3Key();
    const QString hdPath = panel.hdS3Key();

    if (mode == GenerateMode::HD) {
        if (!hdPath.isEmpty()) {
            return hdPath;
        }
        if (!previewPath.isEmpty()) {
            return previewPath;
        }
        return QString();
    }

    if (!previewPath.isEmpty()) {
        return previewPath;
    }
    if (!hdPath.isEmpty()) {
        return hdPath;
    }

    return QString();
}

void ImageService::prepareEditReferenceImages(GenerationContext& ctx)
{
    ctx.sourceImagePath = preferredPanelSourceImagePath(ctx.panel, ctx.mode);
    ctx.allowReferenceEdit = !ctx.sourceImagePath.isEmpty();

    if (!ctx.allowReferenceEdit) {
        return;
    }

    ctx.referenceImages.prepend(ctx.sourceImagePath);
    LOG_INFO("ImageService", QString("Using panel source image for edit: %1").arg(ctx.sourceImagePath));
}

QString ImageService::buildFaceEditPrompt(const GenerationContext& ctx) const
{
    if (ctx.editExpression.trimmed().isEmpty()
        && ctx.editReplacementDescription.trimmed().isEmpty()) {
        return QString();
    }

    if (ctx.editExpression.trimmed().isEmpty()
        && !ctx.editReplacementDescription.trimmed().isEmpty()) {
        return LocalEditPromptUtils::buildLocalReplacementEditPrompt(
            ctx.editSubjectDescription,
            ctx.editReplacementDescription,
            ctx.editPreserveDescription,
            1.30);
    }

    return FaceEditPromptUtils::buildFaceExpressionEditPrompt(
        ctx.editSubjectDescription,
        ChangeRequestExpressionUtils::expressionPromptDescription(ctx.editExpression),
        ctx.editPreserveDescription);
}

double ImageService::faceEditMinimumScaleForShotType(const QString& shotType)
{
    if (shotType.contains("close-up") || shotType.contains("portrait") || shotType.contains("single")
        || shotType.contains(QString::fromUtf8("特写")) || shotType.contains(QString::fromUtf8("近景"))) {
        return 1.20;
    }
    if (shotType.contains("medium") || shotType.contains(QString::fromUtf8("中景"))) {
        return 1.14;
    }
    if (shotType.contains("wide") || shotType.contains("full") || shotType.contains("long")
        || shotType.contains(QString::fromUtf8("远景")) || shotType.contains(QString::fromUtf8("全景"))) {
        return 1.08;
    }
    return 1.0;
}

bool ImageService::prepareFaceOnlyEditContext(GenerationContext& ctx) const
{
    if (ctx.editTargetRect.isValid() && !ctx.editTargetRect.isEmpty()
        && !ctx.editSourceImageData.isEmpty()
        && !ctx.editMaskData.isEmpty()
        && ctx.editRegionRect.isValid() && !ctx.editRegionRect.isEmpty()) {
        return true;
    }

    if (!ctx.faceOnlyEdit || ctx.refImageData.isEmpty() || ctx.refImageWidth <= 0 || ctx.refImageHeight <= 0) {
        return false;
    }

    const bool subjectReplacement = ctx.editStrategy == EditHint::Strategy::SubjectReplacement
        || !ctx.editReplacementDescription.trimmed().isEmpty();

    const QRectF editRegion = subjectReplacement
        ? subjectReplacementRegionImpl(ctx.panel, ctx.refImageWidth, ctx.refImageHeight)
        : resolveFaceEditRegion(ctx);
    if (editRegion.isEmpty()) {
        LOG_WARNING("ImageService", QString("Failed to resolve edit region for panel: %1").arg(ctx.panelId));
        return false;
    }

    ctx.editTargetRect = QRect(0, 0, ctx.refImageWidth, ctx.refImageHeight);
    ctx.editRegionRect = editRegion;
    ctx.editSourceImageData = ctx.refImageData;
    if (ctx.editSourceImageData.isEmpty()) {
        LOG_WARNING("ImageService", QString("Failed to prepare face edit source for panel: %1").arg(ctx.panelId));
        return false;
    }

    const double coreRatio = subjectReplacement ? qMin(ctx.editMaskCoreRatio, 0.76) : ctx.editMaskCoreRatio;
    const double featherRatio = subjectReplacement ? qMax(ctx.editMaskFeatherRatio, 0.18) : ctx.editMaskFeatherRatio;
    ctx.editMaskData = buildFaceEditMaskBytesImpl(
        editRegion,
        ctx.editTargetRect.size(),
        coreRatio,
        featherRatio);

    LOG_DEBUG("ImageService", QString("Prepared edit region: panelId=%1, region=%2,%3 %4x%5, imageSize=%6x%7, strategy=%8")
        .arg(ctx.panelId)
        .arg(editRegion.x())
        .arg(editRegion.y())
        .arg(editRegion.width())
        .arg(editRegion.height())
        .arg(ctx.editTargetRect.width())
        .arg(ctx.editTargetRect.height())
        .arg(subjectReplacement ? QStringLiteral("subject_replacement") : QStringLiteral("face_expression")));
    return true;
}

QRectF ImageService::resolveFaceEditRegion(const GenerationContext& ctx) const
{
    if (ctx.refImageWidth <= 0 || ctx.refImageHeight <= 0) {
        return QRectF();
    }

    const QRectF baseRegion = faceEditRegionImpl(ctx.panel, ctx.refImageWidth, ctx.refImageHeight);
    if (baseRegion.isEmpty()) {
        return QRectF();
    }

    const QString shotType = ctx.panel.shotType().trimmed().toLower();
    const double scale = qMax(qBound(1.0, ctx.editRegionScale, 1.8), faceEditMinimumScaleForShotType(shotType));

    return expandEditRegion(baseRegion, scale, QSize(ctx.refImageWidth, ctx.refImageHeight));
}

QByteArray ImageService::buildFaceEditMask(const GenerationContext& ctx) const
{
    if (ctx.refImageWidth <= 0 || ctx.refImageHeight <= 0) {
        return QByteArray();
    }

    const QRectF region = resolveFaceEditRegion(ctx);
    if (region.isEmpty()) {
        return QByteArray();
    }

    const QSize canvasSize = QSize(ctx.refImageWidth, ctx.refImageHeight);
    LOG_DEBUG("ImageService", QString("Building face edit mask: panelId=%1, shotType=%2, region=%3,%4 %5x%6, canvas=%7x%8")
        .arg(ctx.panelId)
        .arg(ctx.panel.shotType())
        .arg(region.x())
        .arg(region.y())
        .arg(region.width())
        .arg(region.height())
        .arg(canvasSize.width())
        .arg(canvasSize.height()));
    return buildFaceEditMaskBytesImpl(region, canvasSize, ctx.editMaskCoreRatio, ctx.editMaskFeatherRatio);
}

void ImageService::applyFaceOnlyEditHints(GenerationContext& ctx, QwenImageClient::EditOptions& options) const
{
    if (!ctx.faceOnlyEdit) {
        return;
    }

    if (!prepareFaceOnlyEditContext(ctx)) {
        return;
    }

    options.prompt = buildFaceEditPrompt(ctx);
    options.sourceImage = ctx.editSourceImageData;
    options.size = QwenImageClient::ImageSize::Custom;
    options.width = ctx.refImageWidth > 0 ? ctx.refImageWidth : ctx.editTargetRect.width();
    options.height = ctx.refImageHeight > 0 ? ctx.refImageHeight : ctx.editTargetRect.height();
}

bool ImageService::applyFaceOnlyEditPostProcess(GenerationContext& ctx)
{
    if (!ctx.faceOnlyEdit || ctx.imageData.isEmpty()) {
        return true;
    }
    return !ctx.imageData.isEmpty();
}

QRectF ImageService::expandEditRegion(const QRectF& region, double scale, const QSize& canvasSize)
{
    return expandFaceRegionImpl(region, scale, canvasSize);
}

bool ImageService::finalizeGeneratedImage(GenerationContext& ctx, const QByteArray& imageData)
{
    if (imageData.isEmpty()) {
        return false;
    }

    int targetWidth = 0;
    int targetHeight = 0;
    if (!resolveFinalImageTargetSize(ctx.resolution.isPreset,
                                     ctx.resolution.width,
                                     ctx.resolution.height,
                                     ctx.refImageWidth,
                                     ctx.refImageHeight,
                                     targetWidth,
                                     targetHeight)) {
        ctx.imageData = imageData;
        return applyFaceOnlyEditPostProcess(ctx);
    }

    ctx.imageData = normalizeImageSize(imageData, targetWidth, targetHeight);
    if (!ctx.allowReferenceEdit) {
        const QByteArray before = ctx.imageData;
        const QByteArray after = ImageBorderTrimmer::trimDarkBorderImageData(before);
        if (after != before) {
            QImage beforeImage;
            QImage afterImage;
            beforeImage.loadFromData(before);
            afterImage.loadFromData(after);
            if (!beforeImage.isNull() && !afterImage.isNull()) {
                LOG_INFO("ImageService", QString("Trimmed dark border from %1x%2 to %3x%4")
                    .arg(beforeImage.width())
                    .arg(beforeImage.height())
                    .arg(afterImage.width())
                    .arg(afterImage.height()));
            }
        }
        ctx.imageData = after;
    }
    return applyFaceOnlyEditPostProcess(ctx);
}

ImageService::ProviderConfig ImageService::getProviderConfig() const
{
    ProviderConfig config;
    QString provider = AppConfig::instance()->imageService().provider;
    
    if (provider == "volcengine") {
        config.name = "volcengine";
        config.maxRefImages = 5;
        config.supportsImg2Img = true;
        config.previewWidth = 2048;
        config.previewHeight = 2048;
        config.hdWidth = 2048;
        config.hdHeight = 2048;
    } else {
        config.name = "qwen";
        config.maxRefImages = 3;
        config.supportsImg2Img = true;
        config.previewWidth = 512;
        config.previewHeight = 768;
        config.hdWidth = 1024;
        config.hdHeight = 1024;
    }
    
    return config;
}

QString ImageService::getProviderName() const
{
    return getProviderConfig().name;
}

bool ImageService::loadReferenceImage(GenerationContext& ctx)
{
    if (ctx.referenceImages.isEmpty()) {
        LOG_DEBUG("ImageService", "No reference images to load, using text2img");
        return true;
    }
    
    ProviderConfig config = getProviderConfig();
    
    if (ctx.referenceImages.size() > config.maxRefImages) {
        LOG_INFO("ImageService", QString("Limiting reference images from %1 to %2 for provider: %3")
            .arg(ctx.referenceImages.size()).arg(config.maxRefImages).arg(config.name));
        ctx.referenceImages = ctx.referenceImages.mid(0, config.maxRefImages);
    }
    
    const QString refPath = selectPrimaryReferencePath(ctx.referenceImages,
                                                        ctx.sourceImagePath,
                                                        ctx.primaryReferenceImagePath);
    QString fullPath = FileStorage::instance()->getFullPath(refPath);
    
    LOG_INFO("ImageService", QString("Loading reference image: %1 (full path: %2)")
        .arg(refPath).arg(fullPath));
    
    QFile refFile(fullPath);
    
    if (!refFile.exists()) {
        LOG_WARNING("ImageService", QString("Reference image not found: %1, falling back to text2image")
            .arg(refPath));
        ctx.referenceImages.clear();
        return true;
    }
    
    if (!refFile.open(QIODevice::ReadOnly)) {
        LOG_WARNING("ImageService", QString("Cannot open reference image: %1, falling back to text2image")
            .arg(refPath));
        ctx.referenceImages.clear();
        return true;
    }
    
    ctx.refImageData = refFile.readAll();
    refFile.close();

    const ImageDimensions refImageDimensions = readImageDimensions(ctx.refImageData);
    ctx.refImageWidth = refImageDimensions.width;
    ctx.refImageHeight = refImageDimensions.height;
    
    LOG_INFO("ImageService", QString("Loaded reference image: %1 bytes").arg(ctx.refImageData.size()));
    LOG_INFO("ImageService", QString("Reference image size: %1x%2")
        .arg(ctx.refImageWidth)
        .arg(ctx.refImageHeight));
    
    if (ctx.referenceImages.size() > 1) {
        LOG_INFO("ImageService", QString("Multiple reference images detected (%1), using first as primary")
            .arg(ctx.referenceImages.size()));
    }
    
    return true;
}

bool ImageService::generateImage(GenerationContext& ctx)
{
    emit progressChanged(tr("正在生成图像..."), 30);
    
    if (!loadReferenceImage(ctx)) {
        return false;
    }
    
    return executeImageGeneration(ctx);
}

bool ImageService::collectJimengReferenceUrls(const GenerationContext& ctx,
                                              VolcEngineImageClient::GenerateOptions& options)
{
    const int maxImages = qMin(ctx.referenceImages.size(), 10);
    for (int i = 0; i < maxImages; ++i) {
        const QString& refPath = ctx.referenceImages.at(i);
        const QString fullPath = FileStorage::instance()->getFullPath(refPath);
        const QByteArray refData = FileManager::readBinaryFile(fullPath);

        LOG_INFO("ImageService", QString("  参考图[%1]: path=%2, loaded=%3, size=%4bytes")
            .arg(i + 1).arg(refPath).arg(!refData.isEmpty() ? "成功" : "失败").arg(refData.size()));

        if (refData.isEmpty()) {
            continue;
        }

        const QString fileName = QStringLiteral("%1_%2.png").arg(ctx.panelId.left(8)).arg(i + 1);
        const ReferenceImageUploader::UploadResult uploadResult =
            ReferenceImageUploader::uploadImage(refData, fileName);
        if (!uploadResult.success) {
            setError(QStringLiteral("参考图上传失败: %1").arg(uploadResult.errorMessage));
            return false;
        }
        options.referenceImageUrls.append(uploadResult.url);
    }
    return true;
}

void ImageService::collectSeedEditReferenceImages(const GenerationContext& ctx,
                                                   VolcEngineImageClient::GenerateOptions& options)
{
    const int maxImages = qMin(ctx.referenceImages.size(), 5);
    for (int i = 0; i < maxImages; ++i) {
        const QString& refPath = ctx.referenceImages.at(i);
        const QString fullPath = FileStorage::instance()->getFullPath(refPath);
        const QByteArray refData = FileManager::readBinaryFile(fullPath);

        LOG_INFO("ImageService", QString("  参考图[%1]: path=%2, loaded=%3, size=%4bytes")
            .arg(i + 1).arg(refPath).arg(!refData.isEmpty() ? "成功" : "失败").arg(refData.size()));

        if (!refData.isEmpty()) {
            options.referenceImages.append(refData);
        }
    }
}

QString ImageService::buildJimengPromptPrefix(const GenerationContext& ctx,
                                               const VolcEngineImageClient::GenerateOptions& options)
{
    const QStringList refTypes = determineRefTypes(ctx.referenceImages, ctx.referenceImageTypes);
    QStringList imgLabels;
    for (int i = 0; i < options.referenceImageUrls.size(); ++i) {
        const QString refType = (i < refTypes.size()) ? refTypes.at(i) : QStringLiteral("AUTO");
        if (refType == QStringLiteral("IP")) {
            imgLabels.append(QString("图%1为角色外貌参考").arg(i + 1));
        } else if (refType == QStringLiteral("STYLE")) {
            imgLabels.append(QString("图%1为场景风格参考").arg(i + 1));
        } else {
            imgLabels.append(QString("图%1为参考图").arg(i + 1));
        }
    }
    return imgLabels.join("，") + "，漫画插画风格，";
}

bool ImageService::executeWithVolcEngine(GenerationContext& ctx)
{
    VolcEngineImageClient* client = ServiceContainer::instance()->volcEngineImageClient();
    if (!client || !client->isInitialized()) {
        setError(tr("VolcEngine客户端不可用"));
        return false;
    }

    const ProviderConfig config = getProviderConfig();
    const RequestedResolution resolution = requestedResolution(ctx.presetMode,
                                                               ctx.mode,
                                                               QStringLiteral("volcengine"),
                                                               config.previewWidth,
                                                               config.previewHeight,
                                                               config.hdWidth,
                                                               config.hdHeight);

    VolcEngineImageClient::GenerateOptions options;
    options.prompt = ctx.prompt;
    options.negativePrompt = ctx.negativePrompt;
    options.width = resolution.width;
    options.height = resolution.height;
    options.seed = -1;
    options.returnUrl = true;
    options.scale = 0.5f;

    const bool useJimengV40 = isJimengV40ReqKey(AppConfig::instance()->volcEngine().img2imgReqKey);

    if (ctx.allowReferenceEdit && !ctx.referenceImages.isEmpty()) {
        LOG_INFO("ImageService", QString("开始收集参考图: panelId=%1, 待收集数量=%2")
            .arg(ctx.panelId).arg(ctx.referenceImages.size()));

        if (useJimengV40) {
            if (!collectJimengReferenceUrls(ctx, options)) {
                return false;
            }
            if (!options.referenceImageUrls.isEmpty()) {
                const QString prefix = buildJimengPromptPrefix(ctx, options);
                options.prompt = prefix + options.prompt;
                LOG_INFO("ImageService", QString("即梦4.0 prompt前缀注入: %1").arg(prefix));
            }
        } else {
            collectSeedEditReferenceImages(ctx, options);
            options.refTypeList = determineRefTypes(ctx.referenceImages, ctx.referenceImageTypes);
            if (!options.refTypeList.isEmpty()) {
                LOG_INFO("ImageService", QString("  参考图类型(ref_type_list): %1").arg(options.refTypeList.join(", ")));
            }
        }

        LOG_INFO("ImageService", QString(
            "VolcEngine 请求准备完成: panelId=%1, 参考图=%2, imageUrls=%3, promptLen=%4")
            .arg(ctx.panelId).arg(options.referenceImages.size())
            .arg(options.referenceImageUrls.size()).arg(options.prompt.length()));
    } else {
        LOG_INFO("ImageService", QString(
            "VolcEngine 文生图: panelId=%1, promptLen=%2")
            .arg(ctx.panelId).arg(ctx.prompt.length()));
    }

    const VolcEngineImageClient::GenerateResult result = client->generate(options);
    if (!result.success) {
        setError(result.errorMessage);
        return false;
    }

    if (!result.imageUrl.isEmpty() && result.imageData.isEmpty()) {
        LOG_INFO("ImageService", QString("Downloading image from URL: %1").arg(result.imageUrl.left(100)));
        ctx.imageData = client->downloadImage(result.imageUrl);
        if (ctx.imageData.isEmpty()) {
            setError("Failed to download image from URL");
            return false;
        }
    } else {
        ctx.imageData = result.imageData;
    }

    return finalizeGeneratedImage(ctx, ctx.imageData) && !ctx.imageData.isEmpty();
}

bool ImageService::executeWithQwen(GenerationContext& ctx)
{
    QwenImageClient::GenerateResult result;
    const ProviderConfig config = getProviderConfig();
    const RequestedResolution requested = requestedResolution(ctx.presetMode,
                                                              ctx.mode,
                                                              QStringLiteral("qwen"),
                                                              config.previewWidth,
                                                              config.previewHeight,
                                                              config.hdWidth,
                                                              config.hdHeight);
    ResolutionConfig resolution;
    resolution.width = requested.width;
    resolution.height = requested.height;
    resolution.isPreset = requested.isPreset;
    
    if (ctx.allowReferenceEdit && !ctx.refImageData.isEmpty()) {
        result = QwenImageClient::instance()->edit(buildQwenEditOptions(ctx, resolution));
    } else {
        result = QwenImageClient::instance()->generate(buildQwenGenerateOptions(ctx, resolution));
    }

    if (!result.success) {
        setError(result.errorMessage);
        return false;
    }

    if (!result.imageData.isEmpty()) {
        return finalizeGeneratedImage(ctx, result.imageData);
    }

    if (!result.imageUrl.isEmpty()) {
        LOG_INFO("ImageService", QString("Downloading image from URL: %1").arg(result.imageUrl.left(100)));
        const QByteArray downloaded = QwenImageClient::instance()->downloadImage(result.imageUrl);
        if (downloaded.isEmpty()) {
            setError(tr("Failed to download image from URL"));
            return false;
        }
        return finalizeGeneratedImage(ctx, downloaded);
    }

    setError(tr("Qwen返回的结果没有图像数据"));
    return false;
}

QwenImageClient::GenerateOptions ImageService::buildQwenGenerateOptions(const GenerationContext& ctx,
                                                                        const ResolutionConfig& resolution) const
{
    QwenImageClient::GenerateOptions options;
    options.prompt = ctx.prompt;
    options.negativePrompt = ctx.negativePrompt;
    options.size = resolution.isPreset
        ? QwenImageClient::ImageSize::Custom
        : (ctx.mode == GenerateMode::HD
            ? QwenImageClient::ImageSize::Square
            : QwenImageClient::ImageSize::Portrait);
    options.width = resolution.width;
    options.height = resolution.height;
    return options;
}

QwenImageClient::EditOptions ImageService::buildQwenEditOptions(GenerationContext& ctx,
                                                                const ResolutionConfig& resolution) const
{
    QwenImageClient::EditOptions options;
    // 编辑操作只传原始指令，不传完整的6层prompt（含角色/场景描述会干扰编辑效果）
    options.prompt = (!ctx.editDirective.isEmpty()) ? ctx.editDirective : ctx.prompt;
    options.sourceImage = referenceImageForEdit(ctx.editSourceImageData, ctx.refImageData, ctx.faceOnlyEdit);
    // 编辑操作使用简洁的negative prompt，避免干扰编辑效果
    options.negativePrompt = QStringLiteral("nsfw, blurry, low quality, distorted, watermark");
    const EditDimensions editDimensions = resolveEditDimensions(
        ctx.faceOnlyEdit && ctx.editTargetRect.isValid() && !ctx.editTargetRect.isEmpty()
            ? ctx.editTargetRect.width()
            : ctx.refImageWidth,
        ctx.faceOnlyEdit && ctx.editTargetRect.isValid() && !ctx.editTargetRect.isEmpty()
            ? ctx.editTargetRect.height()
            : ctx.refImageHeight,
        resolution.width,
        resolution.height,
        resolution.isPreset,
        ctx.mode);
    options.size = editDimensions.size;
    options.width = editDimensions.width;
    options.height = editDimensions.height;
    applyFaceOnlyEditHints(ctx, options);
    return options;
}

bool ImageService::executeImageGeneration(GenerationContext& ctx)
{
    ProviderConfig config = getProviderConfig();
    QString operationName = ctx.refImageData.isEmpty() ? "generateImage" : "generateWithReference";
    
    LOG_INFO("ImageService", QString("图像提供商: %1, 有参考图数据: %2, 允许参考编辑: %3, 是否编辑操作: %4")
        .arg(config.name)
        .arg(!ctx.refImageData.isEmpty() ? "是" : "否")
        .arg(ctx.allowReferenceEdit ? "是" : "否")
        .arg((ctx.allowReferenceEdit && !ctx.refImageData.isEmpty()) ? "是(走千问编辑)" : "否(走生成)"));

    if (ctx.faceOnlyEdit && !ctx.refImageData.isEmpty() && !prepareFaceOnlyEditContext(ctx)) {
        setError(tr("无法准备局部编辑区域"));
        LOG_ERROR("ImageService", QString("Failed to prepare face-only edit context for panel: %1").arg(ctx.panelId));
        return false;
    }

    // 编辑操作判断：forceProvider="qwen" 时强制走千问（来自自然语言修改请求）；
    // 否则按原逻辑：sourceImagePath 在 referenceImages 首位时视为编辑操作
    const bool forceQwen = ctx.forceProvider == QStringLiteral("qwen");
    const bool isEditOperation = forceQwen
        || (!ctx.sourceImagePath.isEmpty()
            && !ctx.refImageData.isEmpty()
            && !ctx.referenceImages.isEmpty()
            && ctx.referenceImages.first() == ctx.sourceImagePath);
    if (isEditOperation && config.name == "volcengine") {
        LOG_INFO("ImageService", QString("编辑操作强制使用千问编辑API (当前配置: %1, forceQwen: %2)")
            .arg(config.name).arg(forceQwen ? "yes" : "no"));
    }

    bool success = m_apiRetryPolicy->executeWithRetry(
        [&]() -> bool {
            if (config.name == "volcengine" && !isEditOperation) {
                return executeWithVolcEngine(ctx);
            } else {
                return executeWithQwen(ctx);
            }
        },
        [&](int attempt, const QString& error) {
            LOG_WARNING("ImageService", 
                QString("%1 retry %2/%3: %4")
                    .arg(operationName)
                    .arg(attempt)
                    .arg(m_apiRetryPolicy->config().maxAttempts)
                    .arg(error));
        },
        [&](const QList<RetryAttempt>& attempts) {
            if (m_apiRetryPolicy->isFinalFailure()) {
                QString errorType = ctx.refImageData.isEmpty()
                    ? tr("图像生成失败")
                    : tr("参考图生成失败");
                setError(TR_FMT("%1 failed after %2 attempts: %3",
                    errorType,
                    QString::number(attempts.size()),
                    m_apiRetryPolicy->lastError()));
                
                LOG_ERROR("ImageService", 
                    QString("%1 failed after %2 attempts: %3")
                        .arg(operationName)
                        .arg(attempts.size())
                        .arg(m_apiRetryPolicy->lastError()));
            }
        }
    );

    return success;
}

bool ImageService::storeImage(GenerationContext& ctx)
{
    emit progressChanged(tr("正在保存图像..."), 70);

    try {
        ctx.s3Key = !ctx.presetMode.isEmpty()
            ? QString("panels/%1/%2_%3.png").arg(ctx.panelId.left(8)).arg(ctx.panelId).arg(ctx.presetMode)
            : generateS3Key(ctx.panelId, ctx.mode);

        QString modeStr = (ctx.mode == GenerateMode::Preview) ? "preview" : "hd";
        QString localPath = FileStorage::instance()->savePanelImage(ctx.panelId, ctx.imageData, modeStr);

        if (localPath.isEmpty()) {
            setError(TR_FMT("Local save failed: %1", FileStorage::instance()->lastError()));
            return false;
        }

        ctx.s3Key = localPath;
        LOG_INFO("ImageService", QString("Image stored locally: %1").arg(localPath));

        // hd 编辑结果同步写入 preview，确保 UI 能立即看到变化
        if (ctx.mode == GenerateMode::HD) {
            QString previewPath = FileStorage::instance()->savePanelImage(ctx.panelId, ctx.imageData, "preview");
            if (!previewPath.isEmpty()) {
                LOG_INFO("ImageService", QString("HD edit result synced to preview: %1").arg(previewPath));
            }
        }

        return true;
    } catch (...) {
        return executeWithErrorHandlingBool("storeImage",
            [&]() -> bool { throw; });
    }
}

bool ImageService::safeFetchPanel(GenerationContext& ctx)
{
    emit progressChanged(tr("正在获取面板数据..."), 10);
    bool success = m_dbRetryPolicy->executeWithRetry(
        [&]() -> bool {
            Panel panel = StoryboardService::instance()->getPanel(ctx.panelId);
            if (!panel.id().isEmpty()) {
                ctx.panel = panel;
                return true;
            }
            return false;
        },
        [&](int attempt, const QString& error) {
            LOG_WARNING("ImageService", 
                QString("safeFetchPanel retry %1/%2: %3")
                    .arg(attempt)
                    .arg(m_dbRetryPolicy->config().maxAttempts)
                    .arg(error));
        },
        [&](const QList<RetryAttempt>& attempts) {
            if (m_dbRetryPolicy->isFinalFailure()) {
                setError(TR_FMT("Database fetch failed after %1 attempts: %2",
                    QString::number(attempts.size()),
                    m_dbRetryPolicy->lastError()));
                
                LOG_ERROR("ImageService", 
                    QString("safeFetchPanel final failure, attempts=%1, error=%2")
                        .arg(attempts.size())
                        .arg(m_dbRetryPolicy->lastError()));
            }
        }
    );
    
    if (!success && !m_dbRetryPolicy->isFinalFailure()) {
        setError(TR_FMT("Failed to fetch panel %1", ctx.panelId));
    }
    
    return success;
}

bool ImageService::buildPromptForPanel(GenerationContext& ctx)
{
    emit progressChanged(tr("正在构建提示词..."), 20);

    try {
        QJsonObject panelJson = buildPanelJson(ctx.panel);

        QStringList panelCharacterNames;
        for (const auto& value : panelJson.value("characters").toArray()) {
            const QJsonObject charObj = value.toObject();
            const QString name = charObj.value("name").toString().trimmed();
            if (!name.isEmpty()) {
                panelCharacterNames.append(name);
            }
        }

        const QString sceneText = panelJson.value("scene").toString().trimmed();
        const QString visualPromptEn = panelJson.value("visualPromptEn").toString().trimmed();
        const QString visualPromptCn = panelJson.value("visualPromptCn").toString().trimmed();

        QString novelId = fetchNovelIdByStoryboardId(ctx.panel.storyboardId());
        LOG_DEBUG("ImageService", QString("buildPromptForPanel: resolved novelId=%1").arg(novelId));

        LOG_DEBUG("ImageService", QString("buildPromptForPanel: fetching reference data for panelId=%1").arg(ctx.panelId));
        PanelReferenceData refData = m_batchReferenceData.characterRefs.isEmpty()
            ? fetchPanelReferenceData(novelId)
            : m_batchReferenceData;
        LOG_DEBUG("ImageService", QString("buildPromptForPanel: reference data loaded, characters=%1, scenes=%2, refImages=%3")
            .arg(refData.characterRefs.size())
            .arg(refData.sceneRefs.size())
            .arg(refData.allAvailableRefImages.size()));

        // 兜底：LLM 偶尔会把"逆光剪影/远景出场"的角色漏出 panel.characters。
        // 只扫 visualPromptCn（画面描述），不扫 sceneText（叙事文本可能只是内心独白/回忆中提到角色）。
        {
            QJsonArray panelCharsArray = panelJson.value("characters").toArray();
            for (auto it = refData.characterRefs.constBegin();
                 it != refData.characterRefs.constEnd(); ++it) {
                const QString bibleName = it.key().trimmed();
                if (bibleName.isEmpty()) continue;
                if (panelCharacterNames.contains(bibleName)) continue;
                if (!visualPromptCn.contains(bibleName)) continue;
                QJsonObject augmented;
                augmented["name"] = bibleName;
                panelCharsArray.append(augmented);
                panelCharacterNames.append(bibleName);
                LOG_INFO("ImageService", QString("Auto-augmented missing character '%1' for panel %2 (matched in visualPromptCn)")
                    .arg(bibleName).arg(ctx.panelId));
            }
            panelJson["characters"] = panelCharsArray;
        }

        LOG_INFO("ImageService", QString(
            "buildPromptForPanel input: panelId=%1, page=%2, index=%3, scene='%4', visualPromptCn='%5', visualPromptEn='%6', characters=[%7]")
            .arg(ctx.panelId)
            .arg(ctx.panel.page())
            .arg(ctx.panel.index())
            .arg(sceneText.left(120))
            .arg(visualPromptCn.left(120))
            .arg(visualPromptEn.left(120))
            .arg(panelCharacterNames.join(", ")));

        LOG_INFO("ImageService", QString("buildPromptForPanel: panelId=%1, novelId=%2, characters=%3, scenes=%4")
            .arg(ctx.panelId).arg(novelId).arg(refData.characterRefs.size()).arg(refData.sceneRefs.size()));
        
        // 有参考图时直接启用图生图模式，避免后续二次构建 prompt
        // 但如果已有面板原图（编辑操作），不覆盖——编辑链路用原图，不用圣经参考图
        if (!refData.allAvailableRefImages.isEmpty() && !ctx.allowReferenceEdit
            && ctx.sourceImagePath.isEmpty()) {
            ctx.allowReferenceEdit = true;
            LOG_INFO("ImageService", QString("Enabling img2img mode due to character/scene reference images: %1")
                .arg(refData.allAvailableRefImages.first()));
        }

        // editDirective 需要在 allowReferenceEdit=true 后、构建 prompt 前赋值（用英文版给图像模型）
        // 当 editIntent 非空时（局部编辑操作），优先用 content["visualPrompt"]（调用方写入的编辑指令）
        // 否则用 visualPromptEn（面板原有英文场景描述）作为兜底
        const QString editIntent = panelJson.value("editIntent").toString().trimmed();
        const QString contentVisualPrompt = panelJson.value("visualPrompt").toString().trimmed();
        const QString visualPromptForEdit = (!editIntent.isEmpty() && !contentVisualPrompt.isEmpty())
            ? contentVisualPrompt
            : (visualPromptEn.isEmpty() ? contentVisualPrompt : visualPromptEn);
        if (ctx.allowReferenceEdit && !visualPromptForEdit.isEmpty()) {
            ctx.editDirective = visualPromptForEdit;
        }

        QJsonObject options = buildPanelPromptOptions(ctx, panelJson);
        LOG_DEBUG("ImageService", QString("buildPromptForPanel: invoking PromptBuilder for panelId=%1").arg(ctx.panelId));
        PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panelJson, refData.characterRefs, refData.sceneRefs, options);

        LOG_DEBUG("ImageService", QString("buildPromptForPanel: PromptBuilder returned for panelId=%1").arg(ctx.panelId));

        ctx.prompt = result.text;
        ctx.negativePrompt = result.negativePrompt;
        ctx.referenceImages = result.referenceImages;
        ctx.referenceImageTypes = result.referenceImageTypes;
        if (!result.referenceImages.isEmpty()) {
            ctx.primaryReferenceImagePath = result.referenceImages.first();
        }

        // 编辑操作：确保面板原图排在参考图列表最前面
        // buildPromptForPanel 会覆盖 ctx.referenceImages，导致 prepareEditReferenceImages
        // 预置的原图路径丢失，进而使 loadReferenceImage 无法识别原图，编辑操作退化为生成操作
        if (ctx.allowReferenceEdit && !ctx.sourceImagePath.isEmpty()
            && !ctx.referenceImages.contains(ctx.sourceImagePath)) {
            ctx.referenceImages.prepend(ctx.sourceImagePath);
            LOG_INFO("ImageService", QString("编辑模式：将面板原图补回参考图列表首位: %1").arg(ctx.sourceImagePath));
        }

        LOG_INFO("ImageService", QString("Panel reference images selected: panelId=%1, needed=%2, total=%3")
            .arg(ctx.panelId)
            .arg(result.referenceImages.size())
            .arg(refData.allAvailableRefImages.size()));
        if (!result.referenceImages.isEmpty()) {
            for (int i = 0; i < result.referenceImages.size(); ++i) {
                LOG_INFO("ImageService", QString("  Panel ref[%1]: %2").arg(i+1).arg(result.referenceImages.at(i)));
            }
        }

        limitReferenceImagesForProvider(ctx);
        
        if (ctx.prompt.isEmpty()) {
            setError(tr("提示词构建为空"));
            return false;
        }

        const QString promptPreview = ctx.prompt.left(2000).replace('\n', ' ').replace('\r', ' ');
        const QString negativePreview = ctx.negativePrompt.left(500).replace('\n', ' ').replace('\r', ' ');
        LOG_INFO("ImageService", QString("Prompt built: len=%1, refImages=%2, prompt=%3...")
            .arg(ctx.prompt.length()).arg(ctx.referenceImages.size()).arg(ctx.prompt.left(100)));
        LOG_DEBUG("ImageService", QString("Final panel prompt: panelId=%1, promptLen=%2, negativeLen=%3, prompt=%4")
            .arg(ctx.panelId)
            .arg(ctx.prompt.length())
            .arg(ctx.negativePrompt.length())
            .arg(promptPreview));
        LOG_DEBUG("ImageService", QString("Final panel negative prompt: panelId=%1, negativePrompt=%2")
            .arg(ctx.panelId)
            .arg(negativePreview));

        return true;
    } catch (...) {
        return executeWithErrorHandlingBool("buildPromptForPanel",
            [&]() -> bool { throw; });
    }
}

QJsonObject ImageService::buildPanelPromptOptions(const GenerationContext& ctx, const QJsonObject& panelJson) const
{
    QJsonObject options;
    options["mode"] = !ctx.presetMode.isEmpty()
        ? ctx.presetMode
        : ((ctx.mode == GenerateMode::HD) ? "hd" : "preview");
    options["promptTarget"] = PromptTargetUtils::classifyPanelPromptTarget(panelJson);
    
    // 🔑 关键修复：确保 isImg2Img 参数根据实际模式正确设置
    // 原来使用 ctx.allowReferenceEdit 可能导致 img2img 模式下 isImg2Img=false
    // 从而让 PromptBuilder 走文生图分支（不生成 [气泡]）
    // 
    // 判断逻辑：
    // - presetMode 不为空 → 批量生成模式（如图生图）→ isImg2Img=true
    // - allowReferenceEdit=true → 允许参考图编辑 → isImg2Img=true
    // - mode=HD → 高清模式通常是图生图 → isImg2Img=true
    bool shouldBeImg2Img = (!ctx.presetMode.isEmpty() ||   // 有预设模式 = 图生图
                           ctx.allowReferenceEdit ||         // 允许参考编辑 = 图生图
                           ctx.mode == GenerateMode::HD);     // HD 模式通常也是图生图
    
    options["isImg2Img"] = shouldBeImg2Img;
    
    LOG_DEBUG("ImageService", QString("buildPromptForPanel: mode=%1, isImg2Img=%2, allowRefEdit=%3")
        .arg(static_cast<int>(ctx.mode))
        .arg(options["isImg2Img"].toBool() ? "true" : "false")
        .arg(ctx.allowReferenceEdit ? "true" : "false"));

    if (ctx.presetMode.isEmpty()) {
        return options;
    }

    options["aspectRatio"] = ctx.presetMode;
    if (ctx.presetMode == QStringLiteral("1x1") || ctx.presetMode == QStringLiteral("1:1")) {
        options["composition"] = QStringLiteral("square");
    }
    return options;
}

void ImageService::appendUniqueReferenceImages(QStringList& target, const QStringList& source) const
{
    for (const QString& ref : source) {
        if (!ref.isEmpty() && !target.contains(ref)) {
            target.append(ref);
        }
    }
}

QString ImageService::selectPrimaryReferenceImagePath(const GenerationContext& ctx) const
{
    if (!ctx.primaryReferenceImagePath.isEmpty()) {
        return ctx.primaryReferenceImagePath;
    }

    if (!ctx.sourceImagePath.isEmpty()) {
        return ctx.sourceImagePath;
    }

    return ctx.referenceImages.isEmpty() ? QString() : ctx.referenceImages.first();
}

void ImageService::limitReferenceImagesForProvider(GenerationContext& ctx) const
{
    const ProviderConfig config = getProviderConfig();
    const QString primaryRef = selectPrimaryReferenceImagePath(ctx);
    if (primaryRef.isEmpty()) {
        if (config.name == QLatin1String("volcengine")) {
            ctx.referenceImages.clear();
        }
        return;
    }

    if (config.name == QLatin1String("volcengine")) {
        // VolcEngine 多图版 API 支持最多5张参考图
        // 确保 primaryRef 在第一位
        QStringList orderedRefs;
        orderedRefs.append(primaryRef);
        
        for (const QString& ref : ctx.referenceImages) {
            if (!orderedRefs.contains(ref) && orderedRefs.size() < 5) {
                orderedRefs.append(ref);
            }
        }
        
        ctx.referenceImages = orderedRefs;
        ctx.primaryReferenceImagePath = primaryRef;
    }
}

QJsonObject ImageService::buildCharacterRef(const Character& ch, QStringList& outPortraitPaths,
                                            const QMap<QString, QList<CharacterPortraitVersion>>& allVersions)
{
    QJsonObject charJson;
    charJson["id"] = ch.id();
    charJson["name"] = ch.name();
    charJson["role"] = ch.role();

    QStringList portraitPaths;
    if (!ch.portraitPath().isEmpty()) {
        portraitPaths.append(ch.portraitPath());
    }
    charJson["portraitPath"] = ch.portraitPath();
    charJson["portraitPaths"] = QJsonArray::fromStringList(portraitPaths);

    CharacterAppearance app = ch.appearance();

    // Prefer the appearance snapshot bound to the current portrait version so that
    // the prompt description matches the actual reference image.
    const QString versionId = ch.currentPortraitVersionId();
    if (!versionId.isEmpty()) {
        const QList<CharacterPortraitVersion>& versions = allVersions.contains(ch.id())
            ? allVersions[ch.id()]
            : CharacterExtractor::instance()->loadPortraitVersions(ch.id());
        for (const CharacterPortraitVersion& v : versions) {
            if (v.id() == versionId && !v.appearanceSnapshot().isEmpty()) {
                app = CharacterAppearance::fromJson(v.appearanceSnapshot());
                break;
            }
        }
    }

    charJson["appearance"] = app.toPromptJson();

    for (const QString& path : portraitPaths) {
        if (!path.isEmpty() && !outPortraitPaths.contains(path)) {
            outPortraitPaths.append(path);
        }
    }

    return charJson;
}

QMap<QString, QJsonObject> ImageService::fetchCharacterRefs(const QString& novelId, QStringList& outReferenceImages)
{
    QMap<QString, QJsonObject> characterRefs;

    if (novelId.isEmpty()) {
        LOG_WARNING("ImageService", "fetchCharacterRefs: novelId is empty");
        return characterRefs;
    }

    QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(novelId);

    // Load all portrait versions in one query to avoid N per-character round-trips.
    const QMap<QString, QList<CharacterPortraitVersion>> allVersions =
        CharacterExtractor::instance()->loadPortraitVersionsByNovel(novelId);

    for (const Character& ch : characters) {
        QJsonObject charJson = buildCharacterRef(ch, outReferenceImages, allVersions);
        characterRefs[ch.name()] = charJson;
    }

    QStringList charNames = characterRefs.keys();
    LOG_INFO("ImageService", QString("fetchCharacterRefs: %1 chars [%2], refImages=%3")
        .arg(characters.size()).arg(charNames.join(", ")).arg(outReferenceImages.size()));

    return characterRefs;
}

QJsonObject ImageService::buildSceneRef(const Scene& scene, QStringList& outReferenceImages)
{
    QJsonObject sceneJson;
    sceneJson["id"] = scene.id();
    sceneJson["name"] = scene.name();
    sceneJson["sceneId"] = scene.sceneId();
    
    SceneDetails details = scene.details();
    QJsonObject detailsJson;
    detailsJson["description"] = details.description;
    detailsJson["building"] = details.building;
    detailsJson["color"] = details.color;
    detailsJson["landmark"] = details.landmark;
    detailsJson["layout"] = details.layout;
    detailsJson["atmosphere"] = details.atmosphere;
    detailsJson["type"] = details.type;
    detailsJson["setting"] = details.setting;
    detailsJson["timeOfDay"] = details.timeOfDay;
    detailsJson["weather"] = details.weather;
    detailsJson["details"] = QJsonArray::fromStringList(details.details);
    sceneJson["details"] = detailsJson;
    
    QString referenceImagePath = scene.referenceImagePath();
    sceneJson["referenceImagePath"] = referenceImagePath;
    if (!referenceImagePath.isEmpty() && !outReferenceImages.contains(referenceImagePath)) {
        outReferenceImages.append(referenceImagePath);
    }
    
    return sceneJson;
}

QMap<QString, QJsonObject> ImageService::fetchSceneRefs(const QString& novelId, QStringList& outReferenceImages)
{
    QMap<QString, QJsonObject> sceneRefs;
    
    if (novelId.isEmpty()) {
        return sceneRefs;
    }
    
    QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(novelId);
    for (const Scene& scene : scenes) {
        QJsonObject sceneJson = buildSceneRef(scene, outReferenceImages);
        // 多形态索引：id(UUID) / sceneId(稳定key) / name(展示名) / 规范化name / 别名
        // 配合查询侧（PromptBuilder）的规范化匹配，让"拾光书店内景/门口/柜台"等子位置
        // 都能命中同一个"拾光书店"圣经条目
        auto registerKey = [&sceneRefs, &sceneJson](const QString& key) {
            const QString trimmed = key.trimmed();
            if (trimmed.isEmpty() || sceneRefs.contains(trimmed)) {
                return;
            }
            sceneRefs[trimmed] = sceneJson;
        };
        registerKey(scene.id());
        registerKey(scene.sceneId());
        registerKey(scene.name());
        registerKey(SceneKeyUtils::normalizeSceneLabel(scene.name()));
        const QStringList aliasKeys = SceneKeyUtils::buildSceneAliasKeys(scene.name(), scene.details());
        for (const QString& key : aliasKeys) {
            registerKey(key);
        }
    }
    
    QStringList sceneNames = sceneRefs.keys();
    LOG_INFO("ImageService", QString("fetchSceneRefs: %1 scenes [%2], refImages=%3")
        .arg(scenes.size()).arg(sceneNames.join(", ")).arg(outReferenceImages.size()));
    
    return sceneRefs;
}

/**
 * @brief 获取面板生成所需的参考图数据
 * 
 * 该函数从小说中提取所有角色和场景的参考信息，用于后续的面板生成。
 * 
 * 业务逻辑：
 * 1. 获取小说中的所有角色信息（包括参考图路径）
 * 2. 获取小说中的所有场景信息（包括参考图路径）
 * 3. 收集所有可用的参考图路径到 allAvailableRefImages
 * 
 * 注意：这里收集的是所有可用的参考图，实际使用时由 PromptBuilder 智能选择
 * 面板实际需要的参考图（通过 ctx.referenceImages = result.referenceImages）
 * 
 * @param novelId 小说ID
 * @return PanelReferenceData 包含角色、场景参考信息和所有可用参考图路径
 */
ImageService::PanelReferenceData ImageService::fetchPanelReferenceData(const QString& novelId)
{
    PanelReferenceData data;
    
    if (novelId.isEmpty()) {
        LOG_WARNING("ImageService", "fetchPanelReferenceData: novelId is empty");
        return data;
    }
    
    data.characterRefs = fetchCharacterRefs(novelId, data.allAvailableRefImages);
    data.sceneRefs = fetchSceneRefs(novelId, data.allAvailableRefImages);
    
    LOG_INFO("ImageService", QString("fetchPanelReferenceData: novelId=%1, characters=%2, scenes=%3, totalRefImages=%4")
        .arg(novelId)
        .arg(data.characterRefs.size())
        .arg(data.sceneRefs.size())
        .arg(data.allAvailableRefImages.size()));
    
    return data;
}

QString ImageService::fetchNovelIdByStoryboardId(const QString& storyboardId)
{
    if (storyboardId.isEmpty()) {
        return QString();
    }
    
    DatabaseManager *db = DatabaseManager::instance();
    if (!db) {
        return QString();
    }
    
    QString result;
    
    m_dbRetryPolicy->executeWithRetry(
        [&]() -> bool {
            const QVariantMap row = db->selectOne(
                QStringLiteral("storyboards"),
                QStringLiteral("id = ?"),
                QVariantList{storyboardId});
            result = row.value(QStringLiteral("novel_id")).toString();
            return !result.isEmpty();
        },
        [&](int attempt, const QString& error) {
            LOG_WARNING("ImageService", 
                QString("fetchNovelIdByStoryboardId retry %1/%2: %3")
                    .arg(attempt)
                    .arg(m_dbRetryPolicy->config().maxAttempts)
                    .arg(error));
        },
        [&](const QList<RetryAttempt>& attempts) {
            if (m_dbRetryPolicy->isFinalFailure()) {
                LOG_ERROR("ImageService", 
                    QString("fetchNovelIdByStoryboardId failed after %1 attempts: %2")
                        .arg(attempts.size())
                        .arg(m_dbRetryPolicy->lastError()));
            }
        }
    );
    
    return result;
}

bool ImageService::safeUpdateDatabase(GenerationContext& ctx)
{
    emit progressChanged(tr("正在完成..."), 90);

    QJsonObject content = ctx.panel.rawContent();

    if (!ctx.presetMode.isEmpty()) {
        content["previewS3Key"] = ctx.s3Key;
        content["hdS3Key"] = ctx.s3Key;
    } else if (ctx.mode == GenerateMode::Preview) {
        content["previewS3Key"] = ctx.s3Key;
    } else {
        content["hdS3Key"] = ctx.s3Key;
    }
    content["status"] = "completed";
    content["generatedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    bool success = m_dbRetryPolicy->executeWithRetry(
        [&]() -> bool {
            return StoryboardService::instance()->updatePanel(ctx.panelId, content);
        },
        [&](int attempt, const QString& error) {
            LOG_WARNING("ImageService", 
                QString("safeUpdateDatabase retry %1/%2: %3")
                    .arg(attempt)
                    .arg(m_dbRetryPolicy->config().maxAttempts)
                    .arg(error));
        },
        [&](const QList<RetryAttempt>& attempts) {
            if (m_dbRetryPolicy->isFinalFailure()) {
                setError(TR_FMT("Database update failed (retry %1 times): %2", 
                    QString::number(attempts.size()),
                    m_dbRetryPolicy->lastError()));
                
                LOG_ERROR("ImageService", 
                    QString("safeUpdateDatabase final failure, attempts=%1, error=%2")
                        .arg(attempts.size())
                        .arg(m_dbRetryPolicy->lastError()));
            }
        }
    );
    
    if (!success && !m_dbRetryPolicy->isFinalFailure()) {
        setError(tr("数据库更新失败，正在重试..."));
    }
    
    return success;
}

ImageService::GenerateResult ImageService::finalizeResult(GenerationContext& ctx)
{
    emit progressChanged(tr("\u751f\u6210\u5b8c\u6210"), 100);
    
    GenerateResult result;
    result.success = true;
    result.panelId = ctx.panelId;
    result.s3Key = ctx.s3Key;
    
    try {
        result.imageUrl = FileStorage::instance()->getFullPath(ctx.s3Key);
    } catch (...) {
        result.imageUrl = FileStorage::instance()->getFullPath(ctx.s3Key);
    }
    
    if (!ctx.imageData.isEmpty()) {
        QImage image;
        if (image.loadFromData(ctx.imageData)) {
            result.width = image.width();
            result.height = image.height();
        }
    }
    
    result.timestamp = QDateTime::currentMSecsSinceEpoch();

    return result;
}

ImageService::GenerateResult ImageService::createErrorResult(const QString& panelId, const QString& message)
{
    GenerateResult result;
    result.success = false;
    result.panelId = panelId;
    result.errorMessage = message;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    return result;
}

QString ImageService::presetModeToString(BatchPresetMode presetMode) const
{
    return ImageModeUtils::presetModeString(presetMode);
}

ImageService::BatchPresetMode ImageService::presetModeFromString(const QString& presetMode) const
{
    return ImageModeUtils::presetModeFromString(presetMode);
}

void ImageService::getPresetResolution(BatchPresetMode presetMode, int& width, int& height) const
{
    applyPresetResolutionForProvider(
        presetMode,
        AppConfig::instance()->imageService().provider,
        width,
        height);
}

void ImageService::getPresetResolution(const QString& presetMode, int& width, int& height) const
{
    getPresetResolution(ImageModeUtils::presetModeFromString(presetMode), width, height);
}

QString ImageService::enqueuePanelImageGeneration(const QString& panelId, GenerateMode mode)
{
    return enqueuePanelTask(panelId, TaskType::GeneratePanelImage, ImageModeUtils::generateModeString(mode));
}

QString ImageService::enqueuePanelImageGeneration(const QString& panelId, BatchPresetMode presetMode)
{
    return enqueuePanelTask(panelId, TaskType::GeneratePanelImage, ImageModeUtils::presetModeString(presetMode));
}

QString ImageService::enqueueBatchPanelImageGeneration(const QStringList& panelIds, GenerateMode mode)
{
    return panelIds.isEmpty()
        ? QString()
        : enqueuePanelTask(QString(), TaskType::GeneratePanels, ImageModeUtils::generateModeString(mode), panelIds);
}

QString ImageService::enqueueBatchPanelImageGeneration(const QStringList& panelIds, BatchPresetMode presetMode)
{
    return panelIds.isEmpty()
        ? QString()
        : enqueuePanelTask(QString(), TaskType::GeneratePanels, ImageModeUtils::presetModeString(presetMode), panelIds);
}

QStringList ImageService::enqueuePanelImageGenerations(const QStringList& panelIds, GenerateMode mode)
{
    QStringList taskIds;
    taskIds.reserve(panelIds.size());

    for (const QString& panelId : panelIds) {
        taskIds.append(enqueuePanelImageGeneration(panelId, mode));
    }
    return taskIds;
}

QStringList ImageService::enqueuePanelImageGenerations(const QStringList& panelIds, BatchPresetMode presetMode)
{
    QStringList taskIds;
    taskIds.reserve(panelIds.size());

    for (const QString& panelId : panelIds) {
        taskIds.append(enqueuePanelImageGeneration(panelId, presetMode));
    }
    return taskIds;
}

QJsonObject ImageService::handleGeneratePanelImageTask(const QJsonObject& taskJson)
{
    TaskData task = TaskData::fromJson(taskJson);

    const QString modeStr = task.params.value("mode").toString();
    if (isPresetTaskMode(modeStr)) {
        return generateResultToJson(generatePanelImage(task.panelId, ImageModeUtils::presetModeFromString(modeStr)));
    }

    return generateResultToJson(generatePanelImage(task.panelId, parseTaskMode(modeStr)));
}

QJsonObject ImageService::buildPanelJson(const Panel& panel)
{
    QJsonObject json = panel.content();
    if (!json.contains("id")) {
        json["id"] = panel.id();
    }
    if (!json.contains("page")) {
        json["page"] = panel.page();
    }
    if (!json.contains("index")) {
        json["index"] = panel.index();
    }
    if (!json.contains("status")) {
        json["status"] = panel.status();
    }
    return json;
}

QString ImageService::generateS3Key(const QString& panelId, GenerateMode mode)
{
    QString modeStr = (mode == GenerateMode::Preview) ? "preview" : "hd";
    return QString("panels/%1/%2_%3.png")
        .arg(panelId.left(8))
        .arg(panelId)
        .arg(modeStr);
}

ImageService::GenerateResult ImageService::generateWithRetryInternal(const QString& panelId, 
                                                       const ResolutionConfig& resolution, 
                                                       int maxRetries)
{
    GenerateResult result;
    int retryCount = 0;

    while (retryCount < maxRetries) {
        // 检查对象是否已被销毁
        if (m_destroyed.load()) {
            LOG_WARNING("ImageService", "generateWithRetryInternal: Object destroyed, aborting");
            return createErrorResult(panelId, tr("服务已关闭"));
        }

        // 检查是否已取消（更频繁地检查，提高响应速度）
        {
            // 🔧 优化：使用原子变量检查取消状态，无需加锁
            if (m_batchCancelled.load() || isCancelled()) {
                LOG_INFO("ImageService", QString("generateWithRetryInternal: Generation cancelled for panel %1").arg(panelId));
                return createErrorResult(panelId, tr("用户取消了操作"));
            }
        }

        result = generatePanelImageInternal(panelId, resolution);

        if (result.success) {
            return result;
        }

        retryCount++;
        if (retryCount < maxRetries) {
            int delaySeconds = calculateRetryDelay(result.errorMessage, retryCount);
            
            if (isRateLimitError(result.errorMessage)) {
                LOG_WARNING("ImageService", QString("[限流保护] Panel %1 API限流 (attempt %2/%3), retrying in %4s...")
                    .arg(panelId).arg(retryCount).arg(maxRetries).arg(delaySeconds));
            } else if (isQueueTimeoutError(result.errorMessage)) {
                LOG_WARNING("ImageService", QString("[队列超时] Panel %1 轮询超时，服务器队列可能拥堵 (attempt %2/%3), retrying in %4s...")
                    .arg(panelId).arg(retryCount).arg(maxRetries).arg(delaySeconds));
            } else {
                LOG_WARNING("ImageService", QString("Panel %1 generation failed (attempt %2/%3): %4, retrying in %5s...")
                    .arg(panelId).arg(retryCount).arg(maxRetries).arg(result.errorMessage).arg(delaySeconds));
            }
            
            int totalWaitMs = delaySeconds * 1000;
            int waitedMs = 0;

            while (waitedMs < totalWaitMs) {
                // 每100ms检查一次取消状态
                if (m_destroyed.load()) {
                    LOG_WARNING("ImageService", "generateWithRetryInternal: Object destroyed during retry wait");
                    return createErrorResult(panelId, tr("服务已关闭"));
                }

                {
                    // 🔧 优化：使用原子变量检查取消状态
                    if (m_batchCancelled.load() || isCancelled()) {
                        LOG_INFO("ImageService", QString("generateWithRetryInternal: Cancelled during retry wait for panel %1").arg(panelId));
                        return createErrorResult(panelId, tr("用户取消了操作"));
                    }
                }

                QThread::msleep(20);  // 缩短到 20ms，提高响应性
                waitedMs += 20;
            }
        }
    }

    LOG_ERROR("ImageService", QString("Panel %1 generation failed after %2 attempts: %3")
        .arg(panelId).arg(maxRetries).arg(result.errorMessage));

    return result;
}

ImageService::GenerateResult ImageService::generateWithRetry(const QString& panelId, GenerateMode mode, int maxRetries)
{
    return generateWithRetryInternal(panelId, ResolutionConfig::fromMode(mode), maxRetries);
}

ImageService::GenerateResult ImageService::generateWithRetry(const QString& panelId, const QString& presetMode, int maxRetries)
{
    return generateWithRetryInternal(panelId, ResolutionConfig::fromPreset(presetModeFromString(presetMode)), maxRetries);
}

void ImageService::processNextPanel()
{
    if (m_destroyed.load()) {
        LOG_WARNING("ImageService", "Object destroyed, skipping panel processing");
        return;
    }

    QString panelId;
    int currentIndex = 0;
    int total = 0;
    ResolutionConfig resolution;

    const bool hasMorePanels = [this, &panelId, &currentIndex, &total, &resolution]() -> bool {
        QMutexLocker locker(&m_mutex);
        return takeNextBatchItem(panelId, currentIndex, total, resolution);
    }();

    if (!hasMorePanels) {
        completeBatchGeneration();
        return;
    }

    emit batchProgressChanged(currentIndex + 1, total, tr("正在处理..."));

    const QFuture<void> future = QtConcurrent::run(this, &ImageService::processPanelAsync, panelId, resolution);

    {
        QMutexLocker locker(&m_mutex);
        m_runningTasks.append(future);

        for (auto it = m_runningTasks.begin(); it != m_runningTasks.end(); ) {
            if (it->isFinished()) {
                it = m_runningTasks.erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool ImageService::shouldAdvanceToNextBatchItem() const
{
    return !m_batchCancelled.load() && m_currentProcessIndex.load() < m_pendingPanelIds.size();
}


void ImageService::queueNextBatchItemProcessing()
{
    LOG_INFO("ImageService", "queueNextBatchItemProcessing: invoked, scheduling next panel...");

    // 检查对象是否有效
    if (m_destroyed.load()) {
        LOG_WARNING("ImageService", "queueNextBatchItemProcessing: Object is destroyed, skipping");
        return;
    }

    QTimer::singleShot(kBatchNextItemDelayMs, this, &ImageService::processNextPanel);
}

void ImageService::processPanelAsync(const QString& panelId, const ResolutionConfig& resolution)
{
    // 检查对象是否已被销毁，防止访问已失效的对象
    if (m_destroyed.load()) {
        LOG_WARNING("ImageService", "processPanelAsync: Object is being destroyed, aborting");
        return;
    }

    const int maxRetries = kBatchPanelMaxRetries;
    GenerateResult result = generateWithRetryInternal(panelId, resolution, maxRetries);

    bool cancelled = false;
    {
        QMutexLocker locker(&m_mutex);
        cancelled = m_batchCancelled.load();  // 🔧 原子读取

        // 再次检查对象有效性（可能在处理过程中被销毁）
        if (m_destroyed.load()) {
            LOG_WARNING("ImageService", "processPanelAsync: Object destroyed during processing");
            return;
        }

        if (!cancelled) {
            recordBatchItemResult(panelId, result, maxRetries);
            int newIndex = m_currentProcessIndex.fetch_add(1) + 1;  // 🔧 原子递增
            LOG_INFO("ImageService", QString("recordBatchItemResult: panelId=%1, currentIndex=%2/%3")
                .arg(panelId).arg(newIndex).arg(m_pendingPanelIds.size()));
        }
    }

    // 使用 QMetaObject::invokeMethod 确保在主线程调用，避免跨线程问题
    QMetaObject::invokeMethod(this, [this]() {
        queueNextBatchItemProcessing();
    }, Qt::QueuedConnection);
}
