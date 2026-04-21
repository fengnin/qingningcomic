#include "api/StorageClient.h"
#include "services/ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QUrl>
#include <QDateTime>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

StorageClient* StorageClient::m_instance = nullptr;
QMutex StorageClient::m_instanceMutex;

StorageClient::StorageClient()
    : m_networkManager(new QNetworkAccessManager(this))
    , m_initialized(false)
{
}

StorageClient::~StorageClient()
{
}

DEFINE_SINGLETON_INSTANCE(StorageClient, storageClient)

bool StorageClient::init(const Config& config)
{
    if (config.endpoint.isEmpty()) {
        m_lastError = TR("存储服务地址不能为空");
        LOG_ERROR("StorageClient", m_lastError);
        return false;
    }
    if (config.bucket.isEmpty()) {
        m_lastError = TR("存储桶名称不能为空");
        LOG_ERROR("StorageClient", m_lastError);
        return false;
    }
    if (config.accessKey.isEmpty() || config.secretKey.isEmpty()) {
        m_lastError = TR("存储访问密钥不能为空");
        LOG_ERROR("StorageClient", m_lastError);
        return false;
    }
    
    m_config = config;
    if (m_config.region.isEmpty()) {
        m_config.region = "us-east-1";
    }
    
    m_initialized = true;
    LOG_INFO("StorageClient", QString("Initialized with endpoint: %1, bucket: %2")
        .arg(m_config.endpoint).arg(m_config.bucket));
    return true;
}

    // ==================== 存储相关 ====================

StorageClient::UploadResult StorageClient::upload(const QString& key, const QByteArray& data,
                                                   const QString& contentType,
                                                   const QMap<QString, QString>& metadata)
{
    UploadResult result;
    
    if (!m_initialized) {
        result.errorMessage = TR("存储客户端未初始化");
        m_lastError = result.errorMessage;
        emit errorOccurred("upload", result.errorMessage);
        return result;
    }
    
    if (key.isEmpty()) {
        result.errorMessage = TR("上传路径不能为空");
        m_lastError = result.errorMessage;
        return result;
    }
    
    QString date = formatDateHeader();
    
    // 构建请求 URL
    QUrl url(QString("%1/%2").arg(m_config.endpoint).arg(key));
    QNetworkRequest request(url);
    
    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setRawHeader("Host", url.host().toUtf8());
    request.setRawHeader("Date", date.toUtf8());
    request.setRawHeader("Content-Length", QByteArray::number(data.size()));
    // 添加自定义元数据
    
    QMap<QString, QString> headers;
    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
        QString headerName = QString("x-amz-meta-%1").arg(it.key().toLower());
        request.setRawHeader(headerName.toUtf8(), it.value().toUtf8());
        headers[headerName] = it.value();
    }
    // 生成签名
    
    QByteArray authHeader = buildAuthorizationHeader("PUT", key, contentType, headers, date);
    request.setRawHeader("Authorization", authHeader);
    
    // 发送请求
    QNetworkReply* reply = m_networkManager->put(request, data);
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(60000); // 60 秒超时
    loop.exec();
    
    if (!timer.isActive()) {
        reply->abort();
        reply->deleteLater();
        result.errorMessage = TR("上传超时");
        m_lastError = result.errorMessage;
        return result;
    }
    timer.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        result.errorMessage = reply->errorString();
        m_lastError = result.errorMessage;
        reply->deleteLater();
        return result;
    }
    
    reply->deleteLater();
    
    result.success = true;
    result.uri = getS3Uri(key);
    result.url = url.toString();
    
    return result;
}

StorageClient::DownloadResult StorageClient::download(const QString& key)
{
    DownloadResult result;
    
    if (!m_initialized) {
        result.errorMessage = TR("下载客户端未初始化");
        m_lastError = result.errorMessage;
        emit errorOccurred("download", result.errorMessage);
        return result;
    }
    
    if (key.isEmpty()) {
        result.errorMessage = TR("下载路径不能为空");
        m_lastError = result.errorMessage;
        return result;
    }
    
    QString date = formatDateHeader();
    
    // 构建请求 URL
    QUrl url(QString("%1/%2").arg(m_config.endpoint).arg(key));
    QNetworkRequest request(url);
    
    request.setRawHeader("Host", url.host().toUtf8());
    request.setRawHeader("Date", date.toUtf8());
    
    // 生成签名
    QByteArray authHeader = buildAuthorizationHeader("GET", key, "", QMap<QString, QString>(), date);
    request.setRawHeader("Authorization", authHeader);
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(60000);
    loop.exec();
    
    if (!timer.isActive()) {
        reply->abort();
        reply->deleteLater();
        result.errorMessage = TR("下载超时");
        m_lastError = result.errorMessage;
        return result;
    }
    timer.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        result.errorMessage = reply->errorString();
        m_lastError = result.errorMessage;
        reply->deleteLater();
        return result;
    }
    
    result.success = true;
    result.data = reply->readAll();
    result.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    
    reply->deleteLater();
    
    return result;
}

bool StorageClient::remove(const QString& key)
{
    if (!m_initialized) {
        m_lastError = TR("存储客户端未初始化");
        emit errorOccurred("remove", m_lastError);
        return false;
    }
    
    if (key.isEmpty()) {
        m_lastError = TR("删除路径不能为空");
        return false;
    }
    
    QString date = formatDateHeader();
    
    QUrl url(QString("%1/%2").arg(m_config.endpoint).arg(key));
    QNetworkRequest request(url);
    
    request.setRawHeader("Host", url.host().toUtf8());
    request.setRawHeader("Date", date.toUtf8());
    
    QByteArray authHeader = buildAuthorizationHeader("DELETE", key, "", QMap<QString, QString>(), date);
    request.setRawHeader("Authorization", authHeader);
    
    QNetworkReply* reply = m_networkManager->deleteResource(request);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    bool success = (reply->error() == QNetworkReply::NoError);
    if (!success) {
        m_lastError = reply->errorString();
    }
    
    reply->deleteLater();
    
    return success;
}

QString StorageClient::getPresignedUrl(const QString& key, int expiresIn)
{
    if (!m_initialized) {
        m_lastError = TR("存储客户端未初始化");
        return QString();
    }
    
    int expire = (expiresIn > 0) ? expiresIn : m_config.presignExpiresIn;
    return generatePresignedUrl(key, expire);
}

bool StorageClient::exists(const QString& key)
{
    if (!m_initialized || key.isEmpty()) {
        return false;
    }
    
    QString date = formatDateHeader();
    
    QUrl url(QString("%1/%2").arg(m_config.endpoint).arg(key));
    QNetworkRequest request(url);
    
    request.setRawHeader("Host", url.host().toUtf8());
    request.setRawHeader("Date", date.toUtf8());
    
    QByteArray authHeader = buildAuthorizationHeader("HEAD", key, "", QMap<QString, QString>(), date);
    request.setRawHeader("Authorization", authHeader);
    
    QNetworkReply* reply = m_networkManager->head(request);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    bool exists = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    
    return exists;
}

bool StorageClient::copy(const QString& sourceKey, const QString& destKey)
{
    if (!m_initialized) {
        m_lastError = TR("存储客户端未初始化");
        return false;
    }
    
    QString date = formatDateHeader();
    
    QUrl url(QString("%1/%2").arg(m_config.endpoint).arg(destKey));
    QNetworkRequest request(url);
    
    request.setRawHeader("Host", url.host().toUtf8());
    request.setRawHeader("Date", date.toUtf8());
    request.setRawHeader("x-amz-copy-source", QString("%1/%2").arg(m_config.bucket).arg(sourceKey).toUtf8());
    
    QByteArray authHeader = buildAuthorizationHeader("PUT", destKey, "", 
        {{"x-amz-copy-source", QString("%1/%2").arg(m_config.bucket).arg(sourceKey)}}, date);
    request.setRawHeader("Authorization", authHeader);
    
    QNetworkReply* reply = m_networkManager->put(request, QByteArray());
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    bool success = (reply->error() == QNetworkReply::NoError);
    if (!success) {
        m_lastError = reply->errorString();
    }
    
    reply->deleteLater();
    
    return success;
}

QString StorageClient::getS3Uri(const QString& key) const
{
    return QString("s3://%1/%2").arg(m_config.bucket).arg(key);
}

bool StorageClient::parseS3Uri(const QString& uri, QString& bucket, QString& key)
{
    if (!uri.startsWith("s3://")) {
        return false;
    }
    
    QString path = uri.mid(5);
    int slashIndex = path.indexOf('/');
    if (slashIndex < 0) {
        return false;
    }
    
    bucket = path.left(slashIndex);
    key = path.mid(slashIndex + 1);
    return true;
}

// ==================== 预签名 API 方法 ====================

StorageClient::PresignedUrlResponse StorageClient::fetchPresignedUrlFromApi(
    const QString& key,
    const QString& method,
    int expiresIn,
    const QString& contentType,
    qint64 contentLength)
{
    PresignedUrlResponse response;
    
    if (m_config.presignApiEndpoint.isEmpty()) {
        response.errorMessage = TR("预签名接口地址不能为空");
        m_lastError = response.errorMessage;
        LOG_ERROR("StorageClient", response.errorMessage);
        return response;
    }
    
    QUrl url(m_config.presignApiEndpoint);
    QUrlQuery query;
    query.addQueryItem("key", key);
    query.addQueryItem("method", method);
    query.addQueryItem("expiresIn", QString::number(expiresIn));
    query.addQueryItem("contentType", contentType);
    if (contentLength > 0) {
        query.addQueryItem("contentLength", QString::number(contentLength));
    }
    url.setQuery(query);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    if (!m_config.authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_config.authToken).toUtf8());
    }
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        response.errorMessage = reply->errorString();
        m_lastError = response.errorMessage;
        LOG_ERROR("StorageClient", QString("Failed to fetch presigned URL: %1").arg(response.errorMessage));
        reply->deleteLater();
        return response;
    }
    
    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        response.errorMessage = TR("预签名接口返回无效");
        m_lastError = response.errorMessage;
        return response;
    }
    
    QJsonObject obj = doc.object();
    response.putUrl = obj["putUrl"].toString();
    response.getUrl = obj["getUrl"].toString();
    response.key = obj["key"].toString();
    response.expiresIn = obj["expiresIn"].toInt();
    response.success = true;
    
    return response;
}

StorageClient::UploadResult StorageClient::uploadWithPresignedUrl(
    const QString& presignedUrl,
    const QByteArray& data,
    const QString& contentType)
{
    UploadResult result;
    
    if (presignedUrl.isEmpty()) {
        result.errorMessage = TR("获取预签名地址失败");
        m_lastError = result.errorMessage;
        return result;
    }
    
    QUrl url(presignedUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setRawHeader("Content-Length", QByteArray::number(data.size()));
    
    QNetworkReply* reply = m_networkManager->put(request, data);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        result.errorMessage = reply->errorString();
        m_lastError = result.errorMessage;
        LOG_ERROR("StorageClient", QString("Upload failed: %1").arg(result.errorMessage));
    } else {
        result.success = true;
        result.url = presignedUrl.left(presignedUrl.indexOf("?"));
    }
    
    reply->deleteLater();
    return result;
}

StorageClient::DownloadResult StorageClient::downloadWithPresignedUrl(const QString& presignedUrl)
{
    DownloadResult result;
    
    if (presignedUrl.isEmpty()) {
        result.errorMessage = TR("获取预签名地址失败");
        m_lastError = result.errorMessage;
        return result;
    }
    
    QUrl url(presignedUrl);
    QNetworkRequest request(url);
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        result.errorMessage = reply->errorString();
        m_lastError = result.errorMessage;
        LOG_ERROR("StorageClient", QString("Download failed: %1").arg(result.errorMessage));
    } else {
        result.success = true;
        result.data = reply->readAll();
        result.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    }
    
    reply->deleteLater();
    return result;
}

// ==================== 签名相关 ====================

QByteArray StorageClient::buildAuthorizationHeader(const QString& method, const QString& key,
                                                    const QString& contentType,
                                                    const QMap<QString, QString>& headers,
                                                    const QString& date)
{
    // AWS Signature Version 4 简化实现
    // 格式: AWS4-HMAC-SHA256 Credential=access/date/region/s3/aws4_request,
    //       SignedHeaders=host;date;x-amz-..., Signature=...
    
    QString dateStamp = date.left(8); // YYYYMMDD
    
    // 构建规范请求
    QStringList signedHeadersList;
    QStringList canonicalHeaders;
    
    // 添加 host
    QUrl endpointUrl(m_config.endpoint);
    canonicalHeaders << QString("host:%1").arg(endpointUrl.host());
    signedHeadersList << "host";
    
    // 添加 date
    canonicalHeaders << QString("x-amz-date:%1").arg(date);
    signedHeadersList << "x-amz-date";
    
    // 添加 content-type (用于 PUT 请求)
    if (!contentType.isEmpty() && method == "PUT") {
        canonicalHeaders << QString("content-type:%1").arg(contentType);
        signedHeadersList << "content-type";
    }
    
    // 添加自定义头
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        canonicalHeaders << QString("%1:%2").arg(it.key().toLower()).arg(it.value());
        signedHeadersList << it.key().toLower();
    }
    
    std::sort(signedHeadersList.begin(), signedHeadersList.end());
    QString signedHeaders = signedHeadersList.join(";");
    
    // 构建待签名字符串
    QString canonicalRequest = QString("%1\n/%2\n\n%3\n\n%4\nUNSIGNED-PAYLOAD")
        .arg(method)
        .arg(key)
        .arg(canonicalHeaders.join("\n"))
        .arg(signedHeaders);
    
    QString stringToSign = QString("AWS4-HMAC-SHA256\n%1\n%2/%3/s3/aws4_request\n%4")
        .arg(date)
        .arg(dateStamp)
        .arg(m_config.region)
        .arg(sha256Hash(canonicalRequest.toUtf8()));
    
    // 计算签名
    QByteArray kSecret = QString("AWS4%1").arg(m_config.secretKey).toUtf8();
    QByteArray kDate = hmacSha256(kSecret, dateStamp.toUtf8());
    QByteArray kRegion = hmacSha256(kDate, m_config.region.toUtf8());
    QByteArray kService = hmacSha256(kRegion, "s3");
    QByteArray kSigning = hmacSha256(kService, "aws4_request");
    QByteArray signature = hmacSha256(kSigning, stringToSign.toUtf8());
    
    QString authHeader = QString("AWS4-HMAC-SHA256 Credential=%1/%2/%3/s3/aws4_request,"
                                 "SignedHeaders=%4,Signature=%5")
        .arg(m_config.accessKey)
        .arg(dateStamp)
        .arg(m_config.region)
        .arg(signedHeaders)
        .arg(QString::fromLatin1(signature.toHex()));
    
    return authHeader.toUtf8();
}

QString StorageClient::generatePresignedUrl(const QString& key, int expiresIn, const QString& method)
{
    // 生成预签名 URL
    QString date = QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmssZ");
    QString dateStamp = date.left(8);
    
    // 构建规范请求
    QString canonicalRequest = QString("%1\n/%2\n\nhost:%3\n\nhost\nUNSIGNED-PAYLOAD")
        .arg(method)
        .arg(key)
        .arg(QUrl(m_config.endpoint).host());
    
    QString stringToSign = QString("AWS4-HMAC-SHA256\n%1\n%2/%3/s3/aws4_request\n%4")
        .arg(date)
        .arg(dateStamp)
        .arg(m_config.region)
        .arg(sha256Hash(canonicalRequest.toUtf8()));
    
    // 计算签名
    QByteArray kSecret = QString("AWS4%1").arg(m_config.secretKey).toUtf8();
    QByteArray kDate = hmacSha256(kSecret, dateStamp.toUtf8());
    QByteArray kRegion = hmacSha256(kDate, m_config.region.toUtf8());
    QByteArray kService = hmacSha256(kRegion, "s3");
    QByteArray kSigning = hmacSha256(kService, "aws4_request");
    QByteArray signature = hmacSha256(kSigning, stringToSign.toUtf8());
    
    // 构建预签名 URL
    QString presignedUrl = QString("%1/%2?"
        "X-Amz-Algorithm=AWS4-HMAC-SHA256&"
        "X-Amz-Credential=%3/%4/%5/s3/aws4_request&"
        "X-Amz-Date=%6&"
        "X-Amz-Expires=%7&"
        "X-Amz-SignedHeaders=host&"
        "X-Amz-Signature=%8")
        .arg(m_config.endpoint)
        .arg(key)
        .arg(urlEncode(m_config.accessKey))
        .arg(dateStamp)
        .arg(m_config.region)
        .arg(date)
        .arg(expiresIn)
        .arg(QString::fromLatin1(signature.toHex()));
    
    return presignedUrl;
}

QString StorageClient::formatDateHeader()
{
    return QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmssZ");
}

QByteArray StorageClient::hmacSha256(const QByteArray& key, const QByteArray& data)
{
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

QString StorageClient::sha256Hash(const QByteArray& data)
{
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

QString StorageClient::urlEncode(const QString& str, bool encodeSlash)
{
    QString result = QUrl::toPercentEncoding(str).constData();
    if (!encodeSlash) {
        result.replace("%2F", "/");
    }
    return result;
}
