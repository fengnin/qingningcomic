#pragma once

#include <QString>
#include <QStringList>
#include <QJsonArray>
#include <QJsonObject>
#include "utils/BibleUtils.h"

namespace DialogueTextUtils {

inline bool isNarrationSpeaker(const QString& speaker)
{
    const QString normalized = speaker.trimmed().toLower();
    return normalized == QStringLiteral("narration")
        || normalized == QStringLiteral("旁白")
        || normalized == QStringLiteral("叙述");
}

inline bool hasExplicitDialogueMarker(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    return trimmed.contains(QStringLiteral("："))
        || trimmed.contains(QStringLiteral(":"))
        || trimmed.contains(QStringLiteral("“"))
        || trimmed.contains(QStringLiteral("”"))
        || trimmed.contains(QStringLiteral("\""))
        || trimmed.contains(QStringLiteral("『"))
        || trimmed.contains(QStringLiteral("』"));
}

inline bool shouldTreatAsNarration(const QJsonObject& dialogueObj, const QString& sourceText = QString())
{
    const QString speaker = dialogueObj.value(QStringLiteral("speaker")).toString();
    if (isNarrationSpeaker(speaker)) {
        return true;
    }

    const QString text = dialogueObj.value(QStringLiteral("text")).toString();
    if (text.trimmed().isEmpty()) {
        return true;
    }

    if (!sourceText.trimmed().isEmpty()
        && !BibleUtils::BibleUpdateStrategy::hasDirectTextEvidence(sourceText, text)
        && !hasExplicitDialogueMarker(text)) {
        return true;
    }

    return false;
}

inline QJsonObject normalizeDialogueEntry(const QJsonObject& dialogueObj, const QString& sourceText = QString())
{
    QJsonObject normalized = dialogueObj;
    const QString text = normalized.value(QStringLiteral("text")).toString().trimmed();
    if (shouldTreatAsNarration(normalized, sourceText)) {
        normalized[QStringLiteral("speaker")] = QStringLiteral("narration");
        normalized[QStringLiteral("bubbleType")] = QStringLiteral("narration");
        normalized.remove(QStringLiteral("speakerSide"));
        normalized.remove(QStringLiteral("speaker_side"));
        normalized[QStringLiteral("text")] = text;
    } else {
        normalized[QStringLiteral("bubbleType")] = normalized.value(QStringLiteral("bubbleType")).toString(QStringLiteral("speech"));
        normalized[QStringLiteral("text")] = text;
    }
    return normalized;
}

inline QJsonArray normalizeDialogueArray(const QJsonArray& dialogueArray, const QString& sourceText = QString())
{
    QJsonArray normalized;
    for (const QJsonValue& value : dialogueArray) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject entry = normalizeDialogueEntry(value.toObject(), sourceText);
        if (!entry.value(QStringLiteral("text")).toString().trimmed().isEmpty()) {
            normalized.append(entry);
        }
    }
    return normalized;
}

}
