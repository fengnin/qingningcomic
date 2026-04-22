# Bible Consistency Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make character bible data consistent with source text, ensure sparse supporting characters still get usable bible entries, and keep batch image generation from stopping early.

**Architecture:** Keep the fix local to the existing extraction and image-batch flow. Character extraction should prefer source-text evidence when it is stronger than AI defaults, while still preserving AI-provided fields when the text is silent. Batch image generation should advance through every pending item and only finish once all queued characters and scenes are processed.

**Tech Stack:** C++11, Qt 5.15.2, existing `CharacterExtractor`, `BibleImageService`, and `BibleSectionWidget` classes.

---

### Task 1: Make character extraction prefer source text for obvious appearance cues

**Files:**
- Modify: `src/services/CharacterExtractor.cpp`
- Modify: `include/services/CharacterExtractor.h`

- [ ] **Step 1: Tighten the extraction rules**

Add a helper that can recognize the wider set of hair-color phrases seen in prose, including `浅亚麻色`, `亚麻色`, `深棕色`, `棕褐色`, `银灰色`, `灰白色`, and `黑褐色`.

```cpp
static const QList<QRegularExpression> hairColorPatterns = {
    QRegularExpression(QString::fromUtf8("((?:浅亚麻|亚麻|浅棕|深棕|棕褐|黑褐|乌黑|灰白|银灰|花白|银白|金|栗)[色]?)[^，。；\\n]{0,8}(?:低马尾|高马尾|马尾|短发|长发|卷发|碎发|头发)")),
    QRegularExpression(QString::fromUtf8("(浅亚麻色|亚麻色|浅棕色|深棕色|棕褐色|黑褐色|黑色|乌黑|灰白色|银灰色|花白|银白|金色|栗色)"))
};
```

- [ ] **Step 2: Make source text able to override weak AI defaults**

Update `enrichCharacterFromText` so that if the extracted hair color is missing or a generic AI default, the text-derived color can replace it. Keep other fields as current behavior unless the text has a stronger match.

```cpp
if (extracted.hairColor.isEmpty() || isGenericHairColor(extracted.hairColor)) {
    const QString textHairColor = matchFirstCapture(context, hairColorPatterns);
    if (!textHairColor.isEmpty()) {
        extracted.hairColor = textHairColor;
    }
}
```

- [ ] **Step 3: Verify the saved record carries the corrected field**

Ensure `saveCharacter`, `mergeCharacters`, and `characterToData` still persist the updated appearance object without losing the rewritten hair color.

---

### Task 2: Stop saving empty supporting characters

**Files:**
- Modify: `src/services/CharacterExtractor.cpp`
- Modify: `src/services/AnalysisService.cpp`

- [ ] **Step 1: Fill sparse characters from the full source text**

If `parseAICharacter` returns only `name` and `role`, run the same text enrichment against the full novel text before saving. This lets `林阿姨` inherit usable fields from the prose even when the AI response is empty.

```cpp
QList<ExtractedCharacter> extractedChars = CharacterExtractor::instance()->extractFromCharacters(
    result.characters, NovelService::instance()->loadText(novelId));
```

- [ ] **Step 2: Preserve characters that still have no appearance fields only when they are meaningful**

Keep the generic-character filter, but do not silently accept fully empty supporting characters if a specific name exists and the source text can be searched for it. Let the enrichment pass populate at least the name-linked fields before save.

```cpp
if (!extracted.name.isEmpty()) {
    enrichCharacterFromText(extracted, sourceText);
}
```

- [ ] **Step 3: Confirm the bible UI renders the saved values**

No UI rewrite is needed unless the data shape changes. The current `BibleSectionWidget::buildCharacterDetails` should render non-empty fields once the extractor writes them.

---

### Task 3: Fix batch image generation so it does not stop after the first skipped character

**Files:**
- Modify: `src/services/BibleImageService.cpp`
- Test: `src/控制台.txt` during a manual run

- [ ] **Step 1: Fix the control flow in the batch loop**

Change `processNextCharacter()` so the “already has portrait” path advances to the next item without allowing `processNextItem()` to fall through to `finishBatch()` too early.

```cpp
if (!character.portraitPath().isEmpty()) {
    LOG_INFO("BibleImageService", QString("Skipping character %1, already has portrait")
        .arg(character.name()));
    recordSuccess();
    m_currentCharIndex++;
    scheduleNext([this]() { processNextItem(); });
    return true;
}
```

- [ ] **Step 2: Make `processNextItem()` only finish when the queue is truly exhausted**

Guard the final `finishBatch()` call so it happens only when both pending character and scene queues are done.

```cpp
if (m_currentType == BibleImageConstants::TYPE_CHARACTER && m_currentCharIndex < m_pendingCharacters.size()) {
    hasMore = processNextCharacter();
} else if (m_currentType == BibleImageConstants::TYPE_SCENE && m_currentSceneIndex < m_pendingScenes.size()) {
    hasMore = processNextScene();
}

if (!hasMore) {
    if (m_combinedMode
        && (m_currentCharIndex < m_pendingCharacters.size() || m_currentSceneIndex < m_pendingScenes.size())) {
        scheduleNext([this]() { processNextItem(); });
        return;
    }
    finishBatch();
}
```

- [ ] **Step 3: Run the app once and verify the log**

Confirm the log shows every character and scene being processed, and that `All Bible images completed` only appears after the last queued item.

---

### Self-Review

- [ ] Confirm `浅亚麻色` is covered by extraction.
- [ ] Confirm `林阿姨` can be saved with non-empty appearance data when the text supports it.
- [ ] Confirm combined bible image generation reaches later characters/scenes instead of ending after the first skipped portrait.
