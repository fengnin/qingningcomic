# Change Request Pipeline Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Harden the natural-language change-request pipeline so parsed DSL values are normalized, validated, and routed to the correct execution target.

**Architecture:** Normalize `scope`, `type`, and `action` as soon as JSON is parsed into `ChangeRequestDsl`, then revalidate historical DSL before execution. Resolve `targetId` from context for panel-based edits and from `storyboardId` for style edits so requests do not fail late in execution.

**Tech Stack:** C++11, Qt 5.15.2, QJsonObject, service-layer validation

---

### Task 1: Normalize and validate DSL values

**Files:**
- Modify: `include/models/ChangeRequest.h`
- Modify: `include/utils/ChangeRequestParseUtils.h`

- [ ] **Step 1: Update the model and parser helpers to canonicalize DSL fields**

```cpp
// include/models/ChangeRequest.h
// Add helper declarations before the structs.
struct ChangeRequestDsl;

namespace ChangeRequestNormalization {
inline QString normalizeScope(const QString& value);
inline QString normalizeType(const QString& value);
inline QString normalizeAction(const QString& value);
inline bool isSupportedScope(const QString& value);
inline bool isSupportedType(const QString& value);
inline void normalize(ChangeRequestDsl& dsl);
}
```

```cpp
// include/utils/ChangeRequestParseUtils.h
// Validate normalized values and make action matching case-insensitive through canonicalization.
if (!ChangeRequestNormalization::isSupportedScope(dsl.scope)) {
    ...
}
if (!ChangeRequestNormalization::isSupportedType(dsl.type)) {
    ...
}
if (!isActionValidForType(op.action, dsl.type)) {
    ...
}
```

- [ ] **Step 2: Run a focused build check**

Run: `cmake --build . --target QingningComic`
Expected: the build reaches the same target without new DSL normalization errors.

- [ ] **Step 3: Commit**

```bash
git add include/models/ChangeRequest.h include/utils/ChangeRequestParseUtils.h
git commit -m "fix: normalize change request dsl values"
```

### Task 2: Revalidate stored DSL and resolve targets before execution

**Files:**
- Modify: `include/services/ChangeRequestService.h`
- Modify: `src/services/ChangeRequestService.cpp`

- [ ] **Step 1: Add target resolution for existing DSL values**

```cpp
// include/services/ChangeRequestService.h
void resolveDslTarget(ChangeRequestDsl& dsl, const QString& novelId, const QJsonObject& context);
```

```cpp
// src/services/ChangeRequestService.cpp
void ChangeRequestService::resolveDslTarget(ChangeRequestDsl& dsl, const QString& novelId, const QJsonObject& context)
{
    if (!dsl.targetId.isEmpty()) {
        return;
    }

    if (dsl.type == QStringLiteral("style")) {
        const QString storyboardId = context.value("storyboardId").toString();
        if (!storyboardId.isEmpty()) {
            dsl.targetId = storyboardId;
            return;
        }
        dsl.targetId = resolveStoryboardIdForChangeRequest(novelId, context);
        return;
    }

    if (context.contains("targetPanelId")) {
        dsl.targetId = context.value("targetPanelId").toString();
        return;
    }

    dsl.targetId = resolvePanelIdFromContext(m_db, context);
}
```

- [ ] **Step 2: Normalize and revalidate in `prepareChangeRequestDsl`**

```cpp
// src/services/ChangeRequestService.cpp
ChangeRequestDsl dsl = cr.dsl();
ChangeRequestParseUtils::normalizeParsedDsl(dsl);
resolveDslTarget(dsl, cr.novelId(), cr.context());

QString validationError;
if (ChangeRequestParseUtils::isValidParsedDsl(dsl, &validationError)) {
    ...
}
```

- [ ] **Step 3: Re-run parse flow with style target backfill**

```cpp
// src/services/ChangeRequestService.cpp
ChangeRequestDsl dsl = ChangeRequestDsl::fromJson(result);
ChangeRequestParseUtils::normalizeParsedDsl(dsl);
resolveDslTarget(dsl, QString(), context);
```

- [ ] **Step 4: Commit**

```bash
git add include/services/ChangeRequestService.h src/services/ChangeRequestService.cpp
git commit -m "fix: revalidate change requests before execution"
```

