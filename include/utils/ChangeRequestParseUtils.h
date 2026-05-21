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

inline bool containsAnyKeyword(const QString& text, const QStringList& keywords)
{
    for (const QString& keyword : keywords) {
        if (text.contains(keyword.toLower())) {
            return true;
        }
    }
    return false;
}

QJsonObject buildChangeRequestSchema();

QString buildChangeRequestSystemPrompt();

QString buildChangeRequestUserPrompt(const QString& naturalLanguage, const QJsonObject& context);

bool isValidParsedDsl(const ChangeRequestDsl& dsl, QString* errorMessage = nullptr);

QStringList getValidActionsForType(const QString& type);

bool isValidAction(const QString& action);

bool isActionValidForType(const QString& action, const QString& type);

QString inferDefaultScopeForType(const QString& type);

QString inferEditIntentFromText(const QString& text);

void normalizeParsedDsl(ChangeRequestDsl& dsl);

}

#endif // CHANGEREQUESTPARSEUTILS_H
