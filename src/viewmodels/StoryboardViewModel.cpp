#include "viewmodels/StoryboardViewModel.h"
#include "ServiceContainer.h"
#include "StoryboardService.h"
#include "AnalysisService.h"
#include "ImageService.h"
#include "Logger.h"
#include "EncodingUtils.h"
#include <QElapsedTimer>

IMPLEMENT_SINGLETON(StoryboardViewModel)

StoryboardViewModel::StoryboardViewModel(QObject* parent)
    : BaseViewModel(parent)
    , m_storyboardService(nullptr)
    , m_analysisService(nullptr)
    , m_imageService(nullptr)
{
}

StoryboardViewModel::~StoryboardViewModel()
{
}

void StoryboardViewModel::initialize()
{
    m_storyboardService = getService<StoryboardService, StoryboardService*>(
        &ServiceContainer::storyboardService, StoryboardService::instance);
    
    AnalysisService* analysisService = AnalysisService::instance();
    if (!analysisService) {
        LOG_ERROR("StoryboardViewModel", "AnalysisService is null, cannot connect signals");
        return;
    }
    m_analysisService = analysisService;
    
    m_imageService = getService<ImageService, ImageService*>(
        &ServiceContainer::imageService, ImageService::instance);
    
    connectServiceSignals();
}

void StoryboardViewModel::connectServiceSignals()
{
    if (m_storyboardService) {
        connect(m_storyboardService, &StoryboardService::storyboardSaved, 
                this, &StoryboardViewModel::onStoryboardSaved);
    } else {
        LOG_WARNING("StoryboardViewModel", "m_storyboardService is null!");
    }
    
    if (m_analysisService) {
        connect(m_analysisService, &AnalysisService::analysisStarted,
                this, &StoryboardViewModel::onAnalysisStarted, Qt::QueuedConnection);
        connect(m_analysisService, &AnalysisService::analysisCompleted,
                this, &StoryboardViewModel::onAnalysisCompleted, Qt::QueuedConnection);
        connect(m_analysisService, &AnalysisService::analysisFailed,
                this, &StoryboardViewModel::onAnalysisFailed, Qt::QueuedConnection);
        connect(m_analysisService, &AnalysisService::analysisProgress,
                this, [this](int current, int total, const QString& message) {
                    int progress = total > 0 ? (current * 100 / total) : current;
                    emit analysisProgress(message, progress);
                }, Qt::QueuedConnection);
        connect(m_analysisService, &AnalysisService::streamProgress,
                this, [this](const QString& content) {
                    m_analysisProgress = content;
                }, Qt::QueuedConnection);
        connect(m_analysisService, &AnalysisService::streamCompleted,
                this, [this](const QString& novelId, const QJsonObject& result) {
                    Q_UNUSED(novelId)
                    Q_UNUSED(result)
                    m_analysisProgress.clear();
                }, Qt::QueuedConnection);
    } else {
        LOG_ERROR("StoryboardViewModel", "m_analysisService is null!");
    }
}

void StoryboardViewModel::resetAnalysisState()
{
    m_analyzing = true;
    m_currentJobId.clear();
    m_analysisStage = TR("分析中");
    setBusy(true);
}

bool StoryboardViewModel::hasCachedStoryboards(const QString& novelId) const
{
    return !m_storyboards.isEmpty() && m_cachedNovelId == novelId;
}

bool StoryboardViewModel::hasCachedPanels(const QString& novelId, int chapterNumber) const
{
    return m_cachedNovelId == novelId && m_cachedChapterNumber == chapterNumber && !m_currentPanels.isEmpty();
}

void StoryboardViewModel::clearCache()
{
    m_storyboards.clear();
    m_currentStoryboard = Storyboard();
    m_currentPanels.clear();
    m_cachedNovelId.clear();
    m_cachedChapterNumber = 0;
    m_panelsCache.clear();
}

void StoryboardViewModel::loadStoryboards(const QString& novelId, bool forceReload)
{
    if (!m_storyboardService) { return; }
    
    if (!forceReload && hasCachedStoryboards(novelId)) {
        emit storyboardsLoaded(m_storyboards);
        return;
    }
    
    m_cachedNovelId = novelId;
    m_storyboards = m_storyboardService->getAllStoryboards(novelId);
    
    emit storyboardsLoaded(m_storyboards);
}

void StoryboardViewModel::loadStoryboard(const QString& novelId, int chapterNumber, bool forceReload)
{
    if (!m_storyboardService) { return; }
    
    if (!forceReload && hasCachedPanels(novelId, chapterNumber)) {
        emit storyboardLoaded(m_currentStoryboard);
        emit panelsLoaded(m_currentPanels);
        return;
    }
    
    m_cachedNovelId = novelId;
    m_cachedChapterNumber = chapterNumber;
    
    m_currentStoryboard = m_storyboardService->getStoryboardByChapter(novelId, chapterNumber);
    
    if (!m_currentStoryboard.id().isEmpty()) {
        QString cacheKey = m_currentStoryboard.id();
        if (forceReload) {
            m_panelsCache.remove(cacheKey);
        }
        if (m_panelsCache.contains(cacheKey)) {
            m_currentPanels = m_panelsCache[cacheKey];
        } else {
            m_currentPanels = m_storyboardService->getPanels(m_currentStoryboard.id());
            m_panelsCache[cacheKey] = m_currentPanels;
        }
        emit panelsLoaded(m_currentPanels);
    } else {
        m_currentPanels.clear();
    }
    
    emit storyboardLoaded(m_currentStoryboard);
}

void StoryboardViewModel::loadPanels(const QString& storyboardId)
{
    if (!m_storyboardService) { return; }
    
    if (m_panelsCache.contains(storyboardId)) {
        m_currentPanels = m_panelsCache[storyboardId];
    } else {
        m_currentPanels = m_storyboardService->getPanels(storyboardId);
        m_panelsCache[storyboardId] = m_currentPanels;
    }
    
    emit panelsLoaded(m_currentPanels);
}

void StoryboardViewModel::startAnalysis(const QString& novelId, const QString& text, int chapterNumber)
{
    if (!m_analysisService) { return; }
    
    resetAnalysisState();
    m_analysisService->analyzeNovel(novelId, text, chapterNumber);
}

void StoryboardViewModel::startAnalysisWithBible(const QString& novelId, const QString& text, 
                                                  int chapterNumber,
                                                  const QJsonArray& characters, 
                                                  const QJsonArray& scenes)
{
    if (!m_analysisService) { return; }
    
    resetAnalysisState();
    m_analysisService->analyzeNovelWithBible(novelId, text, chapterNumber, characters, scenes);
}

void StoryboardViewModel::generatePanelImages(const QStringList& panelIds, const QString& mode)
{
    if (!m_imageService || panelIds.isEmpty()) { return; }
    
    ImageService::GenerateMode genMode = (mode == QLatin1String("hd")) 
        ? ImageService::GenerateMode::HD 
        : ImageService::GenerateMode::Preview;
    
    m_imageService->generatePanelImages(panelIds, genMode);
}

void StoryboardViewModel::generateAllPanelImages(const QString& storyboardId, const QString& mode)
{
    if (!m_imageService || !m_storyboardService) { return; }
    
    ImageService::GenerateMode genMode = (mode == QLatin1String("hd")) 
        ? ImageService::GenerateMode::HD 
        : ImageService::GenerateMode::Preview;
    
    m_imageService->generateStoryboardImages(storyboardId, genMode);
}

QString StoryboardViewModel::enqueueBatchPanelImageGeneration(const QStringList& panelIds, const QString& mode)
{
    if (!m_imageService || panelIds.isEmpty()) { return QString(); }
    
    ImageService::GenerateMode genMode = (mode == QLatin1String("hd")) 
        ? ImageService::GenerateMode::HD 
        : ImageService::GenerateMode::Preview;
    
    return m_imageService->enqueueBatchPanelImageGeneration(panelIds, genMode);
}

void StoryboardViewModel::createEmptyStoryboard(const QString& novelId, int chapterNumber)
{
    if (!m_storyboardService) { return; }
    
    QJsonObject storyboardData;
    storyboardData[QStringLiteral("totalPages")] = 0;
    storyboardData[QStringLiteral("panels")] = QJsonArray();
    
    m_storyboardService->saveStoryboard(novelId, storyboardData, chapterNumber);
}

bool StoryboardViewModel::deleteStoryboard(const QString& novelId, int chapterNumber)
{
    if (!m_storyboardService) { return false; }
    
    bool result = m_storyboardService->deleteStoryboard(novelId, chapterNumber);
    
    if (result) {
        for (int i = 0; i < m_storyboards.size(); ++i) {
            if (m_storyboards[i].chapterNumber() == chapterNumber) {
                m_storyboards.removeAt(i);
                break;
            }
        }
        m_panelsCache.clear();
    }
    
    return result;
}

Panel StoryboardViewModel::getPanel(const QString& panelId)
{
    if (!m_storyboardService) { return Panel(); }
    
    return m_storyboardService->getPanel(panelId);
}

bool StoryboardViewModel::updatePanel(const QString& panelId, const QJsonObject& content)
{
    if (!m_storyboardService) { return false; }
    
    return m_storyboardService->updatePanel(panelId, content);
}

void StoryboardViewModel::onStoryboardSaved(const QString& novelId)
{
    loadStoryboards(novelId, true);
}

void StoryboardViewModel::onAnalysisStarted(const QString& novelId)
{
    m_currentJobId = novelId;
    m_analyzing = true;
    emit analysisStarted(novelId);
}

void StoryboardViewModel::onAnalysisCompleted(const QString& novelId, const AnalysisResult& result)
{
    m_analyzing = false;
    setBusy(false);
    emit analysisCompleted(novelId, result.chapterNumber);
}

void StoryboardViewModel::onAnalysisFailed(const QString& novelId, const QString& error)
{
    m_analyzing = false;
    setBusy(false);
    setLastError(error);
    emit analysisFailed(novelId, error);
}
