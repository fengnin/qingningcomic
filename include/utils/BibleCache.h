#ifndef BIBLECACHE_H
#define BIBLECACHE_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QMutex>
#include <QDateTime>
#include <mutex>
#include "models/Character.h"
#include "models/Scene.h"

class BibleCache : public QObject
{
    Q_OBJECT

public:
    static BibleCache* instance();
    static void cleanup();
    
    QList<Character> getCharacters(const QString& novelId);
    void setCharacters(const QString& novelId, const QList<Character>& characters);
    void invalidateCharacters(const QString& novelId);
    void invalidateAllCharacters();
    bool hasCharacters(const QString& novelId) const;
    
    QList<Scene> getScenes(const QString& novelId);
    void setScenes(const QString& novelId, const QList<Scene>& scenes);
    void invalidateScenes(const QString& novelId);
    void invalidateAllScenes();
    bool hasScenes(const QString& novelId) const;
    
    void invalidateNovel(const QString& novelId);
    void invalidateAll();
    
    void setMaxCacheAge(int seconds);
    int maxCacheAge() const { return m_maxCacheAgeSeconds; }
    
    void setMaxCacheEntries(int maxEntries);
    int maxCacheEntries() const { return m_maxCacheEntries; }
    
    int characterCacheCount() const;
    int sceneCacheCount() const;

signals:
    void cacheInvalidated(const QString& novelId);
    void charactersCacheUpdated(const QString& novelId);
    void scenesCacheUpdated(const QString& novelId);

private:
    BibleCache(QObject* parent = nullptr);
    ~BibleCache() = default;
    BibleCache(const BibleCache&) = delete;
    BibleCache& operator=(const BibleCache&) = delete;
    
    bool isCacheValid(const QDateTime& timestamp) const;
    
    static BibleCache* m_instance;
    static QMutex m_instanceMutex;
    static std::once_flag m_instanceOnceFlag;
    
    mutable QMutex m_cacheMutex;
    
    QMap<QString, QList<Character>> m_characterCache;
    QMap<QString, QDateTime> m_characterTimestamp;
    
    QMap<QString, QList<Scene>> m_sceneCache;
    QMap<QString, QDateTime> m_sceneTimestamp;
    
    int m_maxCacheAgeSeconds = 300;
    int m_maxCacheEntries = 50;
    
    void evictIfNeeded();
};

#endif // BIBLECACHE_H
