#include "viewmodels/NovelViewModel.h"
#include "ServiceContainer.h"
#include "NovelService.h"
#include "Logger.h"
#include "EncodingUtils.h"
#include "UserSession.h"

IMPLEMENT_SINGLETON(NovelViewModel)

NovelViewModel::NovelViewModel(QObject* parent)
    : BaseViewModel(parent)
    , m_novelService(nullptr)
{
}

NovelViewModel::~NovelViewModel()
{
}

void NovelViewModel::initialize()
{
    m_novelService = getService<NovelService, NovelService*>(
        &ServiceContainer::novelService, NovelService::instance);
    
    connectServiceSignals();
}

void NovelViewModel::connectServiceSignals()
{
    if (!m_novelService) { return; }
    
    connect(m_novelService, &NovelService::novelCreated, 
            this, &NovelViewModel::onNovelCreated);
    connect(m_novelService, &NovelService::novelUpdated, 
            this, &NovelViewModel::onNovelUpdated);
    connect(m_novelService, &NovelService::novelDeleted, 
            this, &NovelViewModel::onNovelDeleted);
}

void NovelViewModel::loadNovels(const QString& userId, int page, int pageSize)
{
    if (!m_novelService) { return; }
    
    QString effectiveUserId = userId.isEmpty() ? UserSession::instance()->currentUserId() : userId;
    
    withBusy([this, effectiveUserId, page, pageSize]() {
        NovelPageResult result = m_novelService->getNovels(effectiveUserId, page, pageSize);
        m_novels = result.novels;
        m_totalCount = result.total;
    });
    
    emit novelsLoaded(m_novels, m_totalCount);
}

void NovelViewModel::loadNovel(const QString& novelId)
{
    if (!m_novelService) { return; }
    
    withBusy([this, &novelId]() {
        m_currentNovel = m_novelService->getNovelById(novelId);
    });
    
    if (m_currentNovel.id().isEmpty()) {
        setLastError(TR_FMT("小说不存在: %1", novelId));
    } else {
        emit novelLoaded(m_currentNovel);
        emit currentNovelChanged(m_currentNovel);
    }
}

void NovelViewModel::createNovel(const QString& userId, const QString& title, const QString& text)
{
    if (!m_novelService) { return; }
    
    Novel novel;
    withBusy([this, &novel, &userId, &title, &text]() {
        novel = m_novelService->createNovel(userId, title, text);
    });
    
    if (novel.id().isEmpty()) {
        setLastError(TR("创建小说失败"));
    } else {
        m_currentNovel = novel;
        emit novelCreated(novel);
        emit currentNovelChanged(m_currentNovel);
    }
}

bool NovelViewModel::updateNovel(const QString& novelId, const QVariantMap& data)
{
    if (!m_novelService) { return false; }
    
    return m_novelService->updateNovel(novelId, data);
}

void NovelViewModel::updateNovelStatus(const QString& novelId, NovelStatus status)
{
    if (!m_novelService) { return; }
    
    m_novelService->updateStatus(novelId, status);
}

void NovelViewModel::deleteNovel(const QString& novelId)
{
    if (!m_novelService) { return; }
    
    m_novelService->deleteNovel(novelId);
}

void NovelViewModel::setCurrentNovel(const Novel& novel)
{
    m_currentNovel = novel;
    emit currentNovelChanged(m_currentNovel);
}

void NovelViewModel::refreshCurrentNovel()
{
    if (!m_currentNovel.id().isEmpty()) {
        loadNovel(m_currentNovel.id());
    }
}

void NovelViewModel::removeNovelFromList(const QString& novelId)
{
    for (int i = 0; i < m_novels.size(); ++i) {
        if (m_novels[i].id() == novelId) {
            m_novels.removeAt(i);
            m_totalCount--;
            break;
        }
    }
}

void NovelViewModel::onNovelCreated(const Novel& novel)
{
    m_novels.prepend(novel);
    m_totalCount++;
    emit novelCreated(novel);
}

void NovelViewModel::onNovelUpdated(const QString& novelId)
{
    if (m_currentNovel.id() == novelId) {
        refreshCurrentNovel();
    }
    emit novelUpdated(novelId);
}

void NovelViewModel::onNovelDeleted(const QString& novelId)
{
    removeNovelFromList(novelId);
    emit novelDeleted(novelId);
}
