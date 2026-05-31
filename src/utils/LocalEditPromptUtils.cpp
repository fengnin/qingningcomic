#include "utils/LocalEditPromptUtils.h"

namespace LocalEditPromptUtils {

QString formatEditStrength(double strength)
{
    if (strength <= 0.0) strength = 1.0;
    if (strength > 2.0) strength = 2.0;
    return QString::number(strength, 'f', 2);
}

QString normalizedOrDefault(const QString& value, const QString& fallback)
{
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

void appendSection(QStringList& parts, const QString& label, const QString& value)
{
    const QString trimmed = value.trimmed();
    if (!trimmed.isEmpty())
        parts.append(QStringLiteral("%1：%2").arg(label, trimmed));
}

QStringList inferExcludeFields(const QString& allowedChangeDescription)
{
    struct Rule { const char* keyword; const char* field; };
    static const Rule rules[] = {
        { "表情",       "表情" },
        { "expression", "表情" },
        { "服装",       "服装" },
        { "衣",         "服装" },
        { "clothing",   "服装" },
        { "发型",       "发型" },
        { "头发",       "发型" },
        { "hair",       "发型" },
        { "背景",       "背景" },
        { "background", "背景" },
        { "颜色",       "颜色" },
        { "color",      "颜色" },
        { "姿态",       "人物姿态" },
        { "姿势",       "人物姿态" },
        { "pose",       "人物姿态" },
    };
    const QString lower = allowedChangeDescription.toLower();
    QStringList exclude;
    for (const Rule& r : rules) {
        const QString field = QString::fromUtf8(r.field);
        if (lower.contains(QString::fromUtf8(r.keyword)) && !exclude.contains(field))
            exclude << field;
    }
    return exclude;
}

QString buildAbsolutePreserveClause(const QStringList& excludeFields)
{
    QStringList fields = {
        QStringLiteral("颜色"), QStringLiteral("光照"), QStringLiteral("背景"),
        QStringLiteral("构图"), QStringLiteral("透视"), QStringLiteral("人物姿态"),
        QStringLiteral("表情"), QStringLiteral("服装"), QStringLiteral("发型"),
        QStringLiteral("比例"), QStringLiteral("风格"), QStringLiteral("文字")
    };
    for (const QString& f : excludeFields)
        fields.removeAll(f);
    return QStringLiteral("绝对约束：未明确提到的内容一律保持原样，不能擅自改变%1。")
        .arg(fields.join(QStringLiteral("、")));
}

QString buildLocalEditPrompt(const LocalEditPromptSpec& spec)
{
    QStringList parts;
    appendSection(parts, QStringLiteral("局部编辑目标"), spec.targetDescription);
    appendSection(parts, QStringLiteral("允许变化"),     spec.allowedChangeDescription);
    appendSection(parts, QStringLiteral("变化目标"),     spec.changeTargetDescription);
    appendSection(parts, QStringLiteral("必须保留"),     spec.preserveDescription);
    parts.append(buildAbsolutePreserveClause(inferExcludeFields(spec.allowedChangeDescription)));
    parts.append(QStringLiteral("编辑强度：%1").arg(formatEditStrength(spec.strength)));
    return parts.join(QStringLiteral("；"));
}

LocalEditPromptSpec buildLocalExpressionEditSpec(const QString& targetDescription,
                                                 const QString& expressionDescription,
                                                 const QString& preserveDescription,
                                                 double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription        = normalizedOrDefault(targetDescription, QStringLiteral("图中人物"));
    spec.allowedChangeDescription = QStringLiteral("仅修改面部表情");
    spec.changeTargetDescription  = normalizedOrDefault(expressionDescription, QStringLiteral("表情需要明显变化"));
    spec.preserveDescription      = normalizedOrDefault(preserveDescription,
        QStringLiteral("保持构图、姿势、服装、发型、背景、镜头、光线和画幅比例不变，其他未提及部分不得改动"));
    spec.strength = strength;
    return spec;
}

LocalEditPromptSpec buildLocalRemovalEditSpec(const QString& targetDescription,
                                              const QString& preserveDescription,
                                              double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription        = normalizedOrDefault(targetDescription, QStringLiteral("目标主体"));
    spec.allowedChangeDescription = QStringLiteral("仅移除目标主体并补全背景");
    spec.changeTargetDescription  = QStringLiteral("移除主体、补全背景、保留其余内容，不要留下半透明残影或贴纸边缘");
    spec.preserveDescription      = normalizedOrDefault(preserveDescription,
        QStringLiteral("保持其余人物、构图、透视、光线、色调和画幅比例不变，未明确要求的元素不得改变"));
    spec.strength = strength;
    return spec;
}

LocalEditPromptSpec buildLocalReplacementEditSpec(const QString& targetDescription,
                                                  const QString& replacementDescription,
                                                  const QString& preserveDescription,
                                                  double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription        = normalizedOrDefault(targetDescription, QStringLiteral("目标主体"));
    spec.allowedChangeDescription = QStringLiteral("仅替换目标主体");
    spec.changeTargetDescription  = normalizedOrDefault(replacementDescription,
        QStringLiteral("用新主体替换当前目标主体，保持原有构图和光线稳定"));
    spec.preserveDescription      = normalizedOrDefault(preserveDescription,
        QStringLiteral("保持其余主体、构图、透视、光线、色调和画幅比例不变，未提及的内容一律不改"));
    spec.strength = strength;
    return spec;
}

LocalEditPromptSpec buildLocalMovementEditSpec(const QString& targetDescription,
                                               const QString& destinationDescription,
                                               const QString& preserveDescription,
                                               double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription        = normalizedOrDefault(targetDescription, QStringLiteral("目标主体"));
    spec.allowedChangeDescription = QStringLiteral("仅移动目标主体并删除原位置");
    spec.changeTargetDescription  = normalizedOrDefault(destinationDescription,
        QStringLiteral("将目标主体从原位置移动到新的位置，并删除原位置的残留，避免同一物体出现两份"));
    spec.preserveDescription      = normalizedOrDefault(preserveDescription,
        QStringLiteral("保持其余主体、构图、透视、光线、色调和画幅比例不变，未提及的内容一律不变"));
    spec.strength = strength;
    return spec;
}

LocalEditPromptSpec buildLocalBackgroundEditSpec(const QString& targetDescription,
                                                 const QString& backgroundDescription,
                                                 const QString& preserveDescription,
                                                 double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription        = normalizedOrDefault(targetDescription, QStringLiteral("背景"));
    spec.allowedChangeDescription = QStringLiteral("仅修改背景环境");
    spec.changeTargetDescription  = normalizedOrDefault(backgroundDescription, QStringLiteral("将背景替换为新的场景氛围"));
    spec.preserveDescription      = normalizedOrDefault(preserveDescription,
        QStringLiteral("保持人物、构图、透视、光线、色调和画幅比例不变，除背景外其余内容不得擅自更改"));
    spec.strength = strength;
    return spec;
}

LocalEditPromptSpec buildLocalEffectEditSpec(const QString& effectDescription, double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription        = QStringLiteral("整体画面");
    spec.allowedChangeDescription = QStringLiteral("仅叠加视觉特效，不改变构图和内容");
    spec.changeTargetDescription  = normalizedOrDefault(effectDescription, QStringLiteral("添加视觉特效"));
    spec.preserveDescription      = QStringLiteral("保持人物、背景、构图、透视、表情、服装和画幅比例完全不变");
    spec.strength = strength;
    return spec;
}

QString buildLocalExpressionEditPrompt(const QString& targetDescription,
                                       const QString& expressionDescription,
                                       const QString& preserveDescription,
                                       double strength)
{
    return buildLocalEditPrompt(buildLocalExpressionEditSpec(
        targetDescription, expressionDescription, preserveDescription, strength));
}

QString buildLocalRemovalEditPrompt(const QString& targetDescription,
                                    const QString& preserveDescription,
                                    double strength)
{
    return buildLocalEditPrompt(buildLocalRemovalEditSpec(targetDescription, preserveDescription, strength));
}

QString buildLocalReplacementEditPrompt(const QString& targetDescription,
                                        const QString& replacementDescription,
                                        const QString& preserveDescription,
                                        double strength)
{
    return buildLocalEditPrompt(buildLocalReplacementEditSpec(
        targetDescription, replacementDescription, preserveDescription, strength));
}

QString buildLocalMovementEditPrompt(const QString& targetDescription,
                                     const QString& destinationDescription,
                                     const QString& preserveDescription,
                                     double strength)
{
    return buildLocalEditPrompt(buildLocalMovementEditSpec(
        targetDescription, destinationDescription, preserveDescription, strength));
}

QString buildLocalBackgroundEditPrompt(const QString& targetDescription,
                                       const QString& backgroundDescription,
                                       const QString& preserveDescription,
                                       double strength)
{
    return buildLocalEditPrompt(buildLocalBackgroundEditSpec(
        targetDescription, backgroundDescription, preserveDescription, strength));
}

QString buildLocalEffectEditPrompt(const QString& effectDescription, double strength)
{
    return buildLocalEditPrompt(buildLocalEffectEditSpec(effectDescription, strength));
}

} // namespace LocalEditPromptUtils
