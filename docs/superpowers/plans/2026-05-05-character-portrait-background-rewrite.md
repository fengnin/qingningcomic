# Character Portrait Background Rewrite Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make character portraits consistently save with a pure white background, even when the model returns gray, beige, or slightly tinted backgrounds.

**Architecture:** Keep image generation and task orchestration unchanged. Replace the current ad hoc portrait save path with a small image post-processing pipeline that trims dark borders, normalizes near-white background pixels to pure white, and then re-encodes the image. Keep the new logic isolated to character portrait saving so scene and panel images stay untouched.

**Tech Stack:** C++11, Qt 5.15.2, QImage/QBuffer, QtTest.

---

### Task 1: Write regression tests for portrait background cleanup

**Files:**
- Modify: `tests/DialogueBubbleRendererTests.cpp`
- Modify: `tests/DialogueBubbleRendererTests.pro`

- [ ] **Step 1: Write the failing test**

```cpp
void DialogueBubbleRendererTests::characterPortraitBackgroundNormalizerTurnsNearWhiteBackgroundPureWhite()
{
    QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(245, 243, 238));

    QPainter painter(&image);
    painter.fillRect(10, 8, 12, 16, QColor(80, 50, 40));
    painter.end();

    const QByteArray inputBytes = []() {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        return bytes;
    }();

    bool normalized = false;
    const QByteArray outputBytes = ImageBackgroundNormalizer::normalizeNearWhiteBackgroundImageData(inputBytes, &normalized);
    QVERIFY(normalized);

    QImage output;
    QVERIFY(output.loadFromData(outputBytes));
    QCOMPARE(output.pixelColor(0, 0), QColor(Qt::white));
    QCOMPARE(output.pixelColor(31, 31), QColor(Qt::white));
    QVERIFY(output.pixelColor(16, 16) != QColor(Qt::white));
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `qmake tests/DialogueBubbleRendererTests.pro && mingw32-make -C tests`

Expected: build fails because `ImageBackgroundNormalizer` does not exist yet.

- [ ] **Step 3: Keep the test and move on**

Do not change production code until the new test fails for the missing feature.

### Task 2: Implement the portrait background normalizer

**Files:**
- Create: `include/utils/ImageBackgroundNormalizer.h`
- Create: `src/utils/ImageBackgroundNormalizer.cpp`

- [ ] **Step 1: Write the minimal implementation**

```cpp
class ImageBackgroundNormalizer
{
public:
    static QImage normalizeNearWhiteBackground(const QImage& image, bool* normalized = nullptr);
    static QByteArray normalizeNearWhiteBackgroundImageData(const QByteArray& imageData, bool* normalized = nullptr);
};
```

Implement a border-seeded flood fill that:
- Samples edge pixels to estimate the background color
- Treats pixels close to that background as background
- Rewrites only the connected background region to pure white
- Leaves the character subject intact

- [ ] **Step 2: Run the test again**

Run: `qmake tests/DialogueBubbleRendererTests.pro && mingw32-make -C tests`

Expected: the new regression test passes.

- [ ] **Step 3: Add focused helper tests if needed**

If the first test passes but the algorithm is too broad, add a second test that protects a light-colored subject from being whitened.

### Task 3: Wire portrait saving to the new post-process path

**Files:**
- Modify: `src/services/BibleImageService.cpp`
- Modify: `include/services/BibleImageService.h` only if the new helper needs a declaration change

- [ ] **Step 1: Replace the old save-time image handling**

Update `saveAndEmitResult()` so character portraits go through:

```cpp
const QByteArray trimmed = ImageBorderTrimmer::trimDarkBorderImageData(imageData);
const QByteArray processed = ImageBackgroundNormalizer::normalizeNearWhiteBackgroundImageData(trimmed);
```

Keep scene images unchanged for now.

- [ ] **Step 2: Add logs for before/after behavior**

Log:
- original size
- whether trimming happened
- whether background normalization happened
- final size

- [ ] **Step 3: Re-run the image regression test**

Run: `qmake tests/DialogueBubbleRendererTests.pro && mingw32-make -C tests`

Expected: passes.

### Task 4: Verify the full build path

**Files:**
- None

- [ ] **Step 1: Build the application**

Run the project build command used in this workspace.

- [ ] **Step 2: Re-generate one character portrait**

Confirm the console log shows the portrait save path using the new background normalization step.

- [ ] **Step 3: Inspect the saved portrait**

Verify the background is visually pure white and the subject is unchanged.

