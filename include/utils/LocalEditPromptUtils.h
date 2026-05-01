#ifndef LOCALEDITPROMPTUTILS_H
#define LOCALEDITPROMPTUTILS_H

#include <QString>
#include <QStringList>

namespace LocalEditPromptUtils {

struct LocalEditPromptSpec {
    QString targetDescription;
    QString allowedChangeDescription;
    QString changeTargetDescription;
    QString preserveDescription;
    double strength = 1.0;
};

inline QString formatEditStrength(double strength)
{
    if (strength <= 0.0) {
        strength = 1.0;
    }
    if (strength > 2.0) {
        strength = 2.0;
    }
    return QString::number(strength, 'f', 2);
}

inline QString normalizedOrDefault(const QString& value, const QString& fallback)
{
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

inline void appendSection(QStringList& parts, const QString& label, const QString& value)
{
    const QString trimmed = value.trimmed();
    if (!trimmed.isEmpty()) {
        parts.append(QStringLiteral("%1：%2").arg(label, trimmed));
    }
}

inline QString buildAbsolutePreserveClause(const QString& extraPreserveDescription = QString())
{
    QString clause = QStringLiteral("绝对约束：未明确提到的内容一律保持原样，不能擅自改变颜色、光照、背景、构图、透视、人物姿态、表情、服装、发型、比例、风格或文字。");
    if (!extraPreserveDescription.trimmed().isEmpty()) {
        clause += QStringLiteral(" %1").arg(extraPreserveDescription.trimmed());
    }
    return clause;
}

inline QString buildLocalEditPrompt(const QString& targetDescription,
                                    const QString& allowedChangeDescription,
                                    const QString& changeTargetDescription,
                                    const QString& preserveDescription,
                                    double strength)
{
    const QString trimmedTarget = targetDescription.trimmed();
    const QString trimmedAllowedChange = allowedChangeDescription.trimmed();
    const QString trimmedChangeTarget = changeTargetDescription.trimmed();
    const QString trimmedPreserve = preserveDescription.trimmed();

    QStringList parts;
    appendSection(parts, QStringLiteral("局部编辑目标"), trimmedTarget);
    appendSection(parts, QStringLiteral("允许变化"), trimmedAllowedChange);
    appendSection(parts, QStringLiteral("变化目标"), trimmedChangeTarget);
    appendSection(parts, QStringLiteral("必须保留"), trimmedPreserve);
    parts.append(buildAbsolutePreserveClause());
    parts.append(QStringLiteral("编辑强度：%1").arg(formatEditStrength(strength)));

    return parts.join(QStringLiteral("；"));
}

inline QString buildLocalEditPrompt(const LocalEditPromptSpec& spec)
{
    return buildLocalEditPrompt(spec.targetDescription,
                                spec.allowedChangeDescription,
                                spec.changeTargetDescription,
                                spec.preserveDescription,
                                spec.strength);
}

inline LocalEditPromptSpec buildLocalExpressionEditSpec(const QString& targetDescription,
                                                        const QString& expressionDescription,
                                                        const QString& preserveDescription,
                                                        double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription = normalizedOrDefault(targetDescription, QStringLiteral("图中人物"));
    spec.allowedChangeDescription = QStringLiteral("仅修改面部表情");
    spec.changeTargetDescription = normalizedOrDefault(expressionDescription, QStringLiteral("表情需要明显变化"));
    spec.preserveDescription = normalizedOrDefault(
        preserveDescription,
        QStringLiteral("保持构图、姿势、服装、发型、背景、镜头、光线和画幅比例不变，其他未提及部分不得改动"));
    spec.strength = strength;
    return spec;
}

inline QString buildLocalExpressionEditPrompt(const QString& targetDescription,
                                              const QString& expressionDescription,
                                              const QString& preserveDescription,
                                              double strength)
{
    return buildLocalEditPrompt(buildLocalExpressionEditSpec(
        targetDescription,
        expressionDescription,
        preserveDescription,
        strength));
}

inline LocalEditPromptSpec buildLocalRemovalEditSpec(const QString& targetDescription,
                                                     const QString& preserveDescription,
                                                     double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription = normalizedOrDefault(targetDescription, QStringLiteral("目标主体"));
    spec.allowedChangeDescription = QStringLiteral("仅移除目标主体并补全背景");
    spec.changeTargetDescription = QStringLiteral("移除主体、补全背景、保留其余内容，不要留下半透明残影或贴纸边缘");
    spec.preserveDescription = normalizedOrDefault(
        preserveDescription,
        QStringLiteral("保持其余人物、构图、透视、光线、色调和画幅比例不变，未明确要求的元素不得改变"));
    spec.strength = strength;
    return spec;
}

inline QString buildLocalRemovalEditPrompt(const QString& targetDescription,
                                           const QString& preserveDescription,
                                           double strength)
{
    return buildLocalEditPrompt(buildLocalRemovalEditSpec(
        targetDescription,
        preserveDescription,
        strength));
}

inline LocalEditPromptSpec buildLocalReplacementEditSpec(const QString& targetDescription,
                                                         const QString& replacementDescription,
                                                         const QString& preserveDescription,
                                                         double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription = normalizedOrDefault(targetDescription, QStringLiteral("目标主体"));
    spec.allowedChangeDescription = QStringLiteral("仅替换目标主体");
    spec.changeTargetDescription = normalizedOrDefault(
        replacementDescription,
        QStringLiteral("用新主体替换当前目标主体，保持原有构图和光线稳定"));
    spec.preserveDescription = normalizedOrDefault(
        preserveDescription,
        QStringLiteral("保持其余主体、构图、透视、光线、色调和画幅比例不变，未提及的内容一律不改"));
    spec.strength = strength;
    return spec;
}

inline LocalEditPromptSpec buildLocalMovementEditSpec(const QString& targetDescription,
                                                      const QString& destinationDescription,
                                                      const QString& preserveDescription,
                                                      double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription = normalizedOrDefault(targetDescription, QStringLiteral("目标主体"));
    spec.allowedChangeDescription = QStringLiteral("仅移动目标主体并删除原位置");
    spec.changeTargetDescription = normalizedOrDefault(
        destinationDescription,
        QStringLiteral("将目标主体从原位置移动到新的位置，并删除原位置的残留，避免同一物体出现两份"));
    spec.preserveDescription = normalizedOrDefault(
        preserveDescription,
        QStringLiteral("保持其余主体、构图、透视、光线、色调和画幅比例不变，未提及的内容一律不变"));
    spec.strength = strength;
    return spec;
}

inline QString buildLocalMovementEditPrompt(const QString& targetDescription,
                                            const QString& destinationDescription,
                                            const QString& preserveDescription,
                                            double strength)
{
    return buildLocalEditPrompt(buildLocalMovementEditSpec(
        targetDescription,
        destinationDescription,
        preserveDescription,
        strength));
}

inline QString buildLocalReplacementEditPrompt(const QString& targetDescription,
                                               const QString& replacementDescription,
                                               const QString& preserveDescription,
                                               double strength)
{
    return buildLocalEditPrompt(buildLocalReplacementEditSpec(
        targetDescription,
        replacementDescription,
        preserveDescription,
        strength));
}

inline LocalEditPromptSpec buildLocalBackgroundEditSpec(const QString& targetDescription,
                                                        const QString& backgroundDescription,
                                                        const QString& preserveDescription,
                                                        double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription = normalizedOrDefault(targetDescription, QStringLiteral("背景"));
    spec.allowedChangeDescription = QStringLiteral("仅修改背景环境");
    spec.changeTargetDescription = normalizedOrDefault(
        backgroundDescription,
        QStringLiteral("将背景替换为新的场景氛围"));
    spec.preserveDescription = normalizedOrDefault(
        preserveDescription,
        QStringLiteral("保持人物、构图、透视、光线、色调和画幅比例不变，除背景外其余内容不得擅自更改"));
    spec.strength = strength;
    return spec;
}

inline QString buildLocalBackgroundEditPrompt(const QString& targetDescription,
                                              const QString& backgroundDescription,
                                              const QString& preserveDescription,
                                              double strength)
{
    return buildLocalEditPrompt(buildLocalBackgroundEditSpec(
        targetDescription,
        backgroundDescription,
        preserveDescription,
        strength));
}

}

#endif // LOCALEDITPROMPTUTILS_H
