#ifndef CHANGEREQUESTEXECUTIONUTILS_H
#define CHANGEREQUESTEXECUTIONUTILS_H

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QSet>
#include <QString>
#include "models/ChangeRequest.h"

namespace ChangeRequestExecutionUtils {

inline QString normalizeType(const QString& type)
{
    return type.trimmed().toLower();
}

inline QString normalizeArtAction(const QString& action);

inline bool requiresTargetPanel(const QString& type)
{
    const QString normalizedType = normalizeType(type);
    return normalizedType == QStringLiteral("art")
        || normalizedType == QStringLiteral("dialogue")
        || normalizedType == QStringLiteral("layout");
}

struct LayoutReorderSpec
{
    QList<int> sourceIndices;
    QList<int> targetIndices;
    QList<int> panelOrder;

    bool hasPanelOrder() const { return !panelOrder.isEmpty(); }
    bool hasPermutation() const { return !sourceIndices.isEmpty() && sourceIndices.size() == targetIndices.size(); }
    bool isValid() const { return hasPanelOrder() || hasPermutation(); }
};

inline QList<int> jsonArrayToIntList(const QJsonArray& array)
{
    QList<int> values;
    values.reserve(array.size());
    for (const QJsonValue& value : array) {
        values.append(value.toInt());
    }
    return values;
}

inline QList<int> normalizePanelOrderNumbers(const QJsonArray& panelOrder)
{
    QList<int> normalizedOrder;
    if (panelOrder.isEmpty()) {
        return normalizedOrder;
    }

    bool containsZero = false;
    for (const QJsonValue& value : panelOrder) {
        if (value.toInt() == 0) {
            containsZero = true;
            break;
        }
    }

    QSet<int> seen;
    for (const QJsonValue& value : panelOrder) {
        const int rawValue = value.toInt();
        const int panelNumber = containsZero ? rawValue + 1 : rawValue;
        if (panelNumber <= 0 || seen.contains(panelNumber)) {
            continue;
        }
        seen.insert(panelNumber);
        normalizedOrder.append(panelNumber);
    }

    return normalizedOrder;
}

inline LayoutReorderSpec normalizeLayoutReorderSpec(const ChangeRequestOp& op)
{
    LayoutReorderSpec spec;

    if (op.params.contains(QStringLiteral("panelOrder"))) {
        spec.panelOrder = normalizePanelOrderNumbers(op.params.value(QStringLiteral("panelOrder")).toArray());
        return spec;
    }

    if (op.params.contains(QStringLiteral("sourceIndices"))
        && op.params.contains(QStringLiteral("targetIndices"))) {
        spec.sourceIndices = jsonArrayToIntList(op.params.value(QStringLiteral("sourceIndices")).toArray());
        spec.targetIndices = jsonArrayToIntList(op.params.value(QStringLiteral("targetIndices")).toArray());
        return spec;
    }

    if ((op.params.contains(QStringLiteral("fromIndex")) || op.params.contains(QStringLiteral("sourceIndex")))
        && (op.params.contains(QStringLiteral("toIndex")) || op.params.contains(QStringLiteral("targetIndex")))) {
        const int sourceIndex = op.params.contains(QStringLiteral("fromIndex"))
            ? op.params.value(QStringLiteral("fromIndex")).toInt()
            : op.params.value(QStringLiteral("sourceIndex")).toInt();
        const int targetIndex = op.params.contains(QStringLiteral("toIndex"))
            ? op.params.value(QStringLiteral("toIndex")).toInt()
            : op.params.value(QStringLiteral("targetIndex")).toInt();
        spec.sourceIndices.append(sourceIndex);
        spec.targetIndices.append(targetIndex);
        return spec;
    }

    return spec;
}

inline QString normalizeArtAction(const QString& action)
{
    const QString trimmed = action.trimmed();
    const QString lower = trimmed.toLower();

    if (lower == QStringLiteral("setexpression")
        || lower == QStringLiteral("updateexpression")
        || lower == QStringLiteral("set_expression")
        || lower == QStringLiteral("update_expression")
        || lower == QStringLiteral("set-expression")
        || lower == QStringLiteral("update-expression")) {
        return QStringLiteral("setExpression");
    }
    return lower;
}

inline bool hasExpressionParameter(const ChangeRequestOp& op)
{
    return !op.params.value("expression").toString().trimmed().isEmpty();
}

inline bool isExpressionEditOperation(const QString& action, const ChangeRequestOp& op)
{
    const QString normalizedAction = normalizeArtAction(action);
    if (normalizedAction == QStringLiteral("setExpression")) {
        return true;
    }
    if (!hasExpressionParameter(op)) {
        return false;
    }

    return normalizedAction == QStringLiteral("inpaint")
        || normalizedAction == QStringLiteral("repose");
}

inline QString normalizeImageMode(const QString& requestedMode, bool hasHdImage)
{
    QString mode = requestedMode.trimmed();
    if (mode.isEmpty()) {
        return hasHdImage ? QStringLiteral("hd") : QStringLiteral("preview");
    }
    return mode;
}

inline QJsonObject buildImageResult(const QString& s3Key, const QString& mode, const QString& imageUrl)
{
    QJsonObject result;
    result["s3Key"] = s3Key;
    result["mode"] = mode;
    result["imageUrl"] = imageUrl;
    return result;
}

inline QJsonObject buildDialogueResult(const QString& dialogueId, const QString& speaker, const QString& text)
{
    QJsonObject result;
    result["dialogueId"] = dialogueId;
    result["speaker"] = speaker;
    result["text"] = text;
    return result;
}

inline QJsonObject buildLayoutResult(int fromIndex, int toIndex, int page)
{
    QJsonObject result;
    result["page"] = page;
    result["fromIndex"] = fromIndex;
    result["toIndex"] = toIndex;
    return result;
}

}

#endif // CHANGEREQUESTEXECUTIONUTILS_H
