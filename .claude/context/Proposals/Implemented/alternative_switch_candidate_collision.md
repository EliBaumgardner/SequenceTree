# Alternatives Overwrite Their Host's Switch Candidate

> Status: Implemented
> Implemented 2026-09-25 at uncommitted (on 7f59922).
> Written 2026-09-25 at 7f59922. Sources: Reviewed/ProgramAudits/software_architecture.md (What Survives: "P3 (test gap): No test covers an alternative with `switchCountLimit > 1`"; ledger rows 11–13). The surviving finding is P3 (test gap). The slot collision below is new: tracing that gap turned it up, and it is not in the reviewed report.
> Verified 2026-09-25 at 7f59922; amended: P1 marked as new beyond the report, Step 2b written in, `alternativeShape` parameterised, Shape 2 pinned to a literal, fail-at-HEAD claims and line numbers corrected.

## Summary
The review's test gap turns out to hide a live bug. When `advanceAlternative` picks the host's first alternative, it writes that alternative into `SwitchCandidate[host]`. That slot belongs to the host's own child switch hold, which `advance` and `ModulatorWalk::decide` read through `selectSwitchNode`. So a host that has alternatives loses its switch hold on its ordinary children. If the alternative's own `switchCountLimit` is above 1, the walker steps *onto the alternative node* as if it were a child. Nothing in the alternative mechanism reads `SwitchCandidate`, so the fix deletes one line (`TraversalLogic.cpp:292`) and adds two tests. Per the owner's decision, a second change (Step 2b) guards the `LastNode` write at `:293`, so a host's `parent.lastChild` is always a real child, and adds a script test for it. I checked this at HEAD against a scratch copy of `TraversalLogic.cpp`. Priority P1.

## The Problem
### Mechanism
1. Entering a host `H` that has alternatives calls `advanceAlternative(nodes, H)`, from `advance` (`Source/Audio/TraversalLogic.cpp:394`), `handleLoopReset` (`:641`), `handleTreeJump` (`:691`), `begin` (`:78`), and for modulators from `TraversalDispatcher.cpp:534`, `:594`.
2. When `ActiveAlternative[H] == H` (the host is voicing itself), `currentAltId == parentId`. Execution reaches `:290`, picks the first alternative `A` with `selectNextChild(..., isAlternative)`, and then:
   - `:292` `nodeState.set(SwitchCandidate, currentAltId, chosen)`, which is **`SwitchCandidate[H] = A`**
   - `:293` `nodeState.set(LastNode, currentAltId, chosen)`, which is `LastNode[H] = A`
3. When `H` is next left, `advance` calls `selectSwitchNode(nodes, H, chosenNodeId)` (`:365`). The modulator walk does the same at `:160`. That function (`:304-323`) reads `SwitchCandidate[H]`, finds `A`, increments `SwitchCount[H]`, and compares it against **`A.switchCountLimit`**:
   - With `A.switchCountLimit == 1`, which is the default, it resets `SwitchCount[H]` to 0 and falls through to a fresh selection. Whatever child `H` was holding is dropped.
   - With `A.switchCountLimit > 1`, it returns `chosenNodeId = A`. `advance` then sets `primary.target = A` (`:393`), and the walker has stepped onto an alternative through an arrow that `isAdvanceableChild` (`:19-22`) excludes.
4. The inline alternative hold at `:266-281` keys `SwitchCount[currentAltId]` and `ActiveAlternative`. It never reads `SwitchCandidate`. When `currentAltId != parentId`, the same statement at `:292` writes `SwitchCandidate[A] = next`. That slot is read only if `A` becomes a walker target, which is the bug itself. Neither effect of `:292` has a reader that wants it.

The write came in with the `ongoing-issues.md` #1 fix, where "each real selection records `SwitchCandidate` next to `LastNode`". Before that fix, `selectNextChild` wrote the slot itself, so the collision is older than #1.

### Evidence
I compiled the JUCE-free core directly: `TraversalLogic.cpp`, `TraversalRule.cpp` and `NodeStateTable.cpp`, with a driver in `/tmp/st_alt`, outside the repo. The drivers use the same `walkPrimary` / `walkModulator` loops as `Tests/TraversalTests.cpp`.

**Shape 1** is `alternativeShape()` (`Tests/TraversalTests.cpp:184-201`) with `firstAlternative.switchCountLimit = 2`. Entries in walk order:

| | Sequence |
|---|---|
| HEAD, nodes | `1 2 1 2 10 10 1 2 10 1 2 11 1 2 1 2 10 10 11 1` (`10` is entered as the primary target) |
| HEAD, modulators | `1 2 1 2 10 10 10 1 2 10 1 2 11 …` (differs from the node walk, so parity is broken too) |
| `:292` removed, nodes | `1 2 1 2 10 1 2 10 1 2 11 1 2 1 2 10 1 2 10 1` |
| `:292` removed, modulators | identical to the node walk |

**Shape 2** is root `1` → host `2` with children `3` (count limit 2, switch count 3) and `4` (count limit 1), plus an alternative `10` with default settings. Entries, alternatives in brackets:

| | Sequence |
|---|---|
| no alternative | `1 2 4 1 2 3 1 2 3 1 2 3 1 2 4 …` (3 is held 3 times) |
| HEAD, with alternative | `1 2 4 1 2 [10] 3 1 2 3 1 2 [10] 4 1 2 3 1 2 [10] 4 …` (3 is held 2 times, then 1) |
| `:292` removed, with alternative | `1 2 4 1 2 [10] 3 1 2 3 1 2 [10] 3 1 2 4 …` (matches the walk with no alternative) |

`alternativeShape` with every switch count at 1 gives the same sequence before and after, so the existing "alternatives rotate…" test and its parity section are unaffected.

### Why It Matters
- Adding an alternative to a node silently changes how its *other* children are held, and the user gets no sign of it. That breaks the rule that a child's switch count holds its parent on that child for that many visits.
- Raising an alternative's switch count makes the walker play the alternative as a standalone step. The step is timed from `A`'s own arrow, and `A` then acts as a host.
- The primary and modulator walks diverge on the same shape, which breaks "every walk is the same traversal".
- The review's refutation of claim 12 still stands for what that claim said: the inline hold itself is right. But claim 12's conclusion that the "hold semantics match" missed this slot collision.

Priority **P1**: the fault is audible on ordinary graphs and needs no unusual settings.

## Current Design
### API Surface
- `TraversalLogic::advanceAlternative(const NodeMap&, int parentId)` (`TraversalLogic.h:140`, `.cpp:229-302`). Callers: `begin` `:78`, `advance` `:394`, `handleLoopReset` `:641`, `handleTreeJump` `:691`, `EventManager.cpp:43`, `TraversalSession.cpp:325`, `TraversalDispatcher.cpp:534`, `:594`, the tests' `walkModulator` (`Tests/TraversalTests.cpp:108`, `:119`), and `GraphTests`' `walkModulator` (`Tests/GraphTests.cpp:197`, `:204`; no alternative shapes, unaffected).
- `TraversalLogic::selectSwitchNode(const NodeMap&, int targetId, int& chosenNodeId)` (private, `.h:164`, `.cpp:304-323`). Callers: `advance` `:365`, `ModulatorWalk::decide` `:160`.
- `NodeStateSlot::SwitchCandidate` / `SwitchCount` / `LastNode` / `ActiveAlternative` (`Source/Audio/NodeStateTable.h:6-18`).
  - `SwitchCandidate` is written at `:167`, `:292`, `:358`, `:372` and read only in `selectSwitchNode`.
  - `LastNode` is written at `:168`, `:293`, `:373` and read only by `ScriptTraversalRule.cpp:22` as `parent.lastChild`, and by the `NodeStateTable` tests at `Tests/TraversalTests.cpp:666–720` (table-level, unaffected).

### Structure
Everything lives in `TraversalLogic`, in `Source/Audio/`, which holds one running traversal instance and is pure graph walking. Its per-traversal state is the `NodeStateTable nodeState` member. Both walks share that one table and one set of slots keyed by node ID. Only `Count` and `ModulatorCount` are split per walk.

### Data Flow
Audio thread only. `processBlock` → `EventManager` → `TraversalDispatcher::handleExpiredNote` → `TraversalLogic::handleNodeEvent` → `advance` → (`selectSwitchNode`, `advanceAlternative`). For modulators: `pushNote` → `decideNextModulator` → `ModulatorWalk::decide`, then `advanceAlternative(mod.walker.target)`. No snapshot or FIFO is involved.

## Approaches Considered
1. **Delete the `SwitchCandidate` write in `advanceAlternative` (`:292`).** One line. The alternative mechanism never reads the slot, so removing the write changes nothing about alternatives. It only stops the collision. Fits every rule. **Recommended.**
2. **Give alternatives their own candidate slot** (a new `NodeStateSlot::AlternativeCandidate`). This adds state that nothing would read. It costs a table row per node and a new enum value for no behaviour. Rejected.
3. **Guard the write with `currentAltId != parentId`.** This keeps a write whose only reader is the bug path. It is more code than approach 1 and does nothing extra.

`LastNode[H] = A` at `:293` is the same kind of collision on the script side: a script's `parent.lastChild` on `H` can read an alternative instead of a child. It is a separate, script-visible behaviour, The owner chose (b). It is Step 2b.

## Plan

### Step 1 — Guard the fix with two failing tests
- **Goal:** pin both symptoms before changing code.
- **Changes:** `Tests/TraversalTests.cpp`.
  - Change `static NodeMap alternativeShape()` to `static NodeMap alternativeShape(int firstAlternativeSwitchLimit)`, and set `firstAlternative.switchCountLimit = firstAlternativeSwitchLimit;` after its `makeNode`. This follows `stepIntoTreeShape(int)`. Pass `1` at the existing call sites `:367`, `:565`, `:566` and `:618`, which leaves their sequences unchanged (verified).
  - Add `TEST_CASE("an alternative's switch count holds only that alternative", "[traversal]")`. It checks `walkPrimary(alternativeShape(3), 1, 14, NativeTraversalRule::instance())` against `expected { 1, 2, 1, 2, 10, 1, 2, 10, 1, 2, 10, 1, 2, 11, 1, 2, 1, 2, 10, 1 }`. At HEAD this walk gives `1 2 1 2 10 10 1 2 10 10 11 1 2 10 …`.
  - Add `static NodeMap hostHoldWithAlternativeShape()`:
    - Root `1` → `{ 2 }`.
    - Host `2`, a `Node` with count limit 1, connections `{ 3, 4, 10 }` in that order, and `alternativeRootId = 10`.
    - `3`: `Node`, count limit 2, `switchCountLimit = 3`, leaf.
    - `4`: `Node`, count limit 1, leaf.
    - `10`: `Alternative`, count limit 1, `alternativeRootId = 10`, leaf.
  - Add `TEST_CASE("an alternative leaves its host's switch hold on its children intact", "[traversal]")`. It checks `walkPrimary(hostHoldWithAlternativeShape(), 1, 14, …)` against `expected { 1, 2, 4, 1, 2, 10, 3, 1, 2, 3, 1, 2, 10, 3, 1, 2, 4 }`. At HEAD this walk gives `1 2 4 1 2 10 3 1 2 3 1 2 10 4 1 2 3`.
  - Add `SECTION("held alternatives")`, using `alternativeShape(3)`, and `SECTION("host hold beside an alternative")` to the parity case at `:535`, walking each shape through `mirrorAsModulators` and `walkModulator`.
  - Optionally add `alternativeShape(3)` and `hostHoldWithAlternativeShape()` to the default-script agreement list at `:617–619`. Both agree before and after the fix.
- **Refactor commands:** none.
- **Behaviour delta:** none (tests only). At HEAD three new checks fail: both new test cases and the "held alternatives" parity section. The "host hold beside an alternative" parity section passes at HEAD, because both walks share the fault. It stays as a guard.
- **Real-time safety:** test code only.
- **Verification:** `cmake --build cmake-build-debug --target SequenceTree_Tests && (cd cmake-build-debug && ctest --output-on-failure)`. Expect the new cases to fail with the HEAD sequences above, and everything else to pass.
- **Rollback:** remove the added shapes and cases.

Steps 1 and 2 land together so `main` never carries a failing test. They are split here only so the failure can be seen first.

### Step 2 — Stop `advanceAlternative` writing the host's switch candidate
- **Goal:** alternatives no longer touch the slot `selectSwitchNode` reads.
- **Changes:** `Source/Audio/TraversalLogic.cpp:292`. Delete `nodeState.set(NodeStateSlot::SwitchCandidate, currentAltId, chosen);`. Nothing else changes.
- **Refactor commands:** none. It is a single-line deletion.
- **Behaviour delta:**
  - A host with alternatives and all switch counts at 1: no change in either walk. The existing alternative tests confirm this.
  - A host with alternatives and a child whose switch count is above 1: the child is now held for its full switch count, as it is without alternatives. Before, the hold was cut short.
  - An alternative with a switch count above 1: it is voiced that many times in a row, then the chain moves on. Before, the walker stepped onto it as a primary target.
  - Modulator walk: same deltas, and it now matches the node walk.
  - Scripts: `parent.lastChild` is unchanged by this step. Step 2b changes it.
- **Real-time safety:** removes a table write and adds nothing.
- **Verification:** both targets build. `ctest` passes, including Step 1's cases. Manual check in the standalone: root → host with two children (one with switch count 3) and an alternative. Listen for the held child repeating three times. Then set the alternative's switch count to 3, and look for the alternative's highlight showing three host visits in a row with no stray step onto the alternative node.
- **Rollback:** restore the line.

### Step 2b — A host's `lastChild` is always a real child
- **Goal:** `parent.lastChild` on a host means the child picked there on the last real step, never an alternative. This is the owner's decision #1 (b).
- **Changes:** `Source/Audio/TraversalLogic.cpp:293` (`:292` after Step 2). Wrap `nodeState.set(NodeStateSlot::LastNode, currentAltId, chosen);` in `if (currentAltId != parentId) { … }`. No function, no ternary, no comment. An alternative's `LastNode` is still its successor in the chain.
- **Refactor commands:** none.
- **Behaviour delta:**
  - Native rule and modulator walk: no change. The native rule never reads `LastNode`.
  - Scripts that read `parent.lastChild` on a host with alternatives: it now returns the host's last real child. It used to return the alternative chosen when the host was entered, including `-1` when the host re-selected and found no eligible alternative, which erased the real child.
  - `parent.lastChild` read while choosing the host's alternative now reads the host's last real child.
- **Real-time safety:** a branch around an existing table write. No allocation, no locks, no new state.
- **Verification:** in `Tests/TraversalTests.cpp`, next to "a script that declines every child sends the walker back to its root", add `TEST_CASE("a host's last child is never one of its alternatives", "[traversal][script]")`:
  - Compile this script and `REQUIRE(compiled.succeeded())`, then set it on a `ScriptTraversalRule`:
    ```
    for child in children {
        if child.eligible and child.id != parent.lastChild { return child.id; }
    }
    for child in children {
        if child.eligible { return child.id; }
    }
    return -1;
    ```
  - Build the shape inline in the test case, as "zero duration and disabled arrows…" does: root `1` → `{ 2 }`; host `2`, a `Node` with count limit 1, connections `{ 3, 4, 10 }` and `alternativeRootId = 10`; leaf `Node`s `3` and `4`, count limit 1; `10`, an `Alternative` leaf with `alternativeRootId = 10`.
  - Check `walkPrimary(shape, 1, 16, scriptRule)` against `expected { 1, 2, 3, 1, 2, 10, 4, 1, 2, 3, 1, 2, 10, 4, 1, 2, 3, 1, 2, 10 }`. With Step 2 alone the walk gives `1 2 3 1 2 10 3 1 2 4 1 2 10 3 …`, so the test fails before this step and passes after it.
  - Build `SequenceTree_Tests`, then run `ctest`.
- **Rollback:** remove the `if` and the test case.

Steps 1, 2 and 2b land together.

### Step 3 — Record the issue
- **Goal:** keep `.claude/notes/ongoing-issues.md` the record of confirmed problems.
- **Changes:**
  - Add an entry, "#12 Alternatives overwrite their host's switch candidate", naming both fixes (the switch candidate, and `lastChild` on hosts), with the mechanism, the two shapes, and "resolved <date>" once Steps 2 and 2b land.
  - Update #1's status line: `advanceAlternative` no longer records `SwitchCandidate`, and writes `LastNode` only for an alternative.
- **Behaviour delta / safety / rollback:** documentation only.

## Risks
- **The hold might rely on the collision somewhere.** Low. `SwitchCandidate` has four writers and one reader, and I traced each one. The parity section and the peek test (`:268`) cover the other writers.
- **Saved graphs sound different.** Certain, for any graph that has a host with alternatives and a held child, or an alternative with a switch count above 1. This is the fix. Mention it in the change notes.
- **Saved scripts behave differently.** Scripts that read `parent.lastChild` on a host with alternatives behave differently after Step 2b. This is intended.

## Out of Scope
- Alternatives owning root arrows (Traversal / cross-root). See `root_arrows_on_alternatives_and_modulators.md`.
- `advanceAlternative` picking its walker from `parent.nodeType` (`:238-240`). When the modulator walk had stepped onto an `AlternativeModulator`, it wrote `primary.alternativeTarget`. Step 2 makes that state unreachable, so no separate change is proposed.

## Decisions for the Owner
1. **Should `advanceAlternative` also stop writing `LastNode[host]` (`:293`) when it picks the host's first alternative?** Today a script's `parent.lastChild` on a host can return an alternative's ID, depending on whether the last write came from `advance` or `advanceAlternative`. The options:
   - (a) Leave it as it is.
   - (b) Write `LastNode` only when `currentAltId != parentId`, so an alternative's `lastChild` is its successor in the chain and a host's `lastChild` is always a real child.
   - (c) Drop the write in `advanceAlternative` entirely.
   
   Recommendation: **(b)**. The script docs say "child picked here on the previous visit" (`ScriptCompiler.cpp:54`), and for a host that means a child. If you choose it, it becomes a Step 2b with its own script test.

   **Owner's answer (2026-09-25): (b).** Write `LastNode` only when `currentAltId != parentId`. This becomes Step 2b, with its own script test. **Resolved by Step 2b.**

## Implementation Notes

- **Step 1:** built as amended. `alternativeShape(int firstAlternativeSwitchLimit)` sets the first alternative's `switchCountLimit`. `refactor.rewrite 'alternativeShape()' 'alternativeShape(1)'` updated the four existing call sites. `hostHoldWithAlternativeShape()` sits next to it. The two test cases follow "alternatives rotate…", and the two parity sections follow "alternatives". The optional default-script agreement entries were added too (`alternativeShape(3)`, `hostHoldWithAlternativeShape()`). At HEAD exactly three checks failed, with the predicted sequences: both new cases and "held alternatives".
- **Step 2:** deleted the `SwitchCandidate` write in `advanceAlternative`. All 39 test cases passed.
- **Step 2b:** the script test was added first and failed with the predicted Step-2-only sequence `1 2 3 1 2 10 3 1 2 4 …`. Then the `LastNode` write was wrapped in `if (currentAltId != parentId) { … }`. `SequenceTree_Tests`, `SequenceTree_GraphTests` and `SequenceTree_Standalone` build, `ctest` passes 53/53, and `design-rules.sh` and `readability.sh` are clean on both changed files. The `ScriptCompiler.cpp:54` note was left as it is, since its wording already matches.
- **Step 3:** added #12 to `.claude/notes/ongoing-issues.md` and updated #1's status line.
- **No deviations from the verified plan.**
- **Manual checks still owed (standalone):**
  - Root → host with two children (one with switch count 3) and an alternative: the held child repeats three times.
  - Set the alternative's switch count to 3: the alternative's highlight shows on three host visits in a row, and the walker never steps onto the alternative node.
  - Saved graphs with a host that has alternatives and a held child, or with an alternative whose switch count is above 1, will sound different. This is intended.
