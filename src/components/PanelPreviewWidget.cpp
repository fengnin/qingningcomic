#include "PanelPreviewWidget.h"
#include "components/PanelCard.h"
#include "components/ImageViewerDialog.h"
#include "services/ImageService.h"
#include "services/StoryboardService.h"
#include "viewmodels/StoryboardViewModel.h"
#include "data/FileStorage.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include "utils/AsyncImageLoader.h"
#include "utils/LayoutUtils.h"
#include <QLabel>
#include <QAbstractScrollArea>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QPixmap>
#include <QWheelEvent>
#include <QScrollBar>

namespace {
constexpr int kPreviewWidth = 180;
constexpr int kPreviewHeight = 190;

void resetPanelLayout(QHBoxLayout* layout)
{
    LayoutUtils::clearLayout(layout);
}

void updateCountLabel(QLabel* label, int count)
{
    if (label) {
        label->setText(QObject::tr("共 %1 个面板").arg(count));
    }
}

void updatePanelContainerGeometry(QWidget* container, QHBoxLayout* layout)
{
    if (!container || !layout) {
        return;
    }

    container->setMinimumWidth(layout->sizeHint().width());
    container->adjustSize();
    container->updateGeometry();
}

void configurePanelContainer(QWidget* container, QHBoxLayout* layout)
{
    if (!container || !layout) {
        return;
    }

    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    updatePanelContainerGeometry(container, layout);
}

QString resolvePreviewPath(const QString& previewUrl)
{
    if (previewUrl.isEmpty()) {
        return QString();
    }

    if (previewUrl.startsWith(QLatin1String("http://")) || previewUrl.startsWith(QLatin1String("https://"))) {
        return previewUrl;
    }

    const QFileInfo relativeInfo(previewUrl);
    if (relativeInfo.exists()) {
        return previewUrl;
    }

    if (FileStorage::instance()) {
        const QString fullPath = FileStorage::instance()->getFullPath(previewUrl);
        if (QFileInfo(fullPath).exists()) {
            return fullPath;
        }
    }

    return previewUrl;
}

bool isRemotePreviewUrl(const QString& previewUrl)
{
    return previewUrl.startsWith(QLatin1String("http://"))
        || previewUrl.startsWith(QLatin1String("https://"));
}

QPixmap loadPreviewPixmap(const QString& path, const QSize& targetSize)
{
    QPixmap pixmap;
    if (!pixmap.load(path)) {
        return QPixmap();
    }

    if (pixmap.width() > targetSize.width() || pixmap.height() > targetSize.height()) {
        pixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return pixmap;
}

int panelNumberForCache(const Panel& panel)
{
    return (panel.page() - 1) * 6 + panel.index() + 1;
}

QString panelCacheId(int chapterNumber, const Panel& panel)
{
    const QString loadPath = resolvePreviewPath(panel.previewUrl());
    if (loadPath.isEmpty()) {
        return QString();
    }

    const QString hash = QString(QCryptographicHash::hash(loadPath.toUtf8(), QCryptographicHash::Md5).toHex());
    return QStringLiteral("panel_%1_%2_%3")
        .arg(chapterNumber)
        .arg(panelNumberForCache(panel))
        .arg(hash);
}

void preloadPanelPreviewCache(int chapterNumber, const QList<Panel>& panels)
{
    AsyncImageLoader* loader = AsyncImageLoader::instance();
    if (!loader) {
        return;
    }

    const QSize targetSize(kPreviewWidth, kPreviewHeight);

    for (const Panel& panel : panels) {
        const QString loadPath = resolvePreviewPath(panel.previewUrl());
        if (loadPath.isEmpty() || isRemotePreviewUrl(loadPath)) {
            continue;
        }

        const QString id = panelCacheId(chapterNumber, panel);
        if (id.isEmpty()) {
            continue;
        }

        const QString cacheKey = AsyncImageLoader::makeCacheKey(id, targetSize);
        const QPixmap pixmap = loadPreviewPixmap(loadPath, targetSize);
        if (pixmap.isNull()) {
            continue;
        }

        loader->insertCache(cacheKey, pixmap);
    }
}
}

PanelPreviewWidget::PanelPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    
    connect(ImageService::instance(), &ImageService::panelGenerated,
            this, &PanelPreviewWidget::onPanelGenerated, Qt::QueuedConnection);
}

void PanelPreviewWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(12);
    
    QWidget* headerRow = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);
    
    m_titleLabel = new QLabel(tr("面板预览"));
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #212121;");
    headerLayout->addWidget(m_titleLabel);
    
    m_countLabel = new QLabel(tr("共 0 个面板"));
    m_countLabel->setStyleSheet("font-size: 14px; color: #6B7280;");
    headerLayout->addWidget(m_countLabel);
    
    headerLayout->addStretch();
    mainLayout->addWidget(headerRow);
    
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(false);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(R"(
        QScrollArea {
            background: transparent;
            border: 1px solid #E5E7EB;
            border-radius: 12px;
        }
    )");
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFixedHeight(470);
    scrollArea->viewport()->installEventFilter(this);
    scrollArea->installEventFilter(this);
    m_scrollArea = scrollArea;
    
    m_panelContainer = new QWidget();
    m_panelLayout = new QHBoxLayout(m_panelContainer);
    m_panelLayout->setContentsMargins(16, 16, 16, 16);
    m_panelLayout->setSpacing(16);
    configurePanelContainer(m_panelContainer, m_panelLayout);

    scrollArea->setWidget(m_panelContainer);
    mainLayout->addWidget(scrollArea);
    populateEmptyState();
}

void PanelPreviewWidget::setChapter(int chapter)
{
    m_currentChapter = chapter;
}

void PanelPreviewWidget::setPanels(const QList<Panel>& panels)
{
    m_panels = panels;
    if (!panels.isEmpty() && !panels.first().storyboardId().isEmpty()) {
        m_storyboardId = panels.first().storyboardId();
    } else {
        m_storyboardId.clear();
    }

    renderPanels(m_panels);
}

void PanelPreviewWidget::clear()
{
    m_panels.clear();
    m_panelCount = 0;
    m_storyboardId.clear();
    showEmptyState();
}

void PanelPreviewWidget::beginBatchRefresh()
{
    m_deferLiveUpdates = true;
}

void PanelPreviewWidget::endBatchRefresh(bool refreshNow)
{
    m_deferLiveUpdates = false;
    if (refreshNow) {
        refresh();
    }
}

void PanelPreviewWidget::refresh()
{
    if (!m_storyboardId.isEmpty()) {
        QList<Panel> freshPanels = StoryboardService::instance()->getPanels(m_storyboardId);
        if (!freshPanels.isEmpty()) {
            m_panels = freshPanels;
        }
    }
    
    renderPanels(m_panels);
}

void PanelPreviewWidget::renderPanels(const QList<Panel>& panels)
{
    if (panels.isEmpty()) {
        showEmptyState();
        return;
    }

    resetPanelView();
    preloadPanelPreviewCache(m_currentChapter, panels);
    populateWithPanels(panels);
}

void PanelPreviewWidget::showEmptyState()
{
    m_panelCount = 0;
    resetPanelView();
    populateEmptyState();
    updateCountLabel(m_countLabel, 0);
    emit panelCountChanged(0);
}

void PanelPreviewWidget::populateWithPanels(const QList<Panel>& panels)
{
    m_panelCount = 0;
    
    for (const Panel& panel : panels) {
        PanelCard* panelCard = createPanelCard(panelNumberFor(panel), panelDescription(panel), panel.id(), panel.previewUrl());
        m_panelLayout->addWidget(panelCard);
        m_panelCount++;
    }
    
    finishPopulate(m_panelCount);
    
    LOG_INFO("PanelPreviewWidget", QString("Populated %1 panels for chapter %2")
        .arg(m_panelCount).arg(m_currentChapter));
}

void PanelPreviewWidget::populateEmptyState()
{
    QLabel* emptyLabel = new QLabel(tr("暂无面板数据"));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("font-size: 14px; color: #9CA3AF; padding: 32px 0;");
    m_panelLayout->addWidget(emptyLabel);
    finishPopulate(0);
}

void PanelPreviewWidget::finishPopulate(int actualCount)
{
    m_panelLayout->addStretch();
    updatePanelContainerGeometry(m_panelContainer, m_panelLayout);
    
    updateCountLabel(m_countLabel, actualCount);
    
    if (actualCount > 0) {
        emit panelCountChanged(actualCount);
    }
}

void PanelPreviewWidget::resetPanelView()
{
    m_panelCards.clear();
    resetPanelLayout(m_panelLayout);
    configurePanelContainer(m_panelContainer, m_panelLayout);
}

QString PanelPreviewWidget::panelDescription(const Panel& panel) const
{
    const QString sceneDescription = panel.scene();
    return sceneDescription.isEmpty() ? panel.visualPrompt() : sceneDescription;
}

int PanelPreviewWidget::panelNumberFor(const Panel& panel) const
{
    return (panel.page() - 1) * 6 + panel.index() + 1;
}

PanelCard* PanelPreviewWidget::createPanelCard(int panelNum, const QString& description,
                                               const QString& panelId, const QString& previewUrl)
{
    PanelCard* panelCard = new PanelCard(m_currentChapter, panelNum, description);
    
    if (!panelId.isEmpty()) {
        panelCard->setPanelId(panelId);
        m_panelCards[panelId] = panelCard;
    }
    if (!previewUrl.isEmpty()) {
        panelCard->setPreviewUrl(previewUrl);
    }
    
    connect(panelCard, &PanelCard::clicked, this, &PanelPreviewWidget::onPanelCardClicked);
    connect(panelCard, &PanelCard::imageClicked, this, [](const QString &imagePath) {
        ImageViewerDialog::showImage(nullptr, imagePath);
    });
    connect(panelCard, &PanelCard::dataChanged, this, &PanelPreviewWidget::onPanelDescriptionChanged);
    
    return panelCard;
}

PanelCard* PanelPreviewWidget::panelCardForId(const QString& panelId) const
{
    return m_panelCards.value(panelId, nullptr);
}

void PanelPreviewWidget::syncPanelPreview(const QString& panelId, const QString& previewUrl, int width, int height)
{
    PanelCard* card = panelCardForId(panelId);
    if (card && !previewUrl.isEmpty()) {
        card->setPreviewUrl(previewUrl);
        card->setImageSize(width, height);
        LOG_INFO("PanelPreviewWidget", QString("Updated panel card: %1 -> %2, size: %3x%4")
            .arg(panelId).arg(previewUrl).arg(width).arg(height));
    } else if (card) {
        LOG_DEBUG("PanelPreviewWidget", QString("Panel card %1 found but preview url is empty").arg(panelId));
    } else {
        LOG_DEBUG("PanelPreviewWidget", QString("panelId %1 not found in m_panelCards").arg(panelId));
    }

    for (Panel& panel : m_panels) {
        if (panel.id() == panelId && !previewUrl.isEmpty()) {
            panel.setPreviewUrl(previewUrl);
            break;
        }
    }
}

bool PanelPreviewWidget::savePanelDescription(const QString& panelId, const QString& description)
{
    for (Panel& panel : m_panels) {
        if (panel.id() != panelId) {
            continue;
        }

        QJsonObject content = panel.rawContent();
        content["scene"] = description;
        panel.setContent(content);

        if (StoryboardService::instance()->updatePanel(panelId, content)) {
            LOG_INFO("PanelPreviewWidget", QString("Saved panel description to database: %1").arg(panelId));
            if (!m_storyboardId.isEmpty()) {
                StoryboardViewModel::instance()->invalidatePanelsCache(m_storyboardId);
            }
            return true;
        }

        LOG_ERROR("PanelPreviewWidget", QString("Failed to save panel description: %1").arg(panelId));
        return false;
    }

    LOG_WARNING("PanelPreviewWidget", QString("Panel not found for description save: %1").arg(panelId));
    return false;
}

void PanelPreviewWidget::onPanelCardClicked(int panelNumber)
{
    PanelCard* card = qobject_cast<PanelCard*>(sender());
    if (card) {
        emit panelClicked(panelNumber, card->panelId());
    }
}

void PanelPreviewWidget::onPanelGenerated(const ImageService::GenerateResult& result)
{
    LOG_DEBUG("PanelPreviewWidget", QString("onPanelGenerated called: success=%1, panelId=%2, imageUrl=%3")
        .arg(result.success).arg(result.panelId).arg(result.imageUrl.left(50)));
    
    if (!result.success) {
        LOG_DEBUG("PanelPreviewWidget", "Result not successful, ignoring");
        return;
    }
    
    const QString panelId = result.panelId;
    if (panelId.isEmpty()) {
        LOG_DEBUG("PanelPreviewWidget", "PanelId is empty, ignoring");
        return;
    }

    if (m_deferLiveUpdates) {
        LOG_DEBUG("PanelPreviewWidget", QString("Batch refresh active, deferring panelId=%1").arg(panelId));
        return;
    }

    LOG_DEBUG("PanelPreviewWidget", QString("m_panelCards contains %1 entries, checking for panelId: %2")
        .arg(m_panelCards.size()).arg(panelId));

    syncPanelPreview(panelId, result.imageUrl, result.width, result.height);
}

bool PanelPreviewWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (m_scrollArea && (watched == m_scrollArea || watched == m_scrollArea->viewport())) {
        if (event->type() == QEvent::Wheel) {
            return handleWheelScroll(static_cast<QWheelEvent*>(event));
        }
    }

    return QWidget::eventFilter(watched, event);
}

bool PanelPreviewWidget::handleWheelScroll(QWheelEvent* wheelEvent)
{
    if (!wheelEvent || !m_scrollArea) {
        return false;
    }

    const QPoint delta = wheelEvent->angleDelta();
    if ((wheelEvent->modifiers().testFlag(Qt::ShiftModifier) || delta.x() != 0)
        && m_scrollArea->horizontalScrollBar()) {
        const int horizontalDelta = delta.x() != 0 ? delta.x() : delta.y();
        if (horizontalDelta != 0) {
            const int step = qMax(1, qAbs(horizontalDelta) / 120);
            const int direction = horizontalDelta > 0 ? -1 : 1;
            m_scrollArea->horizontalScrollBar()->setValue(
                m_scrollArea->horizontalScrollBar()->value() + direction * step * 120);
            wheelEvent->accept();
            return true;
        }
    }

    return forwardVerticalWheelToParent(wheelEvent);
}

bool PanelPreviewWidget::forwardVerticalWheelToParent(QWheelEvent* wheelEvent)
{
    if (!wheelEvent || !m_scrollArea) {
        return false;
    }

    const QPoint delta = wheelEvent->angleDelta();
    if (delta.y() == 0) {
        return false;
    }

    QWidget* ancestor = m_scrollArea->parentWidget();
    while (ancestor) {
        QAbstractScrollArea* parentScrollArea = qobject_cast<QAbstractScrollArea*>(ancestor);
        if (parentScrollArea && parentScrollArea != m_scrollArea) {
            QScrollBar* parentBar = parentScrollArea->verticalScrollBar();
            if (parentBar) {
                const int step = qMax(1, qAbs(delta.y()) / 120);
                const int direction = delta.y() > 0 ? -1 : 1;
                parentBar->setValue(parentBar->value() + direction * step * 120);
                wheelEvent->accept();
                return true;
            }
        }
        ancestor = ancestor->parentWidget();
    }

    return false;
}

void PanelPreviewWidget::onPanelDescriptionChanged(int panelNumber, const QString& description)
{
    Q_UNUSED(panelNumber);
    
    PanelCard* card = qobject_cast<PanelCard*>(sender());
    if (!card) {
        LOG_WARNING("PanelPreviewWidget", "onPanelDescriptionChanged: sender is not a PanelCard");
        return;
    }
    
    const QString panelId = card->panelId();
    if (panelId.isEmpty()) {
        LOG_WARNING("PanelPreviewWidget", "onPanelDescriptionChanged: panelId is empty");
        return;
    }
    
    LOG_INFO("PanelPreviewWidget", QString("Panel description changed: panelId=%1, description=%2")
        .arg(panelId).arg(description.left(50)));

    savePanelDescription(panelId, description);
}
