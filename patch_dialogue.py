import sys

path = r"E:/QingningComic/qingning/comic/src/services/ChangeRequestService.cpp"
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Marker: start of the block we want to replace
START = '    if (action == QStringLiteral("rewrite_dialogue")) {'
# Marker: the closing brace + return statement that ends executeDialogueOperation
END = '''    return ChangeRequestExecutionUtils::buildDialogueResult(
        dialogueId.isEmpty() ? QStringLiteral("dlg-1") : dialogueId,
        speaker,
        newText);
}'''

start_idx = content.find(START)
end_idx = content.find(END, start_idx)
if start_idx == -1 or end_idx == -1:
    print("ERROR: markers not found", file=sys.stderr)
    sys.exit(1)

end_idx += len(END)

NEW_BLOCK = '''    if (action == QStringLiteral("rewrite_dialogue")) {
        if (newText.isEmpty()) {
            QString instruction = op.params["instruction"].toString();
            if (instruction.isEmpty()) {
                instruction = QString::fromUtf8(u8"\\u8bf7\\u76f4\\u63a5\\u91cd\\u5199\\u5bf9\\u767d");
            }

            QString originalText;
            if (!dialogues.isEmpty()) {
                originalText = dialogues.first().text;
            }
            newText = m_qwenClient->rewriteDialogue(originalText, instruction);
        }

        bool updated = false;

        auto applyRewrite = [&](DialogueLine& line) {
            if (!speaker.isEmpty()) line.speaker = speaker;
            line.text = newText;
            // preserve existing bubbleType and speakerSide
        };

        if (!dialogueId.isEmpty()) {
            bool isIndex = false;
            int idx = dialogueId.toInt(&isIndex);

            if (isIndex && idx >= 0 && idx < dialogues.size()) {
                applyRewrite(dialogues[idx]);
                updated = true;
            } else {
                for (int i = 0; i < dialogues.size(); ++i) {
                    if (dialogues[i].speaker == dialogueId) {
                        applyRewrite(dialogues[i]);
                        updated = true;
                        break;
                    }
                }
            }
        }

        if (!updated && !speaker.isEmpty()) {
            for (int i = 0; i < dialogues.size(); ++i) {
                if (dialogues[i].speaker == speaker) {
                    applyRewrite(dialogues[i]);
                    updated = true;
                    break;
                }
            }
        }

        if (!updated) {
            if (!dialogues.isEmpty()) {
                applyRewrite(dialogues[0]);
            } else {
                DialogueLine newLine;
                newLine.speaker = speaker.isEmpty() ? QStringLiteral("Unknown") : speaker;
                newLine.text = newText;
                newLine.bubbleType = QStringLiteral("speech");
                dialogues.append(newLine);
            }
        }

        if (!updatePanelDialogue(dsl.targetId, dialogues, newText)) {
            throw std::runtime_error(m_lastError.toStdString());
        }

        QJsonObject generationResult;
        ImageService::EditHint editHint;
        editHint.forceProvider = QStringLiteral("qwen");
        if (!runPanelImageGeneration(dsl.targetId, QStringLiteral("preview"), generationResult, editHint)) {
            throw std::runtime_error(m_lastError.toStdString());
        }
    } else if (action == QStringLiteral("add_dialogue")) {
        DialogueLine newLine;
        newLine.speaker = speaker.isEmpty() ? QStringLiteral("Unknown") : speaker;
        newLine.text = newText;
        const QString bubbleType = op.params["bubbleType"].toString();
        newLine.bubbleType = bubbleType.isEmpty() ? QStringLiteral("speech") : bubbleType;
        newLine.speakerSide = op.params["speakerSide"].toString();
        dialogues.append(newLine);

        if (!updatePanelDialogue(dsl.targetId, dialogues, newText)) {
            throw std::runtime_error(m_lastError.toStdString());
        }

        QJsonObject generationResult;
        ImageService::EditHint editHint;
        editHint.forceProvider = QStringLiteral("qwen");
        if (!runPanelImageGeneration(dsl.targetId, QStringLiteral("preview"), generationResult, editHint)) {
            throw std::runtime_error(m_lastError.toStdString());
        }
    } else if (action == QStringLiteral("remove_dialogue")) {
        bool removed = false;

        if (!dialogueId.isEmpty()) {
            bool isIndex = false;
            int idx = dialogueId.toInt(&isIndex);
            if (isIndex && idx >= 0 && idx < dialogues.size()) {
                dialogues.removeAt(idx);
                removed = true;
            } else {
                for (int i = 0; i < dialogues.size(); ++i) {
                    if (dialogues[i].speaker == dialogueId) {
                        dialogues.removeAt(i);
                        removed = true;
                        break;
                    }
                }
            }
        }

        if (!removed && !speaker.isEmpty()) {
            for (int i = 0; i < dialogues.size(); ++i) {
                if (dialogues[i].speaker == speaker) {
                    dialogues.removeAt(i);
                    removed = true;
                    break;
                }
            }
        }

        if (!removed) {
            throw std::runtime_error(u8"remove_dialogue: \\u672a\\u627e\\u5230\\u5339\\u914d\\u7684\\u5bf9\\u767d\\uff0c\\u8bf7\\u63d0\\u4f9b dialogueId \\u6216 speaker");
        }

        // empty newText skips editIntent in updatePanelDialogue, triggering full regen
        if (!updatePanelDialogue(dsl.targetId, dialogues, QString())) {
            throw std::runtime_error(m_lastError.toStdString());
        }

        QJsonObject generationResult;
        ImageService::EditHint editHint;
        editHint.forceProvider = QStringLiteral("qwen");
        if (!runPanelImageGeneration(dsl.targetId, QStringLiteral("preview"), generationResult, editHint)) {
            throw std::runtime_error(m_lastError.toStdString());
        }
    } else if (action == QStringLiteral("update_dialogue_side")) {
        const QString newSide = op.params["speakerSide"].toString().toLower().trimmed();
        if (newSide != QStringLiteral("left") && newSide != QStringLiteral("right")) {
            throw std::runtime_error(u8"update_dialogue_side: speakerSide \\u5fc5\\u987b\\u4e3a left \\u6216 right");
        }

        bool updated = false;

        if (!dialogueId.isEmpty()) {
            bool isIndex = false;
            int idx = dialogueId.toInt(&isIndex);
            if (isIndex && idx >= 0 && idx < dialogues.size()) {
                dialogues[idx].speakerSide = newSide;
                updated = true;
            } else {
                for (int i = 0; i < dialogues.size(); ++i) {
                    if (dialogues[i].speaker == dialogueId) {
                        dialogues[i].speakerSide = newSide;
                        updated = true;
                        break;
                    }
                }
            }
        }

        if (!updated && !speaker.isEmpty()) {
            for (int i = 0; i < dialogues.size(); ++i) {
                if (dialogues[i].speaker == speaker) {
                    dialogues[i].speakerSide = newSide;
                    updated = true;
                    break;
                }
            }
        }

        if (!updated) {
            throw std::runtime_error(u8"update_dialogue_side: \\u672a\\u627e\\u5230\\u5339\\u914d\\u7684\\u5bf9\\u767d\\uff0c\\u8bf7\\u63d0\\u4f9b dialogueId \\u6216 speaker");
        }

        // empty newText so updatePanelDialogue triggers full regen to reflect new bubble direction
        if (!updatePanelDialogue(dsl.targetId, dialogues, QString())) {
            throw std::runtime_error(m_lastError.toStdString());
        }

        QJsonObject generationResult;
        ImageService::EditHint editHint;
        editHint.forceProvider = QStringLiteral("qwen");
        if (!runPanelImageGeneration(dsl.targetId, QStringLiteral("preview"), generationResult, editHint)) {
            throw std::runtime_error(m_lastError.toStdString());
        }
    }

    return ChangeRequestExecutionUtils::buildDialogueResult(
        dialogueId.isEmpty() ? QStringLiteral("dlg-1") : dialogueId,
        speaker,
        newText);
}'''

result = content[:start_idx] + NEW_BLOCK + content[end_idx:]
with open(path, 'w', encoding='utf-8') as f:
    f.write(result)

print(f"OK: replaced {end_idx - start_idx} chars at offset {start_idx}")
