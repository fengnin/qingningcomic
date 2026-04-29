#ifndef CHANGEREQUESTPARSEUTILS_H
#define CHANGEREQUESTPARSEUTILS_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include "utils/ChangeRequestIntentUtils.h"
#include "utils/PromptTargetUtils.h"
#include "models/ChangeRequest.h"

namespace ChangeRequestParseUtils {

inline QStringList getValidActionsForType(const QString& type);
inline bool isActionValidForType(const QString& action, const QString& type);
inline bool containsAnyKeyword(const QString& text, const QStringList& keywords)
{
    for (const QString& keyword : keywords) {
        if (text.contains(keyword.toLower())) {
            return true;
        }
    }
    return false;
}

inline QJsonObject buildChangeRequestSchema()
{
    QJsonObject schema;
    schema["$schema"] = QStringLiteral("http://json-schema.org/draft-07/schema#");
    schema["title"] = QStringLiteral("CR-DSL Schema");
    schema["type"] = QStringLiteral("object");

    QJsonObject properties;
    properties["scope"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{
            QStringLiteral("global"),
            QStringLiteral("character"),
            QStringLiteral("panel"),
            QStringLiteral("page")
        }},
        {"description", QStringLiteral("修改作用域")}
    };
    properties["type"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{
            QStringLiteral("art"),
            QStringLiteral("dialogue"),
            QStringLiteral("layout"),
            QStringLiteral("style")
        }},
        {"description", QStringLiteral("修改类型")}
    };
    properties["targetId"] = QJsonObject{{"type", "string"}};
    properties["reason"] = QJsonObject{{"type", "string"}};
    properties["priority"] = QJsonObject{
        {"type", "string"},
        {"enum", QJsonArray{
            QStringLiteral("low"),
            QStringLiteral("medium"),
            QStringLiteral("high"),
            QStringLiteral("urgent")
        }}
    };
    properties["ops"] = QJsonObject{
        {"type", "array"},
        {"items", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"action", QJsonObject{
                    {"type", "string"},
                    {"enum", QJsonArray{
                        QStringLiteral("inpaint"),
                        QStringLiteral("outpaint"),
                        QStringLiteral("bg_swap"),
                        QStringLiteral("repose"),
                        QStringLiteral("regen_panel"),
                        QStringLiteral("setExpression"),
                        QStringLiteral("add_effect"),
                        QStringLiteral("resize"),
                        QStringLiteral("update_style"),
                        QStringLiteral("rewrite_dialogue"),
                        QStringLiteral("reorder")
                    }}
                }},
                {"params", QJsonObject{{"type", "object"}}}
            }}
        }}
    };
    schema["properties"] = properties;

    schema["required"] = QJsonArray{
        QStringLiteral("scope"),
        QStringLiteral("type"),
        QStringLiteral("ops")
    };

    return schema;
}

inline QString buildChangeRequestSystemPrompt()
{
    return QStringLiteral(
        "你是一个漫画修改助手。\n\n"
        "你的任务是将用户的自然语言修改请求转换为结构化的 CR-DSL（Change Request Domain Specific Language）。\n\n"
        "CR-DSL 包含以下要素：\n"
        "- scope: 修改范围\n"
        "- targetId: 目标 ID（如果适用）\n"
        "- type: 修改类型\n"
        "- ops: 操作列表，每个操作包含 action 和 params\n\n"
        "type 与 action 的对应关系：\n"
        "- type=art: inpaint, outpaint, bg_swap, repose, regen_panel, setExpression, add_effect, resize\n"
        "- type=dialogue: rewrite_dialogue\n"
        "- type=layout: reorder, resize\n"
        "- type=style: update_style\n\n"
        "layout/reorder 的标准输出规则：\n"
        "- 如果是完整顺序重排，使用 panelOrder\n"
        "- 如果是交换少量面板，使用 sourceIndices / targetIndices\n"
        "- sourceIndices / targetIndices 使用当前页内的 0-based panel_index\n"
        "- panelOrder 使用 1-based 面板编号\n"
        "- 不要主动生成 fromIndex / toIndex，保留给兼容层处理\n\n"
        "可用的 action 详细说明：\n"
        "- inpaint: 局部重绘（需要遮罩区域）；如果用户要求去掉/删除某个角色或物体，prompt 必须明确写出“移除主体、补全背景、保留其余内容”，不要只写局部重绘\n"
        "- outpaint: 扩展画面\n"
        "- bg_swap: 替换背景\n"
        "- repose: 改变角色姿势\n"
        "- regen_panel: 重新生成整个面板\n"
        "- setExpression: 修改角色表情（params.expression 指定目标表情）\n"
        "- editIntent: 可选的意图标记；去除主体使用 remove_subject，通用局部替换使用 replace_subject，局部移动/放置使用 move_subject，局部插入使用 insert_subject，局部属性修改使用 replace_attribute，背景修改使用 replace_background，表情修改使用 set_expression\n"
        "- add_effect: 添加特效（params.effect 指定特效类型，params.intensity 指定强度 0-1）\n"
        "- resize: 调整面板尺寸（params.width 和 params.height 指定尺寸）\n"
        "- rewrite_dialogue: 重写对白\n"
        "- reorder: 重新排序\n\n"
        "如果是角色表情修改，请把 expression 归一为以下规范值之一：\n"
        "- neutral\n"
        "- happy\n"
        "- sad\n"
        "- angry\n"
        "- surprised\n"
        "- scared\n"
        "- determined\n"
        "- confused\n"
        "- embarrassed\n"
        "- smirking\n\n"
        "如果请求的核心目标是修改角色表情，请优先使用 setExpression，"
        "不要把表情修改误写成 repose、outpaint 或 regen_panel。\n\n"
        "请根据用户的自然语言请求，生成符合 Schema 的 CR-DSL JSON。");
}

inline QString buildChangeRequestUserPrompt(const QString& naturalLanguage, const QJsonObject& context)
{
    QString prompt = QStringLiteral("自然语言修改需求：\n%1").arg(naturalLanguage);
    if (!context.isEmpty()) {
        prompt += QStringLiteral("\n上下文信息：\n%1")
            .arg(QString::fromUtf8(QJsonDocument(context).toJson(QJsonDocument::Indented)));
    }
    prompt += QStringLiteral(
        "\n\n请输出符合 CR-DSL JSON 的内容，并尽量补全：\n"
        "- scope：根据修改范围选择 global / character / panel / page\n"
        "- type：根据修改类型选择 art / dialogue / layout / style\n"
        "- action：根据 type 选择对应的操作（art→inpaint/setExpression等，dialogue→rewrite_dialogue，layout→reorder/resize，style→update_style）\n"
        "- targetId：如果上下文里提供了 targetPanelId / targetPanelNumber，请直接映射到对应 ID\n"
        "- targetId：如果 type=style，请优先映射到 storyboardId\n"
        "- targetId：如果能明确到具体对象，请填写对应 ID\n"
        "- params.editIntent：如果属于局部编辑，尽量填写 remove_subject / replace_subject / move_subject / insert_subject / replace_attribute / replace_background / set_expression\n"
        "- params.maskRegion：如果是局部替换，请填写最小、最具体的被替换对象，不要写容器、整个人物或整页场景\n"
        "- params.prompt：尽量保留原始自然语言中的具体对象名，例如“桌上的水杯”优先于“桌上”\n"
        "- ops：至少包含一条可执行操作，action 必须与 type 匹配");
    return prompt;
}

inline bool isValidParsedDsl(const ChangeRequestDsl& dsl, QString* errorMessage = nullptr)
{
    const QString scope = ChangeRequestNormalization::normalizeScope(dsl.scope);
    const QString type = ChangeRequestNormalization::normalizeType(dsl.type);

    if (!ChangeRequestNormalization::isSupportedScope(scope)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Parsed DSL has invalid scope");
        }
        return false;
    }

    if (!ChangeRequestNormalization::isSupportedType(type)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Parsed DSL has invalid type");
        }
        return false;
    }

    if (dsl.targetId.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Parsed DSL is missing targetId");
        }
        return false;
    }

    if (dsl.ops.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Parsed DSL has no operations");
        }
        return false;
    }

    for (const ChangeRequestOp& op : dsl.ops) {
        const QString normalizedAction = ChangeRequestNormalization::normalizeAction(op.action);
        if (normalizedAction.isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Parsed DSL contains an operation without action");
            }
            return false;
        }

        if (!isActionValidForType(normalizedAction, type)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Action '%1' is not valid for type '%2'")
                    .arg(normalizedAction, type);
            }
            return false;
        }
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

inline QStringList getValidActionsForType(const QString& type)
{
    const QString normalizedType = ChangeRequestNormalization::normalizeType(type);
    static const QStringList artActions = {
        QStringLiteral("inpaint"),
        QStringLiteral("outpaint"),
        QStringLiteral("bg_swap"),
        QStringLiteral("repose"),
        QStringLiteral("regen_panel"),
        QStringLiteral("setExpression"),
        QStringLiteral("add_effect"),
        QStringLiteral("resize")
    };
    static const QStringList dialogueActions = {
        QStringLiteral("rewrite_dialogue")
    };
    static const QStringList layoutActions = {
        QStringLiteral("reorder"),
        QStringLiteral("resize")
    };
    static const QStringList styleActions = {
        QStringLiteral("update_style")
    };

    if (normalizedType == QStringLiteral("art")) {
        return artActions;
    }
    if (normalizedType == QStringLiteral("dialogue")) {
        return dialogueActions;
    }
    if (normalizedType == QStringLiteral("layout")) {
        return layoutActions;
    }
    if (normalizedType == QStringLiteral("style")) {
        return styleActions;
    }
    return QStringList();
}

inline bool isValidAction(const QString& action)
{
    static const QStringList allValidActions = {
        QStringLiteral("inpaint"),
        QStringLiteral("outpaint"),
        QStringLiteral("bg_swap"),
        QStringLiteral("repose"),
        QStringLiteral("regen_panel"),
        QStringLiteral("setExpression"),
        QStringLiteral("add_effect"),
        QStringLiteral("resize"),
        QStringLiteral("update_style"),
        QStringLiteral("rewrite_dialogue"),
        QStringLiteral("reorder")
    };
    return allValidActions.contains(ChangeRequestNormalization::normalizeAction(action));
}

inline bool isActionValidForType(const QString& action, const QString& type)
{
    const QStringList validActions = getValidActionsForType(type);
    if (validActions.isEmpty()) {
        return false;
    }
    return validActions.contains(ChangeRequestNormalization::normalizeAction(action));
}

inline QString inferDefaultScopeForType(const QString& type)
{
    if (type == QStringLiteral("art")
        || type == QStringLiteral("dialogue")
        || type == QStringLiteral("layout")) {
        return QStringLiteral("panel");
    }

    if (type == QStringLiteral("style")) {
        return QStringLiteral("global");
    }

    return QStringLiteral("global");
}

inline QString inferEditIntentFromText(const QString& text)
{
    const QString lower = text.trimmed().toLower();
    if (lower.isEmpty()) {
        return QString();
    }

    if (PromptTargetUtils::isRemovalIntentText(lower)) {
        return QStringLiteral("remove_subject");
    }

    static const QStringList placementKeywords = {
        QString::fromUtf8("移动"),
        QString::fromUtf8("挪动"),
        QString::fromUtf8("搬到"),
        QString::fromUtf8("搬进"),
        QString::fromUtf8("放入"),
        QString::fromUtf8("放进"),
        QString::fromUtf8("放在"),
        QString::fromUtf8("放到"),
        QString::fromUtf8("摆到"),
        QString::fromUtf8("摆在"),
        QString::fromUtf8("移到"),
        QString::fromUtf8("移至"),
        QString::fromUtf8("挪到"),
        QString::fromUtf8("加到"),
        QString::fromUtf8("塞进"),
        QString::fromUtf8("置于"),
        QString::fromUtf8("insert"),
        QString::fromUtf8("place"),
        QString::fromUtf8("put")
    };
    if (containsAnyKeyword(lower, placementKeywords)) {
        return QStringLiteral("move_subject");
    }

    static const QStringList replacementKeywords = {
        QString::fromUtf8("替换"),
        QString::fromUtf8("换成"),
        QString::fromUtf8("改成"),
        QString::fromUtf8("变成"),
        QString::fromUtf8("用"),
        QStringLiteral("replace"),
        QStringLiteral("swap")
    };

    static const QStringList backgroundChangeKeywords = {
        QString::fromUtf8("改"),
        QString::fromUtf8("换"),
        QString::fromUtf8("替换"),
        QString::fromUtf8("变"),
        QStringLiteral("replace"),
        QStringLiteral("change"),
        QStringLiteral("swap")
    };
    if (ChangeRequestIntentUtils::isBackgroundEditIntentText(lower)
        && containsAnyKeyword(lower, backgroundChangeKeywords)) {
        return QStringLiteral("replace_background");
    }

    if (ChangeRequestIntentUtils::isAttributeEditIntentText(lower)
        && (containsAnyKeyword(lower, replacementKeywords)
            || containsAnyKeyword(lower, backgroundChangeKeywords))) {
        return QStringLiteral("replace_attribute");
    }

    static const QStringList expressionKeywords = {
        QString::fromUtf8("表情"),
        QString::fromUtf8("神情"),
        QString::fromUtf8("神态"),
        QString::fromUtf8("脸色"),
        QStringLiteral("expression")
    };
    if (containsAnyKeyword(lower, expressionKeywords)) {
        return QStringLiteral("set_expression");
    }

    if (containsAnyKeyword(lower, replacementKeywords)) {
        return QStringLiteral("replace_subject");
    }

    return QString();
}

inline void normalizeParsedDsl(ChangeRequestDsl& dsl)
{
    ChangeRequestNormalization::normalize(dsl);
    if (dsl.scope.isEmpty()) {
        dsl.scope = inferDefaultScopeForType(dsl.type);
    }

    QString sourceText = dsl.reason;
    if (sourceText.isEmpty()) {
        for (const ChangeRequestOp& op : dsl.ops) {
            const QString prompt = op.params.value("prompt").toString().trimmed();
            if (!prompt.isEmpty()) {
                sourceText = prompt;
                break;
            }
        }
    }

    const QString intent = inferEditIntentFromText(sourceText);
    if (intent.isEmpty()) {
        return;
    }

    for (ChangeRequestOp& op : dsl.ops) {
        if (!op.params.contains("editIntent")) {
            op.params["editIntent"] = intent;
        }
    }
}

}

#endif // CHANGEREQUESTPARSEUTILS_H
