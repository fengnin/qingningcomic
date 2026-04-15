#ifndef ASYNCIMAGELOADER_H
#define ASYNCIMAGELOADER_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QSize>
#include <QMutex>
#include <QSet>

class QNetworkReply;

class AsyncImageLoader : public QObject
{
    Q_OBJECT

public:
    static AsyncImageLoader* instance();
    
    void loadAsync(const QString& id, const QString& pathOrUrl, const QSize& targetSize);
    void cancel(const QString& id);
    void clearCache();
    
    static QString makeCacheKey(const QString& id, const QSize& size);
    bool findCached(const QString& cacheKey, QPixmap* pixmap) const;
    void insertCache(const QString& cacheKey, const QPixmap& pixmap);

signals:
    void imageLoaded(const QString& id, const QString& cacheKey, const QPixmap& pixmap);
    void imageLoadFailed(const QString& id);

private:
    explicit AsyncImageLoader(QObject* parent = nullptr);
    ~AsyncImageLoader() = default;
    
    void loadFromNetwork(const QString& id, const QString& url, const QSize& targetSize);
    void loadFromFile(const QString& id, const QString& path, const QSize& targetSize);
    QPixmap loadImageFromFile(const QString& path, const QSize& targetSize);
    void onNetworkReplyFinished(QNetworkReply* reply, const QString& id, const QString& cacheKey, const QSize& targetSize);
    
    static AsyncImageLoader* m_instance;
    static QMutex m_mutex;
    
    mutable QMutex m_pendingMutex;
    QSet<QString> m_pendingLoads;
};

#endif // ASYNCIMAGELOADER_H
