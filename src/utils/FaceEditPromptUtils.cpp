#include "utils/FaceEditPromptUtils.h"
#include "utils/LocalEditPromptUtils.h"

namespace FaceEditPromptUtils {

QString buildFaceExpressionEditPrompt(const QString& subjectDescription,
                                      const QString& expression,
                                      const QString& preserveDescription)
{
    const QString defaultPreserve = QStringLiteral(
        "保持构图、姿势、服装、发型、背景、镜头、光线和画幅比例不变，不要让修改区域像单独贴上去的图层");
    return LocalEditPromptUtils::buildLocalEditPrompt(
        LocalEditPromptUtils::buildLocalExpressionEditSpec(
            subjectDescription,
            expression,
            preserveDescription.trimmed().isEmpty() ? defaultPreserve : preserveDescription,
            1.20));
}

} // namespace FaceEditPromptUtils
