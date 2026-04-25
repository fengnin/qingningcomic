#include "PanelPreviewWidget.h"
#include "components/PanelCard.h"
#include "components/ImageViewerDialog.h"
#include "services/ImageService.h"
#include "services/StoryboardService.h"
#include "viewmodels/StoryboardViewModel.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include "utils/LayoutUtils.h"
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace {
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
    scrollArea->setWidgetResizable(true);
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
    
    m_panelContainer = new QWidget();
    m_panelLayout = new QHBoxLayout(m_panelContainer);
    m_panelLayout->setContentsMargins(16, 16, 16, 16);
    m_panelLayout->setSpacing(16);

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
    refresh();
}

void PanelPreviewWidget::clear()
{
    m_panels.clear();
    m_panelCount = 0;
    m_storyboardId.clear();
    resetPanelView();
    populateEmptyState();
    updateCountLabel(m_countLabel, 0);
    emit panelCountChanged(0);
}

void PanelPreviewWidget::refresh()
{
    resetPanelView();
    
    if (!m_storyboardId.isEmpty()) {
        QList<Panel> freshPanels = StoryboardService::instance()->getPanels(m_storyboardId);
        if (!freshPanels.isEmpty()) {
            m_panels = freshPanels;
        }
    }
    
    if (m_panels.isEmpty()) {
        m_panelCount = 0;
        populateEmptyState();
        updateCountLabel(m_countLabel, 0);
        emit panelCountChanged(0);
        return;
    }
    
    populateWithPanels(m_panels);
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
    
    updateCountLabel(m_countLabel, actualCount);
    
    if (actualCount > 0) {
        emit panelCountChanged(actualCount);
    }
}

void PanelPreviewWidget::resetPanelView()
{
    m_panelCards.clear();
    resetPanelLayout(m_panelLayout);
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

    LOG_DEBUG("PanelPreviewWidget", QString("m_panelCards contains %1 entries, checking for panelId: %2")
        .arg(m_panelCards.size()).arg(panelId));

    syncPanelPreview(panelId, result.imageUrl, result.width, result.height);
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
