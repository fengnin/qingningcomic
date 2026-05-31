#ifndef FACEEDITPROMPTUTILS_H
#define FACEEDITPROMPTUTILS_H

#include <QString>

namespace FaceEditPromptUtils {

QString buildFaceExpressionEditPrompt(const QString& subjectDescription,
                                      const QString& expression,
                                      const QString& preserveDescription);

} // namespace FaceEditPromptUtils

#endif // FACEEDITPROMPTUTILS_H
