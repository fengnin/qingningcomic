#include "VolcEngineImageClient.h"
#include "ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "Logger.h"
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
    , m_networkManager(new QNetworkAccessManager(this))
    , m_initialized(false)
{
}

VolcEngineImageClient::~VolcEngineImageClient()
{
    cancelAllRequests();
}

DEFINE_SINGLETON_INSTANCE(VolcEngineImageClient, volcEngineImageClient)

bool VolcEngineImageClient::init(const Config& config)
{
    if (config.accessKey.isEmpty() || config.secretKey.isEmpty()) {
        LOG_WARNING("VolcEngineImageClient", "AccessKey 或 SecretKey 为空，将使用占位图模式");
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

// ==================== 异步 API ====================

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
    
    QString requestId = options.requestId.isEmpty() ? QUuid::createUuid().toString() : options.requestId;
    QString url = buildApiUrl();
    QJsonObject payload = buildGenerateRequestBody(options);
    QByteArray bodyData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    
    QNetworkRequest request = createNetworkRequest(url, QString::fromUtf8(bodyData));
    
    LOG_DEBUG("VolcEngineImageClient", QString("Sending async request to: %1").arg(url));
    
    QNetworkReply* reply = m_networkManager->post(request, bodyData);
    
    registerRequest(requestId, reply);
    m_pendingOptions[requestId] = options;
    
    connect(reply, &QNetworkReply::finished, this, &VolcEngineImageClient::onReplyFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &VolcEngineImageClient::onReplyError);
    
    emit progressChanged("sending_request", 0);
}

// ==================== 同步 API ====================

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
    
    // 如果有参考图，使用图生图 API
    if (!options.referenceImage.isEmpty()) {
        return generateWithReference(options);
    }
    
    // 否则使用文生图 API
    QString url = buildApiUrl();
    QJsonObject payload = buildGenerateRequestBody(options);
    
    LOG_INFO("VolcEngineImageClient", QString("Sending text2img request to: %1").arg(url));
    
    QJsonObject response = sendSyncRequest(url, payload);
    
    if (response.isEmpty()) {
        LOG_ERROR("VolcEngineImageClient", QString("API request failed: %1").arg(m_lastError));
        GenerateResult result;
        result.success = false;
        result.errorMessage = m_lastError.isEmpty() ? "API request failed" : m_lastError;
        result.requestId = options.requestId;
        return result;
    }
    
    LOG_INFO("VolcEngineImageClient", "Response received, parsing...");
    return parseResponse(response);
}

// ==================== 图生图 API ====================

VolcEngineImageClient::GenerateResult VolcEngineImageClient::generateWithReference(const GenerateOptions& options)
{
    LOG_INFO("VolcEngineImageClient", "Using img2img API with reference image");
    
    // 步骤1: 提交任务
    QString submitUrl = QString("%1?Action=CVSync2AsyncSubmitTask&Version=2022-08-31").arg(m_config.baseUrl);
    
    QJsonObject payload;
    payload["req_key"] = m_config.img2imgReqKey;
    payload["prompt"] = options.prompt;
    
    // 参考图使用 base64 编码
    QJsonArray imageDataArray;
    imageDataArray.append(QString(options.referenceImage.toBase64()));
    payload["binary_data_base64"] = imageDataArray;
    
    if (options.seed >= 0) {
        payload["seed"] = options.seed;
    }
    
    // 设置图片尺寸
    if (options.width > 0 && options.height > 0) {
        payload["width"] = options.width;
        payload["height"] = options.height;
    }
    
    LOG_INFO("VolcEngineImageClient", QString("Submitting img2img task to: %1").arg(submitUrl));
    
    QJsonObject submitResponse = sendSyncRequest(submitUrl, payload);
    
    if (submitResponse.isEmpty()) {
        LOG_ERROR("VolcEngineImageClient", QString("Submit task failed: %1").arg(m_lastError));
        GenerateResult result;
        result.success = false;
        result.errorMessage = m_lastError.isEmpty() ? "Submit task failed" : m_lastError;
        result.requestId = options.requestId;
        return result;
    }
    
    // 获取任务 ID
    QJsonObject data = submitResponse["data"].toObject();
    QString taskId = data["task_id"].toString();
    
    if (taskId.isEmpty()) {
        LOG_ERROR("VolcEngineImageClient", "No task_id in response");
        GenerateResult result;
        result.success = false;
        result.errorMessage = "No task_id in response";
        result.requestId = options.requestId;
        return result;
    }
    
    LOG_INFO("VolcEngineImageClient", QString("Task submitted, taskId: %1, polling for result...").arg(taskId));
    
    // 步骤2: 轮询查询结果
    return pollTaskResult(taskId, options);
}

VolcEngineImageClient::GenerateResult VolcEngineImageClient::pollTaskResult(const QString& taskId, const GenerateOptions& options)
{
    QString queryUrl = QString("%1?Action=CVSync2AsyncGetResult&Version=2022-08-31").arg(m_config.baseUrl);
    
    int maxAttempts = 60;  // 最多轮询60次
    int pollInterval = 2000;  // 每2秒查询一次
    
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        QJsonObject queryPayload;
        queryPayload["req_key"] = m_config.img2imgReqKey;
        queryPayload["task_id"] = taskId;
        
        // 配置返回 URL
        QJsonObject reqJson;
        reqJson["return_url"] = true;
        queryPayload["req_json"] = QString::fromUtf8(QJsonDocument(reqJson).toJson(QJsonDocument::Compact));
        
        QJsonObject response = sendSyncRequest(queryUrl, queryPayload);
        
        if (response.isEmpty()) {
            LOG_WARNING("VolcEngineImageClient", QString("Query attempt %1 failed: %2").arg(attempt + 1).arg(m_lastError));
            QThread::msleep(pollInterval);
            continue;
        }
        
        QJsonObject data = response["data"].toObject();
        QString status = data["status"].toString();
        
        LOG_DEBUG("VolcEngineImageClient", QString("Task status: %1 (attempt %2/%3)")
            .arg(status).arg(attempt + 1).arg(maxAttempts));
        
        if (status == "done") {
            // 任务完成，提取图片
            QJsonArray imageUrls = data["image_urls"].toArray();
            if (!imageUrls.isEmpty()) {
                QString imageUrl = imageUrls[0].toString();
                LOG_INFO("VolcEngineImageClient", QString("Got image URL: %1").arg(imageUrl.left(100)));
                
                // 下载图片
                QByteArray imageData = downloadImage(imageUrl);
                
                GenerateResult result;
                result.success = !imageData.isEmpty();
                result.imageUrl = imageUrl;
                result.imageData = imageData;
                result.requestId = options.requestId;
                result.timestamp = QDateTime::currentMSecsSinceEpoch();
                
                if (imageData.isEmpty()) {
                    result.errorMessage = "Failed to download image";
                }
                
                return result;
            } else {
                // 尝试从 base64 获取
                QJsonArray binaryData = data["binary_data_base64"].toArray();
                if (!binaryData.isEmpty()) {
                    QByteArray imageData = QByteArray::fromBase64(binaryData[0].toString().toUtf8());
                    
                    GenerateResult result;
                    result.success = !imageData.isEmpty();
                    result.imageData = imageData;
                    result.requestId = options.requestId;
                    result.timestamp = QDateTime::currentMSecsSinceEpoch();
                    
                    return result;
                }
            }
            
            GenerateResult result;
            result.success = false;
            result.errorMessage = "No image in response";
            result.requestId = options.requestId;
            return result;
        } else if (status == "not_found" || status == "expired") {
            GenerateResult result;
            result.success = false;
            result.errorMessage = QString("Task %1").arg(status);
            result.requestId = options.requestId;
            return result;
        }
        
        // 任务还在处理中，等待后重试
        QThread::msleep(pollInterval);
    }
    
    GenerateResult result;
    result.success = false;
    result.errorMessage = "Task polling timeout";
    result.requestId = options.requestId;
    return result;
}

// ==================== 响应处理槽函数 ====================

void VolcEngineImageClient::onReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QString requestId = reply->property("requestId").toString();
    
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
    
    // 如果返回的是URL，需要下载图片
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
    
    QString errorMessage = QString("Network error: %1").arg(reply->errorString());
    
    LOG_ERROR("VolcEngineImageClient", errorMessage);
    setError(errorMessage);
    
    emit errorOccurred("network", errorMessage);
}

// ==================== 同步请求 ====================

QJsonObject VolcEngineImageClient::sendSyncRequest(const QString& url, const QJsonObject& payload)
{
    try {
        QByteArray bodyData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        QNetworkRequest request = createNetworkRequest(url, QString::fromUtf8(bodyData));
        
        LOG_DEBUG("VolcEngineImageClient", QString("Sending sync request to: %1").arg(url));
        
        QNetworkAccessManager *localManager = new QNetworkAccessManager();
        QNetworkReply* reply = localManager->post(request, bodyData);
        
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        
        timer.start(m_config.requestTimeout);
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

// ==================== 请求构建 ====================

QJsonObject VolcEngineImageClient::buildGenerateRequestBody(const GenerateOptions& options)
{
    QJsonObject body;
    body["req_key"] = m_config.reqKey;
    body["prompt"] = options.prompt;
    
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
    
    return body;
}

QString VolcEngineImageClient::buildApiUrl() const
{
    return QString("%1?Action=CVProcess&Version=2022-08-31").arg(m_config.baseUrl);
}

// ==================== 签名认证 ====================

QNetworkRequest VolcEngineImageClient::createNetworkRequest(const QString& url, const QString& body)
{
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QDateTime now = QDateTime::currentDateTimeUtc();
    QString dateTimeStr = VolcEngineSignature::formatDateTime(now);
    
    // 解析URL获取path和query
    QUrl parsedUrl(url);
    QString path = parsedUrl.path().isEmpty() ? "/" : parsedUrl.path();
    QString query = parsedUrl.query();
    QString host = parsedUrl.host();
    
    // 构建签名配置
    VolcEngineSignature::Config sigConfig;
    sigConfig.accessKey = m_config.accessKey;
    sigConfig.secretKey = m_config.secretKey;
    sigConfig.region = m_config.region;
    sigConfig.service = m_config.service;
    
    // 构建请求信息
    VolcEngineSignature::RequestInfo requestInfo;
    requestInfo.method = "POST";
    requestInfo.path = path;
    requestInfo.query = query;
    requestInfo.body = body;
    requestInfo.host = host;
    requestInfo.timestamp = now;
    
    // 计算请求体哈希（用于调试）
    QString bodyHash = VolcEngineSignature::sha256Hash(body.toUtf8());
    LOG_DEBUG("VolcEngineImageClient", QString("Body size: %1, hash: %2...").arg(body.size()).arg(bodyHash.left(16)));
    
    // 构建Authorization头
    QString authorization = VolcEngineSignature::buildAuthorization(sigConfig, requestInfo);
    
    LOG_DEBUG("VolcEngineImageClient", QString("Authorization: %1...").arg(authorization.left(50)));
    
    request.setRawHeader("X-Date", dateTimeStr.toUtf8());
    request.setRawHeader("Authorization", authorization.toUtf8());
    request.setRawHeader("Host", host.toUtf8());  // 签名计算需要 Host 头
    request.setRawHeader("Content-Type", "application/json");
    
    return request;
}

// ==================== 响应解析 ====================

VolcEngineImageClient::GenerateResult VolcEngineImageClient::parseResponse(const QJsonObject& response)
{
    GenerateResult result;
    
    // 检查错误
    if (response.contains("code") && response["code"].toInt() != 10000) {
        result.success = false;
        result.errorMessage = response["message"].toString();
        result.requestId = response["request_id"].toString();
        LOG_ERROR("VolcEngineImageClient", QString("API error: %1").arg(result.errorMessage));
        return result;
    }
    
    QJsonObject data = response["data"].toObject();
    
    // 优先从image_urls获取
    QJsonArray imageUrls = data["image_urls"].toArray();
    if (!imageUrls.isEmpty()) {
        result.imageUrl = imageUrls.first().toString();
        result.success = !result.imageUrl.isEmpty();
        LOG_INFO("VolcEngineImageClient", QString("Got image URL: %1").arg(result.imageUrl.left(100)));
    }
    
    // 如果没有URL，尝试从binary_data_base64获取
    if (!result.success) {
        QJsonArray binaryData = data["binary_data_base64"].toArray();
        if (!binaryData.isEmpty()) {
            QString base64Data = binaryData.first().toString();
            if (!base64Data.isEmpty()) {
                result.imageData = QByteArray::fromBase64(base64Data.toLatin1());
                result.success = !result.imageData.isEmpty();
                result.mimeType = "image/png";
                LOG_INFO("VolcEngineImageClient", QString("Got base64 image, size: %1 bytes").arg(result.imageData.size()));
            }
        }
    }
    
    result.requestId = response["request_id"].toString();
    if (result.requestId.isEmpty()) {
        result.requestId = data["request_id"].toString();
    }
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    return result;
}

// ==================== 辅助方法 ====================

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
    GenerateResult result;
    result.success = true;
    // 最小的PNG图片（1x1像素）
    result.imageData = QByteArray::fromBase64(
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==");
    result.mimeType = "image/png";
    result.requestId = QString("mock-%1").arg(QDateTime::currentMSecsSinceEpoch());
    result.width = 1;
    result.height = 1;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (!prompt.isEmpty()) {
        LOG_DEBUG("VolcEngineImageClient", QString("Generated placeholder for prompt: %1").arg(prompt.left(100)));
    }
    
    return result;
}
