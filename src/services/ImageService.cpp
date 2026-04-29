#include "services/ImageService.h"
#include "services/ServiceContainer.h"
#include "data/DatabaseManager.h"
#include "utils/SingletonUtils.h"
#include "utils/PromptBuilder.h"
#include "utils/ImageModeUtils.h"
#include "utils/FaceEditPromptUtils.h"
#include "utils/LocalEditPromptUtils.h"
#include "utils/PromptTargetUtils.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "api/StorageClient.h"
#include "data/FileStorage.h"
#include "services/StoryboardService.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "services/TaskQueue.h"
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
#include <QtConcurrent>
#include <QThread>
#include <QTimer>
#include <cmath>

namespace {
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

QImage loadImageFromBytes(const QByteArray& data)
{
    QImage image;
    image.loadFromData(data);
    if (image.isNull()) {
        return image;
    }
    return image.convertToFormat(QImage::Format_ARGB32);
}

QByteArray blendEditedCropIntoOriginalImpl(const QByteArray& originalImage,
                                           const QByteArray& editedImage,
                                           const QRect& cropRect,
                                           const QByteArray& maskImage)
{
    if (originalImage.isEmpty() || editedImage.isEmpty() || !cropRect.isValid() || cropRect.isEmpty()) {
        return editedImage.isEmpty() ? originalImage : editedImage;
    }

    const QImage original = loadImageFromBytes(originalImage);
    QImage edited = loadImageFromBytes(editedImage);
    const QImage mask = loadImageFromBytes(maskImage);

    if (original.isNull() || edited.isNull()) {
        return editedImage;
    }

    const QRect targetRect = cropRect.intersected(original.rect());
    if (targetRect.isEmpty()) {
        return editedImage;
    }

    const QSize targetSize = targetRect.size();
    if (edited.size() != targetSize) {
        edited = edited.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_ARGB32);
    } else {
        edited = edited.convertToFormat(QImage::Format_ARGB32);
    }

    QImage normalizedMask;
    if (!mask.isNull() && mask.size() == targetSize) {
        normalizedMask = mask.convertToFormat(QImage::Format_ARGB32);
    } else if (!mask.isNull() && !mask.size().isEmpty()) {
        normalizedMask = mask.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_ARGB32);
    } else {
        normalizedMask = QImage();
    }

    const QImage originalArgb = original.convertToFormat(QImage::Format_ARGB32);
    QImage blended = originalArgb;
    const bool hasMask = !normalizedMask.isNull();

    for (int y = 0; y < targetSize.height(); ++y) {
        const int originalY = targetRect.y() + y;
        const QRgb* originalLine = reinterpret_cast<const QRgb*>(originalArgb.constScanLine(originalY));
        const QRgb* editedLine = reinterpret_cast<const QRgb*>(edited.constScanLine(y));
        const QRgb* maskLine = hasMask ? reinterpret_cast<const QRgb*>(normalizedMask.constScanLine(y)) : nullptr;
        QRgb* blendedLine = reinterpret_cast<QRgb*>(blended.scanLine(originalY));

        for (int x = 0; x < targetSize.width(); ++x) {
            const int originalX = targetRect.x() + x;
            const int alpha = hasMask ? qAlpha(maskLine[x]) : 255;
            const int inverseAlpha = 255 - alpha;
            const int r = (qRed(editedLine[x]) * alpha + qRed(originalLine[originalX]) * inverseAlpha) / 255;
            const int g = (qGreen(editedLine[x]) * alpha + qGreen(originalLine[originalX]) * inverseAlpha) / 255;
            const int b = (qBlue(editedLine[x]) * alpha + qBlue(originalLine[originalX]) * inverseAlpha) / 255;
            const int a = qMax(qAlpha(originalLine[originalX]), qAlpha(editedLine[x]));
            blendedLine[originalX] = qRgba(r, g, b, a);
        }
    }

    return imageToPngBytes(blended);
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
    if (providerName == QLatin1String("volcengine")) {
        switch (presetMode) {
            case ImageService::BatchPresetMode::Square_1x1:
                width = 1328;
                height = 1328;
                break;
            case ImageService::BatchPresetMode::Standard_3x2:
                width = 1584;
                height = 1056;
                break;
            case ImageService::BatchPresetMode::Widescreen_16x9:
                width = 1664;
                height = 936;
                break;
        }
        return;
    }

    switch (presetMode) {
        case ImageService::BatchPresetMode::Square_1x1:
            width = 1024;
            height = 1024;
            break;
        case ImageService::BatchPresetMode::Standard_3x2:
            width = 1536;
            height = 1024;
            break;
        case ImageService::BatchPresetMode::Widescreen_16x9:
            width = 1280;
            height = 720;
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
    cancelCurrentBatch();
}

void ImageService::cancelCurrentBatch()
{
    QMutexLocker locker(&m_mutex);
    m_batchCancelled = true;
    m_pendingPanelIds.clear();
    m_currentProcessIndex = 0;
    m_currentPresetMode.clear();
    
    setGenerating(false);
    m_batchResult = BatchResult();
    
    emit batchGenerationCancelled();
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
    } catch (const std::exception& e) {
        QString errorMsg = QString::fromUtf8(e.what());
        LOG_ERROR("ImageService", QString("Exception during generation of panel %1: %2").arg(panelId).arg(errorMsg));
        resetState();
        return createErrorResult(panelId, errorMsg);
    } catch (...) {
        LOG_ERROR("ImageService", QString("Unknown exception during generation of panel %1").arg(panelId));
        resetState();
        return createErrorResult(panelId, tr("生成过程中发生未知错误"));
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
        m_currentProcessIndex = 0;
        m_batchCancelled = false;

        m_batchResult = BatchResult();
        m_batchResult.totalCount = panelIds.size();
    }

    startBatch(panelIds.size());
    LOG_INFO("ImageService", QString("Starting batch generation for %1 panels").arg(panelIds.size()));

    QTimer::singleShot(0, this, &ImageService::processNextPanel);
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
        && !ctx.editMaskData.isEmpty()) {
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
    options.maskImage = ctx.editMaskData;
    options.size = QwenImageClient::ImageSize::Custom;
    options.width = ctx.refImageWidth > 0 ? ctx.refImageWidth : ctx.editTargetRect.width();
    options.height = ctx.refImageHeight > 0 ? ctx.refImageHeight : ctx.editTargetRect.height();
}

bool ImageService::applyFaceOnlyEditPostProcess(GenerationContext& ctx)
{
    if (!ctx.faceOnlyEdit || ctx.refImageData.isEmpty() || ctx.imageData.isEmpty()) {
        return true;
    }

    if (!ctx.editTargetRect.isValid() || ctx.editTargetRect.isEmpty()) {
        return true;
    }

    if (ctx.editMaskData.isEmpty()) {
        ctx.editMaskData = buildFaceEditMask(ctx);
    }

    ctx.imageData = blendEditedCropIntoOriginalImpl(ctx.refImageData, ctx.imageData, ctx.editTargetRect, ctx.editMaskData);
    LOG_INFO("ImageService", QString("Applied local edit blend for panel: %1, editRect=%2,%3 %4x%5")
        .arg(ctx.panelId)
        .arg(ctx.editTargetRect.x())
        .arg(ctx.editTargetRect.y())
        .arg(ctx.editTargetRect.width())
        .arg(ctx.editTargetRect.height()));
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
    return applyFaceOnlyEditPostProcess(ctx);
}

ImageService::ProviderConfig ImageService::getProviderConfig() const
{
    ProviderConfig config;
    QString provider = AppConfig::instance()->imageService().provider;
    
    if (provider == "volcengine") {
        config.name = "volcengine";
        config.maxRefImages = 1;
        config.supportsImg2Img = true;
        config.previewWidth = 1056;
        config.previewHeight = 1584;
        config.hdWidth = 1328;
        config.hdHeight = 1328;
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
    
    QString refPath = ctx.referenceImages.first();
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

bool ImageService::executeWithVolcEngine(GenerationContext& ctx)
{
    VolcEngineImageClient* client = ServiceContainer::instance()->volcEngineImageClient();
    if (!client || !client->isInitialized()) {
        setError(tr("VolcEngine客户端不可用"));
        return false;
    }
    
    ProviderConfig config = getProviderConfig();
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
    
    const QByteArray& referenceImage = referenceImageForEdit(ctx.editSourceImageData, ctx.refImageData, ctx.faceOnlyEdit);
    if (ctx.allowReferenceEdit && !referenceImage.isEmpty()) {
        options.referenceImage = referenceImage;
        LOG_INFO("ImageService", "Using img2img API with reference image");
    }
    
    VolcEngineImageClient::GenerateResult result = client->generate(options);
    
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

    if (!finalizeGeneratedImage(ctx, ctx.imageData)) {
        return false;
    }
    
    return !ctx.imageData.isEmpty();
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
    options.prompt = ctx.prompt;
    options.sourceImage = referenceImageForEdit(ctx.editSourceImageData, ctx.refImageData, ctx.faceOnlyEdit);
    options.negativePrompt = ctx.negativePrompt;
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
    
    LOG_INFO("ImageService", QString("Using image provider: %1, hasRefImage: %2")
        .arg(config.name).arg(!ctx.refImageData.isEmpty()));

    if (ctx.faceOnlyEdit && !ctx.refImageData.isEmpty() && !prepareFaceOnlyEditContext(ctx)) {
        setError(tr("无法准备局部编辑区域"));
        LOG_ERROR("ImageService", QString("Failed to prepare face-only edit context for panel: %1").arg(ctx.panelId));
        return false;
    }

    bool success = m_apiRetryPolicy->executeWithRetry(
        [&]() -> bool {
            if (config.name == "volcengine") {
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

        if (StorageClient::instance()->isInitialized()) {
            StorageClient::UploadResult result = StorageClient::instance()->upload(
                ctx.s3Key, ctx.imageData, "image/png");

            if (!result.success) {
                setError(TR_FMT("Cloud upload failed: %1", result.errorMessage));
                return false;
            }

            LOG_INFO("ImageService", QString("Image stored to cloud: %1").arg(ctx.s3Key));
        } else {
            QString modeStr = (ctx.mode == GenerateMode::Preview) ? "preview" : "hd";
            QString localPath = FileStorage::instance()->savePanelImage(ctx.panelId, ctx.imageData, modeStr);
            
            if (localPath.isEmpty()) {
                setError(TR_FMT("Local save failed: %1", FileStorage::instance()->lastError()));
                return false;
            }
            
            ctx.s3Key = localPath;
            LOG_INFO("ImageService", QString("Image stored locally: %1").arg(localPath));
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("ImageService", QString("storeImage exception: %1").arg(e.what()));
        setError(TR_FMT("Store image failed: %1", e.what()));
        return false;
    } catch (...) {
        LOG_ERROR("ImageService", "storeImage unknown exception");
        setError(tr("保存图像时发生异常"));
        return false;
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
        const QString visualPromptCn = panelJson.value("visualPrompt").toString().trimmed();
        const QString visualPromptEn = panelJson.value("visualPromptEn").toString().trimmed();
        LOG_INFO("ImageService", QString(
            "buildPromptForPanel input: panelId=%1, page=%2, index=%3, scene='%4', visualPromptCn='%5', visualPromptEn='%6', characters=[%7]")
            .arg(ctx.panelId)
            .arg(ctx.panel.page())
            .arg(ctx.panel.index())
            .arg(sceneText.left(120))
            .arg(visualPromptCn.left(120))
            .arg(visualPromptEn.left(120))
            .arg(panelCharacterNames.join(", ")));
        
        QString novelId = fetchNovelIdByStoryboardId(ctx.panel.storyboardId());
        LOG_DEBUG("ImageService", QString("buildPromptForPanel: resolved novelId=%1").arg(novelId));

        LOG_DEBUG("ImageService", QString("buildPromptForPanel: fetching character refs for panelId=%1").arg(ctx.panelId));
        QMap<QString, QJsonObject> characterRefs = fetchCharacterRefs(novelId, ctx.referenceImages);
        LOG_DEBUG("ImageService", QString("buildPromptForPanel: character refs loaded=%1, refImages=%2")
            .arg(characterRefs.size())
            .arg(ctx.referenceImages.size()));

        LOG_DEBUG("ImageService", QString("buildPromptForPanel: fetching scene refs for panelId=%1").arg(ctx.panelId));
        QMap<QString, QJsonObject> sceneRefs = fetchSceneRefs(novelId, ctx.referenceImages);
        LOG_DEBUG("ImageService", QString("buildPromptForPanel: scene refs loaded=%1, refImages=%2")
            .arg(sceneRefs.size())
            .arg(ctx.referenceImages.size()));
        
        LOG_INFO("ImageService", QString("buildPromptForPanel: panelId=%1, novelId=%2, characters=%3, scenes=%4")
            .arg(ctx.panelId).arg(novelId).arg(characterRefs.size()).arg(sceneRefs.size()));
        
        QJsonObject options = buildPanelPromptOptions(ctx, panelJson);
        LOG_DEBUG("ImageService", QString("buildPromptForPanel: invoking PromptBuilder for panelId=%1").arg(ctx.panelId));
        PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panelJson, characterRefs, sceneRefs, options);
        LOG_DEBUG("ImageService", QString("buildPromptForPanel: PromptBuilder returned for panelId=%1").arg(ctx.panelId));

        ctx.prompt = result.text;
        ctx.negativePrompt = result.negativePrompt;
        appendUniqueReferenceImages(ctx.referenceImages, result.referenceImages);
        
        if (ctx.prompt.isEmpty()) {
            setError(tr("提示词构建为空"));
            return false;
        }

        LOG_INFO("ImageService", QString("Prompt built: len=%1, refImages=%2, prompt=%3...")
            .arg(ctx.prompt.length()).arg(ctx.referenceImages.size()).arg(ctx.prompt.left(100)));

        return true;
    } catch (const std::exception& e) {
        setError(QString("Prompt build failed: %1").arg(e.what()));
        return false;

    } catch (...) {
        LOG_ERROR("ImageService", "buildPromptForPanel unknown exception");
        setError(tr("构建提示词时发生异常"));
        return false;
    }
}

QJsonObject ImageService::buildPanelPromptOptions(const GenerationContext& ctx, const QJsonObject& panelJson) const
{
    QJsonObject options;
    options["mode"] = !ctx.presetMode.isEmpty()
        ? ctx.presetMode
        : ((ctx.mode == GenerateMode::HD) ? "hd" : "preview");
    options["promptTarget"] = PromptTargetUtils::classifyPanelPromptTarget(panelJson);

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

QJsonObject ImageService::buildCharacterRef(const Character& ch, QStringList& outPortraitPaths)
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
    QJsonObject appearanceJson;
    appearanceJson["gender"] = app.gender;
    appearanceJson["age"] = QString::number(app.age);
    appearanceJson["build"] = app.build;
    appearanceJson["hairStyle"] = app.hairStyle;
    appearanceJson["hairColor"] = app.hairColor;
    appearanceJson["eyeColor"] = app.eyeColor;
    appearanceJson["clothing"] = QJsonArray::fromStringList(app.clothing);
    appearanceJson["accessories"] = QJsonArray::fromStringList(app.accessories);
    appearanceJson["distinctiveFeatures"] = QJsonArray::fromStringList(app.distinctiveFeatures);
    charJson["appearance"] = appearanceJson;
    
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
    
    for (const Character& ch : characters) {
        QJsonObject charJson = buildCharacterRef(ch, outReferenceImages);
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
        sceneRefs[scene.name()] = sceneJson;
        sceneRefs[scene.sceneId()] = sceneJson;
    }
    
    QStringList sceneNames = sceneRefs.keys();
    LOG_INFO("ImageService", QString("fetchSceneRefs: %1 scenes [%2], refImages=%3")
        .arg(scenes.size()).arg(sceneNames.join(", ")).arg(outReferenceImages.size()));
    
    return sceneRefs;
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
        if (StorageClient::instance()->isInitialized()) {
            result.imageUrl = StorageClient::instance()->getPresignedUrl(ctx.s3Key);
        } else {
            result.imageUrl = FileStorage::instance()->getFullPath(ctx.s3Key);
        }
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
    if (panelIds.isEmpty()) {
        return QString();
    }

    return enqueuePanelTask(QString(), TaskType::GeneratePanels, ImageModeUtils::generateModeString(mode), panelIds);
}

QString ImageService::enqueueBatchPanelImageGeneration(const QStringList& panelIds, BatchPresetMode presetMode)
{
    if (panelIds.isEmpty()) {
        return QString();
    }

    return enqueuePanelTask(QString(), TaskType::GeneratePanels, ImageModeUtils::presetModeString(presetMode), panelIds);
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
    if (modeStr.isEmpty() || modeStr == QLatin1String("preview") || modeStr == QLatin1String("hd")) {
        return generateResultToJson(generatePanelImage(task.panelId, ImageModeUtils::generateModeFromString(modeStr)));
    }

    if (ImageModeUtils::isPresetModeString(modeStr)) {
        return generateResultToJson(generatePanelImage(task.panelId, ImageModeUtils::presetModeFromString(modeStr)));
    }

    return generateResultToJson(generatePanelImage(task.panelId, ImageModeUtils::generateModeFromString(modeStr)));
}

QJsonObject ImageService::buildPanelJson(const Panel& panel)
{
    QJsonObject json = panel.rawContent();
    if (json.isEmpty()) {
        json["id"] = panel.id();
        json["page"] = panel.page();
        json["index"] = panel.index();
        json["scene"] = panel.scene();
        json["shotType"] = panel.shotType();
        json["cameraAngle"] = panel.cameraAngle();
        json["visualPrompt"] = panel.visualPrompt();
        json["visualPromptEn"] = panel.visualPromptEn();
        json["status"] = panel.status();
    } else {
        if (!json.contains("id")) {
            json["id"] = panel.id();
        }
        if (!json.contains("status")) {
            json["status"] = panel.status();
        }
    }
    
    QJsonArray charactersArray;
    for (const auto& c : panel.characters()) {
        QJsonObject charObj;
        charObj["charId"] = c.charId;
        charObj["name"] = c.name;
        charObj["pose"] = c.pose;
        charObj["expression"] = c.expression;
        charactersArray.append(charObj);
    }
    json["characters"] = charactersArray;
    
    QJsonArray dialogueArray;
    for (const auto& d : panel.dialogue()) {
        QJsonObject dialogueObj;
        dialogueObj["speaker"] = d.speaker;
        dialogueObj["text"] = d.text;
        dialogueObj["bubbleType"] = d.bubbleType;
        dialogueArray.append(dialogueObj);
    }
    json["dialogue"] = dialogueArray;
    
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

ImageService::GenerateResult ImageService::generateWithRetryInternal(const QString& panelId, const ResolutionConfig& resolution, int maxRetries)
{
    GenerateResult result;
    int retryCount = 0;
    
    while (retryCount < maxRetries) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_batchCancelled) {
                return createErrorResult(panelId, tr("\u7528\u6237\u53d6\u6d88\u4e86\u64cd\u4f5c"));
            }
        }
        
        result = generatePanelImageInternal(panelId, resolution);
        
        if (result.success) {
            return result;
        }
        
        retryCount++;
        if (retryCount < maxRetries) {
            int delaySeconds = static_cast<int>(std::pow(2, retryCount)) * 5;
            LOG_WARNING("ImageService", QString("Panel %1 generation failed (attempt %2/%3): %4, retrying in %5s...")
                .arg(panelId).arg(retryCount).arg(maxRetries).arg(result.errorMessage).arg(delaySeconds));
            QThread::msleep(delaySeconds * 1000);
        }
    }
    
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
    QString panelId;
    int currentIndex = 0;
    int total = 0;
    ResolutionConfig resolution;

    {
        QMutexLocker locker(&m_mutex);
        if (!shouldAdvanceToNextBatchItem()) {
            BatchResult finalResult = m_batchResult;
            finishBatch();

            LOG_INFO("ImageService", QString("Batch completed: %1 success, %2 failed")
                .arg(finalResult.successCount).arg(finalResult.failedCount));

            emit imageBatchCompleted(finalResult);
            return;
        }
        panelId = m_pendingPanelIds[m_currentProcessIndex];
        currentIndex = m_currentProcessIndex;
        total = m_pendingPanelIds.size();
        resolution = m_currentResolution;
    }

    emit batchProgressChanged(currentIndex + 1, total, tr("\u6b63\u5728\u5904\u7406..."));
    
    QtConcurrent::run(this, &ImageService::processPanelAsync, panelId, resolution, currentIndex, total);
}

bool ImageService::shouldAdvanceToNextBatchItem() const
{
    return !m_batchCancelled && m_currentProcessIndex < m_pendingPanelIds.size();
}

void ImageService::queueNextBatchItemProcessing()
{
    QTimer::singleShot(100, this, &ImageService::processNextPanel);
}

void ImageService::processPanelAsync(const QString& panelId, const ResolutionConfig& resolution, int currentIndex, int total)
{
    Q_UNUSED(currentIndex)
    Q_UNUSED(total)
    
    const int MAX_RETRIES = 3;
    GenerateResult result = generateWithRetryInternal(panelId, resolution, MAX_RETRIES);
    
    bool cancelled = false;
    {
        QMutexLocker locker(&m_mutex);
        cancelled = m_batchCancelled;
        if (!cancelled) {
            if (result.success) {
                m_batchResult.successCount++;
            } else {
                m_batchResult.failedCount++;
                LOG_ERROR("ImageService", QString("Panel generation failed after %1 attempts: %2 - %3")
                    .arg(MAX_RETRIES).arg(panelId).arg(result.errorMessage));
            }
            m_batchResult.results.append(result);
        }
        m_currentProcessIndex++;
    }
    
    if (!cancelled && result.success) {
        emit panelGenerated(result);
    }
    
    queueNextBatchItemProcessing();
}
