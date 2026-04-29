#include "api/QwenImageClient.h"
#include "api/QwenJsonRepair.h"
#include "services/ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "utils/Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QUuid>
#include <QThread>

namespace {
    constexpr int DEFAULT_TIMEOUT = 120000;
    constexpr int DEFAULT_POLL_INTERVAL = 2000;
    constexpr int DEFAULT_MAX_POLLS = 60;
    constexpr int DOWNLOAD_TIMEOUT = 30000;

    bool isDashScopeMultimodalModel(const QString& model)
    {
        const QString normalized = model.trimmed().toLower();
        return normalized.startsWith("qwen-image") || normalized.startsWith("z-image");
    }

    bool isWanxModel(const QString& model)
    {
        return model.trimmed().toLower().startsWith("wanx");
    }

    QString encodeImageDataUri(const QByteArray& imageData, const QString& mimeType = QStringLiteral("image/png"))
    {
        if (imageData.isEmpty()) {
            return QString();
        }
        return QStringLiteral("data:%1;base64,%2")
            .arg(mimeType, QString::fromLatin1(imageData.toBase64()));
    }

    QJsonObject buildTextContentItem(const QString& text)
    {
        return QJsonObject{{"text", text}};
    }

    QJsonObject buildImageContentItem(const QByteArray& imageData)
    {
        return QJsonObject{{"image", encodeImageDataUri(imageData)}};
    }

    QJsonArray buildUserMessageContent(const QString& prompt, const QByteArray* imageData = nullptr)
    {
        QJsonArray content;
        if (imageData && !imageData->isEmpty()) {
            content.append(buildImageContentItem(*imageData));
        }
        content.append(buildTextContentItem(prompt));
        return content;
    }

    QJsonObject buildUserMessage(const QJsonArray& content)
    {
        QJsonObject userMessage;
        userMessage["role"] = "user";
        userMessage["content"] = content;
        return userMessage;
    }

    QJsonObject buildRequestEnvelope(const QString& model,
                                     const QJsonObject& input,
                                     const QJsonObject& parameters)
    {
        QJsonObject body;
        body["model"] = model;
        body["input"] = input;
        body["parameters"] = parameters;
        return body;
    }

    QJsonObject buildMessageInput(const QJsonArray& messages, const QString& negativePrompt)
    {
        QJsonObject input;
        input["messages"] = messages;
        if (!negativePrompt.isEmpty()) {
            input["negative_prompt"] = negativePrompt;
        }
        return input;
    }

    QJsonObject buildEditRequestInput(const QwenImageClient::EditOptions& options)
    {
        QJsonObject input;
        input["messages"] = QJsonArray{buildUserMessage(buildUserMessageContent(options.prompt, &options.sourceImage))};
        return input;
    }

    QJsonObject buildEditParameters(const QwenImageClient::EditOptions& options)
    {
        QJsonObject parameters;
        parameters["n"] = 1;
        parameters["watermark"] = false;
        if (!options.negativePrompt.isEmpty()) {
            parameters["negative_prompt"] = options.negativePrompt;
        }
        if (options.seed >= 0) {
            parameters["seed"] = options.seed;
        }

        parameters["size"] = QwenImageClient::sizeToString(options.size, options.width, options.height);
        parameters["prompt_extend"] = true;

        return parameters;
    }

    struct SyncRequestResult {
        bool success = false;
        QByteArray data;
        QString error;
        int statusCode = 0;
    };
    
    SyncRequestResult waitForNetworkReply(QNetworkReply* reply, int timeoutMs) {
        SyncRequestResult result;
        
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        
        timer.start(timeoutMs);
        loop.exec();

        result.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        if (!timer.isActive()) {
            reply->abort();
            result.error = "Request timeout";
            return result;
        }
        
        if (reply->error() != QNetworkReply::NoError) {
            result.error = reply->errorString();
            return result;
        }
        
        result.data = reply->readAll();
        result.success = true;
        return result;
    }
}

QwenImageClient* QwenImageClient::m_instance = nullptr;
QMutex QwenImageClient::m_instanceMutex;

const QByteArray QwenImageClient::PLACEHOLDER_IMAGE = 
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

QwenImageClient::QwenImageClient()
    : QObject(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_initialized(false)
{
}

QwenImageClient::~QwenImageClient()
{
    cancelAllRequests();
}

DEFINE_SINGLETON_INSTANCE(QwenImageClient, qwenImageClient)

bool QwenImageClient::init(const Config& config)
{
    if (config.apiKey.isEmpty()) {
        LOG_WARNING("QwenImageClient", "API key is empty; using placeholder mode.");
    }
    
    m_config = config;
    m_initialized = true;
    
    LOG_INFO("QwenImageClient", QString("Initialized with generateModel: %1, editModel: %2")
        .arg(m_config.generateModel).arg(m_config.editModel));
    
    return true;
}

bool QwenImageClient::shouldMock() const
{
    return m_config.forceMock || !m_initialized || m_config.apiKey.isEmpty();
}

QString QwenImageClient::resolveRequestId(const QString& preferredRequestId) const
{
    return preferredRequestId.isEmpty() ? QUuid::createUuid().toString() : preferredRequestId;
}

QString QwenImageClient::requestTypeName(RequestType type) const
{
    switch (type) {
        case RequestType::Generate:
            return QStringLiteral("generate");
        case RequestType::Edit:
            return QStringLiteral("edit");
        case RequestType::TaskStatus:
            return QStringLiteral("task_status");
        case RequestType::Download:
            return QStringLiteral("download");
    }
    return QStringLiteral("unknown");
}


void QwenImageClient::generateAsync(const GenerateOptions& options)
{
    if (!validateGenerateOptions(options)) {
        emit generateCompleted(createErrorResult(options.requestId, m_lastError));
        return;
    }
    
    if (shouldMock()) {
        LOG_INFO("QwenImageClient", QString("Using placeholder for prompt: %1").arg(options.prompt.left(100)));
        emit generateCompleted(generatePlaceholder(options.prompt));
        return;
    }

    enqueueAsyncRequest(options);
}

void QwenImageClient::editAsync(const EditOptions& options)
{
    if (!validateEditOptions(options)) {
        emit editCompleted(createErrorResult(options.requestId, m_lastError));
        return;
    }
    
    if (shouldMock()) {
        LOG_INFO("QwenImageClient", QString("Using placeholder for edit prompt: %1").arg(options.prompt.left(100)));
        emit editCompleted(generatePlaceholder(options.prompt));
        return;
    }

    enqueueAsyncRequest(options);
}


QwenImageClient::GenerateResult QwenImageClient::generate(const GenerateOptions& options)
{
    LOG_INFO("QwenImageClient", QString("generate() called, shouldMock=%1, initialized=%2, apiKey.isEmpty=%3")
        .arg(shouldMock()).arg(m_initialized).arg(m_config.apiKey.isEmpty()));

    QMutexLocker syncLock(&m_syncRequestMutex);
    
    if (!validateGenerateOptions(options)) {
        LOG_WARNING("QwenImageClient", "validateGenerateOptions failed");
        return generatePlaceholder(options.prompt);
    }
    
    if (shouldMock()) {
        LOG_INFO("QwenImageClient", QString("Using placeholder for prompt: %1").arg(options.prompt.left(100)));
        return generatePlaceholder(options.prompt);
    }
    
    applyRequestThrottle();

    const QString url = resolveRequestUrl(RequestType::Generate);
    const QJsonObject payload = buildAsyncRequestBody(options, RequestType::Generate);
    
    LOG_INFO("QwenImageClient", QString("Sending request to: %1, model: %2").arg(url).arg(m_config.generateModel));
    
    QJsonObject response = sendSyncRequest(url, payload);
    
    if (response.isEmpty()) {
        LOG_ERROR("QwenImageClient", QString("API request failed: %1").arg(m_lastError));
        GenerateResult result;
        result.success = false;
        result.errorMessage = m_lastError.isEmpty() ? "API request failed" : m_lastError;
        result.requestId = options.requestId;
        return result;
    }
    
    LOG_INFO("QwenImageClient", QString("Response received, extracting image..."));
    return extractImageFromResponse(response);
}

QwenImageClient::GenerateResult QwenImageClient::edit(const EditOptions& options)
{
    QMutexLocker syncLock(&m_syncRequestMutex);

    if (!validateEditOptions(options) || shouldMock()) {
        return generatePlaceholder(options.prompt);
    }
    
    applyRequestThrottle();
    
    const QString url = resolveRequestUrl(RequestType::Edit);
    const QJsonObject payload = buildEditRequestBody(options);
    QJsonObject response = sendSyncRequest(url, payload);
    
    if (response.isEmpty()) {
        LOG_WARNING("QwenImageClient", "Edit failed, using placeholder");
        return generatePlaceholder(options.prompt);
    }
    
    return extractImageFromResponse(response);
}


void QwenImageClient::sendAsyncRequest(const GenerateOptions& options, RequestType type)
{
    const QString requestId = resolveRequestId(options.requestId);
    const QString url = resolveRequestUrl(type);
    const QJsonObject payload = buildAsyncRequestBody(options, type);
    const bool useAsyncMode = isWanxModel(resolveRequestModel(type));

    dispatchPreparedAsyncRequest(requestId, url, payload, type, options.prompt, useAsyncMode);
    if (type == RequestType::Generate) {
        storePendingRequest(requestId, options);
    } else {
        storePendingRequest(requestId, EditOptions::fromGenerateOptions(options));
    }
}

void QwenImageClient::sendAsyncRequest(const EditOptions& options, RequestType type)
{
    const QString requestId = resolveRequestId(options.requestId);
    const QString url = resolveRequestUrl(type);
    const QJsonObject payload = buildEditRequestBody(options);
    const bool useAsyncMode = isWanxModel(resolveRequestModel(type));

    dispatchPreparedAsyncRequest(requestId, url, payload, type, options.prompt, useAsyncMode);
    storePendingRequest(requestId, options);
}

void QwenImageClient::enqueueAsyncRequest(const GenerateOptions& options)
{
    {
        QMutexLocker locker(&m_asyncQueueMutex);
        m_asyncRequestQueue.enqueue(QueuedAsyncRequest{RequestType::Generate, options, EditOptions()});
    }

    processNextAsyncRequest();
}

void QwenImageClient::enqueueAsyncRequest(const EditOptions& options)
{
    {
        QMutexLocker locker(&m_asyncQueueMutex);
        m_asyncRequestQueue.enqueue(QueuedAsyncRequest{RequestType::Edit, GenerateOptions(), options});
    }

    processNextAsyncRequest();
}

void QwenImageClient::processNextAsyncRequest()
{
    QueuedAsyncRequest nextRequest;
    {
        QMutexLocker locker(&m_asyncQueueMutex);
        if (m_asyncRequestInFlight || m_asyncRequestQueue.isEmpty()) {
            return;
        }

        nextRequest = m_asyncRequestQueue.dequeue();
        m_asyncRequestInFlight = true;
    }

    const int delayMs = requestDelayMs();
    if (delayMs > 0) {
        QTimer::singleShot(delayMs, this, [this, nextRequest]() {
            noteRequestStarted();
            if (nextRequest.type == RequestType::Generate) {
                sendAsyncRequest(nextRequest.generateOptions, nextRequest.type);
            } else {
                sendAsyncRequest(nextRequest.editOptions, nextRequest.type);
            }
        });
        return;
    }

    noteRequestStarted();
    if (nextRequest.type == RequestType::Generate) {
        sendAsyncRequest(nextRequest.generateOptions, nextRequest.type);
    } else {
        sendAsyncRequest(nextRequest.editOptions, nextRequest.type);
    }
}

void QwenImageClient::finishAsyncRequest()
{
    {
        QMutexLocker locker(&m_asyncQueueMutex);
        m_asyncRequestInFlight = false;
    }

    QTimer::singleShot(0, this, &QwenImageClient::processNextAsyncRequest);
}

QwenImageClient::PendingRequestKind QwenImageClient::pendingRequestKind(const QString& requestId) const
{
    if (m_pendingGenerateOptions.contains(requestId)) {
        return PendingRequestKind::Generate;
    }
    if (m_pendingEditOptions.contains(requestId)) {
        return PendingRequestKind::Edit;
    }
    return PendingRequestKind::None;
}

void QwenImageClient::clearPendingRequestState(const QString& requestId)
{
    m_pendingGenerateOptions.remove(requestId);
    m_pendingEditOptions.remove(requestId);
    m_retryCount.remove(requestId);
}

void QwenImageClient::applyRequestThrottle()
{
    const int delayMs = requestDelayMs();
    if (delayMs > 0) {
        QThread::msleep(static_cast<unsigned long>(delayMs));
    }
    noteRequestStarted();
}

int QwenImageClient::requestDelayMs() const
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    QMutexLocker locker(&m_rateLimitMutex);
    if (m_nextAllowedRequestAtMs <= 0) {
        return 0;
    }

    const qint64 delay = m_nextAllowedRequestAtMs - nowMs;
    return delay > 0 ? static_cast<int>(delay) : 0;
}

void QwenImageClient::noteRequestStarted()
{
    QMutexLocker locker(&m_rateLimitMutex);
    m_nextAllowedRequestAtMs = QDateTime::currentMSecsSinceEpoch() + m_minRequestIntervalMs;
}

void QwenImageClient::noteRateLimitHit()
{
    QMutexLocker locker(&m_rateLimitMutex);
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 backoffUntil = nowMs + m_rateLimitBackoffMs;
    if (backoffUntil > m_nextAllowedRequestAtMs) {
        m_nextAllowedRequestAtMs = backoffUntil;
    }
}

void QwenImageClient::pollTaskStatus(const QString& taskId, const QString& requestId)
{
    QString url = QString("%1/api/v1/tasks/%2").arg(m_config.baseUrl).arg(taskId);
    QNetworkRequest request = createNetworkRequest(url, false);
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    reply->setProperty("requestId", requestId);
    reply->setProperty("taskId", taskId);
    reply->setProperty("requestType", static_cast<int>(RequestType::TaskStatus));
    
    m_activeRequests[requestId + "_status"] = reply;
    
    connect(reply, &QNetworkReply::finished, this, &QwenImageClient::onTaskStatusReplyFinished);
}

void QwenImageClient::downloadImageFromUrl(const QString& url, const QString& requestId)
{
    LOG_INFO("QwenImageClient", QString("downloadImageFromUrl called: url=%1, requestId=%2").arg(url.left(100)).arg(requestId));
    
    QNetworkRequest request{QUrl(url)};
    QNetworkReply* reply = m_networkManager->get(request);
    
    if (!reply) {
        LOG_ERROR("QwenImageClient", "downloadImageFromUrl: m_networkManager->get returned null!");
        emitPendingResult(requestId, createErrorResult(requestId, "Failed to create download request"));
        return;
    }
    
    reply->setProperty("requestId", requestId);
    reply->setProperty("requestType", static_cast<int>(RequestType::Download));
    
    m_activeRequests[requestId + "_download"] = reply;
    
    LOG_INFO("QwenImageClient", QString("Download request registered, key=%1").arg(requestId + "_download"));
    
    connect(reply, &QNetworkReply::finished, this, &QwenImageClient::onImageDownloadFinished);
}


void QwenImageClient::onReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString requestId = reply->property("requestId").toString();
    RequestType type = static_cast<RequestType>(reply->property("requestType").toInt());
    
    QByteArray responseData = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray contentType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
    
    LOG_DEBUG("QwenImageClient", QString("=== 响应调试信息 ==="));
    LOG_DEBUG("QwenImageClient", QString("HTTP Status: %1, Content-Type: %2, Bytes: %3")
        .arg(statusCode)
        .arg(QString::fromUtf8(contentType))
        .arg(responseData.size()));
    LOG_DEBUG("QwenImageClient", QString("Response: %1").arg(QString::fromUtf8(responseData.left(500))));

    if (statusCode == 429) {
        noteRateLimitHit();
    }
    
    cleanupRequest(reply, requestId);
    
    QJsonObject response = parseJsonResponse(responseData);
    if (response.isEmpty()) {
        emitResult(type, requestId, createErrorResult(requestId, m_lastError));
        return;
    }

    QString errorMessage = extractResponseErrorMessage(response);
    if (!errorMessage.isEmpty()) {
        setError(errorMessage);
        emitResult(type, requestId, createErrorResult(requestId, errorMessage));
        return;
    }

    QString imageUrl = extractInlineImageUrl(response);
    if (!imageUrl.isEmpty()) {
        LOG_INFO("QwenImageClient", QString("Found image URL in sync response: %1").arg(imageUrl.left(100)));
        downloadImageFromUrl(imageUrl, requestId);
        return;
    }

    QString taskId = extractTaskId(response);
    if (!taskId.isEmpty()) {
        emit progressChanged("task_submitted", 10);
        pollTaskStatus(taskId, requestId);
        return;
    }
    
    GenerateResult result = extractImageFromResponse(response);
    result.requestId = requestId;
    
    if (!result.success) {
        result.errorMessage = "Failed to extract image from response";
    }
    
    emitPendingResult(requestId, result);
}

void QwenImageClient::onTaskStatusReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString requestId = reply->property("requestId").toString();
    QString taskId = reply->property("taskId").toString();
    
    QByteArray responseData = reply->readAll();
    cleanupRequest(reply, requestId + "_status");
    
    QJsonObject response = parseJsonResponse(responseData);
    if (response.isEmpty()) {
        emitPendingResult(requestId, createErrorResult(requestId, m_lastError));
        return;
    }
    
    QJsonObject output = response["output"].toObject();
    QString taskStatus = output["task_status"].toString();
    
    if (taskStatus == "SUCCEEDED") {
        handleTaskSuccess(output, requestId);
    } else if (taskStatus == "FAILED") {
        QString message = output["message"].toString();
        emitPendingResult(requestId, createErrorResult(requestId, message));
    } else if (taskStatus == "PENDING" || taskStatus == "RUNNING") {
        emit progressChanged("generating", 30);
        QTimer::singleShot(2000, [this, taskId, requestId]() {
            pollTaskStatus(taskId, requestId);
        });
    } else {
        emitPendingResult(requestId, createErrorResult(requestId, 
            QString("Unknown task status: %1").arg(taskStatus)));
    }
}

void QwenImageClient::onImageDownloadFinished()
{
    LOG_INFO("QwenImageClient", "onImageDownloadFinished called");
    
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        LOG_ERROR("QwenImageClient", "onImageDownloadFinished: sender is not a QNetworkReply!");
        return;
    }
    
    QString requestId = reply->property("requestId").toString();
    LOG_INFO("QwenImageClient", QString("onImageDownloadFinished: requestId=%1, error=%2, errorString=%3")
        .arg(requestId).arg(reply->error()).arg(reply->errorString()));
    
    QByteArray imageData;
    
    if (reply->error() == QNetworkReply::NoError) {
        imageData = reply->readAll();
        LOG_INFO("QwenImageClient", QString("onImageDownloadFinished: downloaded %1 bytes").arg(imageData.size()));
    } else {
        LOG_ERROR("QwenImageClient", QString("onImageDownloadFinished: download failed: %1").arg(reply->errorString()));
    }
    
    cleanupRequest(reply, requestId + "_download");
    
    GenerateResult result;
    if (!imageData.isEmpty()) {
        result.success = true;
        result.imageData = imageData;
        result.mimeType = "image/png";
        result.requestId = requestId;
        result.timestamp = QDateTime::currentMSecsSinceEpoch();
        
        LOG_INFO("QwenImageClient", QString("Image downloaded, size: %1 bytes").arg(imageData.size()));
        emit progressChanged("completed", 100);
    } else {
        result = createErrorResult(requestId, "Failed to download image");
    }
    
    LOG_INFO("QwenImageClient", QString("onImageDownloadFinished: calling emitPendingResult, success=%1").arg(result.success));
    emitPendingResult(requestId, result);
}

void QwenImageClient::onReplyError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    const QString requestId = reply->property("requestId").toString();
    
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();
    
    QString errorMessage = QString("Network error: %1, HTTP Status: %2").arg(reply->errorString()).arg(statusCode);
    
    LOG_ERROR("QwenImageClient", QString("=== 错误详情 ==="));
    LOG_ERROR("QwenImageClient", QString("Error: %1").arg(reply->errorString()));
    LOG_ERROR("QwenImageClient", QString("HTTP Status Code: %1").arg(statusCode));
    LOG_ERROR("QwenImageClient", QString("Response Body: %1").arg(QString::fromUtf8(responseData)));

    if (statusCode == 429) {
        noteRateLimitHit();
    }
    
    if (pendingRequestKind(requestId) != PendingRequestKind::None) {
        emitPendingResult(requestId, createErrorResult(requestId, errorMessage));
        return;
    }

    setError(errorMessage);

    emit errorOccurred("network", errorMessage);
}


QJsonObject QwenImageClient::sendSyncRequest(const QString& url, const QJsonObject& payload)
{
    try {
        QNetworkRequest request = createNetworkRequest(url, false);
        QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        
        QNetworkAccessManager manager;
        QScopedPointer<QNetworkReply> reply(manager.post(request, jsonData));
        
        SyncRequestResult netResult = waitForNetworkReply(reply.data(), m_config.requestTimeout);
        if (!netResult.success) {
            if (netResult.statusCode == 429) {
                noteRateLimitHit();
            }
            setError(netResult.error);
            return QJsonObject();
        }
        
        QJsonObject response = parseJsonResponse(netResult.data);
        
        QString taskId = response["output"].toObject()["task_id"].toString();
        if (!taskId.isEmpty()) {
            response = pollTaskSync(taskId);
        }
        
        return response;
    } catch (const std::exception& e) {
        setError(QString("sendSyncRequest exception: %1").arg(QString::fromUtf8(e.what())));
        LOG_ERROR("QwenImageClient", QString("Exception in sendSyncRequest: %1").arg(QString::fromUtf8(e.what())));
        return QJsonObject();
    }
}

QJsonObject QwenImageClient::pollTaskSync(const QString& taskId)
{
    QString url = QString("%1/api/v1/tasks/%2").arg(m_config.baseUrl).arg(taskId);
    
    for (int i = 0; i < DEFAULT_MAX_POLLS; i++) {
        try {
            QNetworkRequest request = createNetworkRequest(url, false);
            QNetworkAccessManager manager;
            QScopedPointer<QNetworkReply> reply(manager.get(request));
            
            SyncRequestResult netResult = waitForNetworkReply(reply.data(), 10000);
            if (!netResult.success) {
                if (netResult.statusCode == 429) {
                    noteRateLimitHit();
                }
                setError(netResult.error);
                return QJsonObject();
            }
            
            QJsonObject response = parseJsonResponse(netResult.data);
            QJsonObject output = response["output"].toObject();
            QString taskStatus = output["task_status"].toString();
            
            if (taskStatus == "SUCCEEDED") {
                return response;
            } else if (taskStatus == "FAILED") {
                setError(output["message"].toString());
                return QJsonObject();
            }
            
            QThread::msleep(DEFAULT_POLL_INTERVAL);
        } catch (const std::exception& e) {
            LOG_WARNING("QwenImageClient", QString("Exception in pollTaskSync: %1").arg(QString::fromUtf8(e.what())));
            QThread::msleep(1000);
        }
    }
    
    setError("Polling timeout");
    return QJsonObject();
}

QByteArray QwenImageClient::downloadImage(const QString& url)
{
    QNetworkRequest request{QUrl(url)};
    QNetworkAccessManager manager;
    QScopedPointer<QNetworkReply> reply(manager.get(request));
    
    SyncRequestResult netResult = waitForNetworkReply(reply.data(), DOWNLOAD_TIMEOUT);
    return netResult.success ? netResult.data : QByteArray();
}


QwenImageClient::GenerateResult QwenImageClient::extractImageFromResponse(const QJsonObject& response)
{
    GenerateResult result = parseSyncResponse(response);
    if (result.success) return result;
    
    result = parseAsyncResponse(response);
    if (result.success) return result;
    
    result.errorMessage = "无法从响应中提取图像数据";
    return result;
}

QwenImageClient::GenerateResult QwenImageClient::parseSyncResponse(const QJsonObject& response)
{
    GenerateResult result;
    result.requestId = response["request_id"].toString();
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    QString inlineImageUrl = extractInlineImageUrl(response);
    if (!inlineImageUrl.isEmpty()) {
        result.imageUrl = inlineImageUrl;
        result.success = true;
        result.mimeType = "image/png";
        return result;
    }

    QJsonObject output = response["output"].toObject();

    QString imageUrl = extractImageUrl(output);
    if (!imageUrl.isEmpty()) {
        result.imageUrl = imageUrl;
        result.success = true;
        result.mimeType = "image/png";
        return result;
    }
    
    QJsonArray dataArray = response["data"].toArray();
    if (!dataArray.isEmpty()) {
        QJsonObject firstData = dataArray.first().toObject();
        imageUrl = firstData["url"].toString();
        if (!imageUrl.isEmpty()) {
            result.imageUrl = imageUrl;
            result.success = true;
            result.mimeType = "image/png";
            return result;
        }
        
        QString base64Image = firstData["b64_json"].toString();
        if (!base64Image.isEmpty()) {
            result.imageData = QByteArray::fromBase64(base64Image.toLatin1());
            result.success = !result.imageData.isEmpty();
            result.mimeType = "image/png";
        }
    }
    
    return result;
}

QString QwenImageClient::extractImageUrl(const QJsonObject& output) const
{
    QJsonArray results = output["results"].toArray();
    if (!results.isEmpty()) {
        return results.first().toObject()["url"].toString();
    }
    return QString();
}

QString QwenImageClient::extractInlineImageUrl(const QJsonObject& response) const
{
    QJsonArray dataArray = response["data"].toArray();
    if (!dataArray.isEmpty()) {
        QJsonObject firstData = dataArray.first().toObject();
        QString imageUrl = firstData["url"].toString();
        if (!imageUrl.isEmpty()) {
            return imageUrl;
        }
    }

    QJsonObject output = response["output"].toObject();
    QJsonArray choices = output["choices"].toArray();
    if (!choices.isEmpty()) {
        QJsonObject message = choices.first().toObject()["message"].toObject();
        QJsonArray content = message["content"].toArray();
        if (!content.isEmpty()) {
            return content.first().toObject()["image"].toString();
        }
    }

    return QString();
}

QString QwenImageClient::extractTaskId(const QJsonObject& response) const
{
    QJsonObject output = response["output"].toObject();
    return output["task_id"].toString();
}

QString QwenImageClient::extractResponseErrorMessage(const QJsonObject& response) const
{
    if (response.contains("error")) {
        return response["error"].toObject()["message"].toString();
    }

    if (response.contains("code") && response["code"].toString() != "Success") {
        return response["message"].toString();
    }

    return QString();
}

QwenImageClient::GenerateResult QwenImageClient::parseAsyncResponse(const QJsonObject& response)
{
    GenerateResult result;
    
    QJsonObject output = response["output"].toObject();
    QString taskStatus = output["task_status"].toString();
    
    result.requestId = response["request_id"].toString();
    if (result.requestId.isEmpty()) {
        result.requestId = output["task_id"].toString();
    }
    
    if (taskStatus != "SUCCEEDED") {
        if (taskStatus == "FAILED") {
            result.errorMessage = output["message"].toString();
        }
        return result;
    }

    QString inlineImageUrl = extractInlineImageUrl(response);
    if (!inlineImageUrl.isEmpty()) {
        result.imageUrl = inlineImageUrl;
        result.success = true;
        result.mimeType = "image/png";
        result.timestamp = QDateTime::currentMSecsSinceEpoch();
        return result;
    }
    
    QJsonArray results = output["results"].toArray();
    if (results.isEmpty()) return result;
    
    QJsonObject firstResult = results.first().toObject();
    QString imageUrl = firstResult["url"].toString();
    
    if (!imageUrl.isEmpty()) {
        result.imageData = downloadImage(imageUrl);
        result.success = !result.imageData.isEmpty();
        result.mimeType = "image/png";
    }
    
    result.width = firstResult["width"].toInt();
    result.height = firstResult["height"].toInt();
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    return result;
}


bool QwenImageClient::validateGenerateOptions(const GenerateOptions& options)
{
    if (options.prompt.isEmpty()) {
        setError("提示词不能为空");
        return false;
    }
    return true;
}

bool QwenImageClient::validateEditOptions(const EditOptions& options)
{
    if (options.prompt.isEmpty() || options.sourceImage.isEmpty()) {
        setError(options.prompt.isEmpty() ? "提示词不能为空" : "源图像不能为空");
        return false;
    }
    return true;
}

QJsonObject QwenImageClient::buildCommonParameters(ImageSize size, int seed)
{
    QJsonObject parameters;
    parameters["size"] = sizeToString(size);
    parameters["format"] = "png";
    parameters["n"] = 1;
    
    if (seed >= 0) {
        parameters["seed"] = seed;
    }
    
    return parameters;
}

QJsonObject QwenImageClient::applyCustomSize(QJsonObject parameters, ImageSize size, int width, int height) const
{
    if (size == ImageSize::Custom && width > 0 && height > 0) {
        parameters["size"] = QString("%1*%2").arg(width).arg(height);
    }
    return parameters;
}

QString QwenImageClient::resolveRequestModel(RequestType type) const
{
    return type == RequestType::Generate ? m_config.generateModel : m_config.editModel;
}

QString QwenImageClient::resolveRequestService(RequestType type) const
{
    return type == RequestType::Generate ? QStringLiteral("text2image") : QStringLiteral("image2image");
}

QString QwenImageClient::resolveRequestUrl(RequestType type) const
{
    const QString model = resolveRequestModel(type);
    if ((type == RequestType::Generate || type == RequestType::Edit) && isDashScopeMultimodalModel(model)) {
        return QString("%1/api/v1/services/aigc/multimodal-generation/generation").arg(m_config.baseUrl);
    }
    return buildApiUrl(resolveRequestService(type), model);
}

QJsonObject QwenImageClient::buildGenerateRequestBody(const GenerateOptions& options)
{
    QJsonObject body;
    body["model"] = m_config.generateModel;
    
    QJsonObject input;
    input["prompt"] = options.prompt;
    if (!options.negativePrompt.isEmpty()) {
        input["negative_prompt"] = options.negativePrompt;
    }
    body["input"] = input;
    
    QJsonObject parameters = buildCommonParameters(options.size, options.seed);
    
    const bool wanxModel = isWanxModel(m_config.generateModel);

    if (!wanxModel && !options.style.isEmpty()) {
        parameters["style"] = options.style;
    }
    if (!wanxModel && options.textRendering) {
        parameters["text_rendering"] = true;
    }
    if (wanxModel) {
        parameters["prompt_extend"] = true;
        parameters["watermark"] = false;
    }
    parameters = applyCustomSize(parameters, options.size, options.width, options.height);
    
    body["parameters"] = parameters;
    return body;
}

QJsonObject QwenImageClient::buildMultimodalRequestBody(const GenerateOptions& options)
{
    QJsonArray messages;
    messages.append(buildUserMessage(buildUserMessageContent(options.prompt)));

    QJsonObject parameters;
    parameters["size"] = sizeToString(options.size, options.width, options.height);
    parameters["n"] = 1;
    parameters["watermark"] = false;
    parameters["prompt_extend"] = true;

    return buildRequestEnvelope(m_config.generateModel,
                                buildMessageInput(messages, options.negativePrompt),
                                parameters);
}

QJsonObject QwenImageClient::buildEditRequestBody(const EditOptions& options)
{
    return buildRequestEnvelope(m_config.editModel,
                                buildEditRequestInput(options),
                                buildEditParameters(options));
}

void QwenImageClient::dispatchAsyncRequest(const QString& requestId, const QNetworkRequest& request,
                                           const QJsonObject& payload, RequestType type)
{
    QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = m_networkManager->post(request, jsonData);

    registerRequest(requestId, reply, type);

    connect(reply, &QNetworkReply::finished, this, &QwenImageClient::onReplyFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &QwenImageClient::onReplyError);

    emit progressChanged("sending_request", 0);
}

void QwenImageClient::dispatchPreparedAsyncRequest(const QString& requestId, const QString& url,
                                                    const QJsonObject& payload, RequestType type,
                                                    const QString& prompt, bool asyncMode)
{
    LOG_DEBUG("QwenImageClient", QString("=== 请求调试信息 ==="));
    LOG_DEBUG("QwenImageClient", QString("URL: %1").arg(url));
    LOG_DEBUG("QwenImageClient", QString("Model: %1").arg(resolveRequestModel(type)));
    LOG_DEBUG("QwenImageClient", QString("API Key (前10位): %1...").arg(m_config.apiKey.left(10)));
    LOG_DEBUG("QwenImageClient", QString("Prompt: %1").arg(prompt.left(100)));
    LOG_DEBUG("QwenImageClient", QString("Request Body: %1")
        .arg(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact))));
    LOG_DEBUG("QwenImageClient", QString("Async Mode: %1").arg(asyncMode ? "true" : "false"));

    QNetworkRequest request = createNetworkRequest(url, asyncMode);
    dispatchAsyncRequest(requestId, request, payload, type);
}

QJsonObject QwenImageClient::buildAsyncRequestBody(const GenerateOptions& options, RequestType type)
{
    if (type == RequestType::Generate) {
        return isDashScopeMultimodalModel(m_config.generateModel)
            ? buildMultimodalRequestBody(options)
            : buildGenerateRequestBody(options);
    }

    return buildEditRequestBody(EditOptions::fromGenerateOptions(options));
}

void QwenImageClient::storePendingRequest(const QString& requestId, const GenerateOptions& options)
{
    m_pendingGenerateOptions[requestId] = options;
}

void QwenImageClient::storePendingRequest(const QString& requestId, const EditOptions& options)
{
    m_pendingEditOptions[requestId] = options;
}

QString QwenImageClient::sizeToString(ImageSize size, int width, int height)
{
    switch (size) {
        case ImageSize::Square: return "1024*1024";
        case ImageSize::Portrait: return "720*1280";
        case ImageSize::Landscape: return "1280*720";
        case ImageSize::Custom:
            return (width > 0 && height > 0) ? QString("%1*%2").arg(width).arg(height) : "1024*1024";
        default: return "1024*1024";
    }
}


QString QwenImageClient::buildApiUrl(const QString& service) const
{
    return buildApiUrl(service, m_config.generateModel);
}

QString QwenImageClient::buildApiUrl(const QString& service, const QString& model) const
{
    if (isDashScopeMultimodalModel(model)) {
        return QString("%1/api/v1/services/aigc/%2").arg(m_config.baseUrl).arg(service);
    }
    return QString("%1/api/v1/services/aigc/%2/image-synthesis").arg(m_config.baseUrl).arg(service);
}

QNetworkRequest QwenImageClient::createNetworkRequest(const QString& url, bool async) const
{
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_config.apiKey).toUtf8());
    if (async) {
        request.setRawHeader("X-DashScope-Async", "enable");
    }
    return request;
}

void QwenImageClient::registerRequest(const QString& requestId, QNetworkReply* reply, RequestType type)
{
    m_activeRequests[requestId] = reply;
    m_retryCount[requestId] = 0;
    
    reply->setProperty("requestId", requestId);
    reply->setProperty("requestType", static_cast<int>(type));
}

void QwenImageClient::cleanupRequest(QNetworkReply* reply, const QString& key)
{
    if (reply) {
        reply->deleteLater();
    }
    m_activeRequests.remove(key);
}

void QwenImageClient::cancelAllRequests()
{
    for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ++it) {
        if (it.value()) {
            it.value()->abort();
            it.value()->deleteLater();
        }
    }
    m_activeRequests.clear();
}

QJsonObject QwenImageClient::parseJsonResponse(const QByteArray& data)
{
    const QByteArray trimmed = data.trimmed();
    if (trimmed.isEmpty()) {
        setError("Empty response body");
        return QJsonObject();
    }

    const QString rawContent = QString::fromUtf8(trimmed);
    QJsonObject repaired = QwenJsonRepair::parseWithRepair(rawContent);
    if (!repaired.isEmpty()) {
        return repaired;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        const QString preview = rawContent.left(160).replace('\n', ' ');
        setError(QString("JSON parse error: %1. Response preview: %2")
            .arg(parseError.errorString(), preview));
        return QJsonObject();
    }
    
    return doc.object();
}

void QwenImageClient::emitResult(RequestType type, const QString& requestId, const GenerateResult& result)
{
    Q_UNUSED(requestId);
    if (type == RequestType::Generate) {
        emit generateCompleted(result);
    } else {
        emit editCompleted(result);
    }
}

void QwenImageClient::emitPendingResult(const QString& requestId, const GenerateResult& result)
{
    const PendingRequestKind kind = pendingRequestKind(requestId);
    const QString kindName = (kind == PendingRequestKind::Generate)
        ? QStringLiteral("generate")
        : (kind == PendingRequestKind::Edit)
            ? QStringLiteral("edit")
            : QStringLiteral("none");

    LOG_INFO("QwenImageClient", QString("emitPendingResult: requestId=%1, kind=%2, success=%3, inGenerateMap=%4, inEditMap=%5")
        .arg(requestId)
        .arg(kindName)
        .arg(result.success)
        .arg(m_pendingGenerateOptions.contains(requestId))
        .arg(m_pendingEditOptions.contains(requestId)));
    
    clearPendingRequestState(requestId);
    
    if (kind == PendingRequestKind::Generate) {
        LOG_INFO("QwenImageClient", QString("emitPendingResult: emitting generateCompleted for requestId=%1").arg(requestId));
        emit generateCompleted(result);
    } else {
        LOG_WARNING("QwenImageClient", QString("emitPendingResult: requestId=%1 not in generate map, emitting editCompleted!").arg(requestId));
        emit editCompleted(result);
    }

    finishAsyncRequest();
}

void QwenImageClient::handleTaskSuccess(const QJsonObject& output, const QString& requestId)
{
    LOG_INFO("QwenImageClient", QString("handleTaskSuccess called: requestId=%1").arg(requestId));
    emit progressChanged("generating", 80);

    QJsonObject response;
    response["output"] = output;
    QString imageUrl = extractInlineImageUrl(response);
    if (imageUrl.isEmpty()) {
        QJsonArray results = output["results"].toArray();
        if (!results.isEmpty()) {
            imageUrl = results.first().toObject()["url"].toString();
        }
    }

    LOG_INFO("QwenImageClient", QString("handleTaskSuccess: imageUrl=%1").arg(imageUrl.left(100)));
    if (imageUrl.isEmpty()) {
        LOG_WARNING("QwenImageClient", "handleTaskSuccess: imageUrl is empty!");
        emitPendingResult(requestId, createErrorResult(requestId, "No image URL in response"));
        return;
    }

    LOG_INFO("QwenImageClient", QString("handleTaskSuccess: calling downloadImageFromUrl"));
    downloadImageFromUrl(imageUrl, requestId);
}

QwenImageClient::GenerateResult QwenImageClient::createErrorResult(const QString& requestId, const QString& message) const
{
    GenerateResult result;
    result.success = false;
    result.errorMessage = message;
    result.requestId = requestId;
    return result;
}

void QwenImageClient::setError(const QString& message)
{
    m_lastError = message;
    LOG_ERROR("QwenImageClient", message);
}

QwenImageClient::GenerateResult QwenImageClient::generatePlaceholder(const QString& prompt)
{
    Q_UNUSED(prompt);
    GenerateResult result;
    result.success = true;
    result.mimeType = "image/png";
    result.requestId = QString("mock-%1").arg(QDateTime::currentMSecsSinceEpoch());
    result.width = 1;
    result.height = 1;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    return result;
}
