#include "api/VolcEngineSignature.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>

namespace VolcEngineSignature {

QString buildCanonicalRequest(const QString& method,
                               const QString& path,
                               const QString& query,
                               const QString& canonicalHeaders,
                               const QString& signedHeaders,
                               const QString& hashedPayload)
{
    return QString("%1\n%2\n%3\n%4\n%5\n%6")
        .arg(method)
        .arg(path.isEmpty() ? "/" : path)
        .arg(query)
        .arg(canonicalHeaders)
        .arg(signedHeaders)
        .arg(hashedPayload);
}

QString buildStringToSign(const QString& algorithm,
                           const QString& dateTime,
                           const QString& credentialScope,
                           const QString& hashedCanonicalRequest)
{
    return QString("%1\n%2\n%3\n%4")
        .arg(algorithm)
        .arg(dateTime)
        .arg(credentialScope)
        .arg(hashedCanonicalRequest);
}

QString buildCredentialScope(const QString& date,
                              const QString& region,
                              const QString& service)
{
    return QString("%1/%2/%3/request")
        .arg(date)
        .arg(region)
        .arg(service);
}

QString buildCanonicalHeaders(const QString& host, const QString& dateTime)
{
    return QString("content-type:application/json\nhost:%1\nx-date:%2\n")
        .arg(host)
        .arg(dateTime);
}

QString sha256Hash(const QByteArray& data)
{
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

QByteArray hmacSha256(const QByteArray& key, const QByteArray& data)
{
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

QString calculateSignature(const QString& secretKey,
                            const QString& date,
                            const QString& region,
                            const QString& service,
                            const QString& stringToSign)
{
    // 火山引擎 V4 签名派生流程
    // 官方文档: https://www.volcengine.com/docs/6369/67269
    QByteArray kDate = hmacSha256(secretKey.toUtf8(), date.toUtf8());
    QByteArray kRegion = hmacSha256(kDate, region.toUtf8());
    QByteArray kService = hmacSha256(kRegion, service.toUtf8());
    QByteArray kSigning = hmacSha256(kService, QByteArray("request"));
    QByteArray signature = hmacSha256(kSigning, stringToSign.toUtf8());
    
    return QString::fromLatin1(signature.toHex());
}

QString buildAuthorization(const Config& config, const RequestInfo& request)
{
    QString dateTimeStr = formatDateTime(request.timestamp);
    QString dateStr = formatDate(request.timestamp);
    
    // 1. 计算请求体哈希
    QString hashedPayload = sha256Hash(request.body.toUtf8());
    
    // 2. 构建规范请求头
    QString canonicalHeaders = buildCanonicalHeaders(request.host, dateTimeStr);
    QString signedHeaders = "content-type;host;x-date";
    
    // 3. 构建规范请求
    QString canonicalRequest = buildCanonicalRequest(
        request.method,
        request.path,
        request.query,
        canonicalHeaders,
        signedHeaders,
        hashedPayload
    );
    
    // 4. 构建凭证范围
    QString credentialScope = buildCredentialScope(dateStr, config.region, config.service);
    
    // 5. 构建待签名字符串
    QString hashedCanonicalRequest = sha256Hash(canonicalRequest.toUtf8());
    QString stringToSign = buildStringToSign(
        "HMAC-SHA256",
        dateTimeStr,
        credentialScope,
        hashedCanonicalRequest
    );
    
    // 6. 计算签名
    QString signature = calculateSignature(
        config.secretKey,
        dateStr,
        config.region,
        config.service,
        stringToSign
    );
    
    // 7. 构建 Authorization 头
    return QString("HMAC-SHA256 Credential=%1/%2, SignedHeaders=%3, Signature=%4")
        .arg(config.accessKey)
        .arg(credentialScope)
        .arg(signedHeaders)
        .arg(signature);
}

QString formatDateTime(const QDateTime& dateTime)
{
    return dateTime.toUTC().toString("yyyyMMddTHHmmssZ");
}

QString formatDate(const QDateTime& dateTime)
{
    return dateTime.toUTC().toString("yyyyMMdd");
}

} // namespace VolcEngineSignature
