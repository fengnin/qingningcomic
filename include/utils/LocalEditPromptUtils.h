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
    if (!trimmedTarget.isEmpty()) {
        parts.append(QStringLiteral("局部编辑目标：%1").arg(trimmedTarget));
    }
    if (!trimmedAllowedChange.isEmpty()) {
        parts.append(QStringLiteral("允许变化：%1").arg(trimmedAllowedChange));
    }
    if (!trimmedChangeTarget.isEmpty()) {
        parts.append(QStringLiteral("变化目标：%1").arg(trimmedChangeTarget));
    }
    if (!trimmedPreserve.isEmpty()) {
        parts.append(QStringLiteral("必须保留：%1").arg(trimmedPreserve));
    }
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
    spec.targetDescription = targetDescription.trimmed().isEmpty()
        ? QStringLiteral("图中人物")
        : targetDescription.trimmed();
    spec.allowedChangeDescription = QStringLiteral("仅修改面部表情");
    spec.changeTargetDescription = expressionDescription.trimmed().isEmpty()
        ? QStringLiteral("表情需要明显变化")
        : expressionDescription.trimmed();
    spec.preserveDescription = preserveDescription.trimmed().isEmpty()
        ? QStringLiteral("保持构图、姿势、服装、发型、背景、镜头、光线和画幅比例不变")
        : preserveDescription.trimmed();
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
    spec.targetDescription = targetDescription.trimmed().isEmpty()
        ? QStringLiteral("目标主体")
        : targetDescription.trimmed();
    spec.allowedChangeDescription = QStringLiteral("仅移除目标主体并补全背景");
    spec.changeTargetDescription = QStringLiteral("移除主体、补全背景、保留其余内容，不要留下半透明残影或贴纸边缘");
    spec.preserveDescription = preserveDescription.trimmed().isEmpty()
        ? QStringLiteral("保持其余人物、构图、透视、光线、色调和画幅比例不变")
        : preserveDescription.trimmed();
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
    spec.targetDescription = targetDescription.trimmed().isEmpty()
        ? QStringLiteral("目标主体")
        : targetDescription.trimmed();
    spec.allowedChangeDescription = QStringLiteral("仅替换目标主体");
    spec.changeTargetDescription = replacementDescription.trimmed().isEmpty()
        ? QStringLiteral("用新主体替换当前目标主体，保持原有构图和光线稳定")
        : replacementDescription.trimmed();
    spec.preserveDescription = preserveDescription.trimmed().isEmpty()
        ? QStringLiteral("保持其余主体、构图、透视、光线、色调和画幅比例不变")
        : preserveDescription.trimmed();
    spec.strength = strength;
    return spec;
}

inline LocalEditPromptSpec buildLocalMovementEditSpec(const QString& targetDescription,
                                                      const QString& destinationDescription,
                                                      const QString& preserveDescription,
                                                      double strength)
{
    LocalEditPromptSpec spec;
    spec.targetDescription = targetDescription.trimmed().isEmpty()
        ? QStringLiteral("目标主体")
        : targetDescription.trimmed();
    spec.allowedChangeDescription = QStringLiteral("仅移动目标主体并删除原位置");
    spec.changeTargetDescription = destinationDescription.trimmed().isEmpty()
        ? QStringLiteral("将目标主体从原位置移动到新的位置，并删除原位置的残留，避免同一物体出现两份")
        : destinationDescription.trimmed();
    spec.preserveDescription = preserveDescription.trimmed().isEmpty()
        ? QStringLiteral("保持其余主体、构图、透视、光线、色调和画幅比例不变")
        : preserveDescription.trimmed();
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
    spec.targetDescription = targetDescription.trimmed().isEmpty()
        ? QStringLiteral("背景")
        : targetDescription.trimmed();
    spec.allowedChangeDescription = QStringLiteral("仅修改背景环境");
    spec.changeTargetDescription = backgroundDescription.trimmed().isEmpty()
        ? QStringLiteral("将背景替换为新的场景氛围")
        : backgroundDescription.trimmed();
    spec.preserveDescription = preserveDescription.trimmed().isEmpty()
        ? QStringLiteral("保持人物、构图、透视、光线、色调和画幅比例不变")
        : preserveDescription.trimmed();
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
