#include "utils/ChangeRequestParseUtils.h"

namespace ChangeRequestParseUtils {

QJsonObject buildChangeRequestSchema()
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
                        QStringLiteral("add_dialogue"),
                        QStringLiteral("remove_dialogue"),
                        QStringLiteral("update_dialogue_side"),
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
QString buildChangeRequestSystemPrompt()
{
    QStringList sections;
    sections << QStringLiteral("你是一个漫画修改助手。");
    sections << QStringLiteral("你的任务是将用户的自然语言修改请求转换为结构化的 CR-DSL（Change Request Domain Specific Language）。");
    sections << QStringLiteral("CR-DSL 包含以下要素：\n- scope: 修改范围\n- targetId: 目标 ID（如果适用）\n- type: 修改类型\n- ops: 操作列表，每个操作包含 action 和 params");
    sections << QStringLiteral("type 与 action 的对应关系：\n- type=art: inpaint, outpaint, bg_swap, repose, regen_panel, setExpression, add_effect, resize\n- type=dialogue: rewrite_dialogue, add_dialogue, remove_dialogue, update_dialogue_side\n- type=layout: reorder, resize\n- type=style: update_style");
    sections << QStringLiteral("layout/reorder 的标准输出规则：\n- 如果是完整顺序重排，使用 panelOrder\n- 如果是交换少量面板，使用 sourceIndices / targetIndices\n- sourceIndices / targetIndices 使用当前页内的 0-based panel_index\n- panelOrder 使用 1-based 面板编号\n- 不要主动生成 fromIndex / toIndex，保留给兼容层处理");
    sections << QStringLiteral("可用的 action 详细说明：\n- inpaint: 局部重绘（需要遮罩区域）；如果用户要求去掉/删除某个角色或物体，prompt 必须明确写出”移除主体、补全背景、保留其余内容”，不要只写局部重绘\n- outpaint: 扩展画面\n- bg_swap: 替换背景\n- repose: 改变角色姿势\n- regen_panel: 重新生成整个面板\n- setExpression: 修改角色表情（params.expression 指定目标表情）\n- editIntent: 可选的意图标记；去除主体使用 remove_subject，通用局部替换使用 replace_subject，局部移动/放置使用 move_subject，局部插入使用 insert_subject，局部属性修改使用 replace_attribute，背景修改使用 replace_background，表情修改使用 set_expression\n- add_effect: 添加特效（params.effect 指定特效类型，params.intensity 指定强度 0-1）\n- resize: 调整面板尺寸（params.width 和 params.height 指定尺寸）\n- rewrite_dialogue: 重写对白（params.text 新文本，params.speaker 说话者，params.dialogueId 目标对白索引或说话者名）\n- add_dialogue: 添加新气泡（params.speaker 说话者，params.text 文本内容，params.bubbleType 气泡类型 speech/narration，params.speakerSide 气泡指向 left/right）\n- remove_dialogue: 删除气泡（params.dialogueId 目标对白索引或说话者名，params.speaker 说话者名）\n- update_dialogue_side: 修改气泡指向（params.dialogueId 目标对白索引或说话者名，params.speaker 说话者名，params.speakerSide 新指向 left/right）\n- reorder: 重新排序");
    sections << QStringLiteral("如果是角色表情修改，请把 expression 归一为以下规范值之一：\n- neutral\n- happy\n- sad\n- angry\n- surprised\n- scared\n- determined\n- confused\n- embarrassed\n- smirking");
    sections << QStringLiteral("如果请求的核心目标是修改角色表情，请优先使用 setExpression，不要把表情修改误写成 repose、outpaint 或 regen_panel。");
    sections << QStringLiteral("解析时必须遵守以下绝对规则：\n- 只允许修改用户明确提到的对象和属性，未提及的内容一律保持原样。\n- 如果用户说的是“对白、台词、说的话、文字内容、文本”，优先视为 dialogue（面板对白），而不是 art。\n- 如果用户明确指向“图上的字、书页上的字、牌子上的字、海报上的字”，才将其视为 art。\n- 如果请求中只提到“改文字”但上下文存在面板对白，则优先映射到 dialogue。\n- 如果请求要求修改图像内容，必须在 prompt 中明确写出哪些内容可以变、哪些内容必须保持不变。");
    sections << QStringLiteral("请根据用户的自然语言请求，生成符合 Schema 的 CR-DSL JSON。");
    return sections.join(QStringLiteral("\n\n"));
}
QString buildChangeRequestUserPrompt(const QString& naturalLanguage, const QJsonObject& context)
{
    QStringList sections;
    sections << QStringLiteral("自然语言修改需求：\n%1").arg(naturalLanguage);
    if (!context.isEmpty()) {
        sections << QStringLiteral("上下文信息：\n%1")
            .arg(QString::fromUtf8(QJsonDocument(context).toJson(QJsonDocument::Indented)));
    }
    sections << QStringLiteral("请输出符合 CR-DSL JSON 的内容，并尽量补全：\n- scope：根据修改范围选择 global / character / panel / page\n- type：根据修改类型选择 art / dialogue / layout / style\n- action：根据 type 选择对应的操作（art→inpaint/setExpression等，dialogue→rewrite_dialogue/add_dialogue/remove_dialogue/update_dialogue_side，layout→reorder/resize，style→update_style）\n- 绝对规则：未明确提及的内容一律不改，不要自行扩展到颜色、光线、背景、构图、人物姿态、服装、发型或其他无关部分\n- 如果用户明确要求”只改文字/对白/台词”，请输出 dialogue + rewrite_dialogue，不要输出 art\n- 如果用户要求”添加/新增气泡/对白”，请输出 dialogue + add_dialogue，params 包含 speaker、text、bubbleType（默认 speech）、speakerSide（left 或 right）\n- 如果用户要求”删除/去掉气泡/对白”，请输出 dialogue + remove_dialogue，params 包含 dialogueId 或 speaker\n- 如果用户要求”修改气泡指向/方向/朝向”，请输出 dialogue + update_dialogue_side，params 包含 dialogueId 或 speaker，以及 speakerSide（left 或 right）\n- targetId：如果上下文里提供了 targetPanelId / targetPanelNumber，请直接映射到对应 ID\n- targetId：如果 type=style，请优先映射到 storyboardId\n- targetId：如果能明确到具体对象，请填写对应 ID\n- params.editIntent：如果属于局部编辑，尽量填写 remove_subject / replace_subject / move_subject / insert_subject / replace_attribute / replace_background / set_expression\n- params.maskRegion：如果是局部替换，请填写最小、最具体的被替换对象，不要写容器、整个人物或整页场景\n- params.prompt：尽量保留原始自然语言中的具体对象名，例如”桌上的水杯”优先于”桌上”\n- params.prompt：如果是局部编辑，必须额外写出”未提到的部分保持不变”\n- ops：至少包含一条可执行操作，action 必须与 type 匹配");
    return sections.join(QStringLiteral("\n\n"));
}
bool isValidParsedDsl(const ChangeRequestDsl& dsl, QString* errorMessage)
{
    auto fail = [&](const QString& msg) -> bool {
        if (errorMessage) *errorMessage = msg;
        return false;
    };

    const QString scope = ChangeRequestNormalization::normalizeScope(dsl.scope);
    const QString type = ChangeRequestNormalization::normalizeType(dsl.type);

    if (!ChangeRequestNormalization::isSupportedScope(scope))
        return fail(QStringLiteral("Parsed DSL has invalid scope"));

    if (!ChangeRequestNormalization::isSupportedType(type))
        return fail(QStringLiteral("Parsed DSL has invalid type"));

    if (dsl.targetId.trimmed().isEmpty())
        return fail(QStringLiteral("Parsed DSL is missing targetId"));

    if (dsl.ops.isEmpty())
        return fail(QStringLiteral("Parsed DSL has no operations"));

    for (const ChangeRequestOp& op : dsl.ops) {
        const QString normalizedAction = ChangeRequestNormalization::normalizeAction(op.action);
        if (normalizedAction.isEmpty())
            return fail(QStringLiteral("Parsed DSL contains an operation without action"));
        if (!isActionValidForType(normalizedAction, type))
            return fail(QStringLiteral("Action '%1' is not valid for type '%2'").arg(normalizedAction, type));
    }

    if (errorMessage) errorMessage->clear();
    return true;
}
QStringList getValidActionsForType(const QString& type)
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
        QStringLiteral("rewrite_dialogue"),
        QStringLiteral("add_dialogue"),
        QStringLiteral("remove_dialogue"),
        QStringLiteral("update_dialogue_side")
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
bool isValidAction(const QString& action)
{
    static const QStringList allTypes = {
        QStringLiteral("art"), QStringLiteral("dialogue"),
        QStringLiteral("layout"), QStringLiteral("style")
    };
    const QString normalized = ChangeRequestNormalization::normalizeAction(action);
    for (const QString& type : allTypes) {
        if (getValidActionsForType(type).contains(normalized)) return true;
    }
    return false;
}
bool isActionValidForType(const QString& action, const QString& type)
{
    const QStringList validActions = getValidActionsForType(type);
    if (validActions.isEmpty()) {
        return false;
    }
    return validActions.contains(ChangeRequestNormalization::normalizeAction(action));
}
QString inferDefaultScopeForType(const QString& type)
{
    static const QStringList panelTypes = {
        QStringLiteral("art"), QStringLiteral("dialogue"), QStringLiteral("layout")
    };
    return panelTypes.contains(type) ? QStringLiteral("panel") : QStringLiteral("global");
}
QString inferEditIntentFromText(const QString& text)
{
    const QString lower = text.trimmed().toLower();
    if (lower.isEmpty()) return QString();

    if (PromptTargetUtils::isRemovalIntentText(lower))
        return QStringLiteral("remove_subject");

    static const QStringList placementKeywords = {
        QString::fromUtf8("移动"), QString::fromUtf8("挪动"), QString::fromUtf8("搬到"),
        QString::fromUtf8("搬进"), QString::fromUtf8("放入"), QString::fromUtf8("放进"),
        QString::fromUtf8("放在"), QString::fromUtf8("放到"), QString::fromUtf8("摆到"),
        QString::fromUtf8("摆在"), QString::fromUtf8("移到"), QString::fromUtf8("移至"),
        QString::fromUtf8("挪到"), QString::fromUtf8("加到"), QString::fromUtf8("塞进"),
        QString::fromUtf8("置于"),
        QStringLiteral("insert"), QStringLiteral("place"), QStringLiteral("put")
    };
    if (PromptTargetUtils::containsAny(lower, placementKeywords))
        return QStringLiteral("move_subject");

    static const QStringList changeVerbKeywords = {
        QString::fromUtf8("改"), QString::fromUtf8("换"), QString::fromUtf8("替换"),
        QString::fromUtf8("变"),
        QStringLiteral("replace"), QStringLiteral("change"), QStringLiteral("swap")
    };
    static const QStringList replacementKeywords = {
        QString::fromUtf8("替换"), QString::fromUtf8("换成"), QString::fromUtf8("改成"),
        QString::fromUtf8("变成"), QString::fromUtf8("用"),
        QStringLiteral("replace"), QStringLiteral("swap")
    };
    static const QStringList pureBackgroundKeywords = {
        QString::fromUtf8("背景"), QString::fromUtf8("场景"), QString::fromUtf8("环境"),
        QString::fromUtf8("风景"), QString::fromUtf8("地点"),
        QStringLiteral("background"), QStringLiteral("scene"), QStringLiteral("environment"),
        QStringLiteral("setting"), QStringLiteral("location")
    };
    static const QStringList lightingTimeKeywords = {
        QString::fromUtf8("白天"), QString::fromUtf8("夜晚"), QString::fromUtf8("夜里"),
        QString::fromUtf8("黄昏"), QString::fromUtf8("傍晚"), QString::fromUtf8("清晨"),
        QString::fromUtf8("下雨"), QString::fromUtf8("下雪"), QString::fromUtf8("阴天"),
        QString::fromUtf8("晴天"), QString::fromUtf8("光线"), QString::fromUtf8("光照"),
        QString::fromUtf8("打光"), QString::fromUtf8("亮度"),
        QStringLiteral("daylight"), QStringLiteral("sunset"), QStringLiteral("rain"),
        QStringLiteral("snow"), QStringLiteral("lighting"), QStringLiteral("weather")
    };

    // Compound rules: keyword group + change verb required
    const bool hasChangeVerb = PromptTargetUtils::containsAny(lower, changeVerbKeywords);
    if (hasChangeVerb) {
        if (PromptTargetUtils::containsAny(lower, pureBackgroundKeywords))
            return QStringLiteral("replace_background");
        if (PromptTargetUtils::containsAny(lower, lightingTimeKeywords))
            return QStringLiteral("change_lighting");
        if (ChangeRequestIntentUtils::isBackgroundEditIntentText(lower))
            return QStringLiteral("replace_background");
        if (ChangeRequestIntentUtils::isAttributeEditIntentText(lower))
            return QStringLiteral("replace_attribute");
    }
    if (!hasChangeVerb && ChangeRequestIntentUtils::isAttributeEditIntentText(lower)
        && PromptTargetUtils::containsAny(lower, replacementKeywords)) {
        return QStringLiteral("replace_attribute");
    }

    // Simple tail rules: single keyword list, no verb required
    static const QStringList expressionKeywords = {
        QString::fromUtf8("表情"), QString::fromUtf8("神情"),
        QString::fromUtf8("神态"), QString::fromUtf8("脸色"),
        QStringLiteral("expression")
    };
    static const QStringList rotationKeywords = {
        QString::fromUtf8("朝向"), QString::fromUtf8("转身"), QString::fromUtf8("转头"),
        QString::fromUtf8("转过"), QString::fromUtf8("面向"), QString::fromUtf8("背对"),
        QString::fromUtf8("背向"), QString::fromUtf8("回头"),
        QStringLiteral("rotate"), QStringLiteral("turn around"), QStringLiteral("face")
    };
    static const QStringList poseKeywords = {
        QString::fromUtf8("姿势"), QString::fromUtf8("姿态"), QString::fromUtf8("动作"),
        QString::fromUtf8("站姿"), QString::fromUtf8("坐姿"), QString::fromUtf8("站着"),
        QString::fromUtf8("坐着"), QString::fromUtf8("躺着"), QString::fromUtf8("跪着"),
        QString::fromUtf8("蹲着"), QString::fromUtf8("趴着"),
        QStringLiteral("pose"), QStringLiteral("posture")
    };
    static const QStringList lightingKeywords = {
        QString::fromUtf8("光线"), QString::fromUtf8("光照"), QString::fromUtf8("打光"),
        QString::fromUtf8("亮度"), QString::fromUtf8("白天"), QString::fromUtf8("夜晚"),
        QString::fromUtf8("夜里"), QString::fromUtf8("黄昏"), QString::fromUtf8("傍晚"),
        QString::fromUtf8("清晨"), QString::fromUtf8("下雨"), QString::fromUtf8("下雪"),
        QString::fromUtf8("阴天"), QString::fromUtf8("晴天"),
        QStringLiteral("lighting"), QStringLiteral("weather"), QStringLiteral("daylight"),
        QStringLiteral("sunset"), QStringLiteral("rain"), QStringLiteral("snow")
    };
    static const QStringList compositionKeywords = {
        QString::fromUtf8("景别"), QString::fromUtf8("远景"), QString::fromUtf8("中景"),
        QString::fromUtf8("近景"), QString::fromUtf8("特写"), QString::fromUtf8("全景"),
        QString::fromUtf8("俯视"), QString::fromUtf8("仰视"), QString::fromUtf8("俯拍"),
        QString::fromUtf8("仰拍"), QString::fromUtf8("构图"), QString::fromUtf8("镜头"),
        QString::fromUtf8("视角"),
        QStringLiteral("composition"), QStringLiteral("camera angle"),
        QStringLiteral("close-up"), QStringLiteral("wide shot")
    };

    struct SimpleRule { const QStringList* kw; const char* intent; };
    static const SimpleRule tailRules[] = {
        {&expressionKeywords,  "set_expression"},
        {&rotationKeywords,    "rotate_subject"},
        {&poseKeywords,        "change_pose"},
        {&lightingKeywords,    "change_lighting"},
        {&compositionKeywords, "change_composition"},
        {&replacementKeywords, "replace_subject"},
    };
    for (const SimpleRule& rule : tailRules) {
        if (PromptTargetUtils::containsAny(lower, *rule.kw))
            return QLatin1String(rule.intent);
    }

    return QString();
}
void normalizeParsedDsl(ChangeRequestDsl& dsl)
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

    // editIntent is only meaningful for art ops; dialogue/layout/style don't use it
    if (ChangeRequestNormalization::normalizeType(dsl.type) != QStringLiteral("art")) {
        return;
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

} // namespace ChangeRequestParseUtils
