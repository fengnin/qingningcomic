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

QString formatEditStrength(double strength);
QString normalizedOrDefault(const QString& value, const QString& fallback);
void appendSection(QStringList& parts, const QString& label, const QString& value);
QStringList inferExcludeFields(const QString& allowedChangeDescription);
QString buildAbsolutePreserveClause(const QStringList& excludeFields = QStringList());
QString buildLocalEditPrompt(const LocalEditPromptSpec& spec);

LocalEditPromptSpec buildLocalExpressionEditSpec(const QString& targetDescription,
                                                 const QString& expressionDescription,
                                                 const QString& preserveDescription,
                                                 double strength);

LocalEditPromptSpec buildLocalRemovalEditSpec(const QString& targetDescription,
                                              const QString& preserveDescription,
                                              double strength);

LocalEditPromptSpec buildLocalReplacementEditSpec(const QString& targetDescription,
                                                  const QString& replacementDescription,
                                                  const QString& preserveDescription,
                                                  double strength);

LocalEditPromptSpec buildLocalMovementEditSpec(const QString& targetDescription,
                                               const QString& destinationDescription,
                                               const QString& preserveDescription,
                                               double strength);

LocalEditPromptSpec buildLocalBackgroundEditSpec(const QString& targetDescription,
                                                 const QString& backgroundDescription,
                                                 const QString& preserveDescription,
                                                 double strength);

LocalEditPromptSpec buildLocalEffectEditSpec(const QString& effectDescription, double strength);

QString buildLocalExpressionEditPrompt(const QString& targetDescription,
                                       const QString& expressionDescription,
                                       const QString& preserveDescription,
                                       double strength);

QString buildLocalRemovalEditPrompt(const QString& targetDescription,
                                    const QString& preserveDescription,
                                    double strength);

QString buildLocalReplacementEditPrompt(const QString& targetDescription,
                                        const QString& replacementDescription,
                                        const QString& preserveDescription,
                                        double strength);

QString buildLocalMovementEditPrompt(const QString& targetDescription,
                                     const QString& destinationDescription,
                                     const QString& preserveDescription,
                                     double strength);

QString buildLocalBackgroundEditPrompt(const QString& targetDescription,
                                       const QString& backgroundDescription,
                                       const QString& preserveDescription,
                                       double strength);

QString buildLocalEffectEditPrompt(const QString& effectDescription, double strength);

} // namespace LocalEditPromptUtils

#endif // LOCALEDITPROMPTUTILS_H
