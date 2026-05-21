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
    QStringList sections;
    sections << QStringLiteral("你是一个漫画修改助手。");
    sections << QStringLiteral("你的任务是将用户的自然语言修改请求转换为结构化的 CR-DSL（Change Request Domain Specific Language）。");
    sections << QStringLiteral("CR-DSL 包含以下要素：\n- scope: 修改范围\n- targetId: 目标 ID（如果适用）\n- type: 修改类型\n- ops: 操作列表，每个操作包含 action 和 params");
    sections << QStringLiteral("type 与 action 的对应关系：\n- type=art: inpaint, outpaint, bg_swap, repose, regen_panel, setExpression, add_effect, resize\n- type=dialogue: rewrite_dialogue\n- type=layout: reorder, resize\n- type=style: update_style");
    sections << QStringLiteral("layout/reorder 的标准输出规则：\n- 如果是完整顺序重排，使用 panelOrder\n- 如果是交换少量面板，使用 sourceIndices / targetIndices\n- sourceIndices / targetIndices 使用当前页内的 0-based panel_index\n- panelOrder 使用 1-based 面板编号\n- 不要主动生成 fromIndex / toIndex，保留给兼容层处理");
    sections << QStringLiteral("可用的 action 详细说明：\n- inpaint: 局部重绘（需要遮罩区域）；如果用户要求去掉/删除某个角色或物体，prompt 必须明确写出“移除主体、补全背景、保留其余内容”，不要只写局部重绘\n- outpaint: 扩展画面\n- bg_swap: 替换背景\n- repose: 改变角色姿势\n- regen_panel: 重新生成整个面板\n- setExpression: 修改角色表情（params.expression 指定目标表情）\n- editIntent: 可选的意图标记；去除主体使用 remove_subject，通用局部替换使用 replace_subject，局部移动/放置使用 move_subject，局部插入使用 insert_subject，局部属性修改使用 replace_attribute，背景修改使用 replace_background，表情修改使用 set_expression\n- add_effect: 添加特效（params.effect 指定特效类型，params.intensity 指定强度 0-1）\n- resize: 调整面板尺寸（params.width 和 params.height 指定尺寸）\n- rewrite_dialogue: 重写对白\n- reorder: 重新排序");
    sections << QStringLiteral("如果是角色表情修改，请把 expression 归一为以下规范值之一：\n- neutral\n- happy\n- sad\n- angry\n- surprised\n- scared\n- determined\n- confused\n- embarrassed\n- smirking");
    sections << QStringLiteral("如果请求的核心目标是修改角色表情，请优先使用 setExpression，不要把表情修改误写成 repose、outpaint 或 regen_panel。");
    sections << QStringLiteral("解析时必须遵守以下绝对规则：\n- 只允许修改用户明确提到的对象和属性，未提及的内容一律保持原样。\n- 如果用户说的是“对白、台词、说的话、文字内容、文本”，优先视为 dialogue（面板对白），而不是 art。\n- 如果用户明确指向“图上的字、书页上的字、牌子上的字、海报上的字”，才将其视为 art。\n- 如果请求中只提到“改文字”但上下文存在面板对白，则优先映射到 dialogue。\n- 如果请求要求修改图像内容，必须在 prompt 中明确写出哪些内容可以变、哪些内容必须保持不变。");
    sections << QStringLiteral("请根据用户的自然语言请求，生成符合 Schema 的 CR-DSL JSON。");
    return sections.join(QStringLiteral("\n\n"));
}

inline QString buildChangeRequestUserPrompt(const QString& naturalLanguage, const QJsonObject& context)
{
    QStringList sections;
    sections << QStringLiteral("自然语言修改需求：\n%1").arg(naturalLanguage);
    if (!context.isEmpty()) {
        sections << QStringLiteral("上下文信息：\n%1")
            .arg(QString::fromUtf8(QJsonDocument(context).toJson(QJsonDocument::Indented)));
    }
    sections << QStringLiteral("请输出符合 CR-DSL JSON 的内容，并尽量补全：\n- scope：根据修改范围选择 global / character / panel / page\n- type：根据修改类型选择 art / dialogue / layout / style\n- action：根据 type 选择对应的操作（art→inpaint/setExpression等，dialogue→rewrite_dialogue，layout→reorder/resize，style→update_style）\n- 绝对规则：未明确提及的内容一律不改，不要自行扩展到颜色、光线、背景、构图、人物姿态、服装、发型或其他无关部分\n- 如果用户明确要求“只改文字/对白/台词”，请输出 dialogue + rewrite_dialogue，不要输出 art\n- targetId：如果上下文里提供了 targetPanelId / targetPanelNumber，请直接映射到对应 ID\n- targetId：如果 type=style，请优先映射到 storyboardId\n- targetId：如果能明确到具体对象，请填写对应 ID\n- params.editIntent：如果属于局部编辑，尽量填写 remove_subject / replace_subject / move_subject / insert_subject / replace_attribute / replace_background / set_expression\n- params.maskRegion：如果是局部替换，请填写最小、最具体的被替换对象，不要写容器、整个人物或整页场景\n- params.prompt：尽量保留原始自然语言中的具体对象名，例如“桌上的水杯”优先于“桌上”\n- params.prompt：如果是局部编辑，必须额外写出“未提到的部分保持不变”\n- ops：至少包含一条可执行操作，action 必须与 type 匹配");
    return sections.join(QStringLiteral("\n\n"));
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

    if (type == QStringLiteral("dialogue")) {
        for (const ChangeRequestOp& op : dsl.ops) {
            if (ChangeRequestNormalization::normalizeAction(op.action) != QStringLiteral("rewrite_dialogue")) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("Dialogue DSL must use rewrite_dialogue");
                }
                return false;
            }
        }
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

    static const QStringList rotationKeywords = {
        QString::fromUtf8("朝向"),
        QString::fromUtf8("转身"),
        QString::fromUtf8("转头"),
        QString::fromUtf8("转过"),
        QString::fromUtf8("面向"),
        QString::fromUtf8("背对"),
        QString::fromUtf8("背向"),
        QString::fromUtf8("回头"),
        QStringLiteral("rotate"),
        QStringLiteral("turn around"),
        QStringLiteral("face")
    };
    if (containsAnyKeyword(lower, rotationKeywords)) {
        return QStringLiteral("rotate_subject");
    }

    static const QStringList poseKeywords = {
        QString::fromUtf8("姿势"),
        QString::fromUtf8("姿态"),
        QString::fromUtf8("动作"),
        QString::fromUtf8("站姿"),
        QString::fromUtf8("坐姿"),
        QString::fromUtf8("站着"),
        QString::fromUtf8("坐着"),
        QString::fromUtf8("躺着"),
        QString::fromUtf8("跪着"),
        QString::fromUtf8("蹲着"),
        QString::fromUtf8("趴着"),
        QStringLiteral("pose"),
        QStringLiteral("posture")
    };
    if (containsAnyKeyword(lower, poseKeywords)) {
        return QStringLiteral("change_pose");
    }

    static const QStringList lightingKeywords = {
        QString::fromUtf8("光线"),
        QString::fromUtf8("光照"),
        QString::fromUtf8("打光"),
        QString::fromUtf8("亮度"),
        QString::fromUtf8("天气"),
        QString::fromUtf8("白天"),
        QString::fromUtf8("夜晚"),
        QString::fromUtf8("夜里"),
        QString::fromUtf8("黄昏"),
        QString::fromUtf8("傍晚"),
        QString::fromUtf8("清晨"),
        QString::fromUtf8("下雨"),
        QString::fromUtf8("下雪"),
        QString::fromUtf8("阴天"),
        QString::fromUtf8("晴天"),
        QStringLiteral("lighting"),
        QStringLiteral("weather"),
        QStringLiteral("daylight"),
        QStringLiteral("sunset"),
        QStringLiteral("rain"),
        QStringLiteral("snow")
    };
    if (containsAnyKeyword(lower, lightingKeywords)) {
        return QStringLiteral("change_lighting");
    }

    static const QStringList compositionKeywords = {
        QString::fromUtf8("景别"),
        QString::fromUtf8("远景"),
        QString::fromUtf8("中景"),
        QString::fromUtf8("近景"),
        QString::fromUtf8("特写"),
        QString::fromUtf8("全景"),
        QString::fromUtf8("俯视"),
        QString::fromUtf8("仰视"),
        QString::fromUtf8("俯拍"),
        QString::fromUtf8("仰拍"),
        QString::fromUtf8("构图"),
        QString::fromUtf8("镜头"),
        QString::fromUtf8("视角"),
        QStringLiteral("composition"),
        QStringLiteral("camera angle"),
        QStringLiteral("close-up"),
        QStringLiteral("wide shot")
    };
    if (containsAnyKeyword(lower, compositionKeywords)) {
        return QStringLiteral("change_composition");
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
