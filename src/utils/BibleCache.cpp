#include "BibleCache.h"
#include "Logger.h"

BibleCache* BibleCache::m_instance = nullptr;
QMutex BibleCache::m_instanceMutex;

BibleCache::BibleCache(QObject* parent)
    : QObject(parent)
{
}

BibleCache* BibleCache::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_instanceMutex);
        if (!m_instance) {
            m_instance = new BibleCache();
        }
    }
    return m_instance;
}

bool BibleCache::isCacheValid(const QDateTime& timestamp) const
{
    if (!timestamp.isValid()) {
        return false;
    }
    
    if (m_maxCacheAgeSeconds <= 0) {
        return true;
    }
    
    qint64 ageSeconds = timestamp.secsTo(QDateTime::currentDateTime());
    return ageSeconds < m_maxCacheAgeSeconds;
}

QList<Character> BibleCache::getCharacters(const QString& novelId)
{
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_characterCache.contains(novelId)) {
        return QList<Character>();
    }
    
    QDateTime timestamp = m_characterTimestamp.value(novelId);
    if (!isCacheValid(timestamp)) {
        m_characterCache.remove(novelId);
        m_characterTimestamp.remove(novelId);
        LOG_DEBUG("BibleCache", QString("Character cache expired for novel: %1").arg(novelId));
        return QList<Character>();
    }
    
    LOG_DEBUG("BibleCache", QString("Character cache hit for novel: %1, count: %2")
        .arg(novelId).arg(m_characterCache[novelId].size()));
    return m_characterCache[novelId];
}

void BibleCache::setCharacters(const QString& novelId, const QList<Character>& characters)
{
    QMutexLocker locker(&m_cacheMutex);
    
    m_characterCache[novelId] = characters;
    m_characterTimestamp[novelId] = QDateTime::currentDateTime();
    
    LOG_DEBUG("BibleCache", QString("Character cache set for novel: %1, count: %2")
        .arg(novelId).arg(characters.size()));
    emit charactersCacheUpdated(novelId);
}

void BibleCache::invalidateCharacters(const QString& novelId)
{
    QMutexLocker locker(&m_cacheMutex);
    
    m_characterCache.remove(novelId);
    m_characterTimestamp.remove(novelId);
    
    LOG_DEBUG("BibleCache", QString("Character cache invalidated for novel: %1").arg(novelId));
    emit cacheInvalidated(novelId);
}

void BibleCache::invalidateAllCharacters()
{
    QMutexLocker locker(&m_cacheMutex);
    
    int count = m_characterCache.size();
    m_characterCache.clear();
    m_characterTimestamp.clear();
    
    LOG_DEBUG("BibleCache", QString("All character cache invalidated, count: %1").arg(count));
}

bool BibleCache::hasCharacters(const QString& novelId) const
{
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_characterCache.contains(novelId)) {
        return false;
    }
    
    QDateTime timestamp = m_characterTimestamp.value(novelId);
    return isCacheValid(timestamp);
}

QList<Scene> BibleCache::getScenes(const QString& novelId)
{
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_sceneCache.contains(novelId)) {
        return QList<Scene>();
    }
    
    QDateTime timestamp = m_sceneTimestamp.value(novelId);
    if (!isCacheValid(timestamp)) {
        m_sceneCache.remove(novelId);
        m_sceneTimestamp.remove(novelId);
        LOG_DEBUG("BibleCache", QString("Scene cache expired for novel: %1").arg(novelId));
        return QList<Scene>();
    }
    
    LOG_DEBUG("BibleCache", QString("Scene cache hit for novel: %1, count: %2")
        .arg(novelId).arg(m_sceneCache[novelId].size()));
    return m_sceneCache[novelId];
}

void BibleCache::setScenes(const QString& novelId, const QList<Scene>& scenes)
{
    QMutexLocker locker(&m_cacheMutex);
    
    m_sceneCache[novelId] = scenes;
    m_sceneTimestamp[novelId] = QDateTime::currentDateTime();
    
    LOG_DEBUG("BibleCache", QString("Scene cache set for novel: %1, count: %2")
        .arg(novelId).arg(scenes.size()));
    emit scenesCacheUpdated(novelId);
}

void BibleCache::invalidateScenes(const QString& novelId)
{
    QMutexLocker locker(&m_cacheMutex);
    
    m_sceneCache.remove(novelId);
    m_sceneTimestamp.remove(novelId);
    
    LOG_DEBUG("BibleCache", QString("Scene cache invalidated for novel: %1").arg(novelId));
    emit cacheInvalidated(novelId);
}

void BibleCache::invalidateAllScenes()
{
    QMutexLocker locker(&m_cacheMutex);
    
    int count = m_sceneCache.size();
    m_sceneCache.clear();
    m_sceneTimestamp.clear();
    
    LOG_DEBUG("BibleCache", QString("All scene cache invalidated, count: %1").arg(count));
}

bool BibleCache::hasScenes(const QString& novelId) const
{
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_sceneCache.contains(novelId)) {
        return false;
    }
    
    QDateTime timestamp = m_sceneTimestamp.value(novelId);
    return isCacheValid(timestamp);
}

void BibleCache::invalidateNovel(const QString& novelId)
{
    invalidateCharacters(novelId);
    invalidateScenes(novelId);
}

void BibleCache::invalidateAll()
{
    invalidateAllCharacters();
    invalidateAllScenes();
}

void BibleCache::setMaxCacheAge(int seconds)
{
    m_maxCacheAgeSeconds = seconds;
}

int BibleCache::characterCacheCount() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_characterCache.size();
}

int BibleCache::sceneCacheCount() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_sceneCache.size();
}
