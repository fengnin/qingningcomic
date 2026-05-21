#include "services/ChangeRequestService.h"
#include "api/QwenClient.h"
#include "services/ImageService.h"
#include "services/StoryboardService.h"
#include "utils/UserSession.h"
#include "data/FileStorage.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "utils/Logger.h"
#include <QImage>
#include <QRegularExpression>

namespace {
    QByteArray extractImageFromResult(
        const QString& providerName,
        const QByteArray& imageData,
        const QString& imageUrl)
    {
        if (!imageData.isEmpty()) {
            return imageData;
        }
        
        if (!imageUrl.isEmpty()) {
            LOG_INFO("ChangeRequest", QString("从 URL 下载编辑后的图片: %1").arg(imageUrl.left(100)));
            
            QByteArray downloaded;
            if (providerName == "volcengine") {
                downloaded = VolcEngineImageClient::instance()->downloadImage(imageUrl);
            } else {
                downloaded = QwenImageClient::instance()->downloadImage(imageUrl);
            }
            
            if (downloaded.isEmpty()) {
                LOG_ERROR("ChangeRequest", QString("无法下载编辑后的图片: %1").arg(imageUrl));
            }
            
            return downloaded;
        }
        
        return QByteArray();
    }
}
#include "utils/ChangeRequestCreationUtils.h"
#include "utils/ChangeRequestParseUtils.h"
#include "utils/ChangeRequestExecutionUtils.h"
#include "utils/ChangeRequestExecutionFlowUtils.h"
#include "utils/ChangeRequestExpressionUtils.h"
#include "utils/ChangeRequestIntentUtils.h"
#include "utils/ChangeRequestTargetUtils.h"
#include "utils/PromptTargetUtils.h"
#include "utils/FaceEditPromptUtils.h"
#include "utils/LocalEditPromptUtils.h"
#include "utils/StatusWriteUtils.h"
#include <QDateTime>
#include <QUuid>
#include <QTimer>
#include <QJsonDocument>
#include <QStringList>
#include <QHash>
#include <QSet>
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

QJsonArray intListToJsonArray(const QList<int>& values)
{
    QJsonArray jsonArray;
    for (int value : values) {
        jsonArray.append(value);
    }
    return jsonArray;
}

QList<QVariantMap> loadPanelsForPage(DatabaseManager* db,
                                    const QString& storyboardId,
                                    int page,
                                    QString* errorMessage)
{
    if (!db || storyboardId.isEmpty() || page <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("loadPanelsForPage received invalid arguments");
        }
        return {};
    }

    QList<QVariantMap> rows = db->selectAll(
        QStringLiteral("panels"),
        QStringLiteral("storyboard_id = ? AND page = ?"),
        QVariantList{storyboardId, page},
        QStringLiteral("panel_index ASC"));
    if (rows.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("No panels found for the target page");
    }
    return rows;
}

bool writePanelIndex(DatabaseManager* db, const QString& panelId, int panelIndex, QString* errorMessage)
{
    if (!db || panelId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("writePanelIndex received invalid arguments");
        }
        return false;
    }

    const QString timestamp = ChangeRequestCreationUtils::currentUtcTimestamp();
    const bool success = db->executeUpdate(
        QStringLiteral("UPDATE panels SET panel_index = ?, updated_at = ? WHERE id = ?"),
        QVariantList{panelIndex, timestamp, panelId});
    if (!success && errorMessage) {
        *errorMessage = db->lastError();
    }
    return success;
}

QList<QVariantMap> buildRowsFromPanelOrder(const QList<QVariantMap>& rows,
                                           const QList<int>& desiredPanelNumbers,
                                           QString* errorMessage)
{
    QList<QVariantMap> orderedRows;
    if (rows.isEmpty() || desiredPanelNumbers.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("buildRowsFromPanelOrder received invalid arguments");
        }
        return orderedRows;
    }

    QHash<int, QVariantMap> rowsByPanelNumber;
    for (const QVariantMap& row : rows) {
        const int panelNumber = ChangeRequestTargetUtils::panelNumberFromPageAndIndex(
            row.value("page").toInt(),
            row.value("panel_index").toInt());
        rowsByPanelNumber.insert(panelNumber, row);
    }

    QSet<int> usedPanelNumbers;
    for (int panelNumber : desiredPanelNumbers) {
        if (usedPanelNumbers.contains(panelNumber)) {
            continue;
        }

        const QVariantMap row = rowsByPanelNumber.value(panelNumber);
        if (row.isEmpty()) {
            continue;
        }

        orderedRows.append(row);
        usedPanelNumbers.insert(panelNumber);
    }

    for (const QVariantMap& row : rows) {
        const int panelNumber = ChangeRequestTargetUtils::panelNumberFromPageAndIndex(
            row.value("page").toInt(),
            row.value("panel_index").toInt());
        if (!usedPanelNumbers.contains(panelNumber)) {
            orderedRows.append(row);
            usedPanelNumbers.insert(panelNumber);
        }
    }

    if (orderedRows.size() != rows.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Panel order resolution produced an incomplete result");
        }
        return {};
    }

    return orderedRows;
}

QList<QVariantMap> buildRowsFromPermutation(const QList<QVariantMap>& rows,
                                           const QList<int>& sourceIndices,
                                           const QList<int>& targetIndices,
                                           QString* errorMessage)
{
    QList<QVariantMap> orderedRows;
    for (int i = 0; i < rows.size(); ++i) {
        orderedRows.append(QVariantMap());
    }
    if (rows.isEmpty() || sourceIndices.isEmpty() || sourceIndices.size() != targetIndices.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("buildRowsFromPermutation received invalid arguments");
        }
        return {};
    }

    QSet<int> occupiedTargets;
    QSet<int> mappedSourceIndices;
    for (int i = 0; i < sourceIndices.size(); ++i) {
        const int sourceIndex = sourceIndices.at(i);
        const int targetIndex = targetIndices.at(i);
        if (sourceIndex < 0 || sourceIndex >= rows.size() || targetIndex < 0 || targetIndex >= rows.size()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("sourceIndices/targetIndices contains out-of-range values");
            }
            return {};
        }
        if (mappedSourceIndices.contains(sourceIndex)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("sourceIndices contains duplicate positions");
            }
            return {};
        }
        if (occupiedTargets.contains(targetIndex)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("targetIndices contains duplicate positions");
            }
            return {};
        }
        mappedSourceIndices.insert(sourceIndex);
        occupiedTargets.insert(targetIndex);
    }

    for (int i = 0; i < sourceIndices.size(); ++i) {
        const int sourceIndex = sourceIndices.at(i);
        const int targetIndex = targetIndices.at(i);
        orderedRows[targetIndex] = rows.at(sourceIndex);
    }

    int fillCursor = 0;
    for (int rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        if (mappedSourceIndices.contains(rowIndex)) {
            continue;
        }

        while (fillCursor < orderedRows.size() && !orderedRows.at(fillCursor).isEmpty()) {
            ++fillCursor;
        }
        if (fillCursor >= orderedRows.size()) {
            break;
        }

        orderedRows[fillCursor] = rows.at(rowIndex);
        ++fillCursor;
    }

    for (int i = 0; i < orderedRows.size(); ++i) {
        if (orderedRows.at(i).isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Permutation result has empty slots");
            }
            return {};
        }
    }

    return orderedRows;
}

bool persistPanelOrdering(DatabaseManager* db, const QList<QVariantMap>& orderedRows, QString* errorMessage)
{
    if (!db || orderedRows.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("persistPanelOrdering received invalid arguments");
        }
        return false;
    }

    for (int i = 0; i < orderedRows.size(); ++i) {
        const QVariantMap& row = orderedRows.at(i);
        if (row.isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("persistPanelOrdering received an empty row");
            }
            return false;
        }

        const int currentIndex = row.value("panel_index").toInt();
        if (currentIndex == i) {
            continue;
        }

        if (!writePanelIndex(db, row.value("id").toString(), i, errorMessage)) {
            return false;
        }
    }

    return true;
}

bool applyPanelOrder(DatabaseManager* db,
                     const QString& storyboardId,
                     int page,
                     const QList<int>& desiredPanelNumbers,
                     QString* errorMessage)
{
    const QList<QVariantMap> rows = loadPanelsForPage(db, storyboardId, page, errorMessage);
    if (rows.isEmpty()) {
        return false;
    }

    const QList<QVariantMap> orderedRows = buildRowsFromPanelOrder(rows, desiredPanelNumbers, errorMessage);
    if (orderedRows.isEmpty()) {
        return false;
    }

    return persistPanelOrdering(db, orderedRows, errorMessage);
}

bool applyPanelIndexPermutation(DatabaseManager* db,
                                const QString& storyboardId,
                                int page,
                                const QList<int>& sourceIndices,
                                const QList<int>& targetIndices,
                                QString* errorMessage)
{
    const QList<QVariantMap> rows = loadPanelsForPage(db, storyboardId, page, errorMessage);
    if (rows.isEmpty()) {
        return false;
    }

    const QList<QVariantMap> orderedRows = buildRowsFromPermutation(rows, sourceIndices, targetIndices, errorMessage);
    if (orderedRows.isEmpty()) {
        return false;
    }

    return persistPanelOrdering(db, orderedRows, errorMessage);
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

QString buildExpressionEditPrompt(const Panel& panel, const QString& expression)
{
    const QString targetName = panel.characters().size() == 1
        ? panel.characters().first().name
        : QString();
    return FaceEditPromptUtils::buildFaceExpressionEditPrompt(
        targetName.isEmpty()
            ? QStringLiteral("图中人物")
            : QStringLiteral("角色%1").arg(targetName),
        ChangeRequestExpressionUtils::expressionPromptDescription(expression),
        targetName.isEmpty()
            ? QStringLiteral("保持构图、姿势、服装、发型、背景、镜头、光线和画幅比例不变")
            : QStringLiteral("保持其余角色、构图、姿势、服装、发型、背景、镜头、光线和画幅比例不变"));
}

void normalizeChangeRequestExpression(ChangeRequestDsl& dsl)
{
    for (ChangeRequestOp& op : dsl.ops) {
        if (!ChangeRequestExecutionUtils::isExpressionEditOperation(op.action, op)) {
            continue;
        }

        const QString rawExpression = op.params.value("expression").toString();
        const QString normalizedExpression = ChangeRequestExpressionUtils::normalizeExpressionValue(rawExpression);
        if (!normalizedExpression.isEmpty() && normalizedExpression != rawExpression) {
            op.params["expression"] = normalizedExpression;
        }
    }
}

void annotateArtEditContent(QJsonObject& content, const QString& action, const QString& editIntent)
{
    if (action == QStringLiteral("bg_swap")) {
        content["editIntent"] = QStringLiteral("replace_background");
        content["editTarget"] = QStringLiteral("background");
        return;
    }

    if (editIntent == QStringLiteral("replace_subject")) {
        content["editIntent"] = QStringLiteral("replace_subject");
        content.remove(QStringLiteral("editTarget"));
        return;
    }

    if (PromptTargetUtils::isLocalObjectMovementIntent(editIntent)) {
        content["editIntent"] = editIntent;
        content["editTarget"] = QStringLiteral("scene");
        return;
    }

    if (editIntent == QStringLiteral("replace_attribute")) {
        content["editIntent"] = QStringLiteral("replace_attribute");
        content["editTarget"] = QStringLiteral("character");
        return;
    }

    if (editIntent == QStringLiteral("remove_subject")) {
        content["editIntent"] = QStringLiteral("remove_subject");
        content["editTarget"] = QStringLiteral("scene");
        return;
    }

    if (editIntent == QStringLiteral("set_expression")) {
        content["editIntent"] = QStringLiteral("set_expression");
        content["editTarget"] = QStringLiteral("character");
    }
}

void annotateEditIntentIfMissing(ChangeRequestOp& op, const QString& editIntent)
{
    if (editIntent.isEmpty()) {
        return;
    }
    if (op.params.contains("editIntent")) {
        return;
    }
    op.params["editIntent"] = editIntent;
}

QString buildInpaintPromptForIntent(const QJsonObject& params,
                                    const QString& editIntent,
                                    const QString& sourcePrompt = QString());

bool isCharacterReplacementRequest(const QJsonObject& params, const QString& sourcePrompt)
{
    const QString sourceText = sourcePrompt.trimmed().isEmpty()
        ? params.value(QStringLiteral("sourceText")).toString().trimmed()
        : sourcePrompt.trimmed();
    if (sourceText.isEmpty()) {
        return false;
    }

    const QString sourceSubject = ChangeRequestIntentUtils::extractReplacementSourceFromText(sourceText);
    return PromptTargetUtils::isCharacterFocusedPrompt(sourceText)
        || PromptTargetUtils::isCharacterFocusedPrompt(sourceSubject);
}

struct SubjectReplacementResolution {
    QString sourceSubject;
    QString replacementTarget;
};

struct LocalObjectMovementResolution {
    QString sourceObject;
    QString destination;
};

QString resolveReplacementTarget(const QJsonObject& params, const QString& sourcePrompt)
{
    const QString sourceText = ChangeRequestIntentUtils::effectiveEditSourceText(params, sourcePrompt);
    QString replacement = params.value(QStringLiteral("replacement")).toString().trimmed();
    if (!replacement.isEmpty()) {
        return replacement;
    }

    replacement = ChangeRequestIntentUtils::extractFirstTrimmedValue(params, QStringList{
        QStringLiteral("character"),
        QStringLiteral("targetCharacter"),
        QStringLiteral("subjectReplacement")
    });
    if (!replacement.isEmpty()) {
        return replacement;
    }

    replacement = ChangeRequestIntentUtils::extractReplacementTargetFromText(sourceText);
    if (!replacement.isEmpty()) {
        return replacement;
    }

    return QStringLiteral("目标对象");
}

SubjectReplacementResolution resolveSubjectReplacementResolution(const QJsonObject& params,
                                                                 const QString& sourcePrompt)
{
    SubjectReplacementResolution resolution;
    resolution.sourceSubject = ChangeRequestIntentUtils::extractLocalObjectSubjectFromParams(params, sourcePrompt);
    resolution.replacementTarget = resolveReplacementTarget(params, sourcePrompt);

    return resolution;
}

LocalObjectMovementResolution resolveMovementResolution(const QJsonObject& params, const QString& sourcePrompt)
{
    LocalObjectMovementResolution resolution;
    resolution.sourceObject = ChangeRequestIntentUtils::extractLocalObjectSubjectFromParams(params, sourcePrompt);
    if (resolution.sourceObject.isEmpty()) {
        resolution.sourceObject = QStringLiteral("目标对象");
    }

    resolution.destination = ChangeRequestIntentUtils::extractLocalObjectDestinationFromParams(params, sourcePrompt);
    if (resolution.destination.isEmpty()) {
        resolution.destination = QStringLiteral("目标容器");
    }

    return resolution;
}

QString buildAttributeEditPrompt(const QJsonObject& params, const QString& sourcePrompt)
{
    const QString sourceText = ChangeRequestIntentUtils::effectiveEditSourceText(params, sourcePrompt);
    QString subject = ChangeRequestIntentUtils::extractFirstTrimmedValue(params, QStringList{
        QStringLiteral("attribute"),
        QStringLiteral("subject"),
        QStringLiteral("target"),
        QStringLiteral("entity"),
        QStringLiteral("object"),
        QStringLiteral("clothing"),
        QStringLiteral("outfit"),
        QStringLiteral("hair"),
        QStringLiteral("appearance"),
        QStringLiteral("detail")
    });
    if (subject.isEmpty()) {
        subject = ChangeRequestIntentUtils::extractReplacementSourceFromText(sourceText);
    }

    QString replacement = resolveReplacementTarget(params, sourcePrompt);
    if (replacement.isEmpty()) {
        replacement = ChangeRequestIntentUtils::extractReplacementTargetFromText(sourceText);
    }

    const auto spec = LocalEditPromptUtils::buildLocalReplacementEditSpec(
        subject.isEmpty() ? QStringLiteral("局部对象") : subject,
        replacement.isEmpty()
            ? QStringLiteral("仅调整衣服、配饰、发型、颜色、材质或局部物体，不改变原有主体")
            : replacement,
        QStringLiteral("保持其余内容、构图、背景、光线和色调不变"),
        1.10);

    return LocalEditPromptUtils::buildLocalReplacementEditPrompt(
        spec.targetDescription,
        spec.changeTargetDescription,
        spec.preserveDescription,
        spec.strength);
}

void applyInpaintPromptForIntent(ChangeRequestOp& op, const QString& editIntent)
{
    const QString normalizedIntent = editIntent.trimmed().toLower();
    if (normalizedIntent == QStringLiteral("replace_subject")
        || normalizedIntent == QStringLiteral("replace_attribute")
        || normalizedIntent == QStringLiteral("replace_background")
        || PromptTargetUtils::isLocalObjectMovementIntent(normalizedIntent)) {
        const QString originalPrompt = op.params.value(QStringLiteral("prompt")).toString();
        if (!originalPrompt.trimmed().isEmpty() && !op.params.contains(QStringLiteral("rawPrompt"))) {
            op.params["rawPrompt"] = originalPrompt;
        }
        op.params["prompt"] = buildInpaintPromptForIntent(op.params, normalizedIntent, originalPrompt);
        op.params["editIntent"] = normalizedIntent;
    } else {
        if (!op.params.value("prompt").toString().trimmed().isEmpty()) {
            return;
        }
        op.params["prompt"] = buildInpaintPromptForIntent(op.params, normalizedIntent);
        op.params["editIntent"] = QStringLiteral("remove_subject");
    }

    const QString subject = ChangeRequestIntentUtils::extractRemovalSubject(op.params);
    if (!subject.isEmpty()) {
        op.params["subject"] = subject;
    }
}

void normalizeSetExpressionOp(ChangeRequestOp& op, const QString& editIntent)
{
    annotateEditIntentIfMissing(op, editIntent);
}

void normalizeInpaintOp(ChangeRequestOp& op, const QString& editIntent)
{
    const QString existingIntent = op.params.value(QStringLiteral("editIntent")).toString().trimmed();
    const QString effectiveIntent = editIntent.isEmpty() ? existingIntent : editIntent;

    if (!effectiveIntent.isEmpty()) {
        op.params["editIntent"] = effectiveIntent;
    }

    applyInpaintPromptForIntent(op, effectiveIntent);
}

void normalizeChangeRequestArtEdits(ChangeRequestDsl& dsl, const QString& naturalLanguage)
{
    const QString editIntent = ChangeRequestParseUtils::inferEditIntentFromText(naturalLanguage);
    const QString specificReplacementSource = ChangeRequestIntentUtils::extractReplacementSourceFromText(naturalLanguage);
    const QString placementSource = ChangeRequestIntentUtils::extractPlacementSourceFromText(naturalLanguage);

    for (ChangeRequestOp& op : dsl.ops) {
        if (!naturalLanguage.trimmed().isEmpty() && !op.params.contains(QStringLiteral("sourceText"))) {
            op.params["sourceText"] = naturalLanguage.trimmed();
        }
        if (!specificReplacementSource.isEmpty()) {
            op.params["maskRegion"] = specificReplacementSource;
        } else if (PromptTargetUtils::isLocalObjectMovementIntent(editIntent)
                   && !placementSource.isEmpty()) {
            op.params["maskRegion"] = placementSource;
        }
        const QString action = ChangeRequestNormalization::normalizeAction(op.action);
        if (action == QStringLiteral("setExpression")) {
            if (editIntent == QStringLiteral("set_expression")) {
                normalizeSetExpressionOp(op, editIntent);
            }
            continue;
        }

        if (action != QStringLiteral("inpaint")) {
            continue;
        }

        normalizeInpaintOp(op, editIntent);
    }
}

bool updateRecordWithTimestamp(
    DatabaseManager* db,
    const QString& table,
    QVariantMap updates,
    const QString& whereClause,
    const QVariantList& whereValues,
    QString* errorMessage = nullptr)
{
    if (!db) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Database manager is null");
        }
        return false;
    }

    updates["updated_at"] = ChangeRequestCreationUtils::currentUtcTimestamp();
    const bool success = db->update(table, updates, whereClause, whereValues);
    if (!success && errorMessage) {
        *errorMessage = db->lastError();
    }
    return success;
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

QString resolvePanelIdFromContext(DatabaseManager* db, const QJsonObject& context)
{
    if (!db || !context.contains("storyboardId")) {
        return QString();
    }

    const QString storyboardId = context.value("storyboardId").toString();
    const int targetPanelNumber = context.value("targetPanelNumber").toInt();
    if (storyboardId.isEmpty() || targetPanelNumber <= 0) {
        return QString();
    }

    const QList<QVariantMap> rows = db->selectAll(
        "panels",
        "storyboard_id = ?",
        QVariantList{storyboardId},
        "page ASC, panel_index ASC");
    return ChangeRequestTargetUtils::resolvePanelIdByNumber(rows, targetPanelNumber);
}

int inferPanelNumberFromDsl(const ChangeRequestDsl& dsl)
{
    const QStringList candidateTexts = {
        dsl.reason
    };

    for (const QString& text : candidateTexts) {
        const int panelNumber = ChangeRequestTargetUtils::extractRequestedPanelNumber(text);
        if (panelNumber > 0) {
            return panelNumber;
        }
    }

    for (const ChangeRequestOp& op : dsl.ops) {
        const QStringList opTexts = {
            op.params.value(QStringLiteral("maskRegion")).toString(),
            QString::number(op.params.value(QStringLiteral("targetPanelNumber")).toInt()),
            QString::number(op.params.value(QStringLiteral("panelNumber")).toInt()),
            op.params.value(QStringLiteral("panel")).toString(),
            op.params.value(QStringLiteral("subject")).toString(),
            op.params.value(QStringLiteral("target")).toString(),
            op.params.value(QStringLiteral("entity")).toString(),
            op.params.value(QStringLiteral("object")).toString(),
            op.params.value(QStringLiteral("prompt")).toString(),
            op.params.value(QStringLiteral("rawPrompt")).toString()
        };

        for (const QString& text : opTexts) {
            const int panelNumber = ChangeRequestTargetUtils::extractRequestedPanelNumber(text);
            if (panelNumber > 0) {
                return panelNumber;
            }
        }
    }

    return 0;
}

QString resolvePanelIdFromDsl(const ChangeRequestDsl& dsl,
                              DatabaseManager* db,
                              const QJsonObject& context)
{
    if (!db) {
        return QString();
    }

    const int panelNumber = inferPanelNumberFromDsl(dsl);
    if (panelNumber <= 0) {
        return QString();
    }

    QString storyboardId = context.value(QStringLiteral("storyboardId")).toString();
    if (storyboardId.isEmpty()) {
        return QString();
    }

    const QList<QVariantMap> rows = db->selectAll(
        QStringLiteral("panels"),
        QStringLiteral("storyboard_id = ?"),
        QVariantList{storyboardId},
        QStringLiteral("page ASC, panel_index ASC"));
    return ChangeRequestTargetUtils::resolvePanelIdByNumber(rows, panelNumber);
}

QString buildInpaintPromptForIntent(const QJsonObject& params,
                                    const QString& editIntent,
                                    const QString& sourcePrompt)
{
    const QString normalizedIntent = editIntent.trimmed().toLower();
    if (PromptTargetUtils::isLocalObjectMovementIntent(normalizedIntent)) {
        const LocalObjectMovementResolution movement = resolveMovementResolution(params, sourcePrompt);

        return ChangeRequestIntentUtils::buildMovementSubjectPrompt(
            movement.sourceObject,
            QStringLiteral("将%1从原位置移动到%2中，并删除原位置的残留，自然融入场景，保持物体外观一致，补全接触区域和阴影，保留其余画面内容不变，不要在原位置和目标位置同时保留两个%1")
                .arg(movement.sourceObject, movement.destination));
    }
    if (normalizedIntent == QStringLiteral("replace_subject")) {
        const SubjectReplacementResolution resolution = resolveSubjectReplacementResolution(
            params,
            sourcePrompt);
        return ChangeRequestIntentUtils::buildReplacementSubjectPrompt(
            resolution.sourceSubject,
            resolution.replacementTarget);
    }

    if (normalizedIntent == QStringLiteral("replace_attribute")) {
        return buildAttributeEditPrompt(
            params,
            sourcePrompt);
    }

    if (normalizedIntent == QStringLiteral("replace_background")) {
        return ChangeRequestIntentUtils::buildBackgroundEditPrompt(params);
    }

    if (normalizedIntent == QStringLiteral("rotate_subject")) {
        const QString subject = ChangeRequestIntentUtils::extractRemovalSubject(params);
        const QString direction = sourcePrompt.trimmed().isEmpty()
            ? params.value(QStringLiteral("rawPrompt")).toString().trimmed()
            : sourcePrompt.trimmed();
        if (!direction.isEmpty()) {
            return direction;
        }
        return subject.isEmpty()
            ? QStringLiteral("调整人物朝向，保持其余画面内容不变")
            : QString(QStringLiteral("调整%1的朝向，保持其余画面内容不变")).arg(subject);
    }

    if (normalizedIntent == QStringLiteral("change_pose")) {
        const QString subject = ChangeRequestIntentUtils::extractRemovalSubject(params);
        const QString poseDesc = sourcePrompt.trimmed().isEmpty()
            ? params.value(QStringLiteral("rawPrompt")).toString().trimmed()
            : sourcePrompt.trimmed();
        if (!poseDesc.isEmpty()) {
            return poseDesc;
        }
        return subject.isEmpty()
            ? QStringLiteral("调整人物姿势和动作，保持其余画面内容不变")
            : QString(QStringLiteral("调整%1的姿势和动作，保持其余画面内容不变")).arg(subject);
    }

    const QString subject = ChangeRequestIntentUtils::extractRemovalSubject(params);
    return ChangeRequestIntentUtils::buildRemovalSubjectPrompt(subject);
}
}

void ChangeRequestService::resolveDslTarget(
    ChangeRequestDsl& dsl,
    const QString& novelId,
    const QJsonObject& context)
{
    if (!dsl.targetId.isEmpty()) {
        // LLM sometimes returns a positional token like "panel_2" instead of a UUID.
        // Detect that pattern and resolve it to a real panel ID before proceeding.
        static const QRegularExpression kPanelNumberPattern(QStringLiteral("^panel_(\\d+)$"));
        const QRegularExpressionMatch m = kPanelNumberPattern.match(dsl.targetId);
        if (m.hasMatch()) {
            const int panelNumber = m.captured(1).toInt();
            const QString storyboardId = context.value(QStringLiteral("storyboardId")).toString();
            if (panelNumber > 0 && !storyboardId.isEmpty()) {
                const QList<QVariantMap> rows = m_db->selectAll(
                    QStringLiteral("panels"),
                    QStringLiteral("storyboard_id = ?"),
                    QVariantList{storyboardId},
                    QStringLiteral("page ASC, panel_index ASC"));
                const QString resolved = ChangeRequestTargetUtils::resolvePanelIdByNumber(rows, panelNumber);
                if (!resolved.isEmpty()) {
                    LOG_DEBUG("ChangeRequest", QString("Resolved positional targetId '%1' -> '%2'")
                        .arg(dsl.targetId, resolved));
                    dsl.targetId = resolved;
                }
            }
        }
        return;
    }

    if (dsl.type == QStringLiteral("style")) {
        const QString storyboardId = context.value("storyboardId").toString();
        if (!storyboardId.isEmpty()) {
            dsl.targetId = storyboardId;
            return;
        }

        dsl.targetId = resolveStoryboardIdForChangeRequest(novelId, context);
        return;
    }

    const QString directTargetPanelId = context.value("targetPanelId").toString();
    if (!directTargetPanelId.isEmpty()) {
        dsl.targetId = directTargetPanelId;
        return;
    }

    dsl.targetId = resolvePanelIdFromContext(m_db, context);
    if (!dsl.targetId.isEmpty()) {
        return;
    }

    dsl.targetId = resolvePanelIdFromDsl(dsl, m_db, context);
    if (!dsl.targetId.isEmpty()) {
        LOG_DEBUG("ChangeRequest", QString("通过 DSL 线索补全目标面板: targetId=%1").arg(dsl.targetId));
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
    const QString timestamp = ChangeRequestCreationUtils::currentUtcTimestamp();
    const int requestedPanelNumber = ChangeRequestTargetUtils::extractRequestedPanelNumber(naturalLanguage);
    QJsonObject enrichedContext = context;
    const QString resolvedStoryboardId = resolveStoryboardIdForChangeRequest(novelId, enrichedContext);
    if (!resolvedStoryboardId.isEmpty()) {
        enrichedContext["storyboardId"] = resolvedStoryboardId;
    }
    if (requestedPanelNumber > 0) {
        enrichedContext["targetPanelNumber"] = requestedPanelNumber;
        if (!resolvedStoryboardId.isEmpty() && !enrichedContext.contains("targetPanelId")) {
            QJsonObject targetContext = enrichedContext;
            targetContext["targetPanelNumber"] = requestedPanelNumber;
            const QString resolvedPanelId = resolvePanelIdFromContext(m_db, targetContext);
            if (!resolvedPanelId.isEmpty()) {
                enrichedContext["targetPanelId"] = resolvedPanelId;
            }
        }
    }

    LOG_INFO("ChangeRequest", QString("创建变更请求: novelId=%1, textLength=%2, contextKeys=%3")
        .arg(novelId)
        .arg(naturalLanguage.length())
        .arg(enrichedContext.keys().join(",")));
    LOG_DEBUG("ChangeRequest", QString("变更请求上下文已补全: storyboardId=%1, targetPanelNumber=%2, targetPanelId=%3")
        .arg(enrichedContext.value("storyboardId").toString())
        .arg(enrichedContext.value("targetPanelNumber").toInt())
        .arg(enrichedContext.value("targetPanelId").toString()));

    ChangeRequest cr = ChangeRequestCreationUtils::buildDraft(
        QUuid::createUuid().toString(QUuid::WithoutBraces),
        novelId,
        UserSession::instance()->currentUserId(),
        naturalLanguage,
        enrichedContext,
        timestamp);
    cr.setStoryboardId(resolvedStoryboardId);
    LOG_DEBUG("ChangeRequest", QString("变更请求草稿已构建: crId=%1, storyboardId=%2, createdAt=%3, status=%4")
        .arg(cr.id())
        .arg(cr.storyboardId())
        .arg(cr.createdAt())
        .arg(cr.status()));

    if (!ensureChangeRequestColumns()) {
        m_lastError = "Failed to ensure change_requests columns";
        LOG_ERROR("ChangeRequest", m_lastError);
        return ChangeRequest();
    }

    const QVariantMap crData = ChangeRequestCreationUtils::buildInsertData(cr);
    LOG_DEBUG("ChangeRequest", QString("写入 change_requests 字段: %1").arg(crData.keys().join(",")));
    const qint64 insertedId = m_db->insert("change_requests", crData);
    if (insertedId >= 0) {
        emit changeRequestCreated(cr);
        LOG_INFO("ChangeRequest", QString("变更请求已创建: %1 (dbRowId=%2)")
            .arg(cr.id())
            .arg(insertedId));
        return cr;
    }

    m_lastError = "Failed to create change request";
    LOG_ERROR("ChangeRequest", m_lastError);
    return ChangeRequest();
}

QString ChangeRequestService::resolveStoryboardIdForChangeRequest(
    const QString& novelId,
    const QJsonObject& context)
{
    if (!m_db) {
        return QString();
    }

    const QString storyboardId = context.value("storyboardId").toString();
    if (!storyboardId.isEmpty()) {
        return storyboardId;
    }

    QVariantMap novel = m_db->selectOne("novels", "id = ?", QVariantList{novelId});
    const QString novelStoryboardId = novel.value("storyboard_id").toString();
    if (!novelStoryboardId.isEmpty()) {
        return novelStoryboardId;
    }

    const int chapterNumber = context.value("chapterNumber").toInt();
    if (novelId.isEmpty() || chapterNumber <= 0 || !m_db) {
        return QString();
    }

    QVariantMap storyboard = m_db->selectOne(
        "storyboards",
        "novel_id = ? AND chapter_number = ?",
        QVariantList{novelId, chapterNumber});
    return storyboard.value("id").toString();
}

QJsonObject ChangeRequestService::executeChangeRequest(const QString& crId)
{
    QJsonObject result;
    LOG_INFO("ChangeRequest", QString("开始执行变更请求: %1").arg(crId));

    ChangeRequest cr = getChangeRequest(crId, QString());
    if (!cr.isValid()) {
        m_lastError = QString("Change request %1 does not exist.").arg(crId);
        LOG_ERROR("ChangeRequest", m_lastError);
        emit changeRequestFailed(crId, m_lastError);
        return result;
    }

    LOG_DEBUG("ChangeRequest", QString("加载到变更请求: novelId=%1, status=%2, hasDsl=%3, contextKeys=%4")
        .arg(cr.novelId())
        .arg(cr.status())
        .arg(cr.dsl().isValid() ? QStringLiteral("true") : QStringLiteral("false"))
        .arg(cr.context().keys().join(",")));

    if (!writeChangeRequestStatus(crId, cr.novelId(), "in_progress")) {
        emit changeRequestFailed(crId, m_lastError);
        return result;
    }
    LOG_DEBUG("ChangeRequest", QString("状态已切换为 in_progress: %1").arg(crId));

    try {
        emit progressChanged(crId, 10, 100, QStringLiteral("正在解析自然语言修改请求"));
        ChangeRequestDsl dsl = prepareChangeRequestDsl(cr);

        result["crId"] = crId;
        result["scope"] = dsl.scope;
        result["type"] = dsl.type;
        result["targetId"] = dsl.targetId;
        result["reason"] = dsl.reason;
        result["priority"] = dsl.priority;

        const QJsonArray opResults = executeDslOperations(crId, dsl);
        const int total = opResults.size();
        LOG_INFO("ChangeRequest", QString("执行 DSL: crId=%1, scope=%2, type=%3, targetId=%4, ops=%5")
            .arg(crId)
            .arg(dsl.scope)
            .arg(dsl.type)
            .arg(dsl.targetId)
            .arg(total));

        emit progressChanged(crId, 95, 100, QStringLiteral("正在写回修改结果"));
        result["results"] = opResults;
        result["status"] = "completed";
        LOG_DEBUG("ChangeRequest", QString("变更请求结果汇总: crId=%1, resultCount=%2")
            .arg(crId)
            .arg(opResults.size()));

        if (!writeChangeRequestStatus(crId, cr.novelId(), "completed")) {
            throw std::runtime_error(m_lastError.toStdString());
        }

        LOG_INFO("ChangeRequest", QString("变更请求执行完成: %1").arg(crId));
        emit changeRequestCompleted(crId, result);
        return result;
    } catch (const std::exception& e) {
        m_lastError = QString::fromUtf8(e.what());
        LOG_ERROR("ChangeRequest", QString("变更请求执行失败: %1 | error=%2").arg(crId, m_lastError));
        writeChangeRequestStatus(crId, cr.novelId(), "failed", m_lastError);
        emit changeRequestFailed(crId, m_lastError);
        return result;
    }
}

ChangeRequestDsl ChangeRequestService::prepareChangeRequestDsl(const ChangeRequest& cr)
{
    ChangeRequestDsl dsl = cr.dsl();
    ChangeRequestParseUtils::normalizeParsedDsl(dsl);
    resolveDslTarget(dsl, cr.novelId(), cr.context());

    QString validationError;
    if (ChangeRequestParseUtils::isValidParsedDsl(dsl, &validationError)) {
        LOG_DEBUG("ChangeRequest", QString("使用已有 DSL: crId=%1, scope=%2, type=%3, ops=%4")
            .arg(cr.id())
            .arg(dsl.scope)
            .arg(dsl.type)
            .arg(dsl.ops.size()));
        emit progressChanged(cr.id(), 25, 100, QStringLiteral("正在校验已生成的 CR-DSL"));
        return dsl;
    }

    LOG_DEBUG("ChangeRequest", QString("现有 DSL 无效，准备重新解析: crId=%1, error=%2")
        .arg(cr.id(), validationError));
    dsl = parseNaturalLanguage(cr.naturalLanguage(), cr.context());
    if (!ChangeRequestParseUtils::isValidParsedDsl(dsl, &validationError)) {
        const std::string errorText = validationError.toStdString();
        throw std::runtime_error(errorText);
    }

    emit progressChanged(cr.id(), 25, 100, QStringLiteral("正在保存 CR-DSL"));
    if (!saveChangeRequestDsl(cr.id(), cr.novelId(), dsl)) {
        throw std::runtime_error(m_lastError.toStdString());
    }
    LOG_DEBUG("ChangeRequest", QString("重新解析后的 DSL 已保存: crId=%1").arg(cr.id()));
    return dsl;
}

QJsonArray ChangeRequestService::executeDslOperations(const QString& crId, const ChangeRequestDsl& dsl)
{
    QJsonArray opResults;
    const int total = dsl.ops.size();
    emit progressChanged(crId, 40, 100, QStringLiteral("正在执行修改操作"));

    for (int i = 0; i < total; ++i) {
        const ChangeRequestOp& op = dsl.ops.at(i);
        LOG_INFO("ChangeRequest", QString("执行操作: crId=%1, step=%2/%3, action=%4")
            .arg(crId)
            .arg(i + 1)
            .arg(total)
            .arg(op.action));
        LOG_DEBUG("ChangeRequest", QString("操作参数: crId=%1, action=%2, paramsKeys=%3")
            .arg(crId)
            .arg(op.action)
            .arg(op.params.keys().join(",")));

        const int progress = ChangeRequestExecutionFlowUtils::computeOperationProgress(i, total);
        emit progressChanged(crId, progress, 100, QString("正在执行第 %1/%2 个操作").arg(i + 1).arg(total));

        QJsonObject opResult;
        opResult["action"] = op.action;
        const QJsonObject output = executeOperationByType(dsl, op, crId);
        opResult["status"] = "completed";
        opResult["output"] = output;
        opResults.append(opResult);

        LOG_DEBUG("ChangeRequest", QString("操作完成: crId=%1, action=%2, outputKeys=%3")
            .arg(crId)
            .arg(op.action)
            .arg(output.keys().join(",")));
    }

    return opResults;
}

QJsonObject ChangeRequestService::executeOperationByType(
    const ChangeRequestDsl& dsl,
    const ChangeRequestOp& op,
    const QString& crId)
{
    const QString normalizedType = ChangeRequestNormalization::normalizeType(dsl.type);

    if (normalizedType == QStringLiteral("art")) {
        return executeArtOperation(dsl, op, crId);
    }
    if (normalizedType == QStringLiteral("dialogue")) {
        return executeDialogueOperation(dsl, op, crId);
    }
    if (normalizedType == QStringLiteral("layout")) {
        return executeLayoutOperation(dsl, op, crId);
    }
    if (normalizedType == QStringLiteral("style")) {
        return executeStyleOperation(dsl, op, crId);
    }

    throw std::runtime_error(QString("Unsupported change request type: %1")
        .arg(normalizedType.isEmpty() ? dsl.type : normalizedType)
        .toStdString());
}

void ChangeRequestService::executeChangeRequestAsync(const QString& crId)
{
    if (crId.isEmpty()) {
        m_lastError = "Change request ID is empty";
        emit changeRequestFailed(crId, m_lastError);
        return;
    }

    LOG_INFO("ChangeRequest", QString("调度异步执行变更请求: %1").arg(crId));
    QtConcurrent::run([this, crId]() {
        executeChangeRequest(crId);
    });
}


ChangeRequestDsl ChangeRequestService::parseNaturalLanguage(
    const QString& naturalLanguage,
    const QJsonObject& context)
{
    const QJsonObject crDslSchema = ChangeRequestParseUtils::buildChangeRequestSchema();
    const int requestedPanelNumber = ChangeRequestTargetUtils::extractRequestedPanelNumber(naturalLanguage);
    LOG_DEBUG("ChangeRequest", QString("解析自然语言: textLength=%1, contextKeys=%2, schemaKeys=%3")
        .arg(naturalLanguage.length())
        .arg(context.keys().join(","))
        .arg(crDslSchema.keys().join(",")));
    LOG_DEBUG("ChangeRequest", QString("自然语言目标面板号: %1").arg(requestedPanelNumber));
    const QJsonObject result = m_qwenClient->parseChangeRequest(naturalLanguage, crDslSchema, context);
    
    if (result.isEmpty()) {
        m_lastError = m_qwenClient->lastError();
        LOG_ERROR("ChangeRequest", QString("解析自然语言失败: %1").arg(m_lastError));
        return ChangeRequestDsl();
    }

    LOG_DEBUG("ChangeRequest", QString("解析结果字段: %1").arg(result.keys().join(", ")));
    LOG_DEBUG("ChangeRequest", QString("解析结果原文: %1")
        .arg(QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)).left(1000)));

    ChangeRequestDsl dsl = ChangeRequestDsl::fromJson(result);
    ChangeRequestParseUtils::normalizeParsedDsl(dsl);
    normalizeChangeRequestExpression(dsl);
    normalizeChangeRequestArtEdits(dsl, naturalLanguage);
    resolveDslTarget(dsl, QString(), context);
    if (dsl.targetId.isEmpty() && dsl.scope == QStringLiteral("panel")) {
        if (requestedPanelNumber <= 0) {
            LOG_WARNING("ChangeRequest", QString("未能从自然语言解析 targetId: textLength=%1, contextKeys=%2")
                .arg(naturalLanguage.length())
                .arg(context.keys().join(",")));
        } else {
            LOG_WARNING("ChangeRequest", QString("未能解析到目标面板: targetPanelNumber=%1, contextKeys=%2")
                .arg(requestedPanelNumber)
                .arg(context.keys().join(",")));
        }
    }
    LOG_DEBUG("ChangeRequest", QString("DSL 归一化后: %1")
        .arg(QString::fromUtf8(QJsonDocument(dsl.toJson()).toJson(QJsonDocument::Compact))));
    QString validationError;
    if (!ChangeRequestParseUtils::isValidParsedDsl(dsl, &validationError)) {
        m_lastError = validationError;
        LOG_ERROR("ChangeRequest", QString("解析自然语言得到无效 DSL: %1 | raw keys: %2")
            .arg(m_lastError, result.keys().join(", ")));
        LOG_ERROR("ChangeRequest", QString("无效 DSL 内容: %1")
            .arg(QString::fromUtf8(QJsonDocument(dsl.toJson()).toJson(QJsonDocument::Compact))));
        return ChangeRequestDsl();
    }

    LOG_INFO("ChangeRequest", QString("解析 DSL 成功: scope=%1, type=%2, targetId=%3, ops=%4")
        .arg(dsl.scope)
        .arg(dsl.type)
        .arg(dsl.targetId)
        .arg(dsl.ops.size()));

    return dsl;
}

QJsonObject ChangeRequestService::executeArtOperation(
    const ChangeRequestDsl& dsl,
    const ChangeRequestOp& op,
    const QString& crId)
{
    Q_UNUSED(crId);
    Panel panel = requirePanelForDsl(dsl, QStringLiteral("art"));
    QString mode = resolveImageModeForPanel(op, panel);
    const QString action = ChangeRequestNormalization::normalizeAction(op.action);
    if (ChangeRequestExecutionUtils::isExpressionEditOperation(action, op)) {
        return executeSetExpressionOperation(op, panel, dsl.targetId, mode);
    }

    QJsonObject result;

    if (action == QStringLiteral("inpaint")
        || action == QStringLiteral("bg_swap")
        || action == QStringLiteral("repose")) {
        const Panel originalPanel = panel;
        QString prompt = op.params["prompt"].toString();
        const QString editIntent = op.params.value("editIntent").toString().trimmed();
        const QString rawPrompt = op.params.value(QStringLiteral("rawPrompt")).toString();
        const QString sourceText = op.params.value(QStringLiteral("sourceText")).toString();
        ImageService::EditHint editHint;
        if (action == QStringLiteral("bg_swap") || editIntent == QStringLiteral("replace_background")) {
            prompt = ChangeRequestIntentUtils::buildBackgroundEditPrompt(op.params);
        } else if (editIntent == QStringLiteral("replace_attribute")) {
            prompt = buildInpaintPromptForIntent(
                op.params,
                editIntent,
                rawPrompt);
        } else if (editIntent == QStringLiteral("replace_subject")) {
            prompt = buildInpaintPromptForIntent(
                op.params,
                editIntent,
                rawPrompt);
            if (isCharacterReplacementRequest(op.params, sourceText)) {
                editHint = buildSubjectReplacementEditHint(op.params);
            } else {
                editHint = buildLocalObjectReplacementEditHint();
            }
        } else if (PromptTargetUtils::isLocalObjectMovementIntent(editIntent)) {
            prompt = buildInpaintPromptForIntent(
                op.params,
                editIntent,
                rawPrompt);
            editHint = buildLocalObjectReplacementEditHint();
        } else if (editIntent == QStringLiteral("rotate_subject")
            || editIntent == QStringLiteral("change_pose")) {
            prompt = buildInpaintPromptForIntent(
                op.params,
                editIntent,
                rawPrompt);
        } else if (prompt.isEmpty()) {
            if (editIntent == QStringLiteral("remove_subject")) {
                prompt = buildInpaintPromptForIntent(op.params, editIntent);
            } else {
                prompt = panel.visualPrompt();
            }
        }

        if (!prompt.isEmpty()) {
            QJsonObject content = buildPanelContentForWrite(panel);
            content["visualPrompt"] = prompt;
            annotateArtEditContent(content, action, editIntent);
            if (action == QStringLiteral("bg_swap")) {
                content["backgroundEditPrompt"] = prompt;
            }
            panel.setVisualPrompt(prompt);
            panel.setContent(content);
            if (!updatePanel(panel)) {
                throw std::runtime_error(m_lastError.toStdString());
            }
        }

        QJsonObject generationResult;

        editHint.forceProvider = QStringLiteral("qwen");
        if (!runPanelImageGeneration(dsl.targetId, mode, generationResult, editHint)) {
            restorePanelAfterFailedArtEdit(originalPanel, dsl.targetId);
            throw std::runtime_error(m_lastError.toStdString());
        }

        result = generationResult;

    } else if (action == QStringLiteral("outpaint")
        || action == QStringLiteral("regen_panel")) {
        const QString editIntent = op.params.value("editIntent").toString().trimmed();
        if (PromptTargetUtils::isCompositionEditIntent(editIntent)) {
            const QString rawPrompt = op.params.value(QStringLiteral("rawPrompt")).toString().trimmed();
            const QString userPrompt = op.params.value(QStringLiteral("prompt")).toString().trimmed();
            const QString desc = !userPrompt.isEmpty() ? userPrompt : rawPrompt;
            if (!desc.isEmpty()) {
                QJsonObject content = buildPanelContentForWrite(panel);
                content["visualPrompt"] = desc;
                annotateArtEditContent(content, action, editIntent);
                panel.setVisualPrompt(desc);
                panel.setContent(content);
                if (!updatePanel(panel)) {
                    throw std::runtime_error(m_lastError.toStdString());
                }
            }
        }
        if (!runPanelImageGeneration(dsl.targetId, mode, result)) {
            throw std::runtime_error(m_lastError.toStdString());
        }

    } else if (action == QStringLiteral("add_effect")) {
        QString effect = op.params["effect"].toString("tone");
        double intensity = op.params["intensity"].toDouble(0.5);

        result["effect"] = effect;
        result["intensity"] = intensity;

    } else if (action == QStringLiteral("resize")) {
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

QJsonObject ChangeRequestService::executeSetExpressionOperation(
    const ChangeRequestOp& op,
    Panel panel,
    const QString& panelId,
    const QString& mode)
{
    QJsonObject result;

    const QString expression = op.params.value("expression").toString().trimmed();
    if (expression.isEmpty()) {
        throw std::runtime_error(u8"setExpression 操作需要有效的 expression 参数");
    }
    const QString normalizedExpression = ChangeRequestExpressionUtils::normalizeExpressionValue(expression);
    const QString finalExpression = normalizedExpression.isEmpty() ? expression : normalizedExpression;

    const Panel originalPanel = panel;

    QList<CharacterPose> characters = panel.characters();
    if (!characters.isEmpty()) {
        characters[0].expression = finalExpression;
        panel.setCharacters(characters);
    }

    QJsonObject content = buildPanelContentForWrite(panel);
    const QString prompt = buildExpressionEditPrompt(panel, finalExpression);
    content["visualPrompt"] = prompt;
    // 表情编辑也是局部编辑，必须设置 editIntent 让 PromptBuilder 走精简模式
    content["editIntent"] = QStringLiteral("set_expression");
    panel.setVisualPrompt(prompt);
    panel.setContent(content);
    if (!updatePanel(panel)) {
        throw std::runtime_error(m_lastError.toStdString());
    }

    QJsonObject generationResult;
    const ImageService::EditHint editHint = buildExpressionEditHint(finalExpression);
    if (!runPanelImageGeneration(panelId, mode, generationResult, editHint)) {
        restorePanelAfterFailedArtEdit(originalPanel, panelId);
        throw std::runtime_error(m_lastError.toStdString());
    }

    result = generationResult;
    result["action"] = QStringLiteral("setExpression");
    result["expression"] = finalExpression;
    return result;
}

QJsonObject ChangeRequestService::executeDialogueOperation(
    const ChangeRequestDsl& dsl,
    const ChangeRequestOp& op,
    const QString& crId)
{
    Q_UNUSED(crId);
    QJsonObject result;

    Panel panel = requirePanelForDsl(dsl, QStringLiteral("dialogue"));

    QList<DialogueLine> dialogues = panel.dialogue();

    // DSL schema uses "text"; "newText" kept as fallback for older DSL versions
    QString newText = op.params["text"].toString();
    if (newText.isEmpty()) newText = op.params["newText"].toString();
    QString speaker = op.params["speaker"].toString();
    QString dialogueId = op.params["dialogueId"].toString();

    const QString action = ChangeRequestNormalization::normalizeAction(op.action);

    if (action == QStringLiteral("rewrite_dialogue")) {
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
            // No dialogueId or speaker match — replace first dialogue if exists, otherwise append
            if (!dialogues.isEmpty()) {
                dialogues[0].text = newText;
                if (!speaker.isEmpty()) dialogues[0].speaker = speaker;
            } else {
                dialogues.append(newDialogue);
            }
        }

        if (!updatePanelDialogue(dsl.targetId, dialogues, newText)) {
            throw std::runtime_error(m_lastError.toStdString());
        }

        QJsonObject generationResult;
        ImageService::EditHint editHint;
        editHint.forceProvider = QStringLiteral("qwen");
        if (!runPanelImageGeneration(dsl.targetId, QStringLiteral("preview"), generationResult, editHint)) {
            throw std::runtime_error(m_lastError.toStdString());
        }
    }

    return ChangeRequestExecutionUtils::buildDialogueResult(
        dialogueId.isEmpty() ? QStringLiteral("dlg-1") : dialogueId,
        speaker,
        newText);
}

QJsonObject ChangeRequestService::executeLayoutOperation(
    const ChangeRequestDsl& dsl,
    const ChangeRequestOp& op,
    const QString& crId)
{
    Q_UNUSED(crId);
    QJsonObject result;

    Panel panel = requirePanelForDsl(dsl, QStringLiteral("layout"));

    const QString action = ChangeRequestNormalization::normalizeAction(op.action);
    const ChangeRequestExecutionUtils::LayoutReorderSpec reorderSpec =
        ChangeRequestExecutionUtils::normalizeLayoutReorderSpec(op);

    if (action == QStringLiteral("reorder")) {
        if (reorderSpec.hasPanelOrder()) {
            if (reorderSpec.panelOrder.isEmpty()) {
                throw std::runtime_error(u8"reorder 操作缺少有效的 panelOrder 参数");
            }

            LOG_DEBUG("ChangeRequest", QString("layout reorder by panelOrder: panelId=%1, page=%2, raw=%3, normalized=%4")
                .arg(panel.id())
                .arg(panel.page())
                .arg(QString::fromUtf8(QJsonDocument(op.params.value(QStringLiteral("panelOrder")).toArray()).toJson(QJsonDocument::Compact)))
                .arg(QString::fromUtf8(QJsonDocument(intListToJsonArray(reorderSpec.panelOrder)).toJson(QJsonDocument::Compact))));
            QString errorMessage;
            if (!m_db->transaction([&]() -> bool {
                    return applyPanelOrder(m_db, panel.storyboardId(), panel.page(), reorderSpec.panelOrder, &errorMessage);
                })) {
                throw std::runtime_error(errorMessage.isEmpty()
                                         ? u8"重新排序面板失败"
                                         : errorMessage.toStdString());
            }

            result["page"] = panel.page();
            result["panelOrder"] = intListToJsonArray(reorderSpec.panelOrder);
        } else if (reorderSpec.hasPermutation()) {
            LOG_DEBUG("ChangeRequest", QString("layout reorder by permutation: panelId=%1, page=%2, sourceIndices=%3, targetIndices=%4")
                .arg(panel.id())
                .arg(panel.page())
                .arg(QString::fromUtf8(QJsonDocument(intListToJsonArray(reorderSpec.sourceIndices)).toJson(QJsonDocument::Compact)))
                .arg(QString::fromUtf8(QJsonDocument(intListToJsonArray(reorderSpec.targetIndices)).toJson(QJsonDocument::Compact))));

            QString errorMessage;
            if (!m_db->transaction([&]() -> bool {
                    return applyPanelIndexPermutation(m_db, panel.storyboardId(), panel.page(), reorderSpec.sourceIndices, reorderSpec.targetIndices, &errorMessage);
                })) {
                throw std::runtime_error(errorMessage.isEmpty()
                                         ? u8"重新排序面板失败"
                                         : errorMessage.toStdString());
            }

            result["page"] = panel.page();
            result["sourceIndices"] = intListToJsonArray(reorderSpec.sourceIndices);
            result["targetIndices"] = intListToJsonArray(reorderSpec.targetIndices);
        } else {
            throw std::runtime_error(u8"reorder 操作缺少有效的 panelOrder 或 sourceIndices/targetIndices 参数");
        }

    } else if (action == QStringLiteral("resize")) {
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
    QJsonObject styleOverrides = buildStyleOverrides(op);

    if (dsl.targetId.isEmpty()) {
        throw std::runtime_error(u8"style 操作需要有效的 storyboardId/targetId");
    }

    if (!updateStoryboardStyle(dsl.targetId, styleOverrides)) {
        throw std::runtime_error(m_lastError.toStdString());
    }

    return styleOverrides;
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

bool ChangeRequestService::writeChangeRequestStatus(
    const QString& crId,
    const QString& novelId,
    const QString& status,
    const QString& error)
{
    LOG_DEBUG("ChangeRequest", QString("写回变更请求状态: crId=%1, novelId=%2, status=%3, hasError=%4")
        .arg(crId)
        .arg(novelId)
        .arg(status)
        .arg(error.isEmpty() ? QStringLiteral("false") : QStringLiteral("true")));
    QVariantMap updates;
    StatusWriteUtils::setStatus(updates, status);
    StatusWriteUtils::setErrorMessage(updates, error);

    QString whereClause = "id = ?";
    QVariantList whereValues;
    whereValues << crId;
    if (!novelId.isEmpty()) {
        whereClause += " AND novel_id = ?";
        whereValues << novelId;
    }

    const bool success = updateRecordWithTimestamp(m_db, "change_requests", updates, whereClause, whereValues, &m_lastError);
    if (!success) {
        LOG_ERROR("ChangeRequest", QString("写回变更请求状态失败: crId=%1, status=%2, error=%3")
            .arg(crId)
            .arg(status)
            .arg(m_lastError));
    }
    return success;
}

bool ChangeRequestService::saveChangeRequestDsl(
    const QString& crId,
    const QString& novelId,
    const ChangeRequestDsl& dsl)
{
    LOG_DEBUG("ChangeRequest", QString("保存 CR-DSL: crId=%1, novelId=%2, dsl=%3")
        .arg(crId)
        .arg(novelId)
        .arg(QString::fromUtf8(QJsonDocument(dsl.toJson()).toJson(QJsonDocument::Compact))));
    QVariantMap updates;
    updates["dsl"] = jsonToString(dsl.toJson());
    updates["status"] = "pending";

    QString whereClause = "id = ?";
    QVariantList whereValues;
    whereValues << crId;
    
    if (!novelId.isEmpty()) {
        whereClause += " AND novel_id = ?";
        whereValues << novelId;
    }

    const bool success = updateRecordWithTimestamp(m_db, "change_requests", updates, whereClause, whereValues, &m_lastError);
    if (!success) {
        LOG_ERROR("ChangeRequest", QString("保存 CR-DSL 失败: crId=%1, error=%2").arg(crId, m_lastError));
    } else {
        LOG_INFO("ChangeRequest", QString("CR-DSL 已保存: crId=%1").arg(crId));
    }
    return success;
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

Panel ChangeRequestService::requirePanelForDsl(const ChangeRequestDsl& dsl, const QString& operationKind)
{
    if (!ChangeRequestExecutionUtils::requiresTargetPanel(operationKind)) {
        throw std::runtime_error(QString("Unsupported operation kind: %1").arg(operationKind).toStdString());
    }

    if (dsl.targetId.isEmpty()) {
        throw std::runtime_error(QString("%1 operation requires targetId (panel ID)").arg(operationKind).toStdString());
    }

    Panel panel = loadPanelById(dsl.targetId);
    if (!panel.isValid()) {
        throw std::runtime_error(QString("Panel %1 does not exist.").arg(dsl.targetId).toStdString());
    }

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
    return updateRecordWithTimestamp(m_db, "panels", updates, "id = ?", QVariantList{panel.id()}, &m_lastError);
}

bool ChangeRequestService::updatePanelDialogue(
    const QString& panelId,
    const QList<DialogueLine>& dialogue,
    const QString& newText)
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
    // 对话修改也是编辑操作，需要设置 editIntent 让 PromptBuilder 走编辑精简模式
    // 否则会走批量生成分支导致整张图重绘而非局部编辑
    // 替换 visualPrompt 为对话编辑指令，让 AI 只改气泡文字不改场景
    // newText 为空时（如删除对话）不设置 editIntent，走批量生成重绘整图
    if (!newText.isEmpty()) {
        content["editIntent"] = QStringLiteral("rewrite_dialogue");
        content["visualPrompt"] = QString(
            "edit the speech bubble text to: %1, keep everything else in the image unchanged, "
            "do not change characters, background, composition or any other elements")
            .arg(newText);
    }

    QVariantMap updates;
    updates["content"] = jsonToString(content);
    return updateRecordWithTimestamp(m_db, "panels", updates, "id = ?", QVariantList{panelId}, &m_lastError);
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
    return updateRecordWithTimestamp(m_db, "panels", updates, "id = ?", QVariantList{panelId}, &m_lastError);
}

bool ChangeRequestService::updateStoryboardStyle(
    const QString& storyboardId,
    const QJsonObject& styleOverrides)
{
    QVariantMap updates;
    updates["style_overrides"] = jsonToString(styleOverrides);
    return updateRecordWithTimestamp(m_db, "storyboards", updates, "id = ?", QVariantList{storyboardId}, &m_lastError);
}

QString ChangeRequestService::resolveImageModeForPanel(const ChangeRequestOp& op, const Panel& panel) const
{
    return ChangeRequestExecutionUtils::normalizeImageMode(op.params.value("mode").toString(), panel.hasHdImage());
}

bool ChangeRequestService::runPanelImageGeneration(const QString& panelId,
                                                   const QString& mode,
                                                   QJsonObject& output,
                                                   const ImageService::EditHint& editHint)
{
    ImageService::GenerateMode generateMode = mode == "hd"
        ? ImageService::GenerateMode::HD
        : ImageService::GenerateMode::Preview;

    LOG_INFO("ChangeRequest", QString("开始生成面板图片: panelId=%1, mode=%2")
        .arg(panelId)
        .arg(mode));
    if (editHint.faceOnly) {
        QString strategyName = QStringLiteral("default");
        if (editHint.strategy == ImageService::EditHint::Strategy::FaceExpression) {
            strategyName = QStringLiteral("face_expression");
        } else if (editHint.strategy == ImageService::EditHint::Strategy::SubjectReplacement) {
            strategyName = QStringLiteral("subject_replacement");
        }
        LOG_DEBUG("ChangeRequest", QString("局部编辑提示: faceOnly=true, strategy=%1, expression=%2")
            .arg(strategyName)
            .arg(editHint.expression));
    }
    
    ImageService::GenerateResult genResult = m_imageService->regeneratePanelImage(panelId, generateMode, editHint);
    if (!genResult.success) {
        m_lastError = genResult.errorMessage;
        LOG_ERROR("ChangeRequest", QString("面板图片生成失败: panelId=%1, mode=%2, error=%3")
            .arg(panelId)
            .arg(mode)
            .arg(m_lastError));
        return false;
    }

    output = ChangeRequestExecutionUtils::buildImageResult(genResult.s3Key, mode, genResult.imageUrl);
    LOG_INFO("ChangeRequest", QString("面板图片生成完成: panelId=%1, mode=%2, s3Key=%3")
        .arg(panelId)
        .arg(mode)
        .arg(genResult.s3Key));
    return true;
}

bool ChangeRequestService::executeDirectPanelEdit(const QString& panelId,
                                                   const QString& editPrompt,
                                                   const QString& mode,
                                                   QJsonObject& output)
{
    LOG_INFO("ChangeRequest", QString("开始轻量级面板编辑: panelId=%1, mode=%2, promptLen=%3")
        .arg(panelId)
        .arg(mode)
        .arg(editPrompt.length()));

    Panel panel = StoryboardService::instance()->getPanel(panelId);
    if (panel.id().isEmpty()) {
        m_lastError = QStringLiteral("面板不存在");
        LOG_ERROR("ChangeRequest", QString("面板不存在: %1").arg(panelId));
        return false;
    }

    QString imagePath = (mode == "hd") ? panel.hdS3Key() : panel.previewS3Key();
    if (imagePath.isEmpty()) {
        m_lastError = QStringLiteral("面板图片路径为空");
        LOG_ERROR("ChangeRequest", QString("面板图片路径为空: panelId=%1, mode=%2").arg(panelId).arg(mode));
        return false;
    }

    QByteArray originalImage;
    if (imagePath.startsWith("http")) {
        originalImage = QwenImageClient::instance()->downloadImage(imagePath);
    } else {
        originalImage = FileStorage::instance()->loadPanelImage(imagePath);
    }

    if (originalImage.isEmpty()) {
        m_lastError = QStringLiteral("无法加载面板图片");
        LOG_ERROR("ChangeRequest", QString("无法加载面板图片: %1").arg(imagePath));
        return false;
    }

    QwenImageClient::EditOptions editOptions;
    
    QString enhancedPrompt = editPrompt;
    if (editPrompt.contains(QStringLiteral("移除")) || editPrompt.contains(QStringLiteral("删除"))) {
        enhancedPrompt = editPrompt + QStringLiteral("。注意：只修改指定内容，其他所有元素（背景、其他角色、构图、光影）必须完全保持原样不变。");
    } else if (editPrompt.contains(QStringLiteral("替换")) || editPrompt.contains(QStringLiteral("换成"))) {
        enhancedPrompt = editPrompt + QStringLiteral("。注意：只修改指定内容，其他所有元素必须完全保持原样不变。");
    }
    
    editOptions.prompt = enhancedPrompt;
    editOptions.sourceImage = originalImage;
    editOptions.negativePrompt = QStringLiteral("nsfw, blurry, low quality, extra limbs, deformed hands, text watermark, logo, changing background, changing other characters, altering composition");

    QImage tempImage;
    tempImage.loadFromData(originalImage);
    if (!tempImage.isNull()) {
        editOptions.width = tempImage.width();
        editOptions.height = tempImage.height();
        editOptions.size = QwenImageClient::ImageSize::Custom;
        LOG_DEBUG("ChangeRequest", QString("原图尺寸: %1x%2").arg(editOptions.width).arg(editOptions.height));
    }

    LOG_INFO("ChangeRequest", QString("当前图像提供商: %1, 编辑操作强制使用千问编辑 API")
        .arg(ImageService::instance()->getProviderName()));

    QByteArray editedImage;
    LOG_INFO("ChangeRequest", QString("调用千问图像编辑 API: prompt=%1").arg(editPrompt.left(50)));
    
    QwenImageClient::GenerateResult result = QwenImageClient::instance()->edit(editOptions);
    
    if (!result.success) {
        m_lastError = result.errorMessage;
        LOG_ERROR("ChangeRequest", QString("千问编辑失败: %1").arg(result.errorMessage));
        return false;
    }
    
    editedImage = extractImageFromResult("qwen", result.imageData, result.imageUrl);

    if (editedImage.isEmpty()) {
        m_lastError = QStringLiteral("编辑后的图片数据为空");
        LOG_ERROR("ChangeRequest", m_lastError);
        return false;
    }

    LOG_INFO("ChangeRequest", QString("编辑后图片大小: %1 bytes, 准备保存到 preview 和 hd").arg(editedImage.size()));

    QString previewPath = FileStorage::instance()->savePanelImage(panelId, editedImage, "preview");
    LOG_INFO("ChangeRequest", QString("保存 preview 图片结果: %1").arg(previewPath.isEmpty() ? "失败" : previewPath));
    
    QString hdPath = FileStorage::instance()->savePanelImage(panelId, editedImage, "hd");
    LOG_INFO("ChangeRequest", QString("保存 hd 图片结果: %1").arg(hdPath.isEmpty() ? "失败" : hdPath));
    
    if (previewPath.isEmpty() && hdPath.isEmpty()) {
        m_lastError = FileStorage::instance()->lastError();
        LOG_ERROR("ChangeRequest", QString("保存编辑后的图片失败: %1").arg(m_lastError));
        return false;
    }

    if (!previewPath.isEmpty()) {
        QString oldPreviewS3Key = panel.previewS3Key();
        QString oldPreviewUrl = panel.previewUrl();
        panel.setPreviewS3Key(previewPath);
        panel.setPreviewUrl(FileStorage::instance()->getFullPath(previewPath));
        LOG_INFO("ChangeRequest", QString("已更新 preview 图片: oldS3Key=%1, newS3Key=%2, oldUrl=%3, newUrl=%4")
            .arg(oldPreviewS3Key, previewPath, oldPreviewUrl.left(50), panel.previewUrl().left(50)));
    }
    
    if (!hdPath.isEmpty()) {
        QString oldHdS3Key = panel.hdS3Key();
        QString oldHdUrl = panel.hdUrl();
        panel.setHdS3Key(hdPath);
        panel.setHdUrl(FileStorage::instance()->getFullPath(hdPath));
        LOG_INFO("ChangeRequest", QString("已更新 hd 图片: oldS3Key=%1, newS3Key=%2, oldUrl=%3, newUrl=%4")
            .arg(oldHdS3Key, hdPath, oldHdUrl.left(50), panel.hdUrl().left(50)));
    }

    LOG_INFO("ChangeRequest", QString("准备更新面板数据库: panelId=%1, previewS3Key=%2, hdS3Key=%3")
        .arg(panelId, panel.previewS3Key(), panel.hdS3Key()));

    if (!updatePanel(panel)) {
        LOG_ERROR("ChangeRequest", QString("更新面板数据库失败: %1").arg(m_lastError));
        return false;
    }

    LOG_INFO("ChangeRequest", QString("面板数据库更新成功: panelId=%1").arg(panelId));

    QString effectivePath = !hdPath.isEmpty() ? hdPath : previewPath;
    output = ChangeRequestExecutionUtils::buildImageResult(effectivePath, mode, FileStorage::instance()->getFullPath(effectivePath));
    LOG_INFO("ChangeRequest", QString("轻量级面板编辑完成: panelId=%1, previewPath=%2, hdPath=%3, outputUrl=%4")
        .arg(panelId)
        .arg(previewPath)
        .arg(hdPath)
        .arg(output.value("imageUrl").toString().left(80)));
    return true;
}

void ChangeRequestService::restorePanelAfterFailedArtEdit(const Panel& originalPanel, const QString& panelId)
{
    if (!updatePanel(originalPanel)) {
        LOG_ERROR("ChangeRequest", QString("Failed to restore panel %1 after edit failure: %2")
            .arg(panelId).arg(m_lastError));
    }
}

QJsonObject ChangeRequestService::buildStyleOverrides(const ChangeRequestOp& op) const
{
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

    return styleOverrides;
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

ImageService::EditHint ChangeRequestService::buildExpressionEditHint(const QString& expression) const
{
    const auto spec = FaceEditPromptUtils::buildFaceExpressionEditSpec(QString(), expression, QString());

    ImageService::EditHint hint;
    hint.strategy = ImageService::EditHint::Strategy::FaceExpression;
    hint.faceOnly = true;
    hint.expression = expression;
    hint.subjectDescription = spec.targetDescription;
    hint.preserveDescription = spec.preserveDescription;
    hint.regionScale = 1.26;
    hint.maskCoreRatio = 0.88;
    hint.maskFeatherRatio = 0.12;
    return hint;
}

ImageService::EditHint ChangeRequestService::buildSubjectReplacementEditHint(const QJsonObject& params) const
{
    const SubjectReplacementResolution resolution = resolveSubjectReplacementResolution(
        params,
        params.value(QStringLiteral("sourceText")).toString());

    const auto spec = LocalEditPromptUtils::buildLocalReplacementEditSpec(
        resolution.sourceSubject.isEmpty() ? QStringLiteral("原主体") : resolution.sourceSubject,
        resolution.replacementTarget,
        QStringLiteral("保持其余人物、构图、透视、光线、色调和画幅比例不变"),
        1.30);

    ImageService::EditHint hint;
    hint.strategy = ImageService::EditHint::Strategy::SubjectReplacement;
    hint.faceOnly = true;
    hint.subjectDescription = spec.targetDescription;
    hint.replacementDescription = spec.changeTargetDescription;
    hint.preserveDescription = spec.preserveDescription;
    hint.regionScale = 1.34;
    hint.maskCoreRatio = 0.86;
    hint.maskFeatherRatio = 0.14;
    return hint;
}

ImageService::EditHint ChangeRequestService::buildLocalObjectReplacementEditHint() const
{
    ImageService::EditHint hint;
    hint.strategy = ImageService::EditHint::Strategy::Default;
    hint.faceOnly = false;
    hint.regionScale = 1.0;
    hint.maskCoreRatio = 0.90;
    hint.maskFeatherRatio = 0.10;
    return hint;
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
