#ifndef VOLCENGINESIGNATURE_H
#define VOLCENGINESIGNATURE_H

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMap>

/**
 * @brief 火山引擎 API 签名工具类
 * 
 * 实现火山引擎 V4 签名算法，用于 API 请求认证。
 * 
 * 签名流程：
 * 1. 构建规范请求（CanonicalRequest）
 * 2. 构建待签名字符串（StringToSign）
 * 3. 计算签名（Signature）
 * 4. 构建 Authorization 头
 * 
 * 参考：https://www.volcengine.com/docs/6348/71196
 */
namespace VolcEngineSignature {

/**
 * @brief 签名配置
 */
struct Config {
    QString accessKey;      // Access Key ID
    QString secretKey;      // Secret Access Key
    QString region;         // 区域，如 cn-north-1
    QString service;        // 服务名，如 cv
};

/**
 * @brief 请求信息
 */
struct RequestInfo {
    QString method;         // HTTP 方法，如 POST
    QString path;           // 请求路径，如 /
    QString query;          // 查询字符串
    QString body;           // 请求体
    QString host;           // 主机名
    QDateTime timestamp;    // 请求时间戳
};

/**
 * @brief 构建规范请求
 * 
 * 格式：
 * HTTPMethod\n
 * CanonicalURI\n
 * CanonicalQueryString\n
 * CanonicalHeaders\n
 * SignedHeaders\n
 * HashedPayload
 */
QString buildCanonicalRequest(const QString& method,
                               const QString& path,
                               const QString& query,
                               const QString& canonicalHeaders,
                               const QString& signedHeaders,
                               const QString& hashedPayload);

/**
 * @brief 构建待签名字符串
 * 
 * 格式：
 * Algorithm\n
 * RequestDateTime\n
 * CredentialScope\n
 * HashedCanonicalRequest
 */
QString buildStringToSign(const QString& algorithm,
                           const QString& dateTime,
                           const QString& credentialScope,
                           const QString& hashedCanonicalRequest);

/**
 * @brief 构建凭证范围
 * 
 * 格式：YYYYMMDD/region/service/request
 */
QString buildCredentialScope(const QString& date,
                              const QString& region,
                              const QString& service);

/**
 * @brief 构建规范请求头
 * 
 * 格式：
 * content-type:xxx\n
 * host:xxx\n
 * x-date:xxx\n
 */
QString buildCanonicalHeaders(const QString& host, const QString& dateTime);

/**
 * @brief 计算 SHA256 哈希
 */
QString sha256Hash(const QByteArray& data);

/**
 * @brief 计算 HMAC-SHA256
 */
QByteArray hmacSha256(const QByteArray& key, const QByteArray& data);

/**
 * @brief 计算签名
 * 
 * 签名派生流程：
 * kDate = HMAC-SHA256("SecretKey", "YYYYMMDD")
 * kRegion = HMAC-SHA256(kDate, "region")
 * kService = HMAC-SHA256(kRegion, "service")
 * kSigning = HMAC-SHA256(kService, "request")
 * signature = HMAC-SHA256(kSigning, StringToSign)
 */
QString calculateSignature(const QString& secretKey,
                            const QString& date,
                            const QString& region,
                            const QString& service,
                            const QString& stringToSign);

/**
 * @brief 构建 Authorization 头
 * 
 * 格式：
 * HMAC-SHA256 Credential=AccessKey/CredentialScope, 
 * SignedHeaders=xxx, Signature=xxx
 */
QString buildAuthorization(const Config& config,
                            const RequestInfo& request);

/**
 * @brief 生成 X-Date 头值
 * 
 * 格式：YYYYMMDDTHHmmssZ
 */
QString formatDateTime(const QDateTime& dateTime);

/**
 * @brief 生成日期字符串
 * 
 * 格式：YYYYMMDD
 */
QString formatDate(const QDateTime& dateTime);

} // namespace VolcEngineSignature

#endif // VOLCENGINESIGNATURE_H
