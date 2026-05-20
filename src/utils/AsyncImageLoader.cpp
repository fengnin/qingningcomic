#include "utils/AsyncImageLoader.h"
#include "api/SharedNetworkManager.h"
#include "utils/Logger.h"
#include <QFileInfo>
#include <QPixmapCache>
#include <QNetworkReply>
#include <QNetworkRequest>

AsyncImageLoader* AsyncImageLoader::m_instance = nullptr;
QMutex AsyncImageLoader::m_mutex;

AsyncImageLoader::AsyncImageLoader(QObject* parent)
    : QObject(parent)
{
    QPixmapCache::setCacheLimit(10240);
}

AsyncImageLoader* AsyncImageLoader::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_mutex);
        if (!m_instance) {
            m_instance = new AsyncImageLoader();
        }
    }
    return m_instance;
}

QString AsyncImageLoader::makeCacheKey(const QString& id, const QSize& size)
{
    return QString("async_%1_%2x%3").arg(id).arg(size.width()).arg(size.height());
}

bool AsyncImageLoader::findCached(const QString& cacheKey, QPixmap* pixmap) const
{
    return QPixmapCache::find(cacheKey, pixmap);
}

void AsyncImageLoader::insertCache(const QString& cacheKey, const QPixmap& pixmap)
{
    QPixmapCache::insert(cacheKey, pixmap);
}

void AsyncImageLoader::loadAsync(const QString& id, const QString& pathOrUrl, const QSize& targetSize)
{
    QString cacheKey = makeCacheKey(id, targetSize);
    
    QPixmap cachedPixmap;
    if (findCached(cacheKey, &cachedPixmap)) {
        emit imageLoaded(id, cacheKey, cachedPixmap);
        return;
    }
    
    {
        QMutexLocker locker(&m_pendingMutex);
        if (m_pendingLoads.contains(id)) {
            return;
        }
        m_pendingLoads.insert(id);
    }
    
    if (pathOrUrl.startsWith("http://") || pathOrUrl.startsWith("https://")) {
        loadFromNetwork(id, pathOrUrl, targetSize);
    } else {
        loadFromFile(id, pathOrUrl, targetSize);
    }
}

void AsyncImageLoader::loadFromNetwork(const QString& id, const QString& url, const QSize& targetSize)
{
    QString cacheKey = makeCacheKey(id, targetSize);
    
    QNetworkRequest request{QUrl(url)};
    QNetworkReply* reply = SharedNetworkManager::instance()->manager()->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, id, cacheKey, targetSize]() {
        onNetworkReplyFinished(reply, id, cacheKey, targetSize);
    });
}

void AsyncImageLoader::onNetworkReplyFinished(QNetworkReply* reply, const QString& id, const QString& cacheKey, const QSize& targetSize)
{
    reply->deleteLater();
    
    {
        QMutexLocker locker(&m_pendingMutex);
        m_pendingLoads.remove(id);
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        LOG_WARNING("AsyncImageLoader", QString("Network error for %1: %2").arg(id, reply->errorString()));
        emit imageLoadFailed(id);
        return;
    }
    
    QByteArray imageData = reply->readAll();
    QPixmap pixmap;
    
    if (pixmap.loadFromData(imageData)) {
        if (targetSize.isValid() && (pixmap.width() > targetSize.width() || pixmap.height() > targetSize.height())) {
            pixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        emit imageLoaded(id, cacheKey, pixmap);
    } else {
        LOG_WARNING("AsyncImageLoader", QString("Failed to decode network image: %1").arg(id));
        emit imageLoadFailed(id);
    }
}

void AsyncImageLoader::loadFromFile(const QString& id, const QString& path, const QSize& targetSize)
{
    QString cacheKey = makeCacheKey(id, targetSize);
    QPixmap pixmap = loadImageFromFile(path, targetSize);

    {
        QMutexLocker locker(&m_pendingMutex);
        m_pendingLoads.remove(id);
    }

    if (!pixmap.isNull()) {
        emit imageLoaded(id, cacheKey, pixmap);
    } else {
        emit imageLoadFailed(id);
    }
}

void AsyncImageLoader::cancel(const QString& id)
{
    QMutexLocker locker(&m_pendingMutex);
    m_pendingLoads.remove(id);
}

void AsyncImageLoader::clearCache()
{
    QPixmapCache::clear();
    QMutexLocker locker(&m_pendingMutex);
    m_pendingLoads.clear();
}

QPixmap AsyncImageLoader::loadImageFromFile(const QString& path, const QSize& targetSize)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        LOG_DEBUG("AsyncImageLoader", QString("File not found: %1").arg(path));
        return QPixmap();
    }
    
    QPixmap pixmap;
    if (!pixmap.load(path)) {
        LOG_DEBUG("AsyncImageLoader", QString("Failed to load image: %1").arg(path));
        return QPixmap();
    }
    
    if (targetSize.isValid() && (pixmap.width() > targetSize.width() || pixmap.height() > targetSize.height())) {
        pixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    return pixmap;
}
