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
const QStringList SAMPLE_PANEL_DESCRIPTIONS = {
    QStringLiteral("清晨的街道上，主角停下脚步，望向远方。"),
    QStringLiteral("镜头拉近，角色露出思考的神情。"),
    QStringLiteral("同伴从一侧跑来，挥手打招呼。"),
    QStringLiteral("两人短暂对话，气氛逐渐紧张起来。"),
    QStringLiteral("视角切到天空与城市轮廓的交界处。"),
    QStringLiteral("最后以角色背影收束，留下悬念。")
};
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
    
    QWidget *headerRow = new QWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
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
    
    m_container = new QWidget();
    m_containerLayout = new QHBoxLayout(m_container);
    m_containerLayout->setContentsMargins(16, 16, 16, 16);
    m_containerLayout->setSpacing(16);
    
    scrollArea->setWidget(m_container);
    mainLayout->addWidget(scrollArea);
    
    populateWithSample();
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
    }
    refresh();
}

void PanelPreviewWidget::clear()
{
    m_panels.clear();
    m_panelCount = 0;
    m_panelCards.clear();
    LayoutUtils::clearLayout(m_containerLayout);
    populateWithSample();
    
    if (m_countLabel) {
        m_countLabel->setText(tr("共 0 个面板"));
    }
    emit panelCountChanged(0);
}

void PanelPreviewWidget::refresh()
{
    m_panelCards.clear();
    LayoutUtils::clearLayout(m_containerLayout);
    
    if (!m_storyboardId.isEmpty()) {
        QList<Panel> freshPanels = StoryboardService::instance()->getPanels(m_storyboardId);
        if (!freshPanels.isEmpty()) {
            m_panels = freshPanels;
        }
    }
    
    if (m_panels.isEmpty()) {
        m_panelCount = 0;
        populateWithSample();
        if (m_countLabel) {
            m_countLabel->setText(tr("共 0 个面板"));
        }
        emit panelCountChanged(0);
        return;
    }
    
    populateWithPanels(m_panels);
}

void PanelPreviewWidget::populateWithPanels(const QList<Panel>& panels)
{
    m_panelCount = 0;
    
    for (const Panel& panel : panels) {
        QString desc = panel.scene();
        if (desc.isEmpty()) {
            desc = panel.visualPrompt();
        }
        
        int panelNum = (panel.page() - 1) * 6 + panel.index() + 1;
        QString previewUrl = panel.previewUrl();
        
        PanelCard *panelCard = createPanelCard(panelNum, desc, panel.id(), previewUrl);
        m_containerLayout->addWidget(panelCard);
        m_panelCount++;
    }
    
    finishPopulate(m_panelCount);
    
    LOG_INFO("PanelPreviewWidget", QString("Populated %1 panels for chapter %2")
        .arg(m_panelCount).arg(m_currentChapter));
}

void PanelPreviewWidget::populateWithSample()
{
    for (int i = 0; i < SAMPLE_PANEL_DESCRIPTIONS.size(); ++i) {
        PanelCard *panelCard = createPanelCard(i + 1, SAMPLE_PANEL_DESCRIPTIONS[i]);
        m_containerLayout->addWidget(panelCard);
    }
    
    finishPopulate(0);
}

void PanelPreviewWidget::finishPopulate(int actualCount)
{
    m_containerLayout->addStretch();
    
    if (m_countLabel) {
        m_countLabel->setText(tr("共 %1 个面板").arg(actualCount));
    }
    
    if (actualCount > 0) {
        emit panelCountChanged(actualCount);
    }
}

PanelCard* PanelPreviewWidget::createPanelCard(int panelNum, const QString& description, 
                                                const QString& panelId, const QString& previewUrl)
{
    PanelCard *panelCard = new PanelCard(m_currentChapter, panelNum, description);
    
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

void PanelPreviewWidget::onPanelCardClicked(int panelNumber)
{
    PanelCard *card = qobject_cast<PanelCard*>(sender());
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
    
    QString panelId = result.panelId;
    if (panelId.isEmpty()) {
        LOG_DEBUG("PanelPreviewWidget", "PanelId is empty, ignoring");
        return;
    }
    
    LOG_DEBUG("PanelPreviewWidget", QString("m_panelCards contains %1 entries, checking for panelId: %2")
        .arg(m_panelCards.size()).arg(panelId));
    
    if (m_panelCards.contains(panelId)) {
        PanelCard* card = m_panelCards[panelId];
        if (card && !result.imageUrl.isEmpty()) {
            card->setPreviewUrl(result.imageUrl);
            card->setImageSize(result.width, result.height);
            LOG_INFO("PanelPreviewWidget", QString("Updated panel card: %1 -> %2, size: %3x%4")
                .arg(panelId).arg(result.imageUrl).arg(result.width).arg(result.height));
        }
    } else {
        LOG_DEBUG("PanelPreviewWidget", QString("panelId %1 not found in m_panelCards").arg(panelId));
    }
    
    for (int i = 0; i < m_panels.size(); ++i) {
        if (m_panels[i].id() == panelId && !result.imageUrl.isEmpty()) {
            m_panels[i].setPreviewUrl(result.imageUrl);
            break;
        }
    }
}

void PanelPreviewWidget::onPanelDescriptionChanged(int panelNumber, const QString& description)
{
    Q_UNUSED(panelNumber);
    
    PanelCard *card = qobject_cast<PanelCard*>(sender());
    if (!card) {
        LOG_WARNING("PanelPreviewWidget", "onPanelDescriptionChanged: sender is not a PanelCard");
        return;
    }
    
    QString panelId = card->panelId();
    if (panelId.isEmpty()) {
        LOG_WARNING("PanelPreviewWidget", "onPanelDescriptionChanged: panelId is empty");
        return;
    }
    
    LOG_INFO("PanelPreviewWidget", QString("Panel description changed: panelId=%1, description=%2")
        .arg(panelId).arg(description.left(50)));
    
    for (int i = 0; i < m_panels.size(); ++i) {
        if (m_panels[i].id() == panelId) {
            QJsonObject content = m_panels[i].rawContent();
            content["scene"] = description;
            m_panels[i].setContent(content);
            
            if (StoryboardService::instance()->updatePanel(panelId, content)) {
                LOG_INFO("PanelPreviewWidget", QString("Saved panel description to database: %1").arg(panelId));
                if (!m_storyboardId.isEmpty()) {
                    StoryboardViewModel::instance()->invalidatePanelsCache(m_storyboardId);
                }
            } else {
                LOG_ERROR("PanelPreviewWidget", QString("Failed to save panel description: %1").arg(panelId));
            }
            break;
        }
    }
}
