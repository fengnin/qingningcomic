# Speech Bubble Proximity Rule Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep panel dialogue bubbles visually close to the speaking character so they read like they come from the person, not from the page margin.

**Architecture:** Keep the change local to `DialogueBubbleRenderer`. Replace the current fixed left/right column placement for panel rendering with a small set of face-centered candidate rectangles, score them by proximity and in-bounds safety, then draw the best one. Preserve the existing non-panel fallback so generic dialog rendering stays stable.

**Tech Stack:** C++11, Qt 5.15.2, QPainter/QRectF/QPointF.

---

### Task 1: Add proximity-based bubble candidate selection

**Files:**
- Modify: `src/utils/DialogueBubbleRenderer.cpp`

- [ ] **Step 1: Review the current panel layout logic**

```cpp
// createBubbleLayout currently places panel bubbles into fixed columns and
// starts them at the top margin, which is why the bubble can drift far away
// from the speaker.
```

- [ ] **Step 2: Replace fixed placement with face-centered candidates**

```cpp
// Build a small set of candidate rects around the inferred face position:
// above-center, above-left, above-right, below-center, below-left, below-right.
// Clamp them to the image bounds and score them by distance to the speaker face.
```

- [ ] **Step 3: Keep the existing generic renderer fallback**

```cpp
// When panel positioning is unavailable, keep the existing alternating layout
// so the non-panel path does not change behavior unexpectedly.
```

- [ ] **Step 4: Verify the file still compiles cleanly**

Run: `qmake && mingw32-make` or the repository's existing Qt build command
Expected: Build succeeds with no new compile errors in `DialogueBubbleRenderer`.

- [ ] **Step 5: Commit**

```bash
git add src/utils/DialogueBubbleRenderer.cpp docs/superpowers/plans/2026-05-05-speech-bubble-proximity-rule.md
git commit -m "fix: keep panel speech bubbles close to speakers"
```

### Task 2: Sanity-check the output visually

**Files:**
- No code changes

- [ ] **Step 1: Render a representative panel with a speaker near the edge**

```cpp
// Use an existing panel image that previously produced a detached bubble and
// confirm the bubble now stays near the speaker head with a short tail.
```

- [ ] **Step 2: Verify the bubble no longer sits at the top margin**

```cpp
// The expected outcome is that the bubble reads like figure 2 in the example:
// close to the speaking character and not hanging far above the scene.
```

- [ ] **Step 3: Commit if any follow-up fix was needed**

```bash
git add <any touched files>
git commit -m "fix: tighten speech bubble placement"
```

## Self-Review

- Spec coverage: The requested behavioral change is covered by Task 1, with Task 2 reserved for visual confirmation.
- Placeholder scan: No TBD/TODO placeholders remain.
- Type consistency: All types referenced are already present in `DialogueBubbleRenderer` and Qt.
- Scope check: The plan is intentionally narrow and only changes the panel bubble placement rule.
