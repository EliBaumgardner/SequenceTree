# `getTargetNode` / `getRootNode`: Unchecked Lookups on the Audio Thread

> Status: Draft
> Written 2026-09-25 at 7f59922. Sources: Reviewed/ProgramAudits/software_architecture.md (What Survives: "P3: `getTargetNode` is a latent null dereference, and `getRootNode` is dead"; ledger rows 1–6), `.claude/notes/ongoing-issues.md` #7, #8

## Summary
`TraversalLogic::getTargetNode` and `getRootNode` both run `return *nodes.find(...)` (`Source/Audio/TraversalLogic.cpp:530-531`). `NodeMap::find` returns `nullptr` for a missing ID (`Source/Graph/RTData.h:125-134`). No path dereferences null today, but one of the two `getTargetNode` sites is safe only because of how a local is initialised. `getRootNode` has no callers. The plan deletes `getRootNode`, dissolves `getTargetNode` into its two call sites with an explicit null check at each, and brings `ongoing-issues.md` #7/#8 up to date. Their mechanism (`.at()`) and line numbers are stale. The change is 2 steps and about 10 lines. Priority P3.

## The Problem
### Mechanism
`TraversalDispatcher::handleExpiredNote` (`Source/Audio/TraversalDispatcher.cpp:640-750`) has two call sites:
1. **`:721`**, `pushNote(traversal.getTargetNode(nodes), …, true)` for a repeat. It runs only when `runtime.repeatCount < repeatValue`. `repeatValue` starts at 1 (`:700`) and is raised only inside `if (currentEntry != nullptr)` (`:702`), where `currentEntry = nodes.find(traversal.primary.target)` (`:699`). Nothing touches `nodes` or `primary.target` between `:699` and `:721`. With the target missing, `repeatValue` stays 1, `repeatCount` is already at least 1 after `:718`, and the branch is skipped. It is safe, but only as a side effect of `repeatValue`'s initialisation.
2. **`:747`**, after `handleNodeEvent` moves the target. It is guarded by `else if (nodes.find(traversal.primary.target) != nullptr)` at `:746`, which looks up the same ID twice.

`getRootNode` (`:531`, `.h:152`) has no callers in `Source/` or `Tests/`.

**Input that would expose it:** an edit that stops raising `repeatValue` only inside the found branch, for example defaulting it from the traversal instead of the node. The repeat path would then dereference `nullptr` on the audio thread when a node is deleted mid-note. The debug `jassert` at `:688` would catch only the first case.

### Evidence
- Review ledger rows 3–5, re-read at HEAD. The lines above are the lines at 7f59922.
- `ongoing-issues.md` #7 still says `nodes.at(primary.target)` at `:502` and callers at `:738`/`:764`. #8 says `:503` / `.h:149`. Both are stale: the code switched from `.at()` (throws) to `*find` (undefined behaviour).

### Why It Matters
It crashes the host if the guard ever changes. Once `getTargetNode` is gone, a lookup can no longer be used without its null check. It also clears two items from the ongoing issues list. **P3**, hygiene.

## Current Design
### API Surface
- `const RTNode& TraversalLogic::getTargetNode(const NodeMap&) const` (`.h:151`, `.cpp:530`). Callers: `TraversalDispatcher.cpp:721`, `:747`.
- `const RTNode& TraversalLogic::getRootNode(const NodeMap&) const` (`.h:152`, `.cpp:531`). No callers.
- `const RTNode* NodeMap::find(int) const` (`RTData.h:125`).
- `TraversalDispatcher::pushNote(const RTNode&, int runId, const DispatchContext&, double sample, bool isPrimaryRepeat)` (`:166`).

### Structure
`TraversalLogic` (`Source/Audio/`) walks the graph. `TraversalDispatcher` (`Source/Audio/`) turns steps into MIDI and UI commands and owns both call sites. Both getters are one-line accessors onto the `NodeMap` argument, which makes them wrappers under the Key Design Rules.

### Data Flow
Audio thread: `EventManager` → `NoteScheduler` note expiry → `TraversalDispatcher::handleExpiredNote` → (`pushNote` repeat | `handleNodeEvent` → `pushNote` next). The `NodeMap` is the published snapshot's `globalNodes`, which does not change during a block.

## Approaches Considered
1. **Delete `getRootNode`, and dissolve `getTargetNode` with an explicit check at each site.** Removes two wrappers and makes a null dereference impossible to write through these paths. **Recommended.**
2. **Make `getTargetNode` return `const RTNode*`.** It would still be a one-line forwarding wrapper, which the Key Design Rules forbid. Rejected.
3. **Leave it and only fix the notes.** This keeps the latent hazard. Rejected.

## Plan

### Step 1 — Delete `getRootNode`
- **Goal:** remove dead code (#8).
- **Changes:** `TraversalLogic.h:152` and `.cpp:531` are removed.
- **Refactor commands:** `refactor.decap TraversalLogic::getRootNode` (it deletes a function with no call sites).
- **Behaviour delta:** none.
- **Real-time safety:** removes code only.
- **Verification:** build `SequenceTree_Standalone`, `SequenceTree_Tests` and `SequenceTree_GraphTests`, then run `ctest`.
- **Rollback:** `refactor.undo`.

### Step 2 — Dissolve `getTargetNode` into checked lookups
- **Goal:** each site dereferences only a pointer it has just checked (#7).
- **Changes:**
  - `refactor.decap TraversalLogic::getTargetNode` inlines `*nodes.find(traversal.primary.target)` at `:721` and `:747` and removes `.h:151` / `.cpp:530`. Then, by hand:
  - `:720-721`: the condition becomes `if (currentEntry != nullptr && runtime.repeatCount < repeatValue)`, and the call becomes `pushNote(*currentEntry, runId, context, expiryTime, true);`. `currentEntry` is the lookup already made at `:699` for the same ID.
  - `:746-748`: `else if (nodes.find(traversal.primary.target) != nullptr) { pushNote(getTargetNode(nodes), …); }` becomes
    ```cpp
    else {
        const RTNode* const nextEntry = nodes.find(traversal.primary.target);

        if (nextEntry != nullptr) {
            pushNote(*nextEntry, runId, context, expiryTime);
        }
    }
    ```
- **Refactor commands:** `refactor.decap TraversalLogic::getTargetNode`, then the two hand edits above. If the build step of `decap` fails on the const-reference binding, `refactor.undo` and make the edits by hand.
- **Behaviour delta:**
  - Target present: identical.
  - Target missing at `:721`: the branch was already skipped (`repeatValue == 1`, so `repeatCount` is at least 1), and it is still skipped. The `else` path runs as before.
  - Target missing after the step: no note, as before. Each site now does one lookup instead of two.
- **Real-time safety:** the change removes one `find` and adds no allocation, lock or throw.
- **Verification:** all three targets build and `ctest` passes. Manual check in the standalone: during playback, delete the node that is sounding (Shift+Right-Click), both on a node with repeat value above 1 and on one without. The traversal should continue or end with no crash, and the canvas highlight should clear.
- **Rollback:** `refactor.undo`, or revert the hunk.

### Step 3 — Update the notes
- **Changes:** in `.claude/notes/ongoing-issues.md`, mark #7 and #8 resolved with the date, and correct their "Where" to the lines above and the mechanism to `*find` (undefined behaviour), not `.at()` (throw).

## Risks
- **Low.** The only change in behaviour is on paths already proven unreachable. The one thing to watch is `decap` inlining through a `const RTNode&` return. The build step catches that.

## Out of Scope
- `removeDeletedTraversals` checking only the home root (`ongoing-issues.md` #7 "Related"). Its own review is needed.
- The `jassert` at `:688`. It stays as a debug aid.

## Decisions for the Owner
None. The plan follows the review's recommendation and the resolutions already recorded for #7/#8.
