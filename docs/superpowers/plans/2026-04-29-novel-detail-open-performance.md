# Novel Detail Open Performance Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove the main-thread blocking work from `NovelDetailPage` so opening a work no longer freezes the UI.

**Architecture:** Keep the current page layout and services, but move the heaviest open-work loading steps off the UI thread. `NovelDetailPage::setNovel()` will become a lightweight state reset plus async load trigger. `StoryboardViewModel` will continue to own storyboards/panels loading, and the page will rely on its loaded signals rather than direct synchronous queries. Bible data refresh will be deferred so it does not block the initial page paint.

**Tech Stack:** C++11, Qt 5.15.2, QtConcurrent, QPointer, AsyncDbHelper, existing viewmodels/services.

---

### Task 1: Make `NovelDetailPage` open flow non-blocking

**Files:**
- Modify: `src/pages/NovelDetailPage.cpp`
- Modify: `include/pages/NovelDetailPage.h`

- [ ] **Step 1: Switch `setNovel()` to lightweight state setup**

```cpp
void NovelDetailPage::setNovel(const Novel &novel)
{
    LOG_DEBUG("NovelDetailPage", QString("setNovel: %1, id=%2").arg(novel.title()).arg(novel.id()));
    const bool keepActivePanelBatch = isPanelBatchTaskForNovel(m_panelBatchTaskId, novel.id());
    m_currentNovel = novel;
    m_currentChapter = 1;
    m_deferredBibleRefresh = false;
    m_currentPanels.clear();
    if (!keepActivePanelBatch) {
        m_panelBatchRefreshPending = false;
        clearPanelBatchTaskState();
    }

    updateDisplay();

    if (m_bibleSectionWidget) {
        m_bibleSectionWidget->setNovelId(novel.id());
    }
}
```

- [ ] **Step 2: Run the app and verify the page still opens with the same visible widgets**

Run: launch the desktop app and open a novel
Expected: the detail page renders immediately without waiting for all chapters/panels/bible content to finish.

- [ ] **Step 3: Replace synchronous open-time DB reads with async viewmodel loads**

```cpp
void NovelDetailPage::updateDisplay()
{
    if (!m_titleLabel || !m_metaLabel) {
        LOG_WARNING("NovelDetailPage", "updateDisplay: UI components not initialized");
        return;
    }

    if (m_currentNovel.id().isEmpty()) {
        m_titleLabel->setText(tr("作品详情"));
        m_metaLabel->setText(tr("请先选择一个作品"));
        return;
    }

    m_titleLabel->setText(m_currentNovel.title());
    m_metaLabel->setText(QString("作品 ID: %1 | 已完成章节: %2 | 当前章节: %3")
        .arg(m_currentNovel.id())
        .arg(m_completedChapterCount)
        .arg(m_currentChapter));

    if (m_chapterCountLabel) {
        m_chapterCountLabel->setText(QString("章节数: %1").arg(m_completedChapterCount));
    }

    if (m_chapterNumberSpin) {
        m_chapterNumberSpin->setValue(m_currentChapter > 0 ? m_currentChapter : 1);
    }

    updateChapterUI(m_currentChapter > 0 ? m_currentChapter : 1);

    refreshChapterCards({});
    refreshStoryboardItems({});

    QTimer::singleShot(0, this, [this]() {
        updateBibleMetaLabel();
    });
}
```

- [ ] **Step 4: Validate the page no longer calls `getAllStoryboards()` or `getPanels()` from `updateDisplay()`**

Run: `rg -n "getAllStoryboards\\(|getPanels\\(|loadPanelsFromDatabase\\(" src/pages/NovelDetailPage.cpp`
Expected: these reads are not used in the initial display path anymore.

- [ ] **Step 5: Commit**

```bash
git add src/pages/NovelDetailPage.cpp include/pages/NovelDetailPage.h
git commit -m "feat: make novel detail opening non-blocking"
```

### Task 2: Defer bible refresh until after initial paint

**Files:**
- Modify: `src/components/BibleSectionWidget.cpp`
- Modify: `include/components/BibleSectionWidget.h`
- Modify: `src/pages/NovelDetailPage.cpp`

- [ ] **Step 1: Add a deferred refresh entry point to `BibleSectionWidget`**

```cpp
void BibleSectionWidget::setNovelId(const QString& novelId)
{
    m_novelId = novelId;
    QTimer::singleShot(0, this, &BibleSectionWidget::refreshBible);
}
```

- [ ] **Step 2: Keep the existing refresh logic intact**

```cpp
void BibleSectionWidget::refreshBible()
{
    if (m_novelId.isEmpty()) {
        clearBible();
        return;
    }

    int characterScrollPos = 0;
    int sceneScrollPos = 0;
    if (m_characterScrollArea) {
        characterScrollPos = m_characterScrollArea->verticalScrollBar()->value();
    }
    if (m_sceneScrollArea) {
        sceneScrollPos = m_sceneScrollArea->verticalScrollBar()->value();
    }

    setUpdatesEnabled(false);
    clearBibleContents();

    const QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(m_novelId);
    const QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(m_novelId);

    m_characterCount = characters.size();
    m_sceneCount = scenes.size();

    if (m_characterContainer) {
        populateCharacterBible(qobject_cast<QVBoxLayout*>(m_characterContainer->layout()), characters);
    }
    if (m_sceneContainer) {
        populateSceneBible(qobject_cast<QVBoxLayout*>(m_sceneContainer->layout()), scenes);
    }

    updateCountLabels();
    setUpdatesEnabled(true);

    if (m_characterScrollArea) {
        m_characterScrollArea->verticalScrollBar()->setValue(characterScrollPos);
    }
    if (m_sceneScrollArea) {
        m_sceneScrollArea->verticalScrollBar()->setValue(sceneScrollPos);
    }

    emit characterCountChanged(m_characterCount);
    emit sceneCountChanged(m_sceneCount);

    LOG_INFO("BibleSectionWidget", QString("Refreshed bible: %1 characters, %2 scenes")
        .arg(m_characterCount).arg(m_sceneCount));
}
```

- [ ] **Step 3: Verify bible refresh no longer happens inside the same event turn as `setNovel()`**

Run: open a novel and watch the window become responsive before bible counts appear.
Expected: initial page paint happens first, bible population follows a moment later.

- [ ] **Step 4: Commit**

```bash
git add src/components/BibleSectionWidget.cpp include/components/BibleSectionWidget.h src/pages/NovelDetailPage.cpp
git commit -m "feat: defer bible refresh on novel open"
```

### Task 3: Verify and regressions

**Files:**
- Modify: none
- Test: existing UI flow and any available unit tests

- [ ] **Step 1: Search for remaining synchronous open-path reads**

```bash
rg -n "setNovel\\(|updateDisplay\\(|refreshBible\\(|getAllStoryboards\\(|getPanels\\(" src/pages/NovelDetailPage.cpp src/components/BibleSectionWidget.cpp
```

- [ ] **Step 2: Open a novel and confirm no long UI freeze**

Expected: window shows content quickly and the cursor does not stay busy for the full load.

- [ ] **Step 3: Watch for regressions in chapter switching and bible refresh**

Expected: chapter cards, storyboard items, and bible data still appear correctly after the deferred load completes.

- [ ] **Step 4: Commit**

```bash
git add src/pages/NovelDetailPage.cpp src/components/BibleSectionWidget.cpp include/components/BibleSectionWidget.h
git commit -m "test: verify deferred novel detail loading"
```
