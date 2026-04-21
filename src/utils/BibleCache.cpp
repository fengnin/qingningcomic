#include "utils/BibleCache.h"

BibleCache* BibleCache::m_instance = nullptr;
QMutex BibleCache::m_instanceMutex;
std::once_flag BibleCache::m_instanceOnceFlag;

BibleCache::BibleCache(QObject* parent)
    : QObject(parent)
{
}

BibleCache* BibleCache::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new BibleCache();
    });
    return m_instance;
}

void BibleCache::cleanup()
{
    QMutexLocker locker(&m_instanceMutex);
    if (m_instance) {
        delete m_instance;
        m_instance = nullptr;
    }
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
        return QList<Character>();
    }
    
    return m_characterCache[novelId];
}

void BibleCache::setCharacters(const QString& novelId, const QList<Character>& characters)
{
    {
        QMutexLocker locker(&m_cacheMutex);
        m_characterCache[novelId] = characters;
        m_characterTimestamp[novelId] = QDateTime::currentDateTime();
        evictIfNeeded();
    }
    emit charactersCacheUpdated(novelId);
}

void BibleCache::invalidateCharacters(const QString& novelId)
{
    {
        QMutexLocker locker(&m_cacheMutex);
        m_characterCache.remove(novelId);
        m_characterTimestamp.remove(novelId);
    }
    emit cacheInvalidated(novelId);
}

void BibleCache::invalidateAllCharacters()
{
    QMutexLocker locker(&m_cacheMutex);
    m_characterCache.clear();
    m_characterTimestamp.clear();
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
        return QList<Scene>();
    }
    
    return m_sceneCache[novelId];
}

void BibleCache::setScenes(const QString& novelId, const QList<Scene>& scenes)
{
    {
        QMutexLocker locker(&m_cacheMutex);
        m_sceneCache[novelId] = scenes;
        m_sceneTimestamp[novelId] = QDateTime::currentDateTime();
        evictIfNeeded();
    }
    emit scenesCacheUpdated(novelId);
}

void BibleCache::invalidateScenes(const QString& novelId)
{
    {
        QMutexLocker locker(&m_cacheMutex);
        m_sceneCache.remove(novelId);
        m_sceneTimestamp.remove(novelId);
    }
    emit cacheInvalidated(novelId);
}

void BibleCache::invalidateAllScenes()
{
    QMutexLocker locker(&m_cacheMutex);
    m_sceneCache.clear();
    m_sceneTimestamp.clear();
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

void BibleCache::setMaxCacheEntries(int maxEntries)
{
    m_maxCacheEntries = maxEntries;
}

void BibleCache::evictIfNeeded()
{
    if (m_maxCacheEntries <= 0) {
        return;
    }

    while (m_characterCache.size() > m_maxCacheEntries) {
        QString oldestKey;
        QDateTime oldestTime;
        for (auto it = m_characterTimestamp.constBegin(); it != m_characterTimestamp.constEnd(); ++it) {
            if (!oldestTime.isValid() || it.value() < oldestTime) {
                oldestTime = it.value();
                oldestKey = it.key();
            }
        }
        if (!oldestKey.isEmpty()) {
            m_characterCache.remove(oldestKey);
            m_characterTimestamp.remove(oldestKey);
        } else {
            break;
        }
    }

    while (m_sceneCache.size() > m_maxCacheEntries) {
        QString oldestKey;
        QDateTime oldestTime;
        for (auto it = m_sceneTimestamp.constBegin(); it != m_sceneTimestamp.constEnd(); ++it) {
            if (!oldestTime.isValid() || it.value() < oldestTime) {
                oldestTime = it.value();
                oldestKey = it.key();
            }
        }
        if (!oldestKey.isEmpty()) {
            m_sceneCache.remove(oldestKey);
            m_sceneTimestamp.remove(oldestKey);
        } else {
            break;
        }
    }
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
