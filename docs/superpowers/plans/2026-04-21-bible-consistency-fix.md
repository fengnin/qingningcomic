# Bible Consistency Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep character and scene bible updates stable across chapters without over-merging or dropping usable scene records.

**Architecture:** Tighten character identity matching so only exact names or explicit aliases merge. Relax scene persistence so low-confidence scenes are preserved and can be stabilized by later chapters instead of being discarded immediately.

**Tech Stack:** C++11, Qt 5.15.2, qmake, SQLite/MySQL access through the existing `DatabaseManager`.

---

### Task 1: Restrict character merging to explicit identity matches

**Files:**
- Modify: `src/services/CharacterExtractor.cpp`
- Modify: `include/services/CharacterExtractor.h`

- [ ] **Step 1: Narrow identity matching**

Replace the broad shared-key match with a rule that prefers exact normalized name matches and explicit alias matches. Keep role/gender/age as supporting signals only, not primary merge keys.

- [ ] **Step 2: Preserve aliases safely**

Continue storing historical aliases on the merged character, but do not treat every alias-like string as a merge trigger.

- [ ] **Step 3: Verify with syntax-only build**

Run: `g++ -std=c++11 -fsyntax-only ... src/services/CharacterExtractor.cpp`
Expected: PASS.

### Task 2: Stop dropping scene records solely because they are unstable

**Files:**
- Modify: `src/services/SceneExtractor.cpp`
- Modify: `include/utils/SceneKeyUtils.h`

- [ ] **Step 1: Make scene stability a ranking signal**

When a scene cannot produce a stable key, fall back to a deterministic surrogate key instead of returning an empty scene and skipping it.

- [ ] **Step 2: Keep merge behavior conservative**

Only merge scenes when identity keys match or similarity is clearly above threshold. Do not auto-merge on weak similarity alone.

- [ ] **Step 3: Verify with syntax-only build**

Run: `g++ -std=c++11 -fsyntax-only ... src/services/SceneExtractor.cpp`
Expected: PASS.

### Task 3: Smoke test the chapter 3 pipeline

**Files:**
- Test: `src/控制台.txt`

- [ ] **Step 1: Re-run chapter 3 analysis**

Confirm the third chapter now keeps distinct parents separate and persists the scene set instead of dropping everything as unstable.

- [ ] **Step 2: Check dashboard/bible counts**

Verify the bible meta counts and scene list stay consistent after save and reload.
