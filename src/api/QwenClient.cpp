#include "api/QwenClient.h"
#include "services/ServiceContainer.h"
#include "utils/SingletonUtils.h"
#include "api/QwenPromptBuilder.h"
#include "api/QwenJsonRepair.h"
#include "api/QwenStreamHandler.h"
#include "utils/Logger.h"
#include "utils/EncodingUtils.h"
#include "utils/BibleUtils.h"
#include "utils/SceneKeyUtils.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QFile>
#include <QTimer>
#include <QThread>
#include <QRegularExpression>
#include <QtMath>
#include <QHash>
#include <QSet>
#include <QPointer>
#include <QScopedPointer>

namespace {
using namespace SceneKeyUtils;

bool isActionHeavySceneToken(const QString& text)
{
    static const QStringList actionKeywords = {
        QString::fromUtf8("走"),
        QString::fromUtf8("跑"),
        QString::fromUtf8("哭"),
        QString::fromUtf8("喊"),
        QString::fromUtf8("叫"),
        QString::fromUtf8("听"),
        QString::fromUtf8("看"),
        QString::fromUtf8("说"),
        QString::fromUtf8("吞"),
        QString::fromUtf8("喝"),
        QString::fromUtf8("摔"),
        QString::fromUtf8("撞"),
        QString::fromUtf8("追"),
        QString::fromUtf8("惊"),
        QString::fromUtf8("紧绷"),
        QString::fromUtf8("迷路"),
        QString::fromUtf8("僵硬"),
        QString::fromUtf8("沉默"),
        QString::fromUtf8("回忆"),
        QString::fromUtf8("道别"),
        QString::fromUtf8("前行"),
        QString::fromUtf8("凝固"),
        QString::fromUtf8("惊骇"),
        QString::fromUtf8("震惊"),
        QString::fromUtf8("紧绷")
    };

    for (const QString& keyword : actionKeywords) {
        if (text.contains(keyword)) {
            return true;
        }
    }

    return false;
}

QString cleanSceneDescription(const QString& text)
{
    QString result = text.trimmed();
    result.replace(QRegularExpression(QStringLiteral(R"(\s{2,})")), QStringLiteral(" "));
    return result;
}

QString buildSceneMergeKey(const QJsonObject& scene);
QStringList buildSceneMergeKeys(const QJsonObject& scene);
QJsonObject mergeSceneObjects(const QJsonObject& existing, const QJsonObject& incoming);
}

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
        m_lastError = tr("API Key 不能为空");
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


bool QwenClient::checkInitialized(const QString& operation)
{
    if (m_initialized) {
        return true;
    }
    m_lastError = tr("客户端尚未初始化");
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
    log(QString("Storyboard generation started, text: %1 chars").arg(text.length()));
    
    QStringList chunks = splitTextIntelligently(text, m_config.maxChunkLength);
    log(QString("Split into %1 chunks").arg(chunks.size()));
    
    QList<QJsonObject> validResponses = processChunksParallel(
        chunks, jsonSchema, strictMode,
        existingCharacters, existingScenes, chapterNumber
    );
    
    if (validResponses.isEmpty()) {
        m_lastError = tr("分镜生成失败");
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
    log(QString("Storyboard completed: %1 panels, %2 chars, %3 scenes in %4ms")
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
    
    QJsonArray accumulatedCharacters = existingCharacters;
    QJsonArray accumulatedScenes = existingScenes;
    
    for (int i = 0; i < chunks.size(); ++i) {
        emit progressChanged(i + 1, totalChunks,
            tr("正在处理第 %1/%2 个文本块").arg(i + 1).arg(totalChunks));
        
        QJsonObject response = callStoryboardApi(
            chunks[i], jsonSchema, strictMode,
            accumulatedCharacters,
            accumulatedScenes,
            chapterNumber
        );
        
        if (!response.isEmpty()) {
            results.append(response);
            
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
            
            log(QString("Chunk %1/%2 succeeded").arg(i + 1).arg(totalChunks));
        } else {
            failedCount++;
            log(QString("Chunk %1/%2 failed").arg(i + 1).arg(totalChunks));
            m_lastError = tr("第 %1/%2 个文本块处理失败").arg(i + 1).arg(totalChunks);
        }
    }
    
    if (failedCount > 0) {
        m_lastError = tr("分镜生成失败，%1/%2 个文本块处理失败").arg(failedCount).arg(totalChunks);
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
    
    log(tr("解析变更请求"));
    
    QString contextInfo;
    if (!context.isEmpty()) {
        contextInfo = QString("\n上下文信息：\n%1")
            .arg(QString::fromUtf8(QJsonDocument(context).toJson(QJsonDocument::Indented)));
    }
    
    QString userPrompt = QString("自然语言修改需求：\n%1\n%2\n\n请输出符合 CR-DSL JSON 的内容。").arg(naturalLanguage, contextInfo);

    
    QJsonObject payload = buildSimplePayload(
        QwenPromptBuilder::buildChangeRequestPrompt(), userPrompt, 0.2, 2000);
    
    QJsonObject response = sendApiRequest(payload);
    if (response.isEmpty()) {
        return QJsonObject();
    }
    
    QJsonObject result = extractApiResponse(response);
    return QwenJsonRepair::parseWithRepair(result["content"].toString());
}

QString QwenClient::rewriteDialogue(const QString& originalDialogue, const QString& instruction)
{
    if (!checkInitialized("rewriteDialogue")) {
        return QString();
    }

    log(tr("重写对白"));

    QString userPrompt = QString("原对白：\n%1\n\n修改要求：\n%2\n\n请直接返回改写后的对白。")
        .arg(originalDialogue, instruction);

    QJsonObject payload = buildSimplePayload(
        QwenPromptBuilder::buildDialogueRewritePrompt(), userPrompt, 0.5, 200);

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

    log(tr("修复 JSON"));

    QString errorsStr = errors.join("\n");
    QString invalidJsonStr = QString::fromUtf8(QJsonDocument(invalidJson).toJson(QJsonDocument::Indented));

    QString userPrompt = QString("以下 JSON 存在错误，请根据错误信息修复并只返回修正后的 JSON：\n%1\n\n原始 JSON：\n%2")
        .arg(errorsStr)
        .arg(invalidJsonStr);

    QString systemPrompt = tr("你是一个 JSON 修复助手，只输出可解析的 JSON。");

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
    QRegularExpression sentenceRegex("([^。！？!?；;\\n]+[。！？!?；;]*)");
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

            if (character.contains("role") && existing["role"].toString().isEmpty()) {
                existing["role"] = character["role"];
            }

            if (character.contains("appearance")) {
                QJsonObject incomingAppearance = character["appearance"].toObject();
                QJsonObject existingAppearance = existing["appearance"].toObject();
                QJsonObject mergedAppearance = existingAppearance;

                for (auto it = incomingAppearance.begin(); it != incomingAppearance.end(); ++it) {
                    const QString key = it.key();
                    if (!mergedAppearance.contains(key) || mergedAppearance.value(key).isNull() || mergedAppearance.value(key).isUndefined()) {
                        mergedAppearance[key] = it.value();
                    }
                }

                existing["appearance"] = mergedAppearance;
            }

            if (character.contains("personality") && existing["personality"].toArray().isEmpty()) {
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
        const QStringList keys = buildSceneMergeKeys(scene);
        QString matchedKey;

        for (const QString& key : keys) {
            if (!key.isEmpty() && sceneMap.contains(key)) {
                matchedKey = key;
                break;
            }
        }

        if (matchedKey.isEmpty()) {
            for (auto it = sceneMap.constBegin(); it != sceneMap.constEnd(); ++it) {
                const QStringList existingKeys = buildSceneMergeKeys(it.value());
                for (const QString& key : keys) {
                    if (!key.isEmpty() && existingKeys.contains(key, Qt::CaseInsensitive)) {
                        matchedKey = it.key();
                        break;
                    }
                }
                if (!matchedKey.isEmpty()) {
                    break;
                }
            }
        }

        if (!matchedKey.isEmpty()) {
            sceneMap[matchedKey] = mergeSceneObjects(sceneMap[matchedKey], scene);
            continue;
        }

        const QString storeKey = buildSceneMergeKey(scene);
        if (!storeKey.isEmpty()) {
            sceneMap[storeKey] = scene;
        }
    }
}


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
    QString systemPrompt = QwenPromptBuilder::buildSystemPrompt();
    QString userMessage = QwenPromptBuilder::buildUserMessageWithBible(text, existingCharacters, existingScenes, chapterNumber);
    
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
            LOG_WARNING("QwenClient", QStringLiteral("模型输出长度达到上限"));
            }
            
            QJsonObject parsedJson = QwenJsonRepair::parseWithRepair(result["content"].toString());
            
            return ensureStoryboardShape(parsedJson);
            
        } catch (const std::exception& e) {
            lastErrorMsg = QString::fromUtf8(e.what());
            
            if (lastErrorMsg.contains("429") && attempt < m_config.maxRetries - 1) {
                log("Rate limited, retrying...");
                exponentialBackoff(attempt);
                continue;
            }
            
            m_lastError = lastErrorMsg;
            log(QString("API Error: %1").arg(lastErrorMsg));
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
        m_lastError = tr("请求超时");
        return QJsonObject();
    }
    timer.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = reply->errorString();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray errorData = reply->readAll();
        
        LOG_ERROR("QwenClient", QString("请求失败: %1 (HTTP %2)").arg(m_lastError).arg(statusCode));
        LOG_ERROR("QwenClient", QString("请求详情: %1").arg(QString::fromUtf8(errorData.left(200))));

        
        if (statusCode == 429) {
            m_lastError = tr("请求过于频繁，请稍后重试");
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
        m_lastError = tr("JSON 解析失败: ") + parseError.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

QString QwenClient::parseBadRequestError(const QByteArray& errorData)
{
    QJsonDocument errorDoc = QJsonDocument::fromJson(errorData);
    if (errorDoc.isNull() || !errorDoc.object().contains("error")) {
        return tr("请求格式错误");
    }
    
    QJsonObject errorObj = errorDoc.object()["error"].toObject();
    QString errorMsg = errorObj["message"].toString();
    QString errorType = errorObj["type"].toString();
    QString errorCode = errorObj["code"].toString();
    
    if (errorMsg.length() > 200) {
        errorMsg = errorMsg.left(200) + "...";
    }
    
    LOG_ERROR("QwenClient", QString("错误类型: %1, 错误码: %2").arg(errorType).arg(errorCode));
    return QString("请求格式错误: %1 (类型=%2, 代码=%3)").arg(errorMsg, errorType, errorCode);
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
            QString charStr = charVal.toString();
            name = charStr.split("(").first().split("[").first().trimmed();
            if (!name.isEmpty()) {
                charObj["name"] = name;
            }
        } else if (charVal.isObject()) {
            charObj = charVal.toObject();
            name = charObj["name"].toString();
        }

        if (!name.isEmpty()) {
            normalizedChars.append(charObj);
            if (!extractedChars.contains(name)) {
                extractedChars[name] = charObj;
            }
        }
    }

    return normalizedChars;
}

namespace {

QJsonObject mergeCharacterObjects(const QJsonObject& existing, const QJsonObject& incoming)
{
    QJsonObject merged = existing;

    if (merged["name"].toString().isEmpty() && !incoming["name"].toString().isEmpty()) {
        merged["name"] = incoming["name"];
    }

    if (incoming.contains("role") && merged["role"].toString().isEmpty()) {
        merged["role"] = incoming["role"];
    }

    if (incoming.contains("appearance")) {
        QJsonObject incomingAppearance = incoming["appearance"].toObject();
        QJsonObject existingAppearance = merged["appearance"].toObject();
        QJsonObject mergedAppearance = existingAppearance;

        for (auto it = incomingAppearance.begin(); it != incomingAppearance.end(); ++it) {
            const QString key = it.key();
            if (!mergedAppearance.contains(key) || mergedAppearance.value(key).isNull() || mergedAppearance.value(key).isUndefined()) {
                mergedAppearance[key] = it.value();
            }
        }

        merged["appearance"] = mergedAppearance;
    }

    if (incoming.contains("personality") && merged["personality"].toArray().isEmpty()) {
        merged["personality"] = incoming["personality"];
    }

    return merged;
}

QString buildSceneMergeKey(const QJsonObject& scene)
{
    const QStringList keys = SceneKeyUtils::buildSceneIdentityKeys(scene);
    return keys.isEmpty() ? QString() : keys.first();
}

QStringList buildSceneMergeKeys(const QJsonObject& scene)
{
    return SceneKeyUtils::buildSceneIdentityKeys(scene);
}

QJsonObject mergeSceneObjects(const QJsonObject& existing, const QJsonObject& incoming)
{
    QJsonObject merged = existing;

    if (merged["id"].toString().isEmpty() && !incoming["id"].toString().isEmpty()) {
        merged["id"] = incoming["id"];
    }
    if (merged["name"].toString().isEmpty() && !incoming["name"].toString().isEmpty()) {
        merged["name"] = incoming["name"];
    }
    if (merged["description"].toString().isEmpty() && !incoming["description"].toString().isEmpty()) {
        merged["description"] = incoming["description"];
    }
    if (merged["setting"].toString().isEmpty() && !incoming["setting"].toString().isEmpty()) {
        merged["setting"] = incoming["setting"];
    }

    QJsonObject mergedDetails = BibleUtils::mergeObjectPreserve(
        merged["details"].toObject(), incoming["details"].toObject());
    if (!mergedDetails.isEmpty()) {
        merged["details"] = mergedDetails;
    }

    if (incoming.contains("aliases")) {
        QStringList aliases = JsonUtils::jsonArrayToStringList(merged["aliases"].toArray());
        aliases = BibleUtils::mergeStringLists(aliases, JsonUtils::jsonArrayToStringList(incoming["aliases"].toArray()));
        if (!aliases.isEmpty()) {
            merged["aliases"] = QJsonArray::fromStringList(aliases);
        }
    }

    return merged;
}

}

QJsonObject QwenClient::normalizePanel(const QJsonObject& panel, int index, 
                                        QMap<QString, QJsonObject>& extractedChars,
                                        QMap<QString, QJsonObject>& extractedScenes, 
                                        int& sceneCounter,
                                        const QMap<QString, QString>& existingSceneLookup)
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
    
    extractSceneInfo(normalized, extractedScenes, sceneCounter, existingSceneLookup);
    
    return normalized;
}

void QwenClient::extractSceneInfo(QJsonObject& panel, QMap<QString, QJsonObject>& extractedScenes, int& sceneCounter, const QMap<QString, QString>& existingSceneLookup)
{
    Q_UNUSED(sceneCounter);
    QJsonObject background = panel["background"].toObject();
    QString sceneId = background["sceneId"].toString();
    QString setting = background["setting"].toString();
    QString sceneText = panel["scene"].toString();
    QString sceneName = normalizeSceneLabel(setting);
    if (sceneName.isEmpty()) {
        sceneName = normalizeSceneLabel(sceneText);
    }
    if (sceneName.isEmpty()) {
        sceneName = sceneText.trimmed();
    }
    
    if (!sceneId.isEmpty() && existingSceneLookup.contains(sceneId)) {
        return;
    }
    
    if (sceneId.isEmpty() && !sceneName.isEmpty()) {
        const QStringList candidates = {
            sceneName,
            normalizeSceneLabel(sceneName),
            sceneText.trimmed(),
            normalizeSceneLabel(sceneText)
        };
        const QString matchedId = SceneKeyUtils::resolveSceneLookupId(existingSceneLookup, candidates);
        if (!matchedId.isEmpty()) {
            background["sceneId"] = matchedId;
            panel["background"] = background;
            return;
        }
    }

    if (sceneId.isEmpty()) {
        sceneId = sceneName;
    }

    if (sceneName.isEmpty()) {
        sceneName = sceneId;
    }

    if (sceneId.isEmpty()) {
        return;
    }

    if (!extractedScenes.contains(sceneId)) {
        QString sceneDesc = cleanSceneDescription(sceneText);
        
        static QRegularExpression charNamePattern(
            QStringLiteral(R"(^\s*[\p{Han}A-Za-z0-9_·]{2,20}[：:])")
        );
        
        QStringList filteredSentences;
        QStringList sentences = sceneDesc.split(QRegularExpression(QStringLiteral(R"([。！？!?；;\n]+)")));
        for (const QString& sentence : sentences) {
            QString trimmed = sentence.trimmed();
            if (trimmed.isEmpty()) continue;
            
            if (charNamePattern.match(trimmed).hasMatch()) {
                continue;
            }
            if (isActionHeavySceneToken(trimmed)) {
                continue;
            }
            filteredSentences.append(trimmed);
        }
        
        QString cleanedDesc = filteredSentences.join(". ");
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
    QRegularExpression charPattern(QStringLiteral("([A-Za-z0-9_\\-]+)"));
    
    if (!charPattern.isValid()) {
        return characters;
    }
    
    QRegularExpressionMatchIterator it = charPattern.globalMatch(text);
    QSet<QString> foundNames;
    QStringList excludeWords = {"character", "scene", "panel", "dialogue", "background", "expression"};
    
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
    
    LOG_DEBUG("QwenClient", QString("Storyboard shape: panels=%1, characters=%2, scenes=%3")
        .arg(result.contains("panels") ? result["panels"].toArray().size() : -1)
        .arg(result.contains("characters") ? result["characters"].toArray().size() : -1)
        .arg(result.contains("scenes") ? result["scenes"].toArray().size() : -1));

    result = convertScenesToPanelsIfNeeded(result);
    
    QJsonArray existingScenesArr = result.contains("scenes") ? result["scenes"].toArray() : QJsonArray();
    QMap<QString, QString> existingSceneLookup = SceneKeyUtils::buildSceneLookupMap(existingScenesArr);
    
    QJsonArray panels = result["panels"].toArray();
    QJsonArray normalizedPanels;
    QMap<QString, QJsonObject> extractedChars;
    QMap<QString, QJsonObject> extractedScenes;
    int sceneCounter = 0;
    
    for (int i = 0; i < panels.size(); ++i) {
        QJsonObject panel = panels[i].toObject();
        normalizedPanels.append(normalizePanel(panel, i, extractedChars, extractedScenes, sceneCounter, existingSceneLookup));
    }
    result["panels"] = normalizedPanels;
    
    LOG_DEBUG("QwenClient", QString("Extracted chars=%1 scenes=%2 existingScenes=%3").arg(extractedChars.size()).arg(extractedScenes.size()).arg(existingScenesArr.size()));

    
    QJsonArray characters = result.contains("characters") ? result["characters"].toArray() : QJsonArray();
    QJsonArray scenes = existingScenesArr;
    
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
    
    LOG_DEBUG("QwenClient", QString("规范化结果 - 面板:%1 角色:%2 场景:%3")
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
    QHash<QString, int> keyIndex;

    for (int i = 0; i < result.size(); ++i) {
        const QJsonObject obj = result[i].toObject();
        const QString key = obj.value(keyField).toString();
        if (!key.isEmpty()) {
            keyIndex.insert(key, i);
        }
    }
    
    for (const QJsonValue& val : extracted) {
        const QJsonObject incoming = val.toObject();
        const QString key = incoming.value(keyField).toString();
        if (key.isEmpty()) {
            continue;
        }

        if (keyIndex.contains(key)) {
            const int index = keyIndex.value(key);
            result[index] = mergeCharacterObjects(result[index].toObject(), incoming);
            seenKeys.insert(key);
            continue;
        }

        result.append(incoming);
        keyIndex.insert(key, result.size() - 1);
        seenKeys.insert(key);
    }
    
    return result;
}


void QwenClient::logRequest(const QString& model, const QString& text, 
                            const QString& systemPrompt, const QString& userMessage)
{
    log(QString("请求日志 - 模型: %1, 内容: %2 字符, 系统提示: %3 字符, 用户消息: %4 字符")
        .arg(model).arg(text.length()).arg(systemPrompt.length()).arg(userMessage.length()));
}

void QwenClient::logResponse(const QString& content, const QString& finishReason,
                             const QJsonObject& usage, qint64 elapsedMs)
{
    log(QString("API Response - Elapsed: %1 ms, Finish: %2, Tokens: %3/%4/%5, Content: %6 chars")
        .arg(elapsedMs).arg(finishReason)
        .arg(usage["prompt_tokens"].toInt())
        .arg(usage["completion_tokens"].toInt())
        .arg(usage["total_tokens"].toInt())
        .arg(content.length()));
}

void QwenClient::exponentialBackoff(int attempt)
{
    int backoffMs = qPow(2, attempt) * 1000;
    log(QString("Waiting %1ms before retry...").arg(backoffMs));
    QThread::msleep(backoffMs);
}

void QwenClient::log(const QString& content)
{
    QFile logFile(m_logFilePath);
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&logFile);
        stream.setCodec("UTF-8");
        stream << QString("[%1] %2\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"), content);
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
    
    log(QString("Streaming storyboard generation started, text: %1 chars").arg(text.length()));
    
    QString systemPrompt = QwenPromptBuilder::buildSystemPrompt();
    QString userMessage = QwenPromptBuilder::buildUserMessageWithBible(text, existingCharacters, existingScenes, chapterNumber);
    
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
    
    LOG_DEBUG("QwenClient", QStringLiteral("发送流式请求"));
    LOG_DEBUG("QwenClient", QString("请求地址: %1").arg(m_config.endpoint + "/chat/completions"));
    LOG_DEBUG("QwenClient", QString("请求体大小: %1 bytes").arg(requestData.size()));
    
    m_streamAccumulatedContent.clear();
    m_streamBuffer.clear();
    m_streamAborted = false;
    
    QNetworkReply* reply = networkManager->post(request, requestData);
    
    QTimer* timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    
    QPointer<QwenClient> self = this;
    QPointer<QNetworkReply> replyPtr = reply;
    
    connect(reply, &QNetworkReply::readyRead, this, [self, replyPtr]() {
        if (!self || !replyPtr) return;
        
        self->m_streamBuffer += replyPtr->readAll();
        
        QByteArray delimiter = "\n\n";
        int splitPos = 0;
        
        while ((splitPos = self->m_streamBuffer.indexOf(delimiter)) != -1) {
            QByteArray completeEvent = self->m_streamBuffer.left(splitPos);
            self->m_streamBuffer = self->m_streamBuffer.mid(splitPos + delimiter.size());
            
            if (!completeEvent.isEmpty()) {
                QJsonObject streamResult = QwenStreamHandler::handleStreamResponse(completeEvent);
                QString content = streamResult["accumulated"].toString();
                if (!content.isEmpty()) {
                    self->m_streamAccumulatedContent += content;
                    emit self->streamProgress(self->m_streamAccumulatedContent);
                }
            }
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

void QwenClient::handleStreamFinished(QNetworkReply* reply, QNetworkAccessManager* manager, QTimer* timer)
{
    if (timer) {
        timer->stop();
        timer->deleteLater();
    }
    
    if (m_streamAborted) {
        if (reply) reply->deleteLater();
        if (manager) manager->deleteLater();
        return;
    }
    
    m_streamInProgress = false;
    
    if (!reply || !manager) return;
    
    if (!m_streamBuffer.isEmpty()) {
        QJsonObject streamResult = QwenStreamHandler::handleStreamResponse(m_streamBuffer);
        QString content = streamResult["accumulated"].toString();
        if (!content.isEmpty()) {
            m_streamAccumulatedContent += content;
        }
        m_streamBuffer.clear();
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = reply->errorString();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        LOG_ERROR("QwenClient", QString("流式请求失败: %1 (HTTP %2)").arg(m_lastError).arg(statusCode));
        emit errorOccurred("sendApiRequestStream", m_lastError);
    } else {
        LOG_INFO("QwenClient", QString("流式响应已累计 %1 字符").arg(m_streamAccumulatedContent.length()));
        
        QJsonObject parsedJson = QwenJsonRepair::parseWithRepair(m_streamAccumulatedContent);
        
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
    m_streamAborted = true;
    m_streamInProgress = false;
    m_lastError = tr("流式请求超时");
    emit errorOccurred("sendApiRequestStream", m_lastError);
    
    reply->abort();
}


