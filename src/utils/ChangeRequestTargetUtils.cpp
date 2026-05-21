#include "utils/ChangeRequestTargetUtils.h"
#include <QHash>

namespace ChangeRequestTargetUtils {

int chineseNumeralToInt(const QString& text)
{
    static const QHash<QChar, int> digitMap = {
        {QChar(0x96F6), 0},
        {QChar(0x3007), 0},
        {QChar(0x4E00), 1},
        {QChar(0x4E8C), 2},
        {QChar(0x4E09), 3},
        {QChar(0x56DB), 4},
        {QChar(0x4E94), 5},
        {QChar(0x516D), 6},
        {QChar(0x4E03), 7},
        {QChar(0x516B), 8},
        {QChar(0x4E5D), 9},
        {QChar(0x4E24), 2}
    };

    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }

    bool ok = false;
    const int arabic = trimmed.toInt(&ok);
    if (ok) {
        return arabic;
    }

    if (trimmed == QStringLiteral("十")) {
        return 10;
    }

    const int tenIndex = trimmed.indexOf(QChar(0x5341));
    if (tenIndex >= 0) {
        const QString tensPart = trimmed.left(tenIndex);
        const QString onesPart = trimmed.mid(tenIndex + 1);

        const int tens = tensPart.isEmpty() ? 1 : digitMap.value(tensPart.at(0), -1);
        const int ones = onesPart.isEmpty() ? 0 : digitMap.value(onesPart.at(0), -1);
        if (tens >= 0 && ones >= 0) {
            return tens * 10 + ones;
        }
    }

    if (trimmed.size() == 1) {
        const int value = digitMap.value(trimmed.at(0), -1);
        if (value >= 0) {
            return value;
        }
    }

    return 0;
}

int extractRequestedPanelNumber(const QString& text)
{
    const QString normalized = normalizePanelReferenceText(text);
    static const QRegularExpression patterns[] = {
        QRegularExpression(QStringLiteral(R"(第\s*(\d+)\s*(?:个)?\s*(?:面板|分镜)(?:中|里|内|上|下|左|右|的|之中|里面)?)")),
        QRegularExpression(QStringLiteral(R"(第\s*([一二三四五六七八九十两〇零]+)\s*(?:个)?\s*(?:面板|分镜)(?:中|里|内|上|下|左|右|的|之中|里面)?)")),
        QRegularExpression(QStringLiteral(R"((?:面板|分镜)\s*#?\s*(\d+))")),
        QRegularExpression(QStringLiteral(R"(第\s*([一二三四五六七八九十两〇零]+)\s*(?:个)?\s*面板(?:中|里|内|上|下|左|右|的|之中|里面)?)")),
        QRegularExpression(QStringLiteral(R"((?:panel|panels|面板|分镜)[_-]?(?:#|no\.?)?\s*(\d+))"), QRegularExpression::CaseInsensitiveOption)
    };

    for (const QRegularExpression& pattern : patterns) {
        const QRegularExpressionMatch match = pattern.match(normalized);
        if (match.hasMatch()) {
            const QString captured = match.captured(1);
            bool ok = false;
            const int number = captured.toInt(&ok);
            const int normalizedNumber = ok ? number : chineseNumeralToInt(captured);
            if (normalizedNumber > 0) {
                return normalizedNumber;
            }
        }
    }

    return 0;
}

} // namespace ChangeRequestTargetUtils
