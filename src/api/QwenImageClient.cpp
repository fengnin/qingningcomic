#include "QwenImageClient.h"
#include "ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QUuid>
#include <QThread>

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
        LOG_WARNING("QwenImageClient", "API Key 为空，将使用占位图模式");
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

// ==================== 异步 API ====================

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
    
    sendAsyncRequest(options, RequestType::Generate);
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
    
    sendAsyncRequest(options, RequestType::Edit);
}

// ==================== 同步 API ====================

QwenImageClient::GenerateResult QwenImageClient::generate(const GenerateOptions& options)
{
    LOG_INFO("QwenImageClient", QString("generate() called, shouldMock=%1, initialized=%2, apiKey.isEmpty=%3")
        .arg(shouldMock()).arg(m_initialized).arg(m_config.apiKey.isEmpty()));
    
    if (!validateGenerateOptions(options)) {
        LOG_WARNING("QwenImageClient", "validateGenerateOptions failed");
        return generatePlaceholder(options.prompt);
    }
    
    if (shouldMock()) {
        LOG_INFO("QwenImageClient", QString("Using placeholder for prompt: %1").arg(options.prompt.left(100)));
        return generatePlaceholder(options.prompt);
    }
    
    QString url = buildApiUrl("text2image");
    QJsonObject payload = buildGenerateRequestBody(options);
    
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
    if (!validateEditOptions(options) || shouldMock()) {
        return generatePlaceholder(options.prompt);
    }
    
    QString url = buildApiUrl("image2image");
    QJsonObject payload = buildEditRequestBody(options);
    QJsonObject response = sendSyncRequest(url, payload);
    
    if (response.isEmpty()) {
        LOG_WARNING("QwenImageClient", "Edit failed, using placeholder");
        return generatePlaceholder(options.prompt);
    }
    
    return extractImageFromResponse(response);
}

// ==================== 异步请求发送 ====================

void QwenImageClient::sendAsyncRequest(const GenerateOptions& options, RequestType type)
{
    QString requestId = options.requestId.isEmpty() ? QUuid::createUuid().toString() : options.requestId;
    QString url = (type == RequestType::Generate) ? buildApiUrl("text2image") : buildApiUrl("image2image");
    QJsonObject payload = (type == RequestType::Generate) 
        ? buildGenerateRequestBody(options) 
        : buildEditRequestBody(EditOptions::fromGenerateOptions(options));
    
    QNetworkRequest request = createNetworkRequest(url);
    QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    
    LOG_DEBUG("QwenImageClient", QString("Sending async %1 request: %2")
        .arg(type == RequestType::Generate ? "generate" : "edit").arg(url));
    
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    
    registerRequest(requestId, reply, type);
    m_pendingGenerateOptions[requestId] = options;
    
    connect(reply, &QNetworkReply::finished, this, &QwenImageClient::onReplyFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &QwenImageClient::onReplyError);
    
    emit progressChanged("sending_request", 0);
}

void QwenImageClient::sendAsyncRequest(const EditOptions& options, RequestType type)
{
    QString requestId = options.requestId.isEmpty() ? QUuid::createUuid().toString() : options.requestId;
    QString url = buildApiUrl("image2image");
    QJsonObject payload = buildEditRequestBody(options);
    
    QNetworkRequest request = createNetworkRequest(url);
    QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    
    LOG_DEBUG("QwenImageClient", QString("Sending async edit request: %1").arg(url));
    
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    
    registerRequest(requestId, reply, type);
    m_pendingEditOptions[requestId] = options;
    
    connect(reply, &QNetworkReply::finished, this, &QwenImageClient::onReplyFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &QwenImageClient::onReplyError);
    
    emit progressChanged("sending_request", 0);
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

// ==================== 响应处理槽函数 ====================

void QwenImageClient::onReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString requestId = reply->property("requestId").toString();
    RequestType type = static_cast<RequestType>(reply->property("requestType").toInt());
    
    QByteArray responseData = reply->readAll();
    cleanupRequest(reply, requestId);
    
    QJsonObject response = parseJsonResponse(responseData);
    if (response.isEmpty()) {
        emitResult(type, requestId, createErrorResult(requestId, m_lastError));
        return;
    }
    
    if (response.contains("error")) {
        QString errorMsg = response["error"].toObject()["message"].toString();
        setError(errorMsg);
        emitResult(type, requestId, createErrorResult(requestId, errorMsg));
        return;
    }
    
    // 检查是否是异步任务
    QString taskId = response["output"].toObject()["task_id"].toString();
    if (!taskId.isEmpty()) {
        LOG_DEBUG("QwenImageClient", QString("Async task created: %1").arg(taskId));
        emit progressChanged("task_submitted", 10);
        pollTaskStatus(taskId, requestId);
        return;
    }
    
    // 同步响应
    GenerateResult result = extractImageFromResponse(response);
    result.requestId = requestId;
    
    if (!result.success) {
        result.errorMessage = "Failed to extract image from response";
    }
    
    emitResult(type, requestId, result);
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
    
    LOG_DEBUG("QwenImageClient", QString("Task %1 status: %2").arg(taskId).arg(taskStatus));
    
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
    
    QString errorMessage = QString("Network error: %1").arg(reply->errorString());
    
    LOG_ERROR("QwenImageClient", errorMessage);
    setError(errorMessage);
    
    emit errorOccurred("network", errorMessage);
}

// ==================== 同步请求 ====================

QJsonObject QwenImageClient::sendSyncRequest(const QString& url, const QJsonObject& payload)
{
    try {
        QNetworkRequest request = createNetworkRequest(url);
        QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        
        LOG_DEBUG("QwenImageClient", QString("Sending sync request to: %1").arg(url));
        
        QNetworkAccessManager *localManager = new QNetworkAccessManager();
        QNetworkReply* reply = localManager->post(request, jsonData);
        
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        
        timer.start(m_config.requestTimeout);
        // 使用 ExcludeUserInputEvents 避免处理可能导致问题的用户输入事件
        loop.exec(QEventLoop::ExcludeUserInputEvents);
        
        if (!timer.isActive()) {
            reply->abort();
            setError("请求超时");
            reply->deleteLater();
            delete localManager;
            return QJsonObject();
        }
        
        if (reply->error() != QNetworkReply::NoError) {
            setError(reply->errorString());
            reply->deleteLater();
            delete localManager;
            return QJsonObject();
        }
        
        QByteArray responseData = reply->readAll();
        reply->deleteLater();
        delete localManager;
        
        QJsonObject response = parseJsonResponse(responseData);
        
        QString taskId = response["output"].toObject()["task_id"].toString();
        if (!taskId.isEmpty()) {
            LOG_DEBUG("QwenImageClient", QString("Async task created: %1, polling...").arg(taskId));
            response = pollTaskSync(taskId);
        }
        
        return response;
    } catch (const std::exception& e) {
        setError(QString("sendSyncRequest exception: %1").arg(QString::fromUtf8(e.what())));
        LOG_ERROR("QwenImageClient", QString("Exception in sendSyncRequest: %1").arg(QString::fromUtf8(e.what())));
        return QJsonObject();
    } catch (...) {
        setError("sendSyncRequest unknown exception");
        LOG_ERROR("QwenImageClient", "Unknown exception in sendSyncRequest");
        return QJsonObject();
    }
}

QJsonObject QwenImageClient::pollTaskSync(const QString& taskId)
{
    QString url = QString("%1/api/v1/tasks/%2").arg(m_config.baseUrl).arg(taskId);
    
    int maxPolls = 60;
    int pollInterval = 2000;
    
    for (int i = 0; i < maxPolls; i++) {
        try {
            QNetworkRequest request = createNetworkRequest(url, false);
            QNetworkAccessManager *pollManager = new QNetworkAccessManager();
            QNetworkReply* reply = pollManager->get(request);
            
            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);
            
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
            
            timer.start(10000);
            // 使用 ExcludeUserInputEvents 避免处理可能导致问题的用户输入事件
            loop.exec(QEventLoop::ExcludeUserInputEvents);
            
            if (!timer.isActive()) {
                reply->abort();
                reply->deleteLater();
                delete pollManager;
                continue;
            }
            
            if (reply->error() != QNetworkReply::NoError) {
                setError(reply->errorString());
                reply->deleteLater();
                delete pollManager;
                return QJsonObject();
            }
            
            QByteArray responseData = reply->readAll();
            reply->deleteLater();
            delete pollManager;
            
            QJsonObject response = parseJsonResponse(responseData);
            QJsonObject output = response["output"].toObject();
            QString taskStatus = output["task_status"].toString();
            
            LOG_DEBUG("QwenImageClient", QString("Task %1 status: %2").arg(taskId).arg(taskStatus));
            
            if (taskStatus == "SUCCEEDED") {
                return response;
            } else if (taskStatus == "FAILED") {
                setError(output["message"].toString());
                return QJsonObject();
            }
            
            QThread::msleep(pollInterval);
        } catch (const std::exception& e) {
            LOG_WARNING("QwenImageClient", QString("Exception in pollTaskSync: %1").arg(QString::fromUtf8(e.what())));
            QThread::msleep(1000);
        } catch (...) {
            LOG_WARNING("QwenImageClient", "Unknown exception in pollTaskSync");
            QThread::msleep(1000);
        }
    }
    
    setError("任务轮询超时");
    return QJsonObject();
}

QByteArray QwenImageClient::downloadImage(const QString& url)
{
    QNetworkRequest request{QUrl(url)};
    QNetworkAccessManager *dlManager = new QNetworkAccessManager();
    QNetworkReply* reply = dlManager->get(request);
    
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
    delete dlManager;
    return imageData;
}

// ==================== 响应解析 ====================

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
    
    QJsonArray dataArray = response["data"].toArray();
    if (dataArray.isEmpty()) return result;
    
    QString base64Image = dataArray.first().toObject()["b64_json"].toString();
    if (!base64Image.isEmpty()) {
        result.imageData = QByteArray::fromBase64(base64Image.toLatin1());
        result.success = !result.imageData.isEmpty();
        result.mimeType = "image/png";
    }
    
    result.requestId = response["request_id"].toString();
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    return result;
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

// ==================== 辅助方法 ====================

bool QwenImageClient::validateGenerateOptions(const GenerateOptions& options)
{
    if (options.prompt.isEmpty()) {
        setError("Prompt is required");
        return false;
    }
    return true;
}

bool QwenImageClient::validateEditOptions(const EditOptions& options)
{
    if (options.prompt.isEmpty()) {
        setError("Prompt is required");
        return false;
    }
    if (options.sourceImage.isEmpty()) {
        setError("Source image is required");
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
    
    if (!options.style.isEmpty()) {
        parameters["style"] = options.style;
    }
    if (options.textRendering) {
        parameters["text_rendering"] = true;
    }
    if (options.size == ImageSize::Custom && options.width > 0 && options.height > 0) {
        parameters["size"] = QString("%1*%2").arg(options.width).arg(options.height);
    }
    
    body["parameters"] = parameters;
    return body;
}

QJsonObject QwenImageClient::buildEditRequestBody(const EditOptions& options)
{
    QJsonObject body;
    body["model"] = m_config.editModel;
    
    QJsonObject input;
    input["prompt"] = options.prompt;
    input["image"] = QString::fromLatin1(options.sourceImage.toBase64());
    
    if (!options.maskImage.isEmpty()) {
        input["mask"] = QString::fromLatin1(options.maskImage.toBase64());
    }
    if (!options.negativePrompt.isEmpty()) {
        input["negative_prompt"] = options.negativePrompt;
    }
    body["input"] = input;
    
    body["parameters"] = buildCommonParameters(options.size, options.seed);
    return body;
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

// ==================== 私有辅助方法 ====================

QString QwenImageClient::buildApiUrl(const QString& service) const
{
    return QString("%1/api/v1/services/aigc/%2/image-synthesis")
        .arg(m_config.baseUrl).arg(service);
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
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        setError(QString("JSON parse error: %1").arg(parseError.errorString()));
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
    bool isGenerate = m_pendingGenerateOptions.contains(requestId);
    
    LOG_INFO("QwenImageClient", QString("emitPendingResult: requestId=%1, isGenerate=%2, success=%3, inGenerateMap=%4, inEditMap=%5")
        .arg(requestId)
        .arg(isGenerate)
        .arg(result.success)
        .arg(m_pendingGenerateOptions.contains(requestId))
        .arg(m_pendingEditOptions.contains(requestId)));
    
    m_pendingGenerateOptions.remove(requestId);
    m_pendingEditOptions.remove(requestId);
    m_retryCount.remove(requestId);
    
    if (isGenerate) {
        LOG_INFO("QwenImageClient", QString("emitPendingResult: emitting generateCompleted for requestId=%1").arg(requestId));
        emit generateCompleted(result);
    } else {
        LOG_WARNING("QwenImageClient", QString("emitPendingResult: requestId=%1 not in generate map, emitting editCompleted!").arg(requestId));
        emit editCompleted(result);
    }
}

void QwenImageClient::handleTaskSuccess(const QJsonObject& output, const QString& requestId)
{
    LOG_INFO("QwenImageClient", QString("handleTaskSuccess called: requestId=%1").arg(requestId));
    emit progressChanged("generating", 80);
    
    QJsonArray results = output["results"].toArray();
    LOG_DEBUG("QwenImageClient", QString("handleTaskSuccess: results array size=%1").arg(results.size()));
    
    if (results.isEmpty()) {
        LOG_WARNING("QwenImageClient", "handleTaskSuccess: results array is empty!");
        emitPendingResult(requestId, createErrorResult(requestId, "No image URL in response"));
        return;
    }
    
    QString imageUrl = results.first().toObject()["url"].toString();
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
    GenerateResult result;
    result.success = true;
    // 将 base64 字符串解码为真正的图片数据
    result.imageData = QByteArray::fromBase64(PLACEHOLDER_IMAGE);
    result.mimeType = "image/png";
    result.requestId = QString("mock-%1").arg(QDateTime::currentMSecsSinceEpoch());
    result.width = 1;
    result.height = 1;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (!prompt.isEmpty()) {
        LOG_DEBUG("QwenImageClient", QString("Generated placeholder for prompt: %1").arg(prompt.left(100)));
    }
    
    return result;
}
