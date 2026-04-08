#include "QwenClient.h"
#include "ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "SchemaToPrompt.h"
#include "Logger.h"
#include "EncodingUtils.h"
#include "JsonRepairAdapter.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QFile>
#include <QTimer>
#include <QThread>
#include <QRegularExpression>
#include <QtMath>
#include <QSet>
#include <QPointer>

DEFINE_SINGLETON_INSTANCE_SIMPLE(QwenClient)

QwenClient::QwenClient()
    : m_initialized(false)
{
}

QwenClient::~QwenClient()
{
}

bool QwenClient::init(const Config& config)
{
    if (config.apiKey.isEmpty()) {
        m_lastError = TR("API 密钥不能为空");
        LOG_ERROR("QwenClient", m_lastError);
        return false;
    }
    
    m_config = config;
    if (m_config.endpoint.isEmpty()) {
        m_config.endpoint = "https://dashscope.aliyuncs.com/compatible-mode/v1";
    }
    m_logFilePath = "qwen.log";
    m_initialized = true;
    
    LOG_INFO("QwenClient", QString("Initialized with model: %1").arg(m_config.model));
    return true;
}

bool QwenClient::isInitialized() const
{
    return m_initialized;
}

QString QwenClient::lastError() const
{
    return m_lastError;
}

// ==================== 辅助方法 ====================

bool QwenClient::checkInitialized(const QString& operation)
{
    if (m_initialized) {
        return true;
    }
    m_lastError = TR("客户端未初始化");
    emit errorOccurred(operation, m_lastError);
    return false;
}

QJsonArray QwenClient::buildMessages(const QString& systemPrompt, const QString& userPrompt)
{
    QJsonArray messages;
    
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = systemPrompt;
    messages.append(systemMsg);
    
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userPrompt;
    messages.append(userMsg);
    
    return messages;
}

QString QwenClient::buildUserMessageWithBible(
    const QString& text,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes,
    int chapterNumber)
{
    QString userMessage = text;
    
    if (existingCharacters.isEmpty() && existingScenes.isEmpty()) {
        return userMessage;
    }
    
    userMessage = QString(TR("章节 %1：\n\n")).arg(chapterNumber > 0 ? QString::number(chapterNumber) : "?");
    
    if (!existingCharacters.isEmpty()) {
        userMessage += TR("【现有角色圣经】请在生成的 characters 数组中包含以下所有角色，并保持其 appearance 不变：\n");
        userMessage += QString::fromUtf8(QJsonDocument(existingCharacters).toJson(QJsonDocument::Indented));
        userMessage += "\n\n";
    }
    
    if (!existingScenes.isEmpty()) {
        userMessage += TR("【现有场景圣经】请在生成的 scenes 数组中包含以下所有场景，并保持其 visualCharacteristics 不变。在 panel.background.sceneId 中优先使用这些场景ID：\n");
        userMessage += QString::fromUtf8(QJsonDocument(existingScenes).toJson(QJsonDocument::Indented));
        userMessage += "\n\n";
    }
    
    userMessage += TR("【新章节文本】\n") + text;
    
    return userMessage;
}

// ==================== 核心功能 ====================

QwenClient::StoryboardResult QwenClient::generateStoryboard(
    const QString& text,
    const QJsonObject& jsonSchema,
    bool strictMode,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes,
    int chapterNumber)
{
    StoryboardResult result;
    
    if (!checkInitialized("generateStoryboard")) {
        result.errorMessage = m_lastError;
        return result;
    }
    
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    log("🚀 Starting storyboard generation");
    log(QString("  Text length: %1, Chunks: ~%2").arg(text.length()).arg(text.length() / m_config.maxChunkLength + 1));
    
    QStringList chunks = splitTextIntelligently(text, m_config.maxChunkLength);
    log(QString("✂️ Split into %1 chunks").arg(chunks.size()));
    
    // 使用并行处理
    QList<QJsonObject> validResponses = processChunksParallel(
        chunks, jsonSchema, strictMode,
        existingCharacters, existingScenes, chapterNumber
    );
    
    if (validResponses.isEmpty()) {
        m_lastError = TR("所有块都失败了");
        result.errorMessage = m_lastError;
        emit errorOccurred("generateStoryboard", m_lastError);
        return result;
    }
    
    QJsonObject merged = mergeStoryboards(validResponses);
    result.panels = merged["panels"].toArray();
    result.characters = merged["characters"].toArray();
    result.scenes = merged["scenes"].toArray();
    result.totalPages = merged["totalPages"].toInt();
    result.success = true;
    
    qint64 totalMs = QDateTime::currentMSecsSinceEpoch() - startTime;
    log(QString("✅ Completed: %1 panels, %2 chars, %3 scenes in %4ms")
        .arg(result.panels.size()).arg(result.characters.size())
        .arg(result.scenes.size()).arg(totalMs));
    
    return result;
}

QList<QJsonObject> QwenClient::processChunksParallel(
    const QStringList& chunks,
    const QJsonObject& jsonSchema,
    bool strictMode,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes,
    int chapterNumber)
{
    QList<QJsonObject> results;
    int totalChunks = chunks.size();
    int failedCount = 0;
    
    // 累积的角色和场景，用于保持连续性
    QJsonArray accumulatedCharacters = existingCharacters;
    QJsonArray accumulatedScenes = existingScenes;
    
    for (int i = 0; i < chunks.size(); ++i) {
        emit progressChanged(i + 1, totalChunks,
            QString(TR("正在处理第 %1/%2 块")).arg(i + 1).arg(totalChunks));
        
        // 所有 chunk 都传递累积的角色和场景，保持连续性
        QJsonObject response = callStoryboardApi(
            chunks[i], jsonSchema, strictMode,
            accumulatedCharacters,
            accumulatedScenes,
            chapterNumber
        );
        
        if (!response.isEmpty()) {
            results.append(response);
            
            // 从结果中提取新角色和场景，累积到列表中供后续 chunk 使用
            if (response.contains("characters")) {
                QJsonArray newChars = response["characters"].toArray();
                for (const auto& c : newChars) {
                    accumulatedCharacters.append(c);
                }
            }
            if (response.contains("scenes")) {
                QJsonArray newScenes = response["scenes"].toArray();
                for (const auto& s : newScenes) {
                    accumulatedScenes.append(s);
                }
            }
            
            log(QString("✅ Chunk %1/%2 succeeded").arg(i + 1).arg(totalChunks));
        } else {
            failedCount++;
            log(QString("❌ Chunk %1/%2 failed").arg(i + 1).arg(totalChunks));
            m_lastError = TR("第 %1/%2 块处理失败").arg(i + 1).arg(totalChunks);
        }
    }
    
    if (failedCount > 0) {
        m_lastError = TR("%1/%2 块处理失败，已中止").arg(failedCount).arg(totalChunks);
        emit errorOccurred("processChunksParallel", m_lastError);
        return QList<QJsonObject>();
    }
    
    return results;
}

QJsonObject QwenClient::parseChangeRequest(
    const QString& naturalLanguage,
    const QJsonObject& jsonSchema,
    const QJsonObject& context)
{
    Q_UNUSED(jsonSchema);
    
    if (!checkInitialized("parseChangeRequest")) {
        return QJsonObject();
    }
    
    log(TR("解析自然语言改稿请求"));
    
    QString contextInfo = context.contains("panelId") 
        ? QString("\n当前面板 ID: %1").arg(context["panelId"].toString()) 
        : "";
    QString userPrompt = TR("自然语言请求：\n%1\n%2\n\n请输出 CR-DSL JSON。")
        .arg(naturalLanguage).arg(contextInfo);
    
    QJsonObject payload = buildSimplePayload(
        buildChangeRequestPrompt(), userPrompt, 0.2, 2000);
    
    QJsonObject response = sendApiRequest(payload);
    if (response.isEmpty()) {
        return QJsonObject();
    }
    
    QJsonObject result = extractApiResponse(response);
    return parseJsonWithRepair(result["content"].toString());
}

QString QwenClient::rewriteDialogue(const QString& originalDialogue, const QString& instruction)
{
    if (!checkInitialized("rewriteDialogue")) {
        return QString();
    }
    
    log(TR("重写对白"));
    
    QString userPrompt = TR("原始对白：\n%1\n\n修改指示：\n%2\n\n请输出重写后的对白。")
        .arg(originalDialogue).arg(instruction);
    
    QJsonObject payload = buildSimplePayload(
        buildDialogueRewritePrompt(), userPrompt, 0.5, 200);
    
    QJsonObject response = sendApiRequest(payload);
    if (response.isEmpty()) {
        return QString();
    }
    
    QJsonObject result = extractApiResponse(response);
    return result["content"].toString().trimmed();
}

QJsonObject QwenClient::correctJson(const QJsonObject& invalidJson, const QStringList& errors)
{
    if (!checkInitialized("correctJson")) {
        return QJsonObject();
    }
    
    log(TR("尝试修正无效 JSON"));
    
    QString errorsStr = errors.join("\n");
    
    // 和原仓库保持一致：传递完整 JSON，不截断
    QString invalidJsonStr = QString::fromUtf8(QJsonDocument(invalidJson).toJson(QJsonDocument::Indented));
    
    QString userPrompt = TR("以下 JSON 有校验错误，请修正使其符合 Schema。\n\n错误列表：\n%1\n\n原始 JSON：\n%2\n\n请输出修正后的完整 JSON（不要添加任何解释文字）。")
        .arg(errorsStr)
        .arg(invalidJsonStr);
    
    QString systemPrompt = TR("你是一个 JSON 修正助手。你的任务是修正不符合 Schema 的 JSON 对象。");
    
    // 和原仓库保持一致：max_tokens = 8000
    QJsonObject payload = buildSimplePayload(systemPrompt, userPrompt, 0.1, 8000);
    
    QJsonObject response = sendApiRequest(payload);
    if (response.isEmpty()) {
        return QJsonObject();
    }
    
    QJsonObject result = extractApiResponse(response);
    QString content = result["content"].toString();
    
    QRegularExpression codeBlockRegex("```json\\s*([\\s\\S]*?)\\s*```");
    QRegularExpressionMatch match = codeBlockRegex.match(content);
    QString jsonStr = match.hasMatch() ? match.captured(1) : content;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError) {
        return doc.object();
    }
    
    return QJsonObject();
}

// ==================== 文本处理 ====================

QStringList QwenClient::splitTextIntelligently(const QString& text, int maxLength)
{
    QStringList paragraphs = text.split(QRegularExpression("\n\n+"));
    QStringList chunks;
    QString currentChunk;
    
    for (const QString& para : paragraphs) {
        if (para.length() > maxLength) {
            if (!currentChunk.isEmpty()) {
                chunks.append(currentChunk.trimmed());
                currentChunk.clear();
            }
            
            QStringList sentences = extractSentences(para);
            for (const QString& sentence : sentences) {
                if (currentChunk.length() + sentence.length() > maxLength) {
                    if (!currentChunk.isEmpty()) {
                        chunks.append(currentChunk.trimmed());
                    }
                    currentChunk = sentence;
                } else {
                    currentChunk += sentence;
                }
            }
            continue;
        }
        
        if (currentChunk.length() + para.length() + 2 > maxLength) {
            if (!currentChunk.isEmpty()) {
                chunks.append(currentChunk.trimmed());
            }
            currentChunk = para;
        } else {
            if (!currentChunk.isEmpty()) {
                currentChunk += "\n\n";
            }
            currentChunk += para;
        }
    }
    
    if (!currentChunk.isEmpty()) {
        chunks.append(currentChunk.trimmed());
    }
    
    return chunks.isEmpty() ? QStringList{ text } : chunks;
}

QStringList QwenClient::extractSentences(const QString& text)
{
    QRegularExpression sentenceRegex("([^。！？.!?]+[。！？.!?]*)");
    QRegularExpressionMatchIterator it = sentenceRegex.globalMatch(text);
    
    QStringList sentences;
    while (it.hasNext()) {
        sentences.append(it.next().captured(0));
    }
    
    return sentences.isEmpty() ? QStringList{ text } : sentences;
}

QJsonObject QwenClient::mergeStoryboards(const QList<QJsonObject>& storyboards)
{
    QJsonArray mergedPanels;
    QMap<QString, QJsonObject> charMap;
    QMap<QString, QJsonObject> sceneMap;
    
    for (const QJsonObject& sb : storyboards) {
        QJsonArray panels = sb["panels"].toArray();
        for (const QJsonValue& panelVal : panels) {
            QJsonObject panel = panelVal.toObject();
            int absoluteIndex = mergedPanels.size();
            panel["page"] = absoluteIndex / PANELS_PER_PAGE + 1;
            panel["index"] = absoluteIndex % PANELS_PER_PAGE;
            mergedPanels.append(panel);
        }
        
        mergeCharacters(charMap, sb["characters"].toArray());
        mergeScenes(sceneMap, sb["scenes"].toArray());
    }
    
    QJsonObject result;
    result["panels"] = mergedPanels;
    
    QJsonArray charactersArray;
    for (const QJsonObject& c : charMap.values()) {
        charactersArray.append(c);
    }
    result["characters"] = charactersArray;
    
    QJsonArray scenesArray;
    for (const QJsonObject& s : sceneMap.values()) {
        scenesArray.append(s);
    }
    result["scenes"] = scenesArray;
    result["totalPages"] = (mergedPanels.size() + PANELS_PER_PAGE - 1) / PANELS_PER_PAGE;
    
    return result;
}

void QwenClient::mergeCharacters(QMap<QString, QJsonObject>& charMap, const QJsonArray& characters)
{
    for (const QJsonValue& charVal : characters) {
        QJsonObject character = charVal.toObject();
        QString name = character["name"].toString();
        
        if (!charMap.contains(name)) {
            charMap[name] = character;
        } else {
            QJsonObject existing = charMap[name];
            if (character.contains("appearance") && !existing.contains("appearance")) {
                existing["appearance"] = character["appearance"];
            }
            if (character.contains("personality") && !existing.contains("personality")) {
                existing["personality"] = character["personality"];
            }
            charMap[name] = existing;
        }
    }
}

void QwenClient::mergeScenes(QMap<QString, QJsonObject>& sceneMap, const QJsonArray& scenes)
{
    for (const QJsonValue& sceneVal : scenes) {
        QJsonObject scene = sceneVal.toObject();
        QString id = scene["id"].toString();
        if (!sceneMap.contains(id)) {
            sceneMap[id] = scene;
        }
    }
}

// ==================== API 调用 ====================

QNetworkRequest QwenClient::createApiRequest() const
{
    QNetworkRequest request;
    request.setUrl(QUrl(m_config.endpoint + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_config.apiKey).toUtf8());
    return request;
}

QJsonObject QwenClient::callStoryboardApi(
    const QString& text,
    const QJsonObject& schema,
    bool strictMode,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes,
    int chapterNumber)
{
    QString lastErrorMsg;
    QString systemPrompt = buildSystemPrompt();
    QString userMessage = buildUserMessageWithBible(text, existingCharacters, existingScenes, chapterNumber);
    
    logRequest(m_config.model, text, systemPrompt, userMessage);
    
    for (int attempt = 0; attempt < m_config.maxRetries; ++attempt) {
        try {
            QJsonObject payload = buildJsonSchemaPayload(
                systemPrompt, userMessage, schema, strictMode, "storyboard");
            
            qint64 startTime = QDateTime::currentMSecsSinceEpoch();
            QJsonObject response = sendApiRequest(payload);
            qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - startTime;
            
            if (response.isEmpty()) {
                throw std::runtime_error(m_lastError.toStdString());
            }
            
            QJsonObject result = extractApiResponse(response);
            QString finishReason = result["finishReason"].toString();
            QString content = result["content"].toString();
            QJsonObject usage = result["usage"].toObject();
            
            logResponse(content, finishReason, usage, elapsedMs);
            
            if (finishReason == "length") {
                LOG_WARNING("QwenClient", QString::fromUtf8("Qwen response hit max_tokens and was truncated. Consider increasing max_tokens."));
            }
            
            QJsonObject parsedJson = parseJsonWithRepair(result["content"].toString());
            
            return ensureStoryboardShape(parsedJson);
            
        } catch (const std::exception& e) {
            lastErrorMsg = QString::fromUtf8(e.what());
            
            if (lastErrorMsg.contains("429") && attempt < m_config.maxRetries - 1) {
                log("Rate limited, retrying...");
                exponentialBackoff(attempt);
                continue;
            }
            
            m_lastError = lastErrorMsg;
            log(QString("❌ Error: %1").arg(lastErrorMsg));
            break;
        }
    }
    
    emit errorOccurred("callStoryboardApi", lastErrorMsg);
    return QJsonObject();
}

QJsonObject QwenClient::sendApiRequest(const QJsonObject& payload)
{
    QNetworkAccessManager networkManager;
    
    QNetworkRequest request = createApiRequest();
    QByteArray requestData = QJsonDocument(payload).toJson();
    
    QNetworkReply* reply = networkManager.post(request, requestData);
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    timer.start(m_config.requestTimeout);
    loop.exec();
    
    if (!timer.isActive()) {
        reply->abort();
        reply->deleteLater();
        m_lastError = TR("请求超时");
        return QJsonObject();
    }
    timer.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = reply->errorString();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray errorData = reply->readAll();
        
        LOG_ERROR("QwenClient", QString::fromUtf8("API Error: %1, Status: %2").arg(m_lastError).arg(statusCode));
        LOG_ERROR("QwenClient", QString::fromUtf8("Error Response (first 200 chars): %1")
            .arg(QString::fromUtf8(errorData.left(200))));
        
        if (statusCode == 429) {
            m_lastError = TR("速率限制，请稍后重试");
        } else if (statusCode == 400) {
            m_lastError = parseBadRequestError(errorData);
        }
        reply->deleteLater();
        return QJsonObject();
    }
    
    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = TR("JSON 解析失败: ") + parseError.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

QString QwenClient::parseBadRequestError(const QByteArray& errorData)
{
    QJsonDocument errorDoc = QJsonDocument::fromJson(errorData);
    if (errorDoc.isNull() || !errorDoc.object().contains("error")) {
        return TR("请求失败");
    }
    
    QJsonObject errorObj = errorDoc.object()["error"].toObject();
    QString errorMsg = errorObj["message"].toString();
    QString errorType = errorObj["type"].toString();
    QString errorCode = errorObj["code"].toString();
    
    if (errorMsg.length() > 200) {
        errorMsg = errorMsg.left(200) + "...";
    }
    
    LOG_ERROR("QwenClient", QString::fromUtf8("400 Error - type: %1, code: %2").arg(errorType).arg(errorCode));
    return QString::fromUtf8("Bad Request: %1 (type: %2, code: %3)").arg(errorMsg).arg(errorType).arg(errorCode);
}

QJsonObject QwenClient::buildJsonSchemaPayload(
    const QString& systemPrompt,
    const QString& userMessage,
    const QJsonObject& schema,
    bool strictMode,
    const QString& schemaName)
{
    Q_UNUSED(schemaName);
    Q_UNUSED(strictMode);
    
    QJsonObject payload;
    payload["model"] = m_config.model;
    payload["messages"] = buildMessages(systemPrompt, userMessage);
    
    bool supportsJsonSchema = (m_config.model.contains("plus") || m_config.model.contains("max"));
    
    if (supportsJsonSchema) {
        QJsonObject responseFormat;
        responseFormat["type"] = "json_schema";
        QJsonObject jsonSchemaObj;
        jsonSchemaObj["name"] = schemaName;
        jsonSchemaObj["strict"] = strictMode;
        jsonSchemaObj["schema"] = schema;
        responseFormat["json_schema"] = jsonSchemaObj;
        payload["response_format"] = responseFormat;
    }
    
    payload["temperature"] = 0.3;
    
    int maxTokens = 16384;
    if (m_config.model.contains("plus")) {
        maxTokens = 32768;
    } else if (m_config.model.contains("max")) {
        maxTokens = 32768;
    }
    payload["max_tokens"] = maxTokens;
    
    return payload;
}

QJsonObject QwenClient::buildSimplePayload(
    const QString& systemPrompt,
    const QString& userMessage,
    double temperature,
    int maxTokens)
{
    QJsonObject payload;
    payload["model"] = m_config.model;
    payload["messages"] = buildMessages(systemPrompt, userMessage);
    payload["temperature"] = temperature;
    payload["max_tokens"] = maxTokens;
    return payload;
}

QJsonObject QwenClient::extractApiResponse(const QJsonObject& response)
{
    QJsonObject result;
    
    if (response.contains("choices") && !response["choices"].toArray().isEmpty()) {
        QJsonObject choice = response["choices"].toArray().first().toObject();
        QJsonObject message = choice["message"].toObject();
        result["content"] = message["content"].toString();
        result["finishReason"] = choice["finish_reason"].toString();
    }
    
    result["usage"] = response["usage"].toObject();
    return result;
}

// ==================== JSON 处理 ====================

QString QwenClient::getStringField(const QJsonObject& obj, const QString& key, const QString& defaultValue)
{
    return obj.contains(key) ? obj[key].toString() : defaultValue;
}

QString QwenClient::getStringFieldAny(const QJsonObject& obj, const QStringList& keys, const QString& defaultValue)
{
    for (const QString& key : keys) {
        if (obj.contains(key) && !obj[key].toString().isEmpty()) {
            return obj[key].toString();
        }
    }
    return defaultValue;
}

QJsonObject QwenClient::getObjectField(const QJsonObject& obj, const QString& key, const QJsonObject& defaultValue)
{
    return obj.contains(key) && obj[key].isObject() ? obj[key].toObject() : defaultValue;
}

QJsonArray QwenClient::getArrayField(const QJsonObject& obj, const QString& key, const QJsonArray& defaultValue)
{
    return obj.contains(key) && obj[key].isArray() ? obj[key].toArray() : defaultValue;
}

int QwenClient::getIntField(const QJsonObject& obj, const QString& key, int defaultValue)
{
    return obj.contains(key) ? obj[key].toInt() : defaultValue;
}

void QwenClient::setPanelPageAndIndex(QJsonObject& panel, int index)
{
    panel["page"] = (index / PANELS_PER_PAGE) + 1;
    panel["index"] = index % PANELS_PER_PAGE;
}

void QwenClient::setPanelShotInfo(QJsonObject& panel, const QJsonObject& source)
{
    panel["shotType"] = getStringFieldAny(source, {"shotType", "shot_type"}, "medium");
    panel["cameraAngle"] = getStringFieldAny(source, {"cameraAngle", "camera_angle", "camera_movement"}, "eye-level");
}

QJsonObject QwenClient::parseJsonWithRepair(const QString& rawContent)
{
    QString content = rawContent;
    
    if (content.isEmpty()) {
        m_lastError = TR("JSON 解析失败: 内容为空");
        LOG_ERROR("QwenClient", "parseJsonWithRepair: content is empty");
        throw std::runtime_error(m_lastError.toStdString());
    }
    
    content.replace("\u201c", "\"").replace("\u201d", "\"");
    content.replace("\u2018", "'").replace("\u2019", "'");
    
    content = stripMarkdownCodeBlock(content);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError) {
        return doc.object();
    }
    
    LOG_WARNING("QwenClient", QString("parseJsonWithRepair: direct parse failed: %1 at offset %2")
        .arg(parseError.errorString()).arg(parseError.offset));
    
    JsonRepair::RepairResult result = JsonRepair::repair(content);
    
    if (result.success && result.value.isObject()) {
        return result.value.toObject();
    }
    
    QString fixedContent = tryFixMissingColon(content, parseError.offset);
    if (!fixedContent.isEmpty()) {
        doc = QJsonDocument::fromJson(fixedContent.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            return doc.object();
        }
    }
    
    QString repairedJson = tryRepairTruncatedJson(content);
    if (!repairedJson.isEmpty()) {
        doc = QJsonDocument::fromJson(repairedJson.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            return doc.object();
        }
    }
    
    int lastCompleteBrace = findLastCompleteBrace(content);
    if (lastCompleteBrace > 0) {
        QString partialJson = content.left(lastCompleteBrace + 1);
        doc = QJsonDocument::fromJson(partialJson.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            return doc.object();
        }
    }
    
    QJsonArray panelsArray = extractCompleteArray(content, "panels", parseError.offset, "panels");
    if (!panelsArray.isEmpty()) {
        QJsonObject result;
        result["panels"] = panelsArray;
        result["characters"] = QJsonArray();
        result["scenes"] = QJsonArray();
        result["totalPages"] = (panelsArray.size() + 3) / 4;
        return result;
    }
    
    m_lastError = TR("JSON 解析失败: ") + parseError.errorString();
    LOG_ERROR("QwenClient", QString("parseJsonWithRepair: all repair attempts failed"));
    return QJsonObject();
}

QString QwenClient::tryFixMissingColon(const QString& content, int errorOffset)
{
    QString fixed = content;
    
    int pos = errorOffset;
    while (pos > 0 && pos < fixed.length()) {
        QChar c = fixed[pos];
        
        if (c == '"' || c == '\'') {
            int quoteStart = pos;
            pos--;
            while (pos > 0 && fixed[pos] != '"' && fixed[pos] != '\'') {
                pos--;
            }
            if (pos > 0) {
                QString between = fixed.mid(pos + 1, quoteStart - pos - 1).trimmed();
                if (!between.contains(':') && !between.isEmpty()) {
                    QString key = fixed.mid(pos, quoteStart - pos + 1);
                    QString value = fixed.mid(quoteStart, fixed.indexOf(',', quoteStart) - quoteStart);
                    if (!key.isEmpty() && !value.isEmpty()) {
                        QString before = fixed.left(pos);
                        QString after = fixed.mid(quoteStart);
                        fixed = before + key + ":" + after;
                        return fixed;
                    }
                }
            }
            break;
        }
        pos--;
    }
    
    return QString();
}

int QwenClient::findLastCompleteBrace(const QString& json)
{
    int braceCount = 0;
    bool inString = false;
    bool escape = false;
    int lastCompleteBrace = -1;
    
    for (int i = 0; i < json.length(); ++i) {
        QChar c = json[i];
        
        if (escape) { escape = false; continue; }
        if (c == '\\' && inString) { escape = true; continue; }
        if (c == '"') { inString = !inString; continue; }
        if (inString) continue;
        
        if (c == '{') braceCount++;
        else if (c == '}') {
            braceCount--;
            if (braceCount == 0) {
                lastCompleteBrace = i;
            }
        }
    }
    
    return lastCompleteBrace;
}

QString QwenClient::repairJsonContent(const QString& content)
{
    QString repaired = content;
    
    // 移除尾部逗号 (JSON 不允许 ,] 或 ,})
    repaired.replace(QRegularExpression(",\\s*\\]"), "]");
    repaired.replace(QRegularExpression(",\\s*\\}"), "}");
    
    // 移除单行注释
    repaired.remove(QRegularExpression("//[^\n]*"));
    
    // 移除多行注释
    repaired.remove(QRegularExpression("/\\*[\\s\\S]*?\\*/"));
    
    // 修复未转义的换行符在字符串中
    repaired.replace(QRegularExpression("\"([^\"]*)\n([^\"]*)\""), "\"\\1\\\\n\\2\"");
    
    // 修复缺失的引号（简单情况）
    // 例如: {name: "value"} -> {"name": "value"}
    repaired.replace(QRegularExpression("\\{(\\s*)([a-zA-Z_][a-zA-Z0-9_]*)\\s*:"), "{\\1\"\\2\":");
    repaired.replace(QRegularExpression(",\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*:"), ", \"\\1\":");
    
    // 修复单引号字符串为双引号
    // 例如: {'key': 'value'} -> {"key": "value"}
    repaired.replace(QRegularExpression("'([^']*)'"), "\"\\1\"");
    
    // 移除 BOM 和不可见字符
    repaired.remove(QRegularExpression("[\\x00-\\x1F\\x7F]"));
    
    // 修剪空白
    repaired = repaired.trimmed();
    
    return repaired;
}

QString QwenClient::tryRepairTruncatedJson(const QString& content)
{
    QString trimmed = content.trimmed();
    
    if (trimmed.isEmpty() || !trimmed.startsWith("{")) {
        return QString();
    }
    
    QJsonParseError parseError;
    QJsonDocument::fromJson(trimmed.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError) {
        return trimmed;
    }
    
    QString errorMsg = parseError.errorString();
    // 处理多种 JSON 错误类型
    bool isTruncationError = errorMsg.contains("unterminated") || 
                              errorMsg.contains("unexpected end") ||
                              errorMsg.contains("missing value separator") ||
                              errorMsg.contains("missing colon") ||
                              errorMsg.contains("illegal value") ||
                              errorMsg.contains("unexpected token");
    
    if (!isTruncationError) {
        return QString();
    }
    
    log(QString("⚠️ JSON parse error at offset %1: %2").arg(parseError.offset).arg(errorMsg));
    
    QJsonArray panels = extractCompleteArray(trimmed, "panels", parseError.offset, "panels");
    QJsonArray characters = extractCompleteArray(trimmed, "characters", parseError.offset, "characters");
    QJsonArray scenes = extractCompleteArray(trimmed, "scenes", parseError.offset, "scenes");
    
    if (panels.isEmpty() && characters.isEmpty() && scenes.isEmpty()) {
        return QString();
    }
    
    QJsonObject result;
    result["panels"] = panels;
    result["characters"] = characters;
    result["scenes"] = scenes;
    result["totalPages"] = (panels.size() + PANELS_PER_PAGE - 1) / PANELS_PER_PAGE;
    
    QString closedJson = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Indented));
    log(QString("🔧 Reconstructed JSON with %1 panels, %2 characters, %3 scenes")
        .arg(panels.size()).arg(characters.size()).arg(scenes.size()));
    
    QJsonParseError finalError;
    QJsonDocument::fromJson(closedJson.toUtf8(), &finalError);
    if (finalError.error == QJsonParseError::NoError) {
        log("✅ Truncated JSON repair succeeded");
        return closedJson;
    }
    
    return QString();
}

int QwenClient::findLastCompleteObject(const QString& json, int errorOffset)
{
    QString truncated = json.left(errorOffset);
    
    int panelsStart = truncated.indexOf("\"panels\"");
    if (panelsStart < 0) {
        return -1;
    }
    
    int arrayStart = truncated.indexOf("[", panelsStart);
    if (arrayStart < 0) {
        return -1;
    }
    
    int bracketCount = 0;
    int lastCompletePanelEnd = -1;
    bool inString = false;
    bool escape = false;
    int objectStart = -1;
    
    for (int i = arrayStart + 1; i < truncated.length(); ++i) {
        QChar c = truncated[i];
        
        if (escape) {
            escape = false;
            continue;
        }
        
        if (c == '\\' && inString) {
            escape = true;
            continue;
        }
        
        if (c == '"') {
            inString = !inString;
            continue;
        }
        
        if (inString) continue;
        
        if (c == '{') {
            if (objectStart < 0) objectStart = i;
            bracketCount++;
        } else if (c == '}') {
            bracketCount--;
            if (bracketCount == 0) {
                lastCompletePanelEnd = i;
            }
        }
    }
    
    return lastCompletePanelEnd;
}

QString QwenClient::closeOpenBrackets(const QString& json)
{
    QString result = json;
    int openBraces = result.count('{') - result.count('}');
    int openBrackets = result.count('[') - result.count(']');
    
    for (int i = 0; i < openBrackets; ++i) {
        result += ']';
    }
    for (int i = 0; i < openBraces; ++i) {
        result += '}';
    }
    
    return result;
}

QJsonArray QwenClient::extractCompleteArray(const QString& json, const QString& arrayKey, int errorOffset, const QString& logName)
{
    int arrayStartPos = json.indexOf(QString("\"%1\"").arg(arrayKey));
    if (arrayStartPos < 0) {
        return QJsonArray();
    }
    
    // 如果 errorOffset 超出范围，使用 json 长度
    int maxPos = qMin(errorOffset, json.length());
    
    int arrayStart = json.indexOf("[", arrayStartPos);
    if (arrayStart < 0 || arrayStart >= maxPos) {
        return QJsonArray();
    }
    
    int bracketCount = 0;
    int lastCompleteEnd = -1;
    bool inString = false;
    bool escape = false;
    
    // 遍历整个 JSON，而不仅仅是到 errorOffset
    for (int i = arrayStart + 1; i < json.length(); ++i) {
        QChar c = json[i];
        
        if (escape) { escape = false; continue; }
        if (c == '\\' && inString) { escape = true; continue; }
        if (c == '"') { inString = !inString; continue; }
        if (inString) continue;
        
        if (c == '{') bracketCount++;
        else if (c == '}') {
            bracketCount--;
            if (bracketCount == 0) {
                lastCompleteEnd = i;
            }
        }
        else if (c == ']' && bracketCount == 0) {
            // 找到了数组的结束
            break;
        }
    }
    
    if (lastCompleteEnd < 0) {
        return QJsonArray();
    }
    
    QString arrayStr = json.mid(arrayStart, lastCompleteEnd - arrayStart + 1) + "]";
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(arrayStr.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError) {
        QJsonArray result = doc.array();
        log(QString("✅ Extracted %1 complete %2").arg(result.size()).arg(logName));
        return result;
    }
    
    // 如果解析失败，尝试只提取到 lastCompleteEnd
    arrayStr = json.mid(arrayStart, lastCompleteEnd - arrayStart + 1);
    doc = QJsonDocument::fromJson(arrayStr.toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError) {
        QJsonArray result = doc.array();
        log(QString("✅ Extracted %1 complete %2 (fallback)").arg(result.size()).arg(logName));
        return result;
    }
    
    return QJsonArray();
}

QString QwenClient::stripMarkdownCodeBlock(const QString& content)
{
    QRegularExpression codeBlockRegex("```json\\s*([\\s\\S]*?)\\s*```");
    QRegularExpressionMatch match = codeBlockRegex.match(content);
    return match.hasMatch() ? match.captured(1) : content;
}

QJsonObject QwenClient::buildBackground(const QJsonObject& panel)
{
    QJsonObject background;
    
    if (panel.contains("background") && panel["background"].isObject()) {
        background = panel["background"].toObject();
    } else {
        QString setting = getStringFieldAny(panel, {"setting", "location"});
        if (!setting.isEmpty()) {
            background["setting"] = setting;
        }
        QString lighting = getStringField(panel, "lighting");
        if (!lighting.isEmpty()) {
            background["lighting"] = lighting;
        }
        background["timeOfDay"] = "afternoon";
    }
    
    return background;
}

QJsonObject QwenClient::buildAtmosphere(const QJsonObject& panel)
{
    QJsonObject atmosphere;
    
    if (panel.contains("atmosphere") && panel["atmosphere"].isObject()) {
        atmosphere = panel["atmosphere"].toObject();
    } else {
        atmosphere["mood"] = getStringField(panel, "mood", "neutral");
        
        QString soundStr = getStringFieldAny(panel, {"sound_effect", "sound_effects"});
        if (!soundStr.isEmpty()) {
            QJsonArray soundEffects;
            soundEffects.append(QJsonObject{{"sound", soundStr}, {"style", "background"}});
            atmosphere["soundEffects"] = soundEffects;
        }
    }
    
    return atmosphere;
}

QJsonArray QwenClient::normalizeCharacters(const QJsonArray& chars, QMap<QString, QJsonObject>& extractedChars)
{
    QJsonArray normalizedChars;
    
    for (const QJsonValue& charVal : chars) {
        QJsonObject charObj;
        QString name;
        
        if (charVal.isString()) {
            // 字符串格式：提取角色名
            QString charStr = charVal.toString();
            name = charStr.split("（").first().split("(").first().trimmed();
            if (!name.isEmpty()) {
                charObj["name"] = name;
            }
        } else if (charVal.isObject()) {
            charObj = charVal.toObject();
            name = charObj["name"].toString();
        }
        
        if (!name.isEmpty()) {
            normalizedChars.append(charObj);
            
            // 添加到提取的角色列表
            if (!extractedChars.contains(name)) {
                extractedChars[name] = QJsonObject{{"name", name}, {"role", "character"}};
            }
        }
    }
    
    return normalizedChars;
}

QJsonObject QwenClient::normalizePanel(const QJsonObject& panel, int index, 
                                        QMap<QString, QJsonObject>& extractedChars,
                                        QMap<QString, QJsonObject>& extractedScenes, 
                                        int& sceneCounter)
{
    QJsonObject normalized;
    
    setPanelPageAndIndex(normalized, index);
    
    normalized["scene"] = getStringFieldAny(panel, {"scene", "description"});
    normalized["background"] = buildBackground(panel);
    normalized["atmosphere"] = buildAtmosphere(panel);
    
    setPanelShotInfo(normalized, panel);
    
    QJsonArray chars = getArrayField(panel, "characters");
    normalized["characters"] = chars.isEmpty() ? QJsonArray() : normalizeCharacters(chars, extractedChars);
    
    normalized["dialogue"] = getArrayField(panel, "dialogue");
    normalized["visualPrompt"] = getStringFieldAny(panel, {"visualPrompt", "visual_prompt", "imagePrompt", "image_prompt"});
    normalized["visualPromptEn"] = getStringFieldAny(panel, {"visualPromptEn", "visual_prompt_en", "imagePromptEn", "image_prompt_en"});
    
    extractSceneInfo(normalized, extractedScenes, sceneCounter);
    
    return normalized;
}

void QwenClient::extractSceneInfo(const QJsonObject& panel, QMap<QString, QJsonObject>& extractedScenes, int& sceneCounter)
{
    QJsonObject background = panel["background"].toObject();
    QString sceneId = background["sceneId"].toString();
    QString setting = background["setting"].toString();
    
    if (sceneId.isEmpty() && !setting.isEmpty()) {
        sceneId = QString("scene_%1").arg(sceneCounter++);
    }
    
    QString sceneName = setting.isEmpty() ? sceneId : setting;
    
    if (!sceneId.isEmpty() && !extractedScenes.contains(sceneId)) {
        QString sceneDesc = panel["scene"].toString();
        
        static QRegularExpression charNamePattern(QString::fromUtf8(
            "(李|张|王|陈|刘|赵|黄|周|吴|郑|孙|朱|马|林|何|高|罗|梁|谢|韩|唐|冯|于|丁|曹|许|邓|崔|贾|沈|傅|姜|范|方|石|白|谭|邹|龚|文|熊|田|史|陆|龙|叶|齐|贺|尹|潘|董|袁|夏|奚|钱)[\\u4e00-\\u9fa5]{1,2}"
        ));
        
        QStringList filteredSentences;
        QStringList sentences = sceneDesc.split(QRegularExpression(QString::fromUtf8("[。,;!?]")));
        for (const QString& sentence : sentences) {
            QString trimmed = sentence.trimmed();
            if (trimmed.isEmpty()) continue;
            
            if (charNamePattern.match(trimmed).hasMatch()) {
                continue;
            }
            filteredSentences.append(trimmed);
        }
        
        QString cleanedDesc = filteredSentences.join("。");
        if (cleanedDesc.isEmpty()) {
            cleanedDesc = sceneDesc;
        }
        
        extractedScenes[sceneId] = QJsonObject{
            {"id", sceneId},
            {"name", sceneName},
            {"type", "unknown"},
            {"description", cleanedDesc}
        };
    }
}

QJsonObject QwenClient::convertSceneToPanel(const QJsonObject& sceneObj, int index)
{
    QJsonObject panel;
    setPanelPageAndIndex(panel, index);
    
    QString visualDesc = getStringFieldAny(sceneObj, {"visual_description", "setting", "description"});
    panel["scene"] = visualDesc;
    panel["background"] = buildPanelBackground(sceneObj);
    panel["atmosphere"] = buildPanelAtmosphere(sceneObj);
    panel["shotType"] = getStringFieldAny(sceneObj, {"shotType", "shot_type"}, "medium");
    panel["cameraAngle"] = getStringFieldAny(sceneObj, {"cameraAngle", "camera_movement", "camera_angle"}, "eye-level");
    panel["characters"] = extractCharactersFromScene(sceneObj, visualDesc);
    panel["dialogue"] = getArrayField(sceneObj, "dialogue");
    panel["visualPrompt"] = getStringFieldAny(sceneObj, {"visualPrompt", "visual_prompt", "imagePrompt", "image_prompt", "visual_description"}, "");
    panel["visualPromptEn"] = getStringFieldAny(sceneObj, {"visualPromptEn", "visual_prompt_en", "imagePromptEn", "image_prompt_en"}, "");
    
    return panel;
}

QJsonObject QwenClient::buildPanelBackground(const QJsonObject& sceneObj)
{
    QJsonObject background;
    if (sceneObj.contains("setting")) {
        background["setting"] = sceneObj["setting"].toString();
    }
    if (sceneObj.contains("lighting")) {
        background["lighting"] = sceneObj["lighting"].toString();
    }
    background["timeOfDay"] = "afternoon";
    return background;
}

QJsonObject QwenClient::buildPanelAtmosphere(const QJsonObject& sceneObj)
{
    QJsonObject atmosphere;
    atmosphere["mood"] = getStringFieldAny(sceneObj, {"mood", "atmosphere"}, "neutral");
    
    if (sceneObj.contains("sound_design")) {
        QJsonArray soundEffects;
        soundEffects.append(QJsonObject{
            {"sound", sceneObj["sound_design"].toString()},
            {"style", "background"}
        });
        atmosphere["soundEffects"] = soundEffects;
    }
    return atmosphere;
}

QJsonArray QwenClient::extractCharactersFromScene(const QJsonObject& sceneObj, const QString& visualDesc)
{
    QJsonArray characters;
    
    if (sceneObj.contains("characters") && sceneObj["characters"].isArray()) {
        QJsonArray sceneChars = sceneObj["characters"].toArray();
        for (const QJsonValue& charVal : sceneChars) {
            if (charVal.isString()) {
                characters.append(QJsonObject{{"name", charVal.toString()}});
            } else if (charVal.isObject()) {
                characters.append(charVal.toObject());
            }
        }
    }
    
    if (characters.isEmpty() && !visualDesc.isEmpty()) {
        characters = extractCharactersFromText(visualDesc);
    }
    
    return characters;
}

QJsonArray QwenClient::extractCharactersFromText(const QString& text)
{
    QJsonArray characters;
    QRegularExpression charPattern(QString::fromUtf8("([\\u4e00-\\u9fa5]{2,4})(?:（[^）]+）)?(?:，|. |背着|穿着|蹲在|站在|走|坐|躺)"));
    
    if (!charPattern.isValid()) {
        return characters;
    }
    
    QRegularExpressionMatchIterator it = charPattern.globalMatch(text);
    QSet<QString> foundNames;
    QStringList excludeWords = {"夕阳", "街道", "石板", "灯笼", "山峦", "阳光", "热气", "香气", "书摊"};
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString name = match.captured(1);
        
        if (!excludeWords.contains(name) && !foundNames.contains(name)) {
            foundNames.insert(name);
            characters.append(QJsonObject{{"name", name}});
        }
    }
    
    return characters;
}

QJsonObject QwenClient::ensureStoryboardShape(const QJsonObject& storyboard)
{
    QJsonObject result = storyboard;
    
    LOG_DEBUG("QwenClient", QString::fromUtf8("ensureStoryboardShape 输入: panels=%1, characters=%2, scenes=%3")
        .arg(result.contains("panels") ? result["panels"].toArray().size() : -1)
        .arg(result.contains("characters") ? result["characters"].toArray().size() : -1)
        .arg(result.contains("scenes") ? result["scenes"].toArray().size() : -1));
    
    result = convertScenesToPanelsIfNeeded(result);
    
    QJsonArray panels = result["panels"].toArray();
    QJsonArray normalizedPanels;
    QMap<QString, QJsonObject> extractedChars;
    QMap<QString, QJsonObject> extractedScenes;
    int sceneCounter = 0;
    
    for (int i = 0; i < panels.size(); ++i) {
        QJsonObject panel = panels[i].toObject();
        normalizedPanels.append(normalizePanel(panel, i, extractedChars, extractedScenes, sceneCounter));
    }
    result["panels"] = normalizedPanels;
    
    LOG_DEBUG("QwenClient", QString::fromUtf8("从 panels 提取的角色数: %1, 场景数: %2")
        .arg(extractedChars.size()).arg(extractedScenes.size()));
    
    QJsonArray characters = result.contains("characters") ? result["characters"].toArray() : QJsonArray();
    QJsonArray scenes = result.contains("scenes") ? result["scenes"].toArray() : QJsonArray();
    
    QSet<QString> seenCharNames = extractKeysFromArray(characters, "name");
    QSet<QString> seenSceneIds = extractKeysFromArray(scenes, "id");
    
    QJsonArray extractedCharsArray;
    for (const QJsonObject& c : extractedChars.values()) {
        extractedCharsArray.append(c);
    }
    characters = mergeJsonArrays(characters, extractedCharsArray, "name", seenCharNames);
    
    QJsonArray extractedScenesArray;
    for (const QJsonObject& s : extractedScenes.values()) {
        extractedScenesArray.append(s);
    }
    scenes = mergeJsonArrays(scenes, extractedScenesArray, "id", seenSceneIds);
    
    result["characters"] = characters;
    result["scenes"] = scenes;
    
    LOG_DEBUG("QwenClient", QString::fromUtf8("ensureStoryboardShape 输出: panels=%1, characters=%2, scenes=%3")
        .arg(normalizedPanels.size()).arg(characters.size()).arg(scenes.size()));
    
    if (!result.contains("totalPages") || !result["totalPages"].isDouble()) {
        result["totalPages"] = (normalizedPanels.size() + PANELS_PER_PAGE - 1) / PANELS_PER_PAGE;
    }
    
    return result;
}

QJsonObject QwenClient::convertScenesToPanelsIfNeeded(const QJsonObject& storyboard)
{
    QJsonObject result = storyboard;
    
    if (result.contains("panels") && result["panels"].isArray()) {
        return result;
    }
    
    if (!result.contains("scenes") || !result["scenes"].isArray()) {
        result["panels"] = QJsonArray();
        return result;
    }
    
    QJsonArray scenes = result["scenes"].toArray();
    QJsonArray panels;
    for (int i = 0; i < scenes.size(); ++i) {
        panels.append(convertSceneToPanel(scenes[i].toObject(), i));
    }
    result["panels"] = panels;
    
    return result;
}

QSet<QString> QwenClient::extractKeysFromArray(const QJsonArray& arr, const QString& keyField)
{
    QSet<QString> keys;
    for (const QJsonValue& val : arr) {
        QString key = val.toObject()[keyField].toString();
        if (!key.isEmpty()) {
            keys.insert(key);
        }
    }
    return keys;
}

QJsonArray QwenClient::mergeJsonArrays(const QJsonArray& existing, const QJsonArray& extracted, 
                                        const QString& keyField, QSet<QString>& seenKeys)
{
    QJsonArray result = existing;
    
    for (const QJsonValue& val : extracted) {
        QString key = val.toObject()[keyField].toString();
        if (!key.isEmpty() && !seenKeys.contains(key)) {
            result.append(val);
            seenKeys.insert(key);
        }
    }
    
    return result;
}

// ==================== Prompt 构建 ====================

QJsonObject QwenClient::buildExamplePanel()
{
    QJsonObject panel;
    panel["page"] = 1;
    panel["index"] = 0;
    panel["scene"] = QString::fromUtf8("夕阳西下，金色余晖洒在小镇石板路上。李明背着书包独自走在回家路上，街道两旁是低矮砖房。");
    
    QJsonObject background;
    background["sceneId"] = QString::fromUtf8("ancient_town_main_street");
    background["setting"] = QString::fromUtf8("古镇石板街道");
    background["timeOfDay"] = QString::fromUtf8("dusk");
    background["weather"] = QString::fromUtf8("clear");
    background["lighting"] = QString::fromUtf8("natural");
    background["details"] = QJsonArray{QString::fromUtf8("远处山峦"), QString::fromUtf8("街边灯笼"), QString::fromUtf8("砖墙上的爬山虎")};
    panel["background"] = background;
    
    QJsonObject atmosphere;
    atmosphere["mood"] = QString::fromUtf8("peaceful");
    atmosphere["soundEffects"] = QJsonArray{QJsonObject{{"sound", QString::fromUtf8("脚步声")}, {"style", QString::fromUtf8("subtle")}}};
    atmosphere["particleEffects"] = QJsonArray{QString::fromUtf8("光尘飘浮"), QString::fromUtf8("微风吹动树叶")};
    panel["atmosphere"] = atmosphere;
    
    panel["shotType"] = QString::fromUtf8("wide");
    panel["cameraAngle"] = QString::fromUtf8("eye-level");
    
    QJsonObject composition;
    composition["focusPoint"] = QString::fromUtf8("李明的侧影");
    composition["depthOfField"] = QString::fromUtf8("deep");
    composition["rule"] = QString::fromUtf8("rule-of-thirds");
    panel["composition"] = composition;
    
    QJsonObject artStyle;
    artStyle["genre"] = QString::fromUtf8("seinen");
    artStyle["lineWeight"] = QString::fromUtf8("medium");
    artStyle["shading"] = QString::fromUtf8("screentone");
    artStyle["colorPalette"] = QString::fromUtf8("warm sunset tones with golden oranges");
    panel["artStyle"] = artStyle;
    
    panel["characters"] = QJsonArray{QJsonObject{
        {"name", QString::fromUtf8("李明")},
        {"pose", QString::fromUtf8("缓慢行走，微微低头")},
        {"expression", QString::fromUtf8("neutral")},
        {"position", QString::fromUtf8("midground")}
    }};
    panel["dialogue"] = QJsonArray();
    panel["visualPrompt"] = QString::fromUtf8("夕阳下的古镇街道，金色阳光洒在青石板路上，穿着校服的少年背着书包独自走在街上，两旁是低矮的砖房，远处是连绵的山峦，温暖宁静的氛围，少年漫画风格，网点纸阴影");
    panel["visualPromptEn"] = QString::fromUtf8("A quiet small town street at sunset, golden light on cobblestone pavement, teenage boy in school uniform walking alone with backpack, low brick houses on both sides, distant mountains, warm peaceful atmosphere, seinen manga style, screentone shading");
    panel["narrativeFunction"] = QString::fromUtf8("establishing-shot");
    
    return panel;
}

QJsonObject QwenClient::buildExampleCharacter()
{
    QJsonObject character;
    character["name"] = QString::fromUtf8("李明");
    character["role"] = QString::fromUtf8("protagonist");
    
    QJsonObject appearance;
    appearance["gender"] = QString::fromUtf8("male");
    appearance["age"] = 16;
    appearance["hairColor"] = QString::fromUtf8("black");
    appearance["hairStyle"] = QString::fromUtf8("short");
    appearance["eyeColor"] = QString::fromUtf8("brown");
    appearance["height"] = QString::fromUtf8("average");
    appearance["build"] = QString::fromUtf8("slim");
    appearance["clothing"] = QJsonArray{QString::fromUtf8("校服"), QString::fromUtf8("书包")};
    appearance["distinctiveFeatures"] = QJsonArray{QString::fromUtf8("圆框眼镜")};
    character["appearance"] = appearance;
    character["personality"] = QJsonArray{QString::fromUtf8("内向"), QString::fromUtf8("善良"), QString::fromUtf8("细心")};
    
    return character;
}

QJsonObject QwenClient::buildExampleScene()
{
    QJsonObject scene;
    scene["id"] = QString::fromUtf8("ancient_town_main_street");
    scene["name"] = QString::fromUtf8("古镇主街");
    scene["type"] = QString::fromUtf8("outdoor");
    scene["description"] = QString::fromUtf8("小镇中心的古老石板路，两旁是传统砖木结构低矮房屋，承载着小镇几代人的记忆。");
    
    QJsonObject visualChars;
    visualChars["architecture"] = QString::fromUtf8("传统砖木结构，青瓦屋顶，木质门窗，墙面有岁月痕迹");
    visualChars["keyLandmarks"] = QJsonArray{QString::fromUtf8("石拱桥"), QString::fromUtf8("百年老槐树"), QString::fromUtf8("李家茶馆")};
    visualChars["colorScheme"] = QString::fromUtf8("暖色调为主，砖红、木褐、青灰色");
    
    QJsonObject lighting;
    lighting["naturalLight"] = QString::fromUtf8("abundant");
    lighting["artificialLight"] = QString::fromUtf8("moderate");
    lighting["lightSources"] = QJsonArray{QString::fromUtf8("街灯"), QString::fromUtf8("店铺灯光"), QString::fromUtf8("窗户透出的光")};
    visualChars["lighting"] = lighting;
    
    visualChars["atmosphere"] = QString::fromUtf8("宁静中带着烟火气，时间在这里流淌得很慢");
    visualChars["soundscape"] = QJsonArray{QString::fromUtf8("脚步声"), QString::fromUtf8("风铃"), QString::fromUtf8("远处狗吠"), QString::fromUtf8("茶馆里的谈笑")};
    visualChars["textures"] = QJsonArray{QString::fromUtf8("粗糙石板路"), QString::fromUtf8("斑驳墙面"), QString::fromUtf8("光滑木门")};
    scene["visualCharacteristics"] = visualChars;
    
    QJsonObject spatialLayout;
    spatialLayout["size"] = QString::fromUtf8("medium");
    spatialLayout["layout"] = QString::fromUtf8("长约200米的街道，宽约6米，两侧各有店铺和民居");
    spatialLayout["keyAreas"] = QJsonArray{
        QJsonObject{{"name", QString::fromUtf8("石拱桥")}, {"position", QString::fromUtf8("街道中段")}},
        QJsonObject{{"name", QString::fromUtf8("老槐树")}, {"position", QString::fromUtf8("街道北端")}},
        QJsonObject{{"name", QString::fromUtf8("李家茶馆")}, {"position", QString::fromUtf8("街道南侧")}}
    };
    scene["spatialLayout"] = spatialLayout;
    
    scene["timeVariations"] = QJsonArray{
        QJsonObject{
            {"timeOfDay", QString::fromUtf8("morning")},
            {"description", QString::fromUtf8("晨光透过薄雾，石板路泛着湿润光泽，早起的居民开始活动")}
        },
        QJsonObject{
            {"timeOfDay", QString::fromUtf8("dusk")},
            {"description", QString::fromUtf8("夕阳将街道染成金色，街灯开始点亮，炊烟袅袅升起")}
        }
    };
    
    scene["weatherVariations"] = QJsonArray{
        QJsonObject{
            {"weather", QString::fromUtf8("rainy")},
            {"description", QString::fromUtf8("雨水在石板路上形成小水洼，屋檐滴水声清脆，空气中弥漫着泥土香")}
        }
    };
    
    scene["narrativeRole"] = QString::fromUtf8("primary-setting");
    
    QJsonObject firstAppearance;
    firstAppearance["chapter"] = 1;
    firstAppearance["page"] = 1;
    firstAppearance["panelIndex"] = 0;
    scene["firstAppearance"] = firstAppearance;
    
    scene["referenceImages"] = QJsonArray();
    
    return scene;
}

QJsonObject QwenClient::buildCustomExample()
{
    QJsonObject example;
    example["panels"] = QJsonArray{buildExamplePanel()};
    example["characters"] = QJsonArray{buildExampleCharacter()};
    example["scenes"] = QJsonArray{buildExampleScene()};
    example["totalPages"] = 1;
    return example;
}

QString QwenClient::buildSystemPrompt()
{
    QString schemaPath = "schemas/storyboard.json";
    QFile schemaFile(schemaPath);
    QJsonObject schema;
    
    if (schemaFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(schemaFile.readAll());
        schema = doc.object();
        schemaFile.close();
    }
    
    if (schema.isEmpty()) {
        return QString::fromUtf8("你是一个专业的漫画分镜师，擅长将小说文本转换为详细的视觉分镜脚本。请严格按照 JSON Schema 输出。");
    }
    
    SchemaToPrompt::Options options;
    options.customExample = buildCustomExample();
    
    return SchemaToPrompt::buildSystemPrompt(schema, options);
}

QString QwenClient::buildChangeRequestPrompt()
{
    return TR("你是一个漫画修改助手。\n\n"
        "你的任务是将用户的自然语言修改请求转换为结构化的 CR-DSL（Change Request Domain Specific Language）。\n\n"
        "CR-DSL 包含以下要素：\n"
        "- scope: 修改范围\n"
        "- targetId: 目标 ID（如果适用）\n"
        "- type: 修改类型\n"
        "- ops: 操作列表，每个操作包含 action 和 params\n\n"
        "可用的 action：\n"
        "- inpaint: 局部重绘（需要遮罩区域）\n"
        "- outpaint: 扩展画面\n"
        "- bg_swap: 替换背景\n"
        "- repose: 改变角色姿势\n"
        "- regen_panel: 重新生成整个面板\n"
        "- rewrite_dialogue: 重写对白\n"
        "- reorder: 重新排序\n\n"
        "请根据用户的自然语言请求，生成符合 Schema 的 CR-DSL JSON。");
}

QString QwenClient::buildDialogueRewritePrompt()
{
    return TR("你是一个专业的对白编辑。\n\n"
        "你的任务是根据用户的指示重写漫画对白，保持角色性格和语气，同时满足修改要求。\n\n"
        "要求：\n"
        "1. 保持对白简洁（漫画气泡空间有限）\n"
        "2. 符合角色性格\n"
        "3. 自然流畅\n"
        "4. 直接输出重写后的对白，不要添加引号或解释");
}

// ==================== 日志与辅助 ====================

void QwenClient::logRequest(const QString& model, const QString& text, 
                            const QString& systemPrompt, const QString& userMessage)
{
    log("\n📤 ===== QWEN API REQUEST =====");
    log(QString("🔧 Model: %1").arg(model));
    log(QString("📏 Text Length: %1 chars").arg(text.length()));
    log(QString("📏 System Prompt Length: %1 chars").arg(systemPrompt.length()));
    log(QString("📏 User Message Length: %1 chars").arg(userMessage.length()));
    log("\n[System Prompt first 200 chars, truncated for security]:");
    log(systemPrompt.left(200) + "...");
    log("\n[User Message first 200 chars, truncated for security]:");
    log(userMessage.left(200) + "...");
}

void QwenClient::logResponse(const QString& content, const QString& finishReason,
                             const QJsonObject& usage, qint64 elapsedMs)
{
    log("\n📥 ===== QWEN API RESPONSE =====");
    log(QString("⏱️ Elapsed: %1 ms").arg(elapsedMs));
    log(QString("🎯 Finish Reason: %1").arg(finishReason));
    log(QString("📊 Tokens: prompt=%1, completion=%2, total=%3")
        .arg(usage["prompt_tokens"].toInt())
        .arg(usage["completion_tokens"].toInt())
        .arg(usage["total_tokens"].toInt()));
    log(QString("📄 Content Length: %1 chars").arg(content.length()));
}

void QwenClient::exponentialBackoff(int attempt)
{
    int backoffMs = qPow(2, attempt) * 1000;
    log(QString("Waiting %1ms before retry...").arg(backoffMs));
    QThread::msleep(backoffMs);
}

void QwenClient::log(const QString& content)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logEntry = QString("[%1] %2").arg(timestamp, content);
    
    LOG_DEBUG("QwenClient", content);
    
    QFile logFile(m_logFilePath);
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&logFile);
        stream.setCodec("UTF-8");
        stream << logEntry << "\n";
        logFile.close();
    }
}

bool QwenClient::containsChinese(const QString& text)
{
    return text.contains(QRegularExpression("\\p{Han}"));
}

void QwenClient::generateStoryboardStream(
    const QString& text,
    const QJsonObject& jsonSchema,
    bool strictMode,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes,
    int chapterNumber)
{
    if (!checkInitialized("generateStoryboardStream")) {
        emit errorOccurred("generateStoryboardStream", m_lastError);
        return;
    }
    
    log("🚀 Starting storyboard generation (streaming)");
    log(QString("  Text length: %1").arg(text.length()));
    
    QString systemPrompt = buildSystemPrompt();
    QString userMessage = buildUserMessageWithBible(text, existingCharacters, existingScenes, chapterNumber);
    
    QJsonObject payload = buildJsonSchemaPayload(systemPrompt, userMessage, jsonSchema, strictMode, "storyboard");
    payload["stream"] = true;
    
    logRequest(m_config.model, text, systemPrompt, userMessage);
    
    sendApiRequestStream(payload);
}

void QwenClient::sendApiRequestStream(const QJsonObject& payload)
{
    if (m_streamInProgress) {
        LOG_WARNING("QwenClient", "Stream request already in progress, skipping");
        return;
    }
    m_streamInProgress = true;
    
    QNetworkAccessManager* networkManager = new QNetworkAccessManager(this);
    
    QNetworkRequest request = createApiRequest();
    request.setRawHeader("Accept", "text/event-stream");
    
    QByteArray requestData = QJsonDocument(payload).toJson();
    
    LOG_DEBUG("QwenClient", QString::fromUtf8("=== Streaming API Request ==="));
    LOG_DEBUG("QwenClient", QString::fromUtf8("URL: %1").arg(m_config.endpoint + "/chat/completions"));
    LOG_DEBUG("QwenClient", QString::fromUtf8("Payload size: %1 bytes").arg(requestData.size()));
    
    m_streamAccumulatedContent.clear();
    
    QNetworkReply* reply = networkManager->post(request, requestData);
    
    QTimer* timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    
    QPointer<QwenClient> self = this;
    QPointer<QNetworkReply> replyPtr = reply;
    
    connect(reply, &QNetworkReply::readyRead, this, [self, replyPtr]() {
        if (!self || !replyPtr) return;
        QByteArray data = replyPtr->readAll();
        QString content = self->parseSseContent(QString::fromUtf8(data));
        if (!content.isEmpty()) {
            self->m_streamAccumulatedContent += content;
            emit self->streamProgress(self->m_streamAccumulatedContent);
        }
    });
    
    connect(reply, &QNetworkReply::finished, this, [self, replyPtr, networkManager, timeoutTimer]() {
        if (self) {
            self->handleStreamFinished(replyPtr, networkManager, timeoutTimer);
        }
    });
    
    connect(timeoutTimer, &QTimer::timeout, this, [self, replyPtr, networkManager]() {
        if (self) {
            self->handleStreamTimeout(replyPtr, networkManager);
        }
    });
    timeoutTimer->start(m_config.requestTimeout);
}

QString QwenClient::parseSseContent(const QString& data)
{
    QString accumulated;
    QStringList lines = data.split("\n", Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        if (!line.startsWith("data: ")) {
            continue;
        }
        
        QString jsonStr = line.mid(6).trimmed();
        if (jsonStr == "[DONE]") {
            continue;
        }
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            continue;
        }
        
        QJsonObject response = doc.object();
        QJsonArray choices = response["choices"].toArray();
        if (choices.isEmpty()) {
            continue;
        }
        
        QJsonObject delta = choices[0].toObject()["delta"].toObject();
        if (delta.contains("content")) {
            accumulated += delta["content"].toString();
        }
    }
    
    return accumulated;
}

void QwenClient::handleStreamFinished(QNetworkReply* reply, QNetworkAccessManager* manager, QTimer* timer)
{
    if (timer) {
        timer->stop();
        timer->deleteLater();
    }
    m_streamInProgress = false;
    
    if (!reply || !manager) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = reply->errorString();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        LOG_ERROR("QwenClient", QString::fromUtf8("Stream API Error: %1, Status: %2").arg(m_lastError).arg(statusCode));
        emit errorOccurred("sendApiRequestStream", m_lastError);
    } else {
        LOG_INFO("QwenClient", QString::fromUtf8("Stream completed, total content length: %1").arg(m_streamAccumulatedContent.length()));
        
        QJsonObject parsedJson = parseJsonWithRepair(m_streamAccumulatedContent);
        
        if (!parsedJson.isEmpty()) {
            QJsonObject result = ensureStoryboardShape(parsedJson);
            emit streamCompleted(result);
            emit streamReceived(m_streamAccumulatedContent);
        } else {
            emit errorOccurred("sendApiRequestStream", m_lastError);
        }
    }
    
    reply->deleteLater();
    manager->deleteLater();
}

void QwenClient::handleStreamTimeout(QNetworkReply* reply, QNetworkAccessManager* manager)
{
    if (!reply || !manager) return;
    
    LOG_WARNING("QwenClient", "Stream request timeout");
    m_streamInProgress = false;
    m_lastError = TR("请求超时");
    emit errorOccurred("sendApiRequestStream", m_lastError);
    
    reply->abort();
}
