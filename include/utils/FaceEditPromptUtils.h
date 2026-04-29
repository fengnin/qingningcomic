#ifndef FACEEDITPROMPTUTILS_H
#define FACEEDITPROMPTUTILS_H

#include <QString>
#include "utils/LocalEditPromptUtils.h"

namespace FaceEditPromptUtils {

inline LocalEditPromptUtils::LocalEditPromptSpec buildFaceExpressionEditSpec(const QString& subjectDescription,
                                                                            const QString& expression,
                                                                            const QString& preserveDescription)
{
    LocalEditPromptUtils::LocalEditPromptSpec spec;
    spec.targetDescription = subjectDescription.trimmed().isEmpty()
        ? QStringLiteral("图中人物")
        : subjectDescription.trimmed();
    spec.allowedChangeDescription = QStringLiteral("仅修改面部表情");
    spec.changeTargetDescription = expression.trimmed().isEmpty()
        ? QStringLiteral("表情需要明显变化")
        : expression.trimmed();
    spec.preserveDescription = preserveDescription.trimmed().isEmpty()
        ? QStringLiteral("保持构图、姿势、服装、发型、背景、镜头、光线和画幅比例不变，不要让修改区域像单独贴上去的图层")
        : preserveDescription.trimmed();
    spec.strength = 1.20;
    return spec;
}

inline QString buildFaceExpressionEditPrompt(const QString& subjectDescription,
                                             const QString& expression,
                                             const QString& preserveDescription)
{
    const LocalEditPromptUtils::LocalEditPromptSpec spec = buildFaceExpressionEditSpec(
        subjectDescription,
        expression,
        preserveDescription);
    return LocalEditPromptUtils::buildLocalEditPrompt(spec);
}

}

#endif // FACEEDITPROMPTUTILS_H
