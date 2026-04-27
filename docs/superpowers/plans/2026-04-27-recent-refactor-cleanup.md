# Recent Refactor Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reduce duplication and simplify the recently modified delete dialog and panel batch-generation flow without changing behavior.

**Architecture:** Keep the work localized to the page and dialog that already own the behavior. Extract small helpers for shared state transitions, shorten the batch-generation handlers, and make the delete dialog API and sizing method clearer.

**Tech Stack:** C++11, Qt 5.15.2 Widgets, existing app services and view models.

---

### Task 1: Simplify panel batch-generation state handling

**Files:**
- Modify: `src/pages/NovelDetailPage.cpp`
- Modify: `include/pages/NovelDetailPage.h`

- [ ] **Step 1: Identify the shared state transitions**

The batch flow currently repeats button recovery, task-id clearing, and failure UI handling in multiple branches. The helper set should cover:

```cpp
bool isPanelBatchTaskForNovel(const QString& taskId, const QString& novelId) const;
void clearPanelBatchTaskState();
void handlePanelBatchGenerationFailure(const QString& errorMessage);
```

- [ ] **Step 2: Implement the helpers**

Use the helpers to remove repeated `if (m_generatePanelsBtn)` blocks and repeated `m_panelBatchTaskId` / `m_panelBatchNovelId` cleanup code.

- [ ] **Step 3: Update the call sites**

Replace the duplicated guard logic in `setNovel`, `onPanelBatchTaskCompleted`, `onPanelBatchTaskFailed`, and `startPanelBatchGeneration`.

- [ ] **Step 4: Verify the batch flow still compiles**

Run the project build and confirm there are no missing declarations or signature mismatches.

### Task 2: Clean up the delete confirmation dialog

**Files:**
- Modify: `src/components/DeleteConfirmDialog.cpp`
- Modify: `include/components/DeleteConfirmDialog.h`

- [ ] **Step 1: Rename the sizing method**

Rename the custom `adjustSize()` helper to `updateFixedSize()` so it does not read like `QWidget::adjustSize()` override behavior.

- [ ] **Step 2: Extract the repeated button creation logic**

Move the repeated button style setup into a small helper that builds the cancel/confirm buttons with a passed style and font weight.

- [ ] **Step 3: Remove dead code**

Drop the empty `showEvent` override if it is not needed and keep the dialog API focused on the actual customization points.

- [ ] **Step 4: Verify the dialog still opens and sizes correctly**

Run the existing build or smoke-test the delete flow in the app.

### Task 3: Review for remaining duplication

**Files:**
- Modify: `src/pages/NovelDetailPage.cpp`
- Modify: `src/components/DeleteConfirmDialog.cpp`

- [ ] **Step 1: Scan for repeated cleanup blocks**

Look for any remaining repeated button reset, progress reset, or warning-message code in the modified paths.

- [ ] **Step 2: Confirm naming readability**

Check that the helper names describe the state transition they perform and do not leak implementation details.

- [ ] **Step 3: Run a final diff check**

Run `git diff --check` to confirm the patch is clean.
