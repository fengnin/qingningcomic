#include "services/ChangeRequestService.h"
#include "api/QwenClient.h"
#include "services/ImageService.h"
#include "utils/UserSession.h"
#include "data/FileStorage.h"
#include "utils/Logger.h"
#include <QDateTime>
#include <QUuid>
#include <QTimer>
#include <QJsonDocument>
#include <QtConcurrent>
#include <stdexcept>

namespace {
QJsonObject panelContentOrFallback(const Panel& panel)
{
    QJsonObject content = panel.rawContent();
    if (content.isEmpty()) {
        content = panel.content();
    }

    if (!content.contains("scene")) {
        content["scene"] = panel.scene();
    }
    if (!content.contains("shotType")) {
        content["shotType"] = panel.shotType();
    }
    if (!content.contains("cameraAngle")) {
        content["cameraAngle"] = panel.cameraAngle();
    }
    if (!content.contains("visualPrompt")) {
        content["visualPrompt"] = panel.visualPrompt();
    }
    if (!content.contains("visualPromptEn")) {
        content["visualPromptEn"] = panel.visualPromptEn();
    }
    if (!content.contains("status")) {
        content["status"] = panel.status();
    }
    return content;
}

QString jsonToString(const QJsonObject& object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QJsonObject jsonFromVariant(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QJsonObject();
    }

    if (value.type() == QVariant::Map) {
        return QJsonDocument::fromVariant(value.toMap()).object();
    }

    if (value.type() == QVariant::ByteArray) {
        return QJsonDocument::fromJson(value.toByteArray()).object();
    }

    if (value.type() == QVariant::String) {
        return QJsonDocument::fromJson(value.toString().toUtf8()).object();
    }

    return QJsonDocument::fromVariant(value).object();
}

QJsonObject changeRequestRowToJson(const QVariantMap& row)
{
    QJsonObject json;
    json["id"] = row.value("id").toString();
    json["novelId"] = row.value("novel_id").toString();
    json["storyboardId"] = row.value("storyboard_id").toString();
    json["naturalLanguage"] = row.value("natural_language").toString();
    const QString status = row.value("status").toString();
    json["status"] = status.isEmpty() ? "queued" : status;
    json["userId"] = row.value("user_id").toString();
    json["jobId"] = row.value("job_id").toString();
    json["createdAt"] = row.value("created_at").toString();
    json["updatedAt"] = row.value("updated_at").toString();

    const QString errorMessage = row.value("error_message").toString();
    if (!errorMessage.isEmpty()) {
        json["error"] = errorMessage;
    }

    const QJsonObject dsl = jsonFromVariant(row.value("dsl"));
    if (!dsl.isEmpty()) {
        json["dsl"] = dsl;
    }

    const QJsonObject context = jsonFromVariant(row.value("context"));
    if (!context.isEmpty()) {
        json["context"] = context;
    }

    return json;
}
}

ChangeRequestService::ChangeRequestService(DatabaseManager* db, QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_qwenClient(QwenClient::instance())
    , m_imageService(ImageService::instance())
{
}

ChangeRequest ChangeRequestService::createChangeRequest(
    const QString& novelId,
    const QString& naturalLanguage,
    const QJsonObject& context)
{
    ChangeRequest cr;
    cr.setId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    cr.setNovelId(novelId);
    cr.setUserId(UserSession::instance()->currentUserId());
    cr.setNaturalLanguage(naturalLanguage);
    cr.setContext(context);
    cr.setStatus("queued");
    
    QString timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    cr.setCreatedAt(timestamp);
    cr.setUpdatedAt(timestamp);

    QString storyboardId = context["storyboardId"].toString();
    if (storyboardId.isEmpty()) {
        QVariantMap novel = m_db->selectOne("novels", "id = ?", QVariantList{novelId});
        storyboardId = novel.value("storyboard_id").toString();
    }
    cr.setStoryboardId(storyboardId);

    if (!ensureChangeRequestColumns()) {
        m_lastError = "Failed to ensure change_requests columns";
        LOG_ERROR("ChangeRequest", m_lastError);
        return ChangeRequest();
    }



    QVariantMap crData;
    crData["id"] = cr.id();
    crData["novel_id"] = cr.novelId();
    crData["storyboard_id"] = cr.storyboardId();
    crData["natural_language"] = cr.naturalLanguage();
    crData["status"] = cr.status();
    crData["user_id"] = cr.userId();
    crData["job_id"] = cr.jobId();
    crData["context"] = jsonToString(context);
    crData["dsl"] = QVariant();
    crData["error_message"] = QVariant();
    crData["created_at"] = cr.createdAt();
    crData["updated_at"] = cr.updatedAt();
    
    if (m_db->insert("change_requests", crData)) {
        emit changeRequestCreated(cr);
        LOG_INFO("ChangeRequest", QString("变更请求已创建: %1").arg(cr.id()));
        return cr;
    }

    m_lastError = "Failed to create change request";
    LOG_ERROR("ChangeRequest", m_lastError);
    return ChangeRequest();
}
QJsonObject ChangeRequestService::executeChangeRequest(const QString& crId)
{
    QJsonObject result;

    ChangeRequest cr = getChangeRequest(crId, QString());
    if (!cr.isValid()) {
        m_lastError = QString("Change request %1 does not exist.").arg(crId);
        LOG_ERROR("ChangeRequest", m_lastError);
        emit changeRequestFailed(crId, m_lastError);
        return result;
    }

    if (!updateChangeRequestStatus(crId, cr.novelId(), "in_progress")) {
        emit changeRequestFailed(crId, m_lastError);
        return result;
    }

    try {
        ChangeRequestDsl dsl = cr.dsl();
        if (!dsl.isValid()) {
            dsl = parseNaturalLanguage(cr.naturalLanguage(), cr.context());
            if (!dsl.isValid()) {
                throw std::runtime_error("Failed to parse change request DSL");
            }
            if (!saveChangeRequestDsl(crId, cr.novelId(), dsl)) {
                throw std::runtime_error(m_lastError.toStdString());
            }
        }

        result["crId"] = crId;
        result["scope"] = dsl.scope;
        result["type"] = dsl.type;
        result["targetId"] = dsl.targetId;
        result["reason"] = dsl.reason;
        result["priority"] = dsl.priority;

        QJsonArray opResults;
        const int total = dsl.ops.size();
        for (int i = 0; i < total; ++i) {
            const ChangeRequestOp& op = dsl.ops.at(i);
            emit progressChanged(crId, i + 1, total, QString("正在执行第 %1/%2 个操作").arg(i + 1).arg(total));

            QJsonObject opResult;
            opResult["action"] = op.action;

            QJsonObject output;
            if (dsl.type == "art") {
                output = executeArtOperation(dsl, op, crId);
            } else if (dsl.type == "dialogue") {
                output = executeDialogueOperation(dsl, op, crId);
            } else if (dsl.type == "layout") {
                output = executeLayoutOperation(dsl, op, crId);
            } else if (dsl.type == "style") {
                output = executeStyleOperation(dsl, op, crId);
            } else {
                throw std::runtime_error(QString("Unsupported change request type: %1").arg(dsl.type).toStdString());
            }

            opResult["status"] = "completed";
            opResult["output"] = output;
            opResults.append(opResult);
        }

        result["results"] = opResults;
        result["status"] = "completed";

        if (!updateChangeRequestStatus(crId, cr.novelId(), "completed")) {
            throw std::runtime_error(m_lastError.toStdString());
        }

        emit changeRequestCompleted(crId, result);
        return result;
    } catch (const std::exception& e) {
        m_lastError = QString::fromUtf8(e.what());
        updateChangeRequestStatus(crId, cr.novelId(), "failed", m_lastError);
        emit changeRequestFailed(crId, m_lastError);
        return result;
    }
}

void ChangeRequestService::executeChangeRequestAsync(const QString& crId)
{
    if (crId.isEmpty()) {
        m_lastError = "Change request ID is empty";
        emit changeRequestFailed(crId, m_lastError);
        return;
    }

    QtConcurrent::run([this, crId]() {
        executeChangeRequest(crId);
    });
}


ChangeRequestDsl ChangeRequestService::parseNaturalLanguage(
    const QString& naturalLanguage,
    const QJsonObject& context)
{
    QJsonObject crDslSchema;
    crDslSchema["$schema"] = "http://json-schema.org/draft-07/schema#";
    crDslSchema["title"] = "CR-DSL Schema";
    crDslSchema["type"] = "object";
    
    QJsonObject result = m_qwenClient->parseChangeRequest(naturalLanguage, crDslSchema, context);
    
    if (result.isEmpty()) {
        m_lastError = m_qwenClient->lastError();
        LOG_ERROR("ChangeRequest", QString("解析自然语言失败: %1").arg(m_lastError));
        return ChangeRequestDsl();
    }

    return ChangeRequestDsl::fromJson(result);
}

QJsonObject ChangeRequestService::executeArtOperation(
    const ChangeRequestDsl& dsl,
    const ChangeRequestOp& op,
    const QString& crId)
{
    Q_UNUSED(crId);
    QJsonObject result;

    if (dsl.targetId.isEmpty()) {
        throw std::runtime_error(u8"art \u64cd\u4f5c\u9700\u8981 targetId\uff08\u9762\u677f ID\uff09");
    }

    Panel panel = loadPanelById(dsl.targetId);
    if (!panel.isValid()) {
        throw std::runtime_error(QString("Panel %1 does not exist.").arg(dsl.targetId).toStdString());
    }

    QString mode = op.params["mode"].toString("preview");
    if (mode.isEmpty()) {
        mode = panel.hasHdImage() ? "hd" : "preview";
    }

    QString action = op.action;
    
    if (action == "inpaint" || action == "bg_swap" || action == "repose") {
        QString prompt = op.params["prompt"].toString();
        if (prompt.isEmpty()) {
            prompt = panel.visualPrompt();
        }

        if (!prompt.isEmpty()) {
            QJsonObject content = buildPanelContentForWrite(panel);
            content["visualPrompt"] = prompt;
            panel.setVisualPrompt(prompt);
            panel.setContent(content);
            if (!updatePanel(panel)) {
                throw std::runtime_error(m_lastError.toStdString());
            }
        }

        QString imageUrl = panel.previewUrl();
        if (mode == "hd" && !panel.hdUrl().isEmpty()) {
            imageUrl = panel.hdUrl();
        }

        if (imageUrl.isEmpty()) {
            throw std::runtime_error(u8"\u5f53\u524d\u9762\u677f\u6ca1\u6709\u53ef\u7f16\u8f91\u7684\u56fe\u7247");
        }

        ImageService::GenerateResult genResult = m_imageService->regeneratePanelImage(
            dsl.targetId, 
            mode == "hd" ? ImageService::GenerateMode::HD : ImageService::GenerateMode::Preview
        );

        if (!genResult.success) {
            throw std::runtime_error(genResult.errorMessage.toStdString());
        }

        result["s3Key"] = genResult.s3Key;
        result["mode"] = mode;
        result["imageUrl"] = genResult.imageUrl;

    } else if (action == "outpaint" || action == "regen_panel") {
        ImageService::GenerateResult genResult = m_imageService->regeneratePanelImage(
            dsl.targetId,
            mode == "hd" ? ImageService::GenerateMode::HD : ImageService::GenerateMode::Preview
        );

        if (!genResult.success) {
            throw std::runtime_error(genResult.errorMessage.toStdString());
        }

        result["s3Key"] = genResult.s3Key;
        result["mode"] = mode;
        result["imageUrl"] = genResult.imageUrl;

    } else if (action == "add_effect") {
        QString effect = op.params["effect"].toString("tone");
        double intensity = op.params["intensity"].toDouble(0.5);

        result["effect"] = effect;
        result["intensity"] = intensity;

    } else if (action == "resize") {
        int width = op.params["width"].toInt();
        int height = op.params["height"].toInt();

        if (width <= 0 || height <= 0) {
            throw std::runtime_error(u8"resize \u64cd\u4f5c\u9700\u8981\u6709\u6548\u7684 width \u548c height \u53c2\u6570");
        }

        if (!updatePanelLayout(dsl.targetId, width, height)) {
            throw std::runtime_error(m_lastError.toStdString());
        }
        result["width"] = width;
        result["height"] = height;

    } else {
        throw std::runtime_error(QString::fromUtf8(u8"\u4e0d\u652f\u6301\u7684\u827a\u672f\u64cd\u4f5c: %1").arg(action).toStdString());
    }

    return result;
}

QJsonObject ChangeRequestService::executeDialogueOperation(
    const ChangeRequestDsl& dsl,
    const ChangeRequestOp& op,
    const QString& crId)
{
    Q_UNUSED(crId);
    QJsonObject result;

    if (dsl.targetId.isEmpty()) {
        throw std::runtime_error(u8"dialogue \u64cd\u4f5c\u9700\u8981 targetId\uff08\u9762\u677f ID\uff09");
    }

    Panel panel = loadPanelById(dsl.targetId);
    if (!panel.isValid()) {
        throw std::runtime_error(QString("Panel %1 does not exist.").arg(dsl.targetId).toStdString());
    }

    QList<DialogueLine> dialogues = panel.dialogue();

    QString newText = op.params["newText"].toString();
    QString speaker = op.params["speaker"].toString();
    QString dialogueId = op.params["dialogueId"].toString();

    if (op.action == "rewrite_dialogue") {
        if (newText.isEmpty()) {
            QString instruction = op.params["instruction"].toString();
            if (instruction.isEmpty()) {
                instruction = QString::fromUtf8(u8"\u8bf7\u76f4\u63a5\u91cd\u5199\u5bf9\u767d");
            }
            
            QString originalText;
            if (!dialogues.isEmpty()) {
                originalText = dialogues.first().text;
            }
            newText = m_qwenClient->rewriteDialogue(originalText, instruction);
        }

        DialogueLine newDialogue;
        newDialogue.speaker = speaker.isEmpty() ? "Unknown" : speaker;
        newDialogue.text = newText;
        newDialogue.bubbleType = "speech";

        bool updated = false;
        
        if (!dialogueId.isEmpty()) {
            bool isIndex = false;
            int idx = dialogueId.toInt(&isIndex);
            
            if (isIndex && idx >= 0 && idx < dialogues.size()) {
                dialogues[idx] = newDialogue;
                updated = true;
            } else {
                for (int i = 0; i < dialogues.size(); ++i) {
                    if (dialogues[i].speaker == dialogueId) {
                        dialogues[i] = newDialogue;
                        updated = true;
                        break;
                    }
                }
            }
        }
        
        if (!updated && !speaker.isEmpty()) {
            for (int i = 0; i < dialogues.size(); ++i) {
                if (dialogues[i].speaker == speaker) {
                    dialogues[i].text = newText;
                    updated = true;
                    break;
                }
            }
        }
        
        if (!updated) {
            dialogues.append(newDialogue);
        }

        if (!updatePanelDialogue(dsl.targetId, dialogues)) {
            throw std::runtime_error(m_lastError.toStdString());
        }
    }

    result["dialogueId"] = dialogueId.isEmpty() ? "dlg-1" : dialogueId;
    result["speaker"] = speaker;
    result["text"] = newText;

    return result;
}

QJsonObject ChangeRequestService::executeLayoutOperation(
    const ChangeRequestDsl& dsl,
    const ChangeRequestOp& op,
    const QString& crId)
{
    Q_UNUSED(crId);
    QJsonObject result;

    if (dsl.targetId.isEmpty()) {
        throw std::runtime_error(u8"layout \u64cd\u4f5c\u9700\u8981 targetId\uff08\u9762\u677f ID\uff09");
    }

    Panel panel = loadPanelById(dsl.targetId);
    if (!panel.isValid()) {
        throw std::runtime_error(QString("Panel %1 does not exist.").arg(dsl.targetId).toStdString());
    }

    QString action = op.action;

    if (action == "reorder") {
        int fromIndex = op.params["fromIndex"].toInt(panel.index());
        int toIndex = op.params["toIndex"].toInt(panel.index());

        if (!reorderPanel(dsl.targetId, fromIndex, toIndex)) {
            throw std::runtime_error(u8"\u91cd\u65b0\u6392\u5217\u9762\u677f\u5931\u8d25");
        }

        result["page"] = panel.page();
        result["fromIndex"] = fromIndex;
        result["toIndex"] = toIndex;

    } else if (action == "resize") {
        int width = op.params["width"].toInt();
        int height = op.params["height"].toInt();

        if (width <= 0 || height <= 0) {
            throw std::runtime_error(u8"resize \u64cd\u4f5c\u9700\u8981\u6709\u6548\u7684 width \u548c height \u53c2\u6570");
        }

        if (!updatePanelLayout(dsl.targetId, width, height)) {
            throw std::runtime_error(m_lastError.toStdString());
        }
        result["width"] = width;
        result["height"] = height;

    } else {
        throw std::runtime_error(QString::fromUtf8(u8"\u4e0d\u652f\u6301\u7684\u5e03\u5c40\u64cd\u4f5c: %1").arg(action).toStdString());
    }

    return result;
}

QJsonObject ChangeRequestService::executeStyleOperation(
    const ChangeRequestDsl& dsl,
    const ChangeRequestOp& op,
    const QString& crId)
{
    Q_UNUSED(crId);
    QJsonObject result;

    QJsonObject styleOverrides;

    if (op.params.contains("prompt")) {
        styleOverrides["prompt"] = op.params["prompt"].toString();
    }
    if (op.params.contains("style")) {
        styleOverrides["style"] = op.params["style"].toString();
    }
    if (op.params.contains("palette")) {
        styleOverrides["palette"] = op.params["palette"].toString();
    }

    if (!dsl.targetId.isEmpty()) {
        if (!ensureStoryboardStyleOverridesColumn()) {
            throw std::runtime_error("Failed to ensure style_overrides column.");
        }
        if (!updateStoryboardStyle(dsl.targetId, styleOverrides)) {
            throw std::runtime_error(m_lastError.toStdString());
        }
    }

    result = styleOverrides;

    return result;
}

ChangeRequest ChangeRequestService::getChangeRequest(const QString& crId, const QString& novelId)
{
    QString whereClause = "id = ?";
    QVariantList whereValues;
    whereValues << crId;
    
    if (!novelId.isEmpty()) {
        whereClause += " AND novel_id = ?";
        whereValues << novelId;
    }

    QVariantMap data = m_db->selectOne("change_requests", whereClause, whereValues);
    if (data.isEmpty()) {
        return ChangeRequest();
    }

    return ChangeRequest::fromJson(changeRequestRowToJson(data));
}

QList<ChangeRequest> ChangeRequestService::getChangeRequestsByNovel(const QString& novelId)
{
    QList<ChangeRequest> results;

    QList<QVariantMap> dataList = m_db->selectAll("change_requests", "novel_id = ?",
                                                    QVariantList{novelId}, "created_at DESC");
    for (const auto& data : dataList) {
        results.append(ChangeRequest::fromJson(changeRequestRowToJson(data)));
    }

    return results;
}

bool ChangeRequestService::updateChangeRequestStatus(
    const QString& crId,
    const QString& novelId,
    const QString& status,
    const QString& error)
{
    QVariantMap updates;
    updates["status"] = status;
    updates["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    
    if (!error.isEmpty()) {
        updates["error_message"] = error;
    }

    QString whereClause = "id = ?";
    QVariantList whereValues;
    whereValues << crId;
    
    if (!novelId.isEmpty()) {
        whereClause += " AND novel_id = ?";
        whereValues << novelId;
    }

    return m_db->update("change_requests", updates, whereClause, whereValues);
}

bool ChangeRequestService::saveChangeRequestDsl(
    const QString& crId,
    const QString& novelId,
    const ChangeRequestDsl& dsl)
{
    QVariantMap updates;
    updates["dsl"] = jsonToString(dsl.toJson());
    updates["status"] = "pending";
    updates["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QString whereClause = "id = ?";
    QVariantList whereValues;
    whereValues << crId;
    
    if (!novelId.isEmpty()) {
        whereClause += " AND novel_id = ?";
        whereValues << novelId;
    }

    return m_db->update("change_requests", updates, whereClause, whereValues);
}

Panel ChangeRequestService::loadPanelById(const QString& panelId)
{
    QVariantMap data = m_db->selectOne("panels", "id = ?", QVariantList{panelId});
    if (data.isEmpty()) {
        return Panel();
    }

    Panel panel;
    panel.setId(data["id"].toString());
    panel.setStoryboardId(data["storyboard_id"].toString());
    panel.setPage(data["page"].toInt());
    panel.setIndex(data["panel_index"].toInt());
    panel.setPreviewS3Key(data["preview_image_path"].toString());
    panel.setHdS3Key(data["hd_image_path"].toString());
    
    QString previewPath = data["preview_image_path"].toString();
    QString hdPath = data["hd_image_path"].toString();
    
    if (!previewPath.isEmpty()) {
        if (previewPath.startsWith("http")) {
            panel.setPreviewUrl(previewPath);
        } else {
            panel.setPreviewUrl(FileStorage::instance()->getFullPath(previewPath));
        }
    }
    if (!hdPath.isEmpty()) {
        if (hdPath.startsWith("http")) {
            panel.setHdUrl(hdPath);
        } else {
            panel.setHdUrl(FileStorage::instance()->getFullPath(hdPath));
        }
    }

    QJsonObject content;
    if (data.contains("content")) {
        content = jsonFromVariant(data["content"]);
    }
    if (content.isEmpty()) {
        content = panel.content();
    }
    if (!content.contains("visualPrompt")) {
        content["visualPrompt"] = data["visual_prompt"].toString();
    }
    if (!content.contains("layout") && data.contains("layout")) {
        content["layout"] = jsonFromVariant(data["layout"]);
    }
    panel.setContent(content);
    panel.setVisualPrompt(content.value("visualPrompt").toString(panel.visualPrompt()));
    panel.setVisualPromptEn(content.value("visualPromptEn").toString(panel.visualPromptEn()));
    panel.setStatus(content.value("status").toString("pending"));

    return panel;
}

bool ChangeRequestService::updatePanel(const Panel& panel)
{
    QJsonObject content = buildPanelContentForWrite(panel);
    content["visualPrompt"] = panel.visualPrompt();
    content["visualPromptEn"] = panel.visualPromptEn();
    content["status"] = panel.status();

    QVariantMap updates;
    updates["content"] = jsonToString(content);
    updates["visual_prompt"] = panel.visualPrompt();
    if (content.contains("layout")) {
        updates["layout"] = jsonToString(content["layout"].toObject());
    }
    updates["preview_image_path"] = panel.previewS3Key();
    updates["hd_image_path"] = panel.hdS3Key();
    updates["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    return m_db->update("panels", updates, "id = ?", QVariantList{panel.id()});
}

bool ChangeRequestService::updatePanelDialogue(
    const QString& panelId,
    const QList<DialogueLine>& dialogue)
{
    Panel panel = loadPanelById(panelId);
    if (!panel.isValid()) {
        m_lastError = QString("Panel %1 does not exist.").arg(panelId);
        return false;
    }

    QJsonArray dialogueArray;
    for (const auto& line : dialogue) {
        QJsonObject lineObj;
        lineObj["speaker"] = line.speaker;
        lineObj["text"] = line.text;
        lineObj["bubbleType"] = line.bubbleType;
        dialogueArray.append(lineObj);
    }

    QJsonObject content = buildPanelContentForWrite(panel);
    content["dialogue"] = dialogueArray;

    QVariantMap updates;
    updates["content"] = jsonToString(content);
    updates["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    return m_db->update("panels", updates, "id = ?", QVariantList{panelId});
}

bool ChangeRequestService::updatePanelLayout(const QString& panelId, int width, int height)
{
    Panel panel = loadPanelById(panelId);
    if (!panel.isValid()) {
        m_lastError = QString("Panel %1 does not exist.").arg(panelId);
        return false;
    }

    QJsonObject layout;
    layout["width"] = width;
    layout["height"] = height;

    QJsonObject content = buildPanelContentForWrite(panel);
    content["layout"] = layout;

    QVariantMap updates;
    updates["content"] = jsonToString(content);
    updates["layout"] = jsonToString(layout);
    updates["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    return m_db->update("panels", updates, "id = ?", QVariantList{panelId});
}

bool ChangeRequestService::updateStoryboardStyle(
    const QString& storyboardId,
    const QJsonObject& styleOverrides)
{
    QVariantMap updates;
    updates["style_overrides"] = jsonToString(styleOverrides);
    updates["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    return m_db->update("storyboards", updates, "id = ?", QVariantList{storyboardId});
}

QString ChangeRequestService::generateEditS3Key(
    const QString& panelId,
    const QString& crId,
    const QString& action,
    const QString& mode)
{
    return QString("edits/%1/%2/%3-%4-%5.png")
        .arg(panelId)
        .arg(crId)
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(action)
        .arg(mode);
}

QJsonObject ChangeRequestService::buildPanelContentForWrite(const Panel& panel) const
{
    return panelContentOrFallback(panel);
}

bool ChangeRequestService::ensureStoryboardStyleOverridesColumn()
{
    QList<QVariantMap> rows = m_db->executeQuery(
        "SELECT COUNT(*) AS cnt FROM information_schema.columns "
        "WHERE table_schema = DATABASE() AND table_name = 'storyboards' AND column_name = 'style_overrides'");
    if (!rows.isEmpty() && rows.first().value("cnt").toInt() > 0) {
        return true;
    }

    return m_db->executeSql("ALTER TABLE storyboards ADD COLUMN style_overrides JSON NULL");
}

bool ChangeRequestService::ensureChangeRequestColumns()
{
    auto hasColumn = [this](const QString& columnName) -> bool {
        const QString sql = QString(
            "SELECT COUNT(*) AS cnt FROM information_schema.columns "
            "WHERE table_schema = DATABASE() AND table_name = 'change_requests' AND column_name = '%1'")
            .arg(columnName);
        QList<QVariantMap> rows = m_db->executeQuery(sql);
        return !rows.isEmpty() && rows.first().value("cnt").toInt() > 0;
    };

    QStringList alters;
    if (!hasColumn("storyboard_id")) {
        alters << "ADD COLUMN storyboard_id VARCHAR(36) NULL";
    }
    if (!hasColumn("context")) {
        alters << "ADD COLUMN context JSON NULL";
    }
    if (!hasColumn("error_message")) {
        alters << "ADD COLUMN error_message TEXT NULL";
    }

    if (alters.isEmpty()) {
        return true;
    }

    return m_db->executeSql(QString("ALTER TABLE change_requests %1").arg(alters.join(", ")));
}

bool ChangeRequestService::reorderPanel(const QString& panelId, int fromIndex, int toIndex)
{
    Panel panel = loadPanelById(panelId);
    if (!panel.isValid()) {
        return false;
    }

    if (fromIndex < 0) {
        fromIndex = panel.index();
    }

    if (toIndex < 0 || fromIndex == toIndex) {
        return true;
    }

    QVariantMap targetUpdate;
    targetUpdate["panel_index"] = toIndex;
    targetUpdate["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!m_db->update("panels", targetUpdate, "id = ?", QVariantList{panelId})) {
        return false;
    }

    QVariantMap swappedPanel = m_db->selectOne("panels",
        "storyboard_id = ? AND page = ? AND panel_index = ?",
        QVariantList{panel.storyboardId(), panel.page(), toIndex});

    if (!swappedPanel.isEmpty()) {
        QVariantMap swapUpdate;
        swapUpdate["panel_index"] = fromIndex;
        swapUpdate["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        m_db->update("panels", swapUpdate, "id = ?", QVariantList{swappedPanel["id"].toString()});
    }

    return true;
}

