#include "utils/CharacterEditPromptBuilder.h"
#include "services/CharacterExtractor.h"
#include <QStringList>

namespace {

QString fieldLabel(const QString& field)
{
    if (field == QStringLiteral("hairColor")) return QStringLiteral("发色");
    if (field == QStringLiteral("hairStyle")) return QStringLiteral("发型");
    if (field == QStringLiteral("eyeColor")) return QStringLiteral("瞳色");
    if (field == QStringLiteral("height")) return QStringLiteral("身高");
    if (field == QStringLiteral("build")) return QStringLiteral("体型");
    if (field == QStringLiteral("clothing")) return QStringLiteral("服装");
    if (field == QStringLiteral("accessories")) return QStringLiteral("配饰");
    if (field == QStringLiteral("distinctiveFeatures")) return QStringLiteral("显著特征");
    return field;
}

QString buildFragment(const CharacterFieldDiff& d)
{
    const QString label = fieldLabel(d.field);
    const QString oldVal = d.oldValue.trimmed();
    const QString newVal = d.newValue.trimmed();

    if (oldVal.isEmpty() && !newVal.isEmpty()) {
        return QStringLiteral("添加%1为%2").arg(label, newVal);
    }
    if (!oldVal.isEmpty() && newVal.isEmpty()) {
        return QStringLiteral("移除%1").arg(label);
    }
    if (oldVal != newVal) {
        return QStringLiteral("把%1从%2改成%3").arg(label, oldVal, newVal);
    }
    return QString();
}

}

QString CharacterEditPromptBuilder::buildFromDiff(const QList<CharacterFieldDiff>& diff)
{
    QStringList fragments;
    for (const CharacterFieldDiff& d : diff) {
        const QString f = buildFragment(d);
        if (!f.isEmpty()) {
            fragments << f;
        }
    }
    if (fragments.isEmpty()) {
        return QString();
    }
    return QStringLiteral("保持人物身份、面部特征和整体画面风格不变，仅做以下修改：")
        + fragments.join(QStringLiteral("；")) + QStringLiteral("。");
}
