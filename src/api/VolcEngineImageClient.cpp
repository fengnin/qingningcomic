#include "api/VolcEngineImageClient.h"
#include "services/ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "utils/LogSummaryUtils.h"
#include "utils/Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QUuid>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QUrl>
#include <QThread>

VolcEngineImageClient* VolcEngineImageClient::m_instance = nullptr;
QMutex VolcEngineImageClient::m_instanceMutex;

VolcEngineImageClient::VolcEngineImageClient()
    : QObject(nullptr)
    , m_networkManager(new QNetworkAccessManager())
    , m_initialized(false)
{
    m_networkManager->setParent(this);
}

VolcEngineImageClient::~VolcEngineImageClient()
{
    cancelAllRequests();
}

DEFINE_SINGLETON_INSTANCE(VolcEngineImageClient, volcEngineImageClient)

bool VolcEngineImageClient::init(const Config& config)
{
    if (config.accessKey.isEmpty() || config.secretKey.isEmpty()) {
        LOG_WARNING("VolcEngineImageClient", "AccessKey or SecretKey is empty, using placeholder mode.");
    }
    
    m_config = config;
    m_initialized = true;
    
    LOG_INFO("VolcEngineImageClient", QString("Initialized with reqKey: %1, baseUrl: %2")
        .arg(m_config.reqKey).arg(m_config.baseUrl));
    
    return true;
}

bool VolcEngineImageClient::shouldMock() const
{
    return m_config.forceMock || !m_initialized || 
        m_config.accessKey.isEmpty() || m_config.secretKey.isEmpty();
}

void VolcEngineImageClient::applyRequestThrottle()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    int delayMs = 0;
    {
        QMutexLocker locker(&m_requestThrottleMutex);
        if (m_nextAllowedRequestAtMs > nowMs) {
            delayMs = static_cast<int>(m_nextAllowedRequestAtMs - nowMs);
        }
    }

    if (delayMs > 0) {
        QThread::msleep(static_cast<unsigned long>(delayMs));
    }

    noteRequestStarted();
}

void VolcEngineImageClient::noteRequestStarted()
{
    QMutexLocker locker(&m_requestThrottleMutex);
    m_nextAllowedRequestAtMs = QDateTime::currentMSecsSinceEpoch() + m_minRequestIntervalMs;
}

void VolcEngineImageClient::noteRateLimitHit()
{
    QMutexLocker locker(&m_requestThrottleMutex);
    const qint64 backoffUntil = QDateTime::currentMSecsSinceEpoch() + m_rateLimitBackoffMs;
    if (backoffUntil > m_nextAllowedRequestAtMs) {
        m_nextAllowedRequestAtMs = backoffUntil;
    }
}

void VolcEngineImageClient::handleRateLimitResponse(QNetworkReply* reply)
{
    if (!reply) {
        return;
    }

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 429) {
        noteRateLimitHit();
    }
}

            // 异步生成入口

void VolcEngineImageClient::generateAsync(const GenerateOptions& options)
{
    if (options.prompt.isEmpty()) {
        setError("Prompt is required");
        emit generateCompleted(createErrorResult(options.requestId, m_lastError));
        return;
    }
    
    if (shouldMock()) {
        LOG_INFO("VolcEngineImageClient", QString("Using placeholder for prompt: %1").arg(options.prompt.left(100)));
        emit generateCompleted(generatePlaceholder(options.prompt));
        return;
    }

    applyRequestThrottle();
    
    QString requestId = options.requestId.isEmpty() ? QUuid::createUuid().toString() : options.requestId;
    QString url = buildApiUrl();
    QJsonObject payload = buildGenerateRequestBody(options);
    QByteArray bodyData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    
    QNetworkRequest request = createNetworkRequest(url, QString::fromUtf8(bodyData));
    
    QNetworkReply* reply = m_networkManager->post(request, bodyData);
    
    registerRequest(requestId, reply);
    m_pendingOptions[requestId] = options;
    
    connect(reply, &QNetworkReply::finished, this, &VolcEngineImageClient::onReplyFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &VolcEngineImageClient::onReplyError);
    
    emit progressChanged("sending_request", 0);
}

            // 同步生成入口

VolcEngineImageClient::GenerateResult VolcEngineImageClient::generate(const GenerateOptions& options)
{
    LOG_INFO("VolcEngineImageClient", QString("generate() called, shouldMock=%1, hasRefImage=%2")
        .arg(shouldMock()).arg(!options.referenceImage.isEmpty()));
    
    if (options.prompt.isEmpty()) {
        LOG_WARNING("VolcEngineImageClient", "Prompt is required");
        return generatePlaceholder(options.prompt);
    }
    
    if (shouldMock()) {
        LOG_INFO("VolcEngineImageClient", QString("Using placeholder for prompt: %1").arg(options.prompt.left(100)));
        return generatePlaceholder(options.prompt);
    }

    applyRequestThrottle();
    
            // 优先走参考图生成
    if (!options.referenceImage.isEmpty()) {
        return generateWithReference(options);
    }
    
            // 文本生成分支
    QString url = buildApiUrl();
    QJsonObject payload = buildGenerateRequestBody(options);
    
    LOG_INFO("VolcEngineImageClient", QString("Sending text2img request to: %1").arg(url));
    
    QJsonObject response = sendSyncRequest(url, payload);
    
    if (response.isEmpty()) {
        LOG_ERROR("VolcEngineImageClient", QString("API request failed: %1").arg(m_lastError));
        return createErrorResult(options.requestId, m_lastError.isEmpty() ? "API request failed" : m_lastError);
    }
    
    LOG_INFO("VolcEngineImageClient", "Response received, parsing...");
    return parseResponse(response);
}

        // 任务提交后开始轮询

VolcEngineImageClient::GenerateResult VolcEngineImageClient::generateWithReference(const GenerateOptions& options)
{
    LOG_INFO("VolcEngineImageClient", "Using img2img API with reference image");
    
    QString submitUrl = QString("%1?Action=CVSync2AsyncSubmitTask&Version=2022-08-31").arg(m_config.baseUrl);
    QJsonObject payload = buildReferenceSubmitPayload(options);
    
    LOG_INFO("VolcEngineImageClient", QString("Submitting img2img task to: %1").arg(submitUrl));
    
    QJsonObject submitResponse = sendSyncRequest(submitUrl, payload);
    
    if (submitResponse.isEmpty()) {
        LOG_ERROR("VolcEngineImageClient", QString("Submit task failed: %1").arg(m_lastError));
        return createErrorResult(options.requestId, m_lastError.isEmpty() ? "Submit task failed" : m_lastError);
    }
    
    QJsonObject data = submitResponse["data"].toObject();
    QString taskId = data["task_id"].toString();
    
    if (taskId.isEmpty()) {
        LOG_ERROR("VolcEngineImageClient", "No task_id in response");
        return createErrorResult(options.requestId, "No task_id in response");
    }
    
    LOG_INFO("VolcEngineImageClient", QString("Task submitted, taskId: %1, polling for result...").arg(taskId));
    
    // 轮询任务结果
    return pollTaskResult(taskId, options);
}

VolcEngineImageClient::GenerateResult VolcEngineImageClient::handleTaskDone(const QJsonObject& data, const GenerateOptions& options)
{
    const QString imageUrl = extractResponseImageUrl(data);
    if (!imageUrl.isEmpty()) {
        LOG_INFO("VolcEngineImageClient", QString("Got image URL: %1").arg(imageUrl.left(100)));
        const QByteArray imageData = downloadImage(imageUrl);
        return buildImageResult(options.requestId, imageData, imageUrl);
    }
    
    QJsonArray binaryData = data["binary_data_base64"].toArray();
    if (!binaryData.isEmpty()) {
        QByteArray imageData = QByteArray::fromBase64(binaryData.first().toString().toUtf8());
        return buildImageResult(options.requestId, imageData);
    }

    return createErrorResult(options.requestId, "No image in response");
}

VolcEngineImageClient::GenerateResult VolcEngineImageClient::pollTaskResult(const QString& taskId, const GenerateOptions& options)
{
    QString queryUrl = QString("%1?Action=CVSync2AsyncGetResult&Version=2022-08-31").arg(m_config.baseUrl);
    
    const int maxAttempts = 60;
    const int pollInterval = 2000;
    
    LOG_INFO("VolcEngineImageClient", QString("Polling task result: taskId=%1, maxAttempts=%2")
        .arg(taskId).arg(maxAttempts));
    
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        QJsonObject response = sendSyncRequest(queryUrl, buildReferenceQueryPayload(taskId));
        
        if (response.isEmpty()) {
            LOG_WARNING("VolcEngineImageClient", QString("Query attempt %1 failed: %2")
                .arg(attempt + 1).arg(m_lastError));
            QThread::msleep(pollInterval);
            continue;
        }
        
        QJsonObject responseMetadata = response["ResponseMetadata"].toObject();
        QJsonObject errorInfo = responseMetadata["Error"].toObject();
        if (!errorInfo.isEmpty()) {
            QString errorCode = errorInfo["Code"].toString();
            QString errorMessage = errorInfo["Message"].toString();
            LOG_ERROR("VolcEngineImageClient", QString("API error: %1 - %2").arg(errorCode).arg(errorMessage));
            if (errorCode == "InvalidCredential") {
                return createErrorResult(options.requestId, QString("Invalid credential: %1").arg(errorMessage));
            }
            QThread::msleep(pollInterval);
            continue;
        }
        
        QJsonObject data = response["data"].toObject();
        QString status = data["status"].toString();
        
        LOG_DEBUG("VolcEngineImageClient", QString("Query attempt %1: status=%2").arg(attempt + 1).arg(status));
        
        if (status == "done") {
            return handleTaskDone(data, options);
        }
        
        if (status == "not_found" || status == "expired") {
            return createErrorResult(options.requestId, QString("Task result not found or expired: %1").arg(status));
        }
        
        QThread::msleep(pollInterval);
    }
    
    return createErrorResult(options.requestId, "Query task result failed");
}

        // 同步请求辅助函数

void VolcEngineImageClient::onReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString requestId = reply->property("requestId").toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Network error in onReplyFinished: %1 (code=%2, http=%3)")
            .arg(reply->errorString())
            .arg(reply->error())
            .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
        
        LOG_ERROR("VolcEngineImageClient", errorMsg);

        handleRateLimitResponse(reply);
        
        cleanupRequest(reply, requestId);
        emit generateCompleted(createErrorResult(requestId, errorMsg));
        return;
    }
    
    QByteArray responseData = reply->readAll();
    cleanupRequest(reply, requestId);
    
    QJsonObject response = parseJsonResponse(responseData);
    if (response.isEmpty()) {
        emit generateCompleted(createErrorResult(requestId, m_lastError));
        return;
    }
    
    GenerateResult result = parseResponse(response);
    result.requestId = requestId;
    
    if (!result.success && result.imageUrl.isEmpty()) {
        result.errorMessage = "Failed to extract image from response";
        emit generateCompleted(result);
        return;
    }
    
        // 构建生成请求体
    if (!result.imageUrl.isEmpty() && result.imageData.isEmpty()) {
        LOG_INFO("VolcEngineImageClient", QString("Downloading image from URL: %1").arg(result.imageUrl.left(100)));
        
        QNetworkRequest imageRequest{QUrl(result.imageUrl)};
        QNetworkReply* imageReply = m_networkManager->get(imageRequest);
        
        imageReply->setProperty("requestId", requestId);
        imageReply->setProperty("imageUrl", result.imageUrl);
        m_activeRequests[requestId + "_download"] = imageReply;
        
        connect(imageReply, &QNetworkReply::finished, this, &VolcEngineImageClient::onImageDownloadFinished);
        return;
    }
    
    emit progressChanged("completed", 100);
    emit generateCompleted(result);
}

void VolcEngineImageClient::onImageDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString requestId = reply->property("requestId").toString();
    QString imageUrl = reply->property("imageUrl").toString();
    
    QByteArray imageData;
    
    if (reply->error() == QNetworkReply::NoError) {
        imageData = reply->readAll();
        LOG_INFO("VolcEngineImageClient", QString("Downloaded %1 bytes").arg(imageData.size()));
    } else {
        LOG_ERROR("VolcEngineImageClient", QString("Download failed: %1").arg(reply->errorString()));
    }
    
    cleanupRequest(reply, requestId + "_download");
    
    GenerateResult result;
    if (!imageData.isEmpty()) {
        result.success = true;
        result.imageData = imageData;
        result.imageUrl = imageUrl;
        result.mimeType = "image/jpeg";
        result.requestId = requestId;
        result.timestamp = QDateTime::currentMSecsSinceEpoch();
        
        LOG_INFO("VolcEngineImageClient", QString("Image downloaded successfully, size: %1 bytes").arg(imageData.size()));
        emit progressChanged("completed", 100);
    } else {
        result = createErrorResult(requestId, "Failed to download image");
    }
    
    emit generateCompleted(result);
}

void VolcEngineImageClient::onReplyError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString errorMessage = QString("Network error: %1 (code=%2, http=%3)")
        .arg(reply->errorString())
        .arg(reply->error())
        .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    
    LOG_ERROR("VolcEngineImageClient", errorMessage);
    
    setError(errorMessage);
    
    emit errorOccurred("network", errorMessage);
}

        // 构建带签名的网络请求

QJsonObject VolcEngineImageClient::sendSyncRequest(const QString& url, const QJsonObject& payload)
{
    try {
        QByteArray bodyData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        QNetworkRequest request = createNetworkRequest(url, QString::fromUtf8(bodyData));
        
        QNetworkAccessManager localManager;
        QNetworkReply* reply = localManager.post(request, bodyData);
        
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        
        timer.start(m_config.requestTimeout);
        loop.exec(QEventLoop::ExcludeUserInputEvents);
        
        if (!timer.isActive()) {
            setError("Request timeout");
            reply->deleteLater();
            return QJsonObject();
        }
        
        if (reply->error() != QNetworkReply::NoError) {
            handleRateLimitResponse(reply);
            setError(reply->errorString());
            reply->deleteLater();
            return QJsonObject();
        }
        
        QByteArray responseData = reply->readAll();
        reply->deleteLater();
        
        return parseJsonResponse(responseData);
    } catch (const std::exception& e) {
        setError(QString("sendSyncRequest exception: %1").arg(QString::fromUtf8(e.what())));
        LOG_ERROR("VolcEngineImageClient", QString("Exception: %1").arg(QString::fromUtf8(e.what())));
        return QJsonObject();
    } catch (...) {
        setError("sendSyncRequest unknown exception");
        LOG_ERROR("VolcEngineImageClient", "Unknown exception");
        return QJsonObject();
    }
}

QByteArray VolcEngineImageClient::downloadImage(const QString& url)
{
    QNetworkRequest request{QUrl(url)};
    
    QNetworkAccessManager dlManager;
    QNetworkReply* reply = dlManager.get(request);
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    timer.start(30000);
    loop.exec();
    
    QByteArray imageData;
    
    if (timer.isActive() && reply->error() == QNetworkReply::NoError) {
        imageData = reply->readAll();
    }
    
    reply->deleteLater();
    return imageData;
}

        // 解析服务端响应

QJsonObject VolcEngineImageClient::buildGenerateRequestBody(const GenerateOptions& options)
{
    QJsonObject body;
    body["req_key"] = m_config.reqKey;
    body["prompt"] = options.prompt;
    
    if (!options.negativePrompt.isEmpty()) {
        body["negative_prompt"] = options.negativePrompt;
    }
    
    if (options.width > 0 && options.height > 0) {
        body["width"] = options.width;
        body["height"] = options.height;
    }
    
    if (options.seed >= 0) {
        body["seed"] = options.seed;
    }
    
    body["scale"] = options.scale;
    body["return_url"] = options.returnUrl;
    
    if (options.usePreLlm) {
        body["use_pre_llm"] = true;
    }

    LOG_DEBUG("VolcEngineImageClient", QString(
        "buildGenerateRequestBody: reqKey=%1, hasRefImage=%2, promptLen=%3, negativeLen=%4, width=%5, height=%6, returnUrl=%7, promptHead=%8")
        .arg(m_config.reqKey)
        .arg(!options.referenceImage.isEmpty())
        .arg(options.prompt.length())
        .arg(options.negativePrompt.length())
        .arg(options.width)
        .arg(options.height)
        .arg(options.returnUrl)
        .arg(LogSummaryUtils::summarizeText(options.prompt)));
    
    return body;
}

QJsonObject VolcEngineImageClient::buildReferenceSubmitPayload(const GenerateOptions& options)
{
    QJsonObject payload;
    payload["req_key"] = m_config.img2imgReqKey;
    payload["prompt"] = options.prompt;

    if (!options.negativePrompt.isEmpty()) {
        payload["negative_prompt"] = options.negativePrompt;
    }

    QJsonArray imageDataArray;
    imageDataArray.append(QString(options.referenceImage.toBase64()));
    payload["binary_data_base64"] = imageDataArray;

    if (options.seed >= 0) {
        payload["seed"] = options.seed;
    }

    if (options.width > 0 && options.height > 0) {
        payload["width"] = options.width;
        payload["height"] = options.height;
    }

    LOG_DEBUG("VolcEngineImageClient", QString(
        "buildReferenceSubmitPayload: reqKey=%1, promptLen=%2, negativeLen=%3, width=%4, height=%5, refBytes=%6, promptHead=%7")
        .arg(m_config.img2imgReqKey)
        .arg(options.prompt.length())
        .arg(options.negativePrompt.length())
        .arg(options.width)
        .arg(options.height)
        .arg(options.referenceImage.size())
        .arg(LogSummaryUtils::summarizeText(options.prompt)));

    return payload;
}

QJsonObject VolcEngineImageClient::buildReferenceQueryPayload(const QString& taskId) const
{
    QJsonObject payload;
    payload["req_key"] = m_config.img2imgReqKey;
    payload["task_id"] = taskId;
    return payload;
}

QString VolcEngineImageClient::buildApiUrl() const
{
    return QString("%1?Action=CVProcess&Version=2022-08-31").arg(m_config.baseUrl);
}

        // 处理接口错误码

QNetworkRequest VolcEngineImageClient::createNetworkRequest(const QString& url, const QString& body)
{
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QDateTime now = QDateTime::currentDateTimeUtc();
    QString dateTimeStr = VolcEngineSignature::formatDateTime(now);
    
    QUrl parsedUrl(url);
    QString path = parsedUrl.path().isEmpty() ? "/" : parsedUrl.path();
    QString host = parsedUrl.host();
    
    QString query = parsedUrl.query();
    QStringList queryParts = query.split('&', Qt::SkipEmptyParts);
    queryParts.sort();
    QString canonicalQuery = queryParts.join('&');
    
    VolcEngineSignature::Config sigConfig;
    sigConfig.accessKey = m_config.accessKey;
    sigConfig.secretKey = m_config.secretKey;
    sigConfig.region = m_config.region;
    sigConfig.service = m_config.service;
    
    VolcEngineSignature::RequestInfo requestInfo;
    requestInfo.method = "POST";
    requestInfo.path = path;
    requestInfo.query = canonicalQuery;
    requestInfo.body = body;
    requestInfo.host = host;
    requestInfo.timestamp = now;
    
    QString authorization = VolcEngineSignature::buildAuthorization(sigConfig, requestInfo);
    
    LOG_DEBUG("VolcEngineImageClient", QString("Signature debug: method=%1, path=%2, query=%3, host=%4")
        .arg(requestInfo.method).arg(requestInfo.path).arg(requestInfo.query).arg(requestInfo.host));
    LOG_DEBUG("VolcEngineImageClient", QString("Authorization: %1").arg(authorization.left(80)));
    
    request.setRawHeader("X-Date", dateTimeStr.toUtf8());
    request.setRawHeader("Authorization", authorization.toUtf8());
    request.setRawHeader("Host", host.toUtf8());
    request.setRawHeader("Content-Type", "application/json");
    
    return request;
}

        // 提取图片 URL

VolcEngineImageClient::GenerateResult VolcEngineImageClient::parseResponse(const QJsonObject& response)
{
    GenerateResult result;

    QString errorMessage = extractResponseErrorMessage(response);
    if (!errorMessage.isEmpty()) {
        result.success = false;
        result.errorMessage = errorMessage;
        result.requestId = response["request_id"].toString();
        LOG_ERROR("VolcEngineImageClient", QString("API error: %1").arg(result.errorMessage));
        return result;
    }
    
    QJsonObject data = response["data"].toObject();
    QString imageUrl = extractResponseImageUrl(data);
    if (!imageUrl.isEmpty()) {
        result.imageUrl = imageUrl;
        result.success = true;
        LOG_INFO("VolcEngineImageClient", QString("Got image URL: %1").arg(result.imageUrl.left(100)));
    } else {
        QJsonArray binaryData = data["binary_data_base64"].toArray();
        if (!binaryData.isEmpty()) {
            QByteArray imageData = QByteArray::fromBase64(binaryData.first().toString().toLatin1());
            result = buildImageResult(response["request_id"].toString(), imageData);
        }
    }

    result.requestId = response["request_id"].toString();
    if (result.requestId.isEmpty()) {
        result.requestId = data["request_id"].toString();
    }
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    return result;
}

VolcEngineImageClient::GenerateResult VolcEngineImageClient::buildImageResult(const QString& requestId,
                                                                              const QByteArray& imageData,
                                                                              const QString& imageUrl) const
{
    GenerateResult result;
    result.success = !imageData.isEmpty();
    result.imageData = imageData;
    result.imageUrl = imageUrl;
    result.mimeType = imageData.isEmpty() ? QString() : QStringLiteral("image/png");
    result.requestId = requestId;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();

    if (result.success && !imageUrl.isEmpty()) {
        LOG_INFO("VolcEngineImageClient", QString("Built image result from URL: %1").arg(imageUrl.left(100)));
    }
    return result;
}

QString VolcEngineImageClient::extractResponseImageUrl(const QJsonObject& data) const
{
    QJsonArray imageUrls = data["image_urls"].toArray();
    if (!imageUrls.isEmpty()) {
        return imageUrls.first().toString();
    }
    return QString();
}

QString VolcEngineImageClient::extractResponseErrorMessage(const QJsonObject& response) const
{
    if (response.contains("code") && response["code"].toInt() != 10000) {
        return response["message"].toString();
    }

    QJsonObject metadata = response["ResponseMetadata"].toObject();
    QJsonObject error = metadata["Error"].toObject();
    if (!error.isEmpty()) {
        const QString errorCode = error["Code"].toString();
        const QString errorMessage = error["Message"].toString();
        return errorCode.isEmpty()
            ? errorMessage
            : QString("%1: %2").arg(errorCode, errorMessage);
    }

    return QString();
}

    // 请求注册与清理

void VolcEngineImageClient::registerRequest(const QString& requestId, QNetworkReply* reply)
{
    m_activeRequests[requestId] = reply;
    reply->setProperty("requestId", requestId);
}

void VolcEngineImageClient::cleanupRequest(QNetworkReply* reply, const QString& key)
{
    if (reply) {
        reply->deleteLater();
    }
    m_activeRequests.remove(key);
    m_pendingOptions.remove(key);
}

void VolcEngineImageClient::cancelAllRequests()
{
    for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ++it) {
        if (it.value()) {
            it.value()->abort();
            it.value()->deleteLater();
        }
    }
    m_activeRequests.clear();
}

QJsonObject VolcEngineImageClient::parseJsonResponse(const QByteArray& data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        setError(QString("JSON parse error: %1").arg(parseError.errorString()));
        return QJsonObject();
    }
    
    return doc.object();
}

VolcEngineImageClient::GenerateResult VolcEngineImageClient::createErrorResult(const QString& requestId, const QString& message) const
{
    GenerateResult result;
    result.success = false;
    result.errorMessage = message;
    result.requestId = requestId;
    return result;
}

void VolcEngineImageClient::setError(const QString& message)
{
    m_lastError = message;
    LOG_ERROR("VolcEngineImageClient", message);
}

VolcEngineImageClient::GenerateResult VolcEngineImageClient::generatePlaceholder(const QString& prompt)
{
    Q_UNUSED(prompt)
    GenerateResult result;
    result.success = true;
    // 占位图兜底
    result.imageData = QByteArray::fromBase64(
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==");
    result.mimeType = "image/png";
    result.requestId = QString("mock-%1").arg(QDateTime::currentMSecsSinceEpoch());
    result.width = 1;
    result.height = 1;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    return result;
}
