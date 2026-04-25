#include "api/StorageClient.h"
#include "services/ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include <algorithm>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QUrl>
#include <QDateTime>
#include <QList>
#include <QPair>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
const int kStorageRequestTimeoutMs = 60000;

QUrl buildObjectUrl(const StorageClient::Config& config, const QString& key)
{
    return QUrl(QString("%1/%2").arg(config.endpoint).arg(key));
}

void applyObjectRequestHeaders(QNetworkRequest& request, const QUrl& url, const QString& date)
{
    request.setRawHeader("Host", url.host().toUtf8());
    request.setRawHeader("Date", date.toUtf8());
}

bool waitForReply(QNetworkReply* reply, int timeoutMs, const QString& timeoutErrorMessage, QString* errorMessage)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timer.start(timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        if (errorMessage != nullptr) {
            *errorMessage = timeoutErrorMessage;
        }
        reply->deleteLater();
        return false;
    }

    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage = reply->errorString();
        }
        reply->deleteLater();
        return false;
    }

    return true;
}

bool parsePresignedResponse(const QByteArray& responseData, StorageClient::PresignedUrlResponse& response, QString* errorMessage)
{
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = TR("预签名接口返回无效");
        }
        return false;
    }

    QJsonObject obj = doc.object();
    response.putUrl = obj["putUrl"].toString();
    response.getUrl = obj["getUrl"].toString();
    response.key = obj["key"].toString();
    response.expiresIn = obj["expiresIn"].toInt();
    response.success = true;
    return true;
}

QString strippedQueryUrl(const QString& presignedUrl)
{
    QUrl url(presignedUrl);
    url.setQuery(QString());
    return url.toString();
}
}

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

bool StorageClient::ensureInitialized(const QString& operation, const QString& errorMessage, bool emitSignal)
{
    if (m_initialized) {
        return true;
    }

    setLastError(errorMessage, operation, emitSignal);
    return false;
}

bool StorageClient::ensureKey(const QString& key, const QString& errorMessage)
{
    if (!key.isEmpty()) {
        return true;
    }

    setLastError(errorMessage);
    return false;
}

void StorageClient::setLastError(const QString& message, const QString& operation, bool emitSignal)
{
    m_lastError = message;
    if (!operation.isEmpty() && emitSignal) {
        emit errorOccurred(operation, message);
    }
}

bool StorageClient::waitForNetworkReply(QNetworkReply* reply, int timeoutMs, const QString& timeoutErrorMessage)
{
    QString errorMessage;
    if (::waitForReply(reply, timeoutMs, timeoutErrorMessage, &errorMessage)) {
        return true;
    }

    m_lastError = errorMessage;
    return false;
}

bool StorageClient::parsePresignedUrlResponse(const QByteArray& responseData, PresignedUrlResponse& response)
{
    QString errorMessage;
    if (parsePresignedResponse(responseData, response, &errorMessage)) {
        return true;
    }

    m_lastError = errorMessage;
    return false;
}

    // ==================== 存储相关 ====================

StorageClient::UploadResult StorageClient::upload(const QString& key, const QByteArray& data,
                                                   const QString& contentType,
                                                   const QMap<QString, QString>& metadata)
{
    UploadResult result;
    
    if (!ensureInitialized("upload", TR("存储客户端未初始化"), true)) {
        result.errorMessage = m_lastError;
        return result;
    }
    
    if (!ensureKey(key, TR("上传路径不能为空"))) {
        result.errorMessage = m_lastError;
        return result;
    }
    
    const QString date = formatDateHeader();
    const QUrl url = buildObjectUrl(m_config, key);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    applyObjectRequestHeaders(request, url, date);
    request.setRawHeader("Content-Length", QByteArray::number(data.size()));

    QMap<QString, QString> headers;
    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
        const QString headerName = QString("x-amz-meta-%1").arg(it.key().toLower());
        request.setRawHeader(headerName.toUtf8(), it.value().toUtf8());
        headers[headerName] = it.value();
    }

    const QByteArray authHeader = buildAuthorizationHeader("PUT", key, contentType, headers, date);
    request.setRawHeader("Authorization", authHeader);

    QNetworkReply* reply = m_networkManager->put(request, data);
    if (!waitForNetworkReply(reply, kStorageRequestTimeoutMs, TR("上传超时"))) {
        result.errorMessage = m_lastError;
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
    
    if (!ensureInitialized("download", TR("下载客户端未初始化"), true)) {
        result.errorMessage = m_lastError;
        return result;
    }
    
    if (!ensureKey(key, TR("下载路径不能为空"))) {
        result.errorMessage = m_lastError;
        return result;
    }
    
    const QString date = formatDateHeader();
    const QUrl url = buildObjectUrl(m_config, key);
    QNetworkRequest request(url);
    applyObjectRequestHeaders(request, url, date);

    const QByteArray authHeader = buildAuthorizationHeader("GET", key, "", QMap<QString, QString>(), date);
    request.setRawHeader("Authorization", authHeader);

    QNetworkReply* reply = m_networkManager->get(request);
    if (!waitForNetworkReply(reply, kStorageRequestTimeoutMs, TR("下载超时"))) {
        result.errorMessage = m_lastError;
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
    if (!ensureInitialized("remove", TR("存储客户端未初始化"), true)) {
        return false;
    }
    
    if (!ensureKey(key, TR("删除路径不能为空"))) {
        return false;
    }
    
    const QString date = formatDateHeader();
    const QUrl url = buildObjectUrl(m_config, key);
    QNetworkRequest request(url);
    applyObjectRequestHeaders(request, url, date);

    const QByteArray authHeader = buildAuthorizationHeader("DELETE", key, "", QMap<QString, QString>(), date);
    request.setRawHeader("Authorization", authHeader);

    QNetworkReply* reply = m_networkManager->deleteResource(request);
    if (!waitForNetworkReply(reply, kStorageRequestTimeoutMs, TR("删除超时"))) {
        return false;
    }
    
    reply->deleteLater();

    return true;
}

QString StorageClient::getPresignedUrl(const QString& key, int expiresIn)
{
    if (!ensureInitialized("getPresignedUrl", TR("存储客户端未初始化"), false)) {
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
    
    const QString date = formatDateHeader();
    const QUrl url = buildObjectUrl(m_config, key);
    QNetworkRequest request(url);
    applyObjectRequestHeaders(request, url, date);

    const QByteArray authHeader = buildAuthorizationHeader("HEAD", key, "", QMap<QString, QString>(), date);
    request.setRawHeader("Authorization", authHeader);

    QNetworkReply* reply = m_networkManager->head(request);

    if (!waitForNetworkReply(reply, kStorageRequestTimeoutMs, TR("检查对象是否存在超时"))) {
        return false;
    }

    reply->deleteLater();

    return true;
}

bool StorageClient::copy(const QString& sourceKey, const QString& destKey)
{
    if (!ensureInitialized("copy", TR("存储客户端未初始化"), false)) {
        return false;
    }
    
    const QString date = formatDateHeader();
    const QUrl url = buildObjectUrl(m_config, destKey);
    QNetworkRequest request(url);
    applyObjectRequestHeaders(request, url, date);

    const QString copySource = QString("%1/%2").arg(m_config.bucket).arg(sourceKey);
    request.setRawHeader("x-amz-copy-source", copySource.toUtf8());

    const QByteArray authHeader = buildAuthorizationHeader("PUT", destKey, "",
        {{"x-amz-copy-source", copySource}}, date);
    request.setRawHeader("Authorization", authHeader);

    QNetworkReply* reply = m_networkManager->put(request, QByteArray());
    if (!waitForNetworkReply(reply, kStorageRequestTimeoutMs, TR("复制对象超时"))) {
        return false;
    }
    
    reply->deleteLater();

    return true;
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
        setLastError(response.errorMessage);
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

    if (!waitForNetworkReply(reply, kStorageRequestTimeoutMs, TR("获取预签名地址超时"))) {
        response.errorMessage = m_lastError;
        LOG_ERROR("StorageClient", QString("Failed to fetch presigned URL: %1").arg(response.errorMessage));
        return response;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    if (!parsePresignedUrlResponse(responseData, response)) {
        response.errorMessage = m_lastError;
        LOG_ERROR("StorageClient", QString("Failed to parse presigned URL response: %1").arg(response.errorMessage));
        return response;
    }

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
        setLastError(result.errorMessage);
        return result;
    }
    
    QUrl url(presignedUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setRawHeader("Content-Length", QByteArray::number(data.size()));
    
    QNetworkReply* reply = m_networkManager->put(request, data);

    if (!waitForNetworkReply(reply, kStorageRequestTimeoutMs, TR("预签名上传超时"))) {
        result.errorMessage = m_lastError;
        LOG_ERROR("StorageClient", QString("Upload failed: %1").arg(result.errorMessage));
        return result;
    }

    result.success = true;
    result.url = strippedQueryUrl(presignedUrl);
    reply->deleteLater();
    return result;
}

StorageClient::DownloadResult StorageClient::downloadWithPresignedUrl(const QString& presignedUrl)
{
    DownloadResult result;
    
    if (presignedUrl.isEmpty()) {
        result.errorMessage = TR("获取预签名地址失败");
        setLastError(result.errorMessage);
        return result;
    }
    
    QUrl url(presignedUrl);
    QNetworkRequest request(url);
    
    QNetworkReply* reply = m_networkManager->get(request);

    if (!waitForNetworkReply(reply, kStorageRequestTimeoutMs, TR("预签名下载超时"))) {
        result.errorMessage = m_lastError;
        LOG_ERROR("StorageClient", QString("Download failed: %1").arg(result.errorMessage));
        return result;
    }

    result.success = true;
    result.data = reply->readAll();
    result.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
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
    
    QUrl endpointUrl(m_config.endpoint);
    QList<QPair<QString, QString> > canonicalHeaders;
    canonicalHeaders.append(qMakePair(QString("host"), QString("host:%1").arg(endpointUrl.host())));
    canonicalHeaders.append(qMakePair(QString("x-amz-date"), QString("x-amz-date:%1").arg(date)));

    if (!contentType.isEmpty() && method == "PUT") {
        canonicalHeaders.append(qMakePair(QString("content-type"), QString("content-type:%1").arg(contentType)));
    }

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        const QString headerName = it.key().toLower();
        canonicalHeaders.append(qMakePair(headerName, QString("%1:%2").arg(headerName).arg(it.value())));
    }

    std::sort(canonicalHeaders.begin(), canonicalHeaders.end(),
              [](const QPair<QString, QString>& left, const QPair<QString, QString>& right) {
                  return left.first < right.first;
              });

    QStringList signedHeadersList;
    QStringList canonicalHeaderLines;
    for (const auto& header : canonicalHeaders) {
        signedHeadersList << header.first;
        canonicalHeaderLines << header.second;
    }

    const QString signedHeaders = signedHeadersList.join(";");

    QString canonicalRequest = QString("%1\n/%2\n\n%3\n\n%4\nUNSIGNED-PAYLOAD")
        .arg(method)
        .arg(key)
        .arg(canonicalHeaderLines.join("\n"))
        .arg(signedHeaders);

    QString stringToSign = QString("AWS4-HMAC-SHA256\n%1\n%2/%3/s3/aws4_request\n%4")
        .arg(date)
        .arg(dateStamp)
        .arg(m_config.region)
        .arg(sha256Hash(canonicalRequest.toUtf8()));

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
    QString date = QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmssZ");
    QString dateStamp = date.left(8);

    QString canonicalRequest = QString("%1\n/%2\n\nhost:%3\n\nhost\nUNSIGNED-PAYLOAD")
        .arg(method)
        .arg(key)
        .arg(QUrl(m_config.endpoint).host());

    QString stringToSign = QString("AWS4-HMAC-SHA256\n%1\n%2/%3/s3/aws4_request\n%4")
        .arg(date)
        .arg(dateStamp)
        .arg(m_config.region)
        .arg(sha256Hash(canonicalRequest.toUtf8()));

    QByteArray kSecret = QString("AWS4%1").arg(m_config.secretKey).toUtf8();
    QByteArray kDate = hmacSha256(kSecret, dateStamp.toUtf8());
    QByteArray kRegion = hmacSha256(kDate, m_config.region.toUtf8());
    QByteArray kService = hmacSha256(kRegion, "s3");
    QByteArray kSigning = hmacSha256(kService, "aws4_request");
    QByteArray signature = hmacSha256(kSigning, stringToSign.toUtf8());

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
