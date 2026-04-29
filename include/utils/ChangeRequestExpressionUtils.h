#ifndef CHANGEREQUESTEXPRESSIONUTILS_H
#define CHANGEREQUESTEXPRESSIONUTILS_H

#include <QString>
#include <initializer_list>

namespace ChangeRequestExpressionUtils {

inline bool containsAnyKeyword(const QString& normalizedText,
                               std::initializer_list<const char*> keywords)
{
    for (const char* keyword : keywords) {
        if (!keyword) {
            continue;
        }

        const QString normalizedKeyword = QString::fromUtf8(keyword).toLower();
        if (!normalizedKeyword.isEmpty() && normalizedText.contains(normalizedKeyword)) {
            return true;
        }
    }

    return false;
}

inline QString canonicalExpressionFromText(const QString& text)
{
    const QString normalizedText = text.trimmed().toLower();
    if (normalizedText.isEmpty()) {
        return QString();
    }

    struct ExpressionRule {
        const char* canonical;
        std::initializer_list<const char*> keywords;
    };

    static const ExpressionRule rules[] = {
        {"angry", {"生气", "愤怒", "恼火", "发火", "怒", "angry"}},
        {"happy", {"开心", "高兴", "微笑", "笑容", "笑", "happy"}},
        {"sad", {"难过", "伤心", "悲伤", "沮丧", "sad"}},
        {"surprised", {"惊讶", "吃惊", "震惊", "surprised"}},
        {"scared", {"害怕", "恐惧", "紧张", "scared"}},
        {"determined", {"坚定", "认真", "严肃", "决心", "determined"}},
        {"confused", {"困惑", "疑惑", "不解", "confused"}},
        {"embarrassed", {"害羞", "尴尬", "脸红", "embarrassed"}},
        {"smirking", {"坏笑", "狡黠", "得意", "smirking"}},
        {"neutral", {"平静", "中性", "冷静", "neutral"}}
    };

    for (const ExpressionRule& rule : rules) {
        if (containsAnyKeyword(normalizedText, rule.keywords)) {
            return QString::fromLatin1(rule.canonical);
        }
    }

    return QString();
}

inline QString normalizeExpressionValue(const QString& expression)
{
    const QString canonical = canonicalExpressionFromText(expression);
    if (!canonical.isEmpty()) {
        return canonical;
    }
    return expression.trimmed().toLower();
}

inline QString expressionPromptDescription(const QString& expression)
{
    const QString canonical = normalizeExpressionValue(expression);
    if (canonical == QStringLiteral("angry")) {
        return QStringLiteral("生气，眉头紧皱，眼神锐利");
    }
    if (canonical == QStringLiteral("happy")) {
        return QStringLiteral("开心，嘴角上扬，露出笑容");
    }
    if (canonical == QStringLiteral("sad")) {
        return QStringLiteral("难过，眼神低落，嘴角下垂");
    }
    if (canonical == QStringLiteral("surprised")) {
        return QStringLiteral("惊讶，睁大眼睛，眉毛上挑，嘴巴微张");
    }
    if (canonical == QStringLiteral("scared")) {
        return QStringLiteral("害怕，眼神紧张，表情收缩");
    }
    if (canonical == QStringLiteral("determined")) {
        return QStringLiteral("坚定，目光专注，神情坚决");
    }
    if (canonical == QStringLiteral("confused")) {
        return QStringLiteral("困惑，眉头轻皱，带有疑问神情");
    }
    if (canonical == QStringLiteral("embarrassed")) {
        return QStringLiteral("害羞或尴尬，轻微脸红，眼神回避");
    }
    if (canonical == QStringLiteral("smirking")) {
        return QStringLiteral("坏笑，嘴角微扬，神情带一点得意");
    }
    if (canonical == QStringLiteral("neutral")) {
        return QStringLiteral("平静，中性表情");
    }

    return expression.trimmed();
}

inline QString canonicalExpressionFromNaturalLanguage(const QString& naturalLanguage)
{
    return canonicalExpressionFromText(naturalLanguage);
}

}

#endif // CHANGEREQUESTEXPRESSIONUTILS_H
