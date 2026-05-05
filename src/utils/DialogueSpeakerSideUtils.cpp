#include "utils/DialogueSpeakerSideUtils.h"

#include <QRegularExpression>

namespace {
    QString normalizeSideKey(const QString& side)
    {
        const QString normalized = side.trimmed().toLower();
        if (normalized == QStringLiteral("left")
            || normalized == QStringLiteral("right")
            || normalized == QStringLiteral("center")
            || normalized == QStringLiteral("auto")) {
            return normalized;
        }
        return QString();
    }
}

QString DialogueSpeakerSideUtils::normalize(const QString& side)
{
    return normalizeSideKey(side);
}

QString DialogueSpeakerSideUtils::marker(const QString& side)
{
    const QString normalized = normalize(side);
    if (normalized == QStringLiteral("left")) {
        return QStringLiteral("[左]");
    }
    if (normalized == QStringLiteral("right")) {
        return QStringLiteral("[右]");
    }
    if (normalized == QStringLiteral("center")) {
        return QStringLiteral("[中]");
    }
    if (normalized == QStringLiteral("auto")) {
        return QStringLiteral("[自动]");
    }
    return QString();
}

QString DialogueSpeakerSideUtils::labelWithSide(const QString& speaker, const QString& side)
{
    const QString sideMarker = marker(side);
    if (sideMarker.isEmpty()) {
        return speaker;
    }
    return QStringLiteral("%1%2").arg(speaker, sideMarker);
}

void DialogueSpeakerSideUtils::splitLabelAndSide(QString& speaker, QString& side)
{
    speaker = speaker.trimmed();
    side = normalize(side);

    static const QRegularExpression trailingSidePattern(
        QStringLiteral(R"(^(.+?)\s*[\[\(（](左|右|中|自动|auto)[\]\)）]\s*$)"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = trailingSidePattern.match(speaker);
    if (!match.hasMatch()) {
        return;
    }

    speaker = match.captured(1).trimmed();
    const QString parsedSide = match.captured(2).trimmed().toLower();
    if (parsedSide == QStringLiteral("左")) {
        side = QStringLiteral("left");
    } else if (parsedSide == QStringLiteral("右")) {
        side = QStringLiteral("right");
    } else if (parsedSide == QStringLiteral("中")) {
        side = QStringLiteral("center");
    } else if (parsedSide == QStringLiteral("自动") || parsedSide == QStringLiteral("auto")) {
        side = QStringLiteral("auto");
    }
}
