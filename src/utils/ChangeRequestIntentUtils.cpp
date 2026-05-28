#include "utils/ChangeRequestIntentUtils.h"
#include "utils/PromptTargetUtils.h"

namespace ChangeRequestIntentUtils {

bool isBackgroundEditIntentText(const QString& text)
{
    static const QStringList keywords = {
        QString::fromUtf8("背景"), QString::fromUtf8("场景"), QString::fromUtf8("环境"),
        QString::fromUtf8("风景"), QString::fromUtf8("地点"), QString::fromUtf8("夜晚"),
        QString::fromUtf8("白天"), QString::fromUtf8("傍晚"), QString::fromUtf8("黄昏"),
        QString::fromUtf8("黎明"), QString::fromUtf8("清晨"), QString::fromUtf8("午后"),
        QString::fromUtf8("雨天"), QString::fromUtf8("雪天"),
        QStringLiteral("background"), QStringLiteral("scene"), QStringLiteral("environment"),
        QStringLiteral("setting"), QStringLiteral("location"), QStringLiteral("night"),
        QStringLiteral("day"), QStringLiteral("dawn"), QStringLiteral("dusk"),
        QStringLiteral("evening"), QStringLiteral("morning")
    };
    return PromptTargetUtils::containsAny(text, keywords);
}

bool isAttributeEditIntentText(const QString& text)
{
    static const QStringList keywords = {
        QString::fromUtf8("衣服"), QString::fromUtf8("服装"), QString::fromUtf8("服饰"),
        QString::fromUtf8("外套"), QString::fromUtf8("上衣"), QString::fromUtf8("裙子"),
        QString::fromUtf8("裤子"), QString::fromUtf8("鞋子"), QString::fromUtf8("帽子"),
        QString::fromUtf8("配饰"), QString::fromUtf8("饰品"), QString::fromUtf8("发型"),
        QString::fromUtf8("头发"), QString::fromUtf8("刘海"), QString::fromUtf8("颜色"),
        QString::fromUtf8("纹理"), QString::fromUtf8("材质"),
        QString::fromUtf8("图案"), QString::fromUtf8("装饰"), QString::fromUtf8("瞳色"),
        QStringLiteral("clothing"), QStringLiteral("outfit"), QStringLiteral("hair"),
        QStringLiteral("accessory"), QStringLiteral("accessories"), QStringLiteral("color"),
        QStringLiteral("texture"), QStringLiteral("material"), QStringLiteral("pattern")
    };
    return PromptTargetUtils::containsAny(text, keywords);
}

QString extractRemovalSubject(const QJsonObject& params)
{
    const QString value = extractFirstTrimmedValue(params, QStringList{
        QStringLiteral("mask"), QStringLiteral("subject"),
        QStringLiteral("target"), QStringLiteral("entity")
    });
    if (value.isEmpty()) return QString();

    const int colonIndex = value.indexOf(QLatin1Char(':'));
    if (colonIndex >= 0 && colonIndex + 1 < value.size()) {
        const QString suffix = value.mid(colonIndex + 1).trimmed();
        if (!suffix.isEmpty()) return suffix;
    }
    return value;
}

QString cleanupReplacementTarget(QString text)
{
    text = text.trimmed();
    if (text.isEmpty()) return QString();

    const int punctuationIndex = text.indexOf(QRegularExpression(QStringLiteral("[，。,.;；、\\n\\r]")));
    if (punctuationIndex > 0) text = text.left(punctuationIndex).trimmed();

    const int bracketIndex = text.indexOf(QRegularExpression(QStringLiteral("[（(]")));
    if (bracketIndex > 0) text = text.left(bracketIndex).trimmed();

    text.remove(QRegularExpression(QStringLiteral("^[:：\\s]+")));
    text.remove(QRegularExpression(QStringLiteral("\\s+$")));
    return text.trimmed();
}

int lastLocalContextMarkerIndex(const QString& text)
{
    static const QStringList markers = {
        QStringLiteral("的"), QStringLiteral("上"), QStringLiteral("中"),
        QStringLiteral("里"), QStringLiteral("内"), QStringLiteral("下"),
        QString::fromUtf8("旁边"), QString::fromUtf8("前面"), QString::fromUtf8("后面"),
        QString::fromUtf8("左边"), QString::fromUtf8("右边"), QString::fromUtf8("角落")
    };

    int bestIndex = -1;
    for (const QString& marker : markers) {
        const int index = text.lastIndexOf(marker);
        if (index > bestIndex) bestIndex = index;
    }
    return bestIndex;
}

QString cleanupReplacementSource(QString text)
{
    text = cleanupReplacementTarget(text);
    if (text.isEmpty()) return QString();

    text.remove(QRegularExpression(QString::fromUtf8(
        "^(?:把|将|请把|请将|这个|该|这张|这幅|这页|画面中的|图中的|图里|画面里|角色|人物|主体|对象|物体|物品)\\s*")));

    // Keep the most local edited noun phrase when the request mentions a container or location.
    const int lastStructuralMarker = lastLocalContextMarkerIndex(text);
    if (lastStructuralMarker >= 0 && lastStructuralMarker + 1 < text.size()) {
        const QString localSuffix = text.mid(lastStructuralMarker + 1).trimmed();
        if (!localSuffix.isEmpty()) text = localSuffix;
    }
    return text.trimmed();
}

QString cleanupPlacementTarget(QString text)
{
    text = cleanupReplacementTarget(text);
    if (text.isEmpty()) return QString();
    text.remove(QRegularExpression(QString::fromUtf8("(?:里|中|内|上|下|旁边|前面|后面|左边|右边|之中|里面)$")));
    return text.trimmed();
}

QString extractMatchedCapture(const QString& text, const QRegularExpression& pattern, int captureIndex)
{
    const QRegularExpressionMatch match = pattern.match(text);
    if (!match.hasMatch()) return QString();
    return match.captured(captureIndex).trimmed();
}

QString extractMatchedSourceText(const QString& text, const QRegularExpression& pattern, int captureIndex)
{
    return cleanupReplacementSource(extractMatchedCapture(text, pattern, captureIndex));
}

QString extractMatchedTargetText(const QString& text, const QRegularExpression& pattern, int captureIndex)
{
    return cleanupReplacementTarget(extractMatchedCapture(text, pattern, captureIndex));
}

QString extractReplacementSourceFromText(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return QString();

    struct SourcePattern {
        QRegularExpression pattern;
        int captureIndex;
    };
    static const SourcePattern sourcePatterns[] = {
        {QRegularExpression(QString::fromUtf8("(?:把|将|请把|请将)?([^，。,.;；、\\n\\r]+?)(?:替换(?:为|成|改成|变成)|改为|换成|改成|变成)\\s*([^，。,.;；、\\n\\r]+)")), 1},
        {QRegularExpression(QString::fromUtf8("用([^，。,.;；、\\n\\r]+)\\s*替换\\s*([^，。,.;；、\\n\\r]+)")), 2},
        {QRegularExpression(QStringLiteral("(?:replace|swap)\\s+([^，。,.;；、\\n\\r]+?)\\s+(?:with|to)\\s+([^，。,.;；、\\n\\r]+)"), QRegularExpression::CaseInsensitiveOption), 1}
    };

    for (int i = 0; i < static_cast<int>(sizeof(sourcePatterns) / sizeof(sourcePatterns[0])); ++i) {
        const QString candidate = extractMatchedSourceText(trimmed, sourcePatterns[i].pattern, sourcePatterns[i].captureIndex);
        if (!candidate.isEmpty()) return candidate;
    }
    return QString();
}

QString extractReplacementTargetFromText(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return QString();

    static const QRegularExpression patterns[] = {
        QRegularExpression(QString::fromUtf8("替换(?:角色|人物|主体)?为([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("替换(?:角色|人物|主体)?成([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("替换(?:角色|人物|主体)?改成([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("替换(?:角色|人物|主体)?变成([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("替换为([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("替换成([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("改为([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("换成([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("改成([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("变成([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QString::fromUtf8("用([^，。,.;；、\\n\\r]+)替换")),
        QRegularExpression(QStringLiteral("(?:replace\\s+with|swap\\s+to)\\s*([^，。,.;；、\\n\\r]+)"), QRegularExpression::CaseInsensitiveOption)
    };

    for (const QRegularExpression& pattern : patterns) {
        const QString candidate = extractMatchedTargetText(trimmed, pattern, 1);
        if (!candidate.isEmpty()) return candidate;
    }
    return QString();
}

QString extractPlacementSourceFromText(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return QString();

    static const QRegularExpression patterns[] = {
        QRegularExpression(QString::fromUtf8("(?:把|将|请把|请将)?([^，。,.;；、\\n\\r]+?)(?:移动到|挪动到|搬到|放入|放进|放在|放到|塞进|摆到|摆在|移到|移至|挪到|加到|置于)\\s*([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QStringLiteral("(?:insert|place|put)\\s+([^，。,.;；、\\n\\r]+?)\\s+(?:into|in|on|onto|inside)\\s+([^，。,.;；、\\n\\r]+)"), QRegularExpression::CaseInsensitiveOption)
    };

    for (const QRegularExpression& pattern : patterns) {
        const QString candidate = extractMatchedSourceText(trimmed, pattern, 1);
        if (!candidate.isEmpty()) return candidate;
    }
    return QString();
}

QString extractPlacementTargetFromText(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return QString();

    static const QRegularExpression patterns[] = {
        QRegularExpression(QString::fromUtf8("(?:把|将|请把|请将)?[^，。,.;；、\\n\\r]+?(?:移动到|挪动到|搬到|放入|放进|放在|放到|塞进|摆到|摆在|移到|移至|挪到|加到|置于)\\s*([^，。,.;；、\\n\\r]+)")),
        QRegularExpression(QStringLiteral("(?:insert|place|put)\\s+[^，。,.;；、\\n\\r]+?\\s+(?:into|in|on|onto|inside)\\s+([^，。,.;；、\\n\\r]+)"), QRegularExpression::CaseInsensitiveOption)
    };

    for (const QRegularExpression& pattern : patterns) {
        const QString candidate = cleanupPlacementTarget(extractMatchedCapture(trimmed, pattern, 1));
        if (!candidate.isEmpty()) return candidate;
    }
    return QString();
}

QString buildRemovalSubjectPrompt(const QString& subject)
{
    return LocalEditPromptUtils::buildLocalRemovalEditPrompt(subject, QString(), 1.20);
}

QString buildReplacementSubjectPrompt(const QString& subject, const QString& replacementDescription)
{
    return LocalEditPromptUtils::buildLocalReplacementEditPrompt(subject, replacementDescription, QString(), 1.20);
}

QString buildMovementSubjectPrompt(const QString& subject, const QString& destinationDescription)
{
    return LocalEditPromptUtils::buildLocalMovementEditPrompt(subject, destinationDescription, QString(), 1.10);
}

QString extractLocalObjectSubjectFromParams(const QJsonObject& params, const QString& sourcePrompt)
{
    const QString sourceText = effectiveEditSourceText(params, sourcePrompt);
    // maskRegion is included because LLMs often put the target object there
    // (e.g. "speech bubble") rather than in a dedicated subject/target field.
    QString subject = extractFirstTrimmedValue(params, QStringList{
        QStringLiteral("subject"), QStringLiteral("target"), QStringLiteral("entity"),
        QStringLiteral("object"), QStringLiteral("item"), QStringLiteral("thing"),
        QStringLiteral("maskRegion")
    });
    if (subject.isEmpty()) subject = extractReplacementSourceFromText(sourceText);
    if (subject.isEmpty()) subject = extractRemovalSubject(params);
    return subject;
}

QString extractLocalObjectDestinationFromParams(const QJsonObject& params, const QString& sourcePrompt)
{
    const QString sourceText = effectiveEditSourceText(params, sourcePrompt);
    QString destination = extractFirstTrimmedValue(params, QStringList{
        QStringLiteral("destination"), QStringLiteral("targetContainer"),
        QStringLiteral("container"), QStringLiteral("location"),
        QStringLiteral("background"), QStringLiteral("scene")
    });
    if (destination.isEmpty()) destination = extractPlacementTargetFromText(sourceText);
    return destination;
}

QString extractBackgroundSubject(const QJsonObject& params)
{
    const QString explicitSubject = extractFirstTrimmedValue(params, QStringList{
        QStringLiteral("background"), QStringLiteral("scene"),
        QStringLiteral("setting"), QStringLiteral("location")
    });
    if (!explicitSubject.isEmpty()) return explicitSubject;

    const QString timeOfDay = params.value(QStringLiteral("timeOfDay")).toString().trimmed();
    if (!timeOfDay.isEmpty()) {
        const QString translated = BibleUtils::Inference::Translation::translateTimeOfDay(timeOfDay);
        if (!translated.isEmpty()) return translated + QString::fromUtf8("背景");
        return timeOfDay + QString::fromUtf8("背景");
    }

    const QString promptFallback = params.value(QStringLiteral("prompt")).toString().trimmed();
    if (!promptFallback.isEmpty()) return promptFallback;

    return QString::fromUtf8("背景");
}

QString buildBackgroundEditPrompt(const QJsonObject& params)
{
    return LocalEditPromptUtils::buildLocalBackgroundEditPrompt(
        QString::fromUtf8("背景"),
        extractBackgroundSubject(params),
        QString::fromUtf8("保持人物、构图、透视、镜头、光线、色调和画幅比例不变"),
        1.20);
}

static QString describeEffect(const QString& effect, double intensity)
{
    static const QMap<QString, QString> effectLabels = {
        {QStringLiteral("tone"),       QString::fromUtf8("网点纹理特效")},
        {QStringLiteral("halftone"),   QString::fromUtf8("半调网点特效")},
        {QStringLiteral("speedline"),  QString::fromUtf8("速度线特效")},
        {QStringLiteral("rain"),       QString::fromUtf8("雨滴特效")},
        {QStringLiteral("snow"),       QString::fromUtf8("雪花特效")},
        {QStringLiteral("blur"),       QString::fromUtf8("动态模糊特效")},
        {QStringLiteral("glow"),       QString::fromUtf8("发光光晕特效")},
        {QStringLiteral("vignette"),   QString::fromUtf8("暗角特效")},
        {QStringLiteral("noise"),      QString::fromUtf8("颗粒噪点特效")},
    };

    const QString label = effectLabels.value(effect.toLower(),
        QString::fromUtf8("视觉特效（%1）").arg(effect));

    if (intensity >= 0.8) {
        return QString::fromUtf8("强烈的%1").arg(label);
    } else if (intensity <= 0.3) {
        return QString::fromUtf8("轻微的%1").arg(label);
    }
    return label;
}

QString buildEffectEditPrompt(const QString& effect, double intensity)
{
    return LocalEditPromptUtils::buildLocalEffectEditPrompt(
        describeEffect(effect, intensity),
        intensity * 1.5);
}

} // namespace ChangeRequestIntentUtils
