#include "viewmodels/StoryboardViewModel.h"
#include "services/ServiceContainer.h"
#include "services/StoryboardService.h"
#include "services/AnalysisService.h"
#include "services/ImageService.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include "utils/AsyncDbHelper.h"
#include <QtConcurrent>
#include <QPointer>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariantList>
#include <QElapsedTimer>

IMPLEMENT_SINGLETON(StoryboardViewModel)

namespace {
QList<QVariantMap> queryAllRows(QSqlDatabase& db, const QString& sql, const QVariantList& binds, bool* ok, QString* error)
{
    QList<QVariantMap> rows;
    QSqlQuery query(db);
    if (!query.prepare(sql)) {
        if (error) {
            *error = query.lastError().text();
        }
        if (ok) {
            *ok = false;
        }
        return rows;
    }

    DatabaseUtils::bindValues(query, binds);

    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        if (ok) {
            *ok = false;
        }
        return rows;
    }

    while (query.next()) {
        rows.append(DatabaseUtils::recordToMap(query));
    }

    if (ok) {
        *ok = true;
    }
    return rows;
}

Storyboard storyboardFromMap(const QVariantMap& map)
{
    Storyboard storyboard;
    storyboard.setId(map.value(QStringLiteral("id")).toString());
    storyboard.setNovelId(map.value(QStringLiteral("novel_id")).toString());
    storyboard.setChapterNumber(map.value(QStringLiteral("chapter_number")).toInt());
    storyboard.setTotalPages(map.value(QStringLiteral("total_pages")).toInt());
    storyboard.setPanelCount(map.value(QStringLiteral("panel_count")).toInt());
    storyboard.setVersion(map.value(QStringLiteral("version")).toInt());
    return storyboard;
}

Panel panelFromMap(const QVariantMap& map)
{
    Panel panel;
    panel.setId(map.value(QStringLiteral("id")).toString());
    panel.setStoryboardId(map.value(QStringLiteral("storyboard_id")).toString());
    panel.setPage(map.value(QStringLiteral("page")).toInt());
    panel.setIndex(map.value(QStringLiteral("panel_index")).toInt());

    const QString contentText = map.value(QStringLiteral("content")).toString();
    const QJsonDocument doc = QJsonDocument::fromJson(contentText.toUtf8());
    panel.setContent(doc.isObject() ? doc.object() : QJsonObject());
    const QString visualPrompt = map.value(QStringLiteral("visual_prompt")).toString();
    if (!visualPrompt.isEmpty()) {
        panel.setVisualPrompt(visualPrompt);
    }
    panel.setPreviewS3Key(map.value(QStringLiteral("preview_image_path")).toString());
    panel.setHdS3Key(map.value(QStringLiteral("hd_image_path")).toString());
    return panel;
}

QList<Storyboard> loadStoryboardsFromDatabase(const AsyncDbHelper::DatabaseConnectionInfo& info, const QString& novelId,
                                             QString* error)
{
    QList<Storyboard> storyboards;
    const QString connectionName = AsyncDbHelper::makeConnectionName(QStringLiteral("storyboards"));
    QSqlDatabase db;
    if (!AsyncDbHelper::openTemporaryConnection(db, info, connectionName, error)) {
        AsyncDbHelper::closeTemporaryConnection(db, connectionName);
        return storyboards;
    }

    bool ok = false;
    const QString sql = QStringLiteral(
        "SELECT id, novel_id, chapter_number, total_pages, panel_count, version "
        "FROM storyboards WHERE novel_id = ? ORDER BY chapter_number ASC");
    QVariantList binds;
    binds << novelId;
    const QList<QVariantMap> rows = queryAllRows(db, sql, binds, &ok, error);
    if (ok) {
        storyboards.reserve(rows.size());
        for (const QVariantMap& row : rows) {
            storyboards.append(storyboardFromMap(row));
        }
    }

    AsyncDbHelper::closeTemporaryConnection(db, connectionName);
    return storyboards;
}

bool loadStoryboardAndPanelsFromDatabase(const AsyncDbHelper::DatabaseConnectionInfo& info, const QString& novelId,
                                        int chapterNumber, Storyboard* storyboard, QList<Panel>* panels,
                                        QString* error)
{
    if (!storyboard || !panels) {
        return false;
    }

    panels->clear();
    *storyboard = Storyboard();

    const QString connectionName = AsyncDbHelper::makeConnectionName(QStringLiteral("storyboard"));
    QSqlDatabase db;
    if (!AsyncDbHelper::openTemporaryConnection(db, info, connectionName, error)) {
        AsyncDbHelper::closeTemporaryConnection(db, connectionName);
        return false;
    }

    bool ok = false;
    const QString storyboardSql = QStringLiteral(
        "SELECT id, novel_id, chapter_number, total_pages, panel_count, version "
        "FROM storyboards WHERE novel_id = ? AND chapter_number = ? LIMIT 1");
    QVariantList storyboardBinds;
    storyboardBinds << novelId << chapterNumber;
    const QList<QVariantMap> storyboardRows = queryAllRows(
        db, storyboardSql, storyboardBinds, &ok, error);
    if (!ok || storyboardRows.isEmpty()) {
        AsyncDbHelper::closeTemporaryConnection(db, connectionName);
        return ok;
    }

    *storyboard = storyboardFromMap(storyboardRows.first());

    const QString panelsSql = QStringLiteral(
        "SELECT id, storyboard_id, page, panel_index, content, visual_prompt, preview_image_path, hd_image_path "
        "FROM panels WHERE storyboard_id = ? ORDER BY page ASC, panel_index ASC");
    QVariantList panelBinds;
    panelBinds << storyboard->id();
    const QList<QVariantMap> panelRows = queryAllRows(db, panelsSql, panelBinds, &ok, error);
    if (ok) {
        panels->reserve(panelRows.size());
        for (const QVariantMap& row : panelRows) {
            panels->append(panelFromMap(row));
        }
    }

    AsyncDbHelper::closeTemporaryConnection(db, connectionName);
    return ok;
}
} // namespace

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
    qRegisterMetaType<QList<Storyboard>>("QList<Storyboard>");
    qRegisterMetaType<QList<Panel>>("QList<Panel>");

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
    m_analysisStage = tr("分析中");
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
    m_storyboardsLoading = false;
    m_storyboardLoading = false;
}

void StoryboardViewModel::loadStoryboards(const QString& novelId, bool forceReload)
{
    if (!forceReload && hasCachedStoryboards(novelId)) {
        emit storyboardsLoaded(novelId, m_storyboards);
        return;
    }

    if (m_storyboardsLoading && !forceReload && m_cachedNovelId == novelId) {
        return;
    }

    m_cachedNovelId = novelId;
    m_storyboards.clear();
    emit storyboardsLoaded(novelId, m_storyboards);
    m_storyboardsLoading = true;

    const int token = ++m_storyboardsLoadToken;
    const AsyncDbHelper::DatabaseConnectionInfo dbInfo = AsyncDbHelper::snapshotConnection();
    QPointer<StoryboardViewModel> self(this);

    QtConcurrent::run([self, dbInfo, novelId, token]() {
        QString error;
        const QList<Storyboard> storyboards = loadStoryboardsFromDatabase(dbInfo, novelId, &error);
        const bool success = error.isEmpty();

        if (!self) {
            return;
        }

        QMetaObject::invokeMethod(self.data(), [self, novelId, token, storyboards, success, error]() {
            if (!self || token != self->m_storyboardsLoadToken || self->m_cachedNovelId != novelId) {
                return;
            }
            self->applyStoryboardsLoaded(token, novelId, storyboards, success, error);
        }, Qt::QueuedConnection);
    });
}

void StoryboardViewModel::loadStoryboard(const QString& novelId, int chapterNumber, bool forceReload)
{
    if (!forceReload && hasCachedPanels(novelId, chapterNumber)) {
        emit storyboardLoaded(novelId, chapterNumber, m_currentStoryboard);
        emit panelsLoaded(novelId, chapterNumber, m_currentPanels);
        return;
    }

    if (m_storyboardLoading && !forceReload && m_cachedNovelId == novelId
        && m_cachedChapterNumber == chapterNumber) {
        return;
    }
    
    m_cachedNovelId = novelId;
    m_cachedChapterNumber = chapterNumber;

    m_currentStoryboard = Storyboard();
    m_currentPanels.clear();
    emit storyboardLoaded(novelId, chapterNumber, m_currentStoryboard);
    emit panelsLoaded(novelId, chapterNumber, m_currentPanels);
    m_storyboardLoading = true;

    const int token = ++m_storyboardLoadToken;
    const AsyncDbHelper::DatabaseConnectionInfo dbInfo = AsyncDbHelper::snapshotConnection();
    QPointer<StoryboardViewModel> self(this);

    QtConcurrent::run([self, dbInfo, novelId, chapterNumber, token]() {
        Storyboard storyboard;
        QList<Panel> panels;
        QString error;
        const bool success = loadStoryboardAndPanelsFromDatabase(dbInfo, novelId, chapterNumber,
                                                                 &storyboard, &panels, &error);

        if (!self) {
            return;
        }

        QMetaObject::invokeMethod(self.data(), [self, novelId, chapterNumber, token, storyboard, panels, success, error]() {
            if (!self || token != self->m_storyboardLoadToken
                || self->m_cachedNovelId != novelId
                || self->m_cachedChapterNumber != chapterNumber) {
                return;
            }
            self->applyStoryboardLoaded(token, novelId, chapterNumber, storyboard, panels, success, error);
        }, Qt::QueuedConnection);
    });
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
    
    emit panelsLoaded(m_cachedNovelId, m_cachedChapterNumber, m_currentPanels);
}

void StoryboardViewModel::invalidatePanelsCache(const QString& storyboardId)
{
    m_panelsCache.remove(storyboardId);
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

    if (mode == QLatin1String("1x1") || mode == QLatin1String("1:1")) {
        m_imageService->generatePanelImages(panelIds, ImageService::BatchPresetMode::Square_1x1);
    } else if (mode == QLatin1String("3x2") || mode == QLatin1String("3:2")) {
        m_imageService->generatePanelImages(panelIds, ImageService::BatchPresetMode::Standard_3x2);
    } else if (mode == QLatin1String("16x9") || mode == QLatin1String("16:9")) {
        m_imageService->generatePanelImages(panelIds, ImageService::BatchPresetMode::Widescreen_16x9);
    } else {
        ImageService::GenerateMode genMode = (mode == QLatin1String("hd"))
            ? ImageService::GenerateMode::HD
            : ImageService::GenerateMode::Preview;
        m_imageService->generatePanelImages(panelIds, genMode);
    }
}

void StoryboardViewModel::generateAllPanelImages(const QString& storyboardId, const QString& mode)
{
    if (!m_imageService || !m_storyboardService) { return; }

    if (mode == QLatin1String("1x1") || mode == QLatin1String("1:1")) {
        m_imageService->generateStoryboardImages(storyboardId, ImageService::BatchPresetMode::Square_1x1);
    } else if (mode == QLatin1String("3x2") || mode == QLatin1String("3:2")) {
        m_imageService->generateStoryboardImages(storyboardId, ImageService::BatchPresetMode::Standard_3x2);
    } else if (mode == QLatin1String("16x9") || mode == QLatin1String("16:9")) {
        m_imageService->generateStoryboardImages(storyboardId, ImageService::BatchPresetMode::Widescreen_16x9);
    } else {
        ImageService::GenerateMode genMode = (mode == QLatin1String("hd"))
            ? ImageService::GenerateMode::HD
            : ImageService::GenerateMode::Preview;
        m_imageService->generateStoryboardImages(storyboardId, genMode);
    }
}

QString StoryboardViewModel::enqueueBatchPanelImageGeneration(const QStringList& panelIds, const QString& mode)
{
    if (!m_imageService || panelIds.isEmpty()) { return QString(); }

    if (mode == QLatin1String("1x1") || mode == QLatin1String("1:1")) {
        return m_imageService->enqueueBatchPanelImageGeneration(panelIds, ImageService::BatchPresetMode::Square_1x1);
    }
    if (mode == QLatin1String("3x2") || mode == QLatin1String("3:2")) {
        return m_imageService->enqueueBatchPanelImageGeneration(panelIds, ImageService::BatchPresetMode::Standard_3x2);
    }
    if (mode == QLatin1String("16x9") || mode == QLatin1String("16:9")) {
        return m_imageService->enqueueBatchPanelImageGeneration(panelIds, ImageService::BatchPresetMode::Widescreen_16x9);
    }

    ImageService::GenerateMode genMode = (mode == QLatin1String("hd"))
        ? ImageService::GenerateMode::HD
        : ImageService::GenerateMode::Preview;
    return m_imageService->enqueueBatchPanelImageGeneration(panelIds, genMode);
}

bool StoryboardViewModel::createEmptyStoryboard(const QString& novelId, int chapterNumber)
{
    if (!m_storyboardService) { return false; }
    
    QJsonObject storyboardData;
    storyboardData[QStringLiteral("totalPages")] = 0;
    storyboardData[QStringLiteral("panels")] = QJsonArray();
    
    return m_storyboardService->saveStoryboard(novelId, storyboardData, chapterNumber, false);
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
        if (m_cachedNovelId == novelId && m_cachedChapterNumber == chapterNumber) {
            m_currentStoryboard = Storyboard();
            m_currentPanels.clear();
            m_cachedChapterNumber = 0;
        }
        m_panelsCache.clear();
        m_storyboardsLoading = false;
        m_storyboardLoading = false;
        emit storyboardDeleted(novelId, chapterNumber);
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
    
    bool success = m_storyboardService->updatePanel(panelId, content);
    
    if (success) {
        // 清除当前分镜缓存
        if (!m_currentStoryboard.id().isEmpty()) {
            m_panelsCache.remove(m_currentStoryboard.id());
        }
        
        // 通知 UI 刷新
        emit panelUpdated(panelId, content);
    }
    
    return success;
}

void StoryboardViewModel::onStoryboardSaved(const QString& novelId)
{
    loadStoryboards(novelId, true);
}

void StoryboardViewModel::applyStoryboardsLoaded(int token, const QString& novelId,
                                                 const QList<Storyboard>& storyboards,
                                                 bool success, const QString& error)
{
    Q_UNUSED(token)
    if (!success) {
        setLastError(error);
        m_storyboardsLoading = false;
        return;
    }

    m_storyboards = storyboards;
    m_cachedNovelId = novelId;
    m_storyboardsLoading = false;
    emit storyboardsLoaded(novelId, m_storyboards);
}

void StoryboardViewModel::applyStoryboardLoaded(int token, const QString& novelId,
                                                int chapterNumber, const Storyboard& storyboard,
                                                const QList<Panel>& panels, bool success,
                                                const QString& error)
{
    Q_UNUSED(token)
    if (!success) {
        setLastError(error);
        m_storyboardLoading = false;
        return;
    }

    m_currentStoryboard = storyboard;
    m_currentPanels = panels;
    m_cachedNovelId = novelId;
    m_cachedChapterNumber = chapterNumber;
    m_storyboardLoading = false;

    if (!m_currentStoryboard.id().isEmpty()) {
        m_panelsCache[m_currentStoryboard.id()] = m_currentPanels;
    }

    emit storyboardLoaded(m_cachedNovelId, m_cachedChapterNumber, m_currentStoryboard);
    emit panelsLoaded(m_cachedNovelId, m_cachedChapterNumber, m_currentPanels);
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
