# SequenceTree Software Architecture Audit

> Reviewed 2026-09-25 at 7f59922 by Claude. Source: Unreviewed/ProgramAudits/software_architecture.md (Gemini).
> Verdict: Major revision

## Advisor Review

### Assessment
The report set out to audit walk parity in `TraversalLogic`, pointer safety on the audio thread, and accessor/wrapper use against the Key Design Rules. The design-rule section is accurate: it reproduces `refactor.smell` output. The three traversal and safety findings, which carry the report's P0/P1 weight, do not hold as written. The P0 "crash" is unreachable from both call sites, and the underlying hazard is already recorded as `ongoing-issues.md` #7/#8. The P1 cross-tree finding rests on a false count ("called exactly once"), and it misses the real asymmetry it was circling. The P1 switch-hold finding is wrong, and its recommendation would corrupt the primary walk's switch-hold state. Every `TraversalLogic.cpp` line number is wrong at the commit the report itself names, so it was not read at `HEAD`. What survives is one narrowed parity gap (tree-jump arrows on an active alternative), an open design question about modulators and cross-tree arrows, a hygiene note, and the P3 accessor list.

### Claim Ledger
| # | Claim (short, quoted) | Where (current file:line) | Verdict | Evidence / correction |
|---|---|---|---|---|
| 1 | "`getTargetNode` and `getRootNode` … `return *nodes.find(...)`" | `Source/Audio/TraversalLogic.cpp:530-531` | Corrected | The code is as described. The location is wrong: `:530-531`, not `:151-152`. |
| 2 | "`nodes.find` … returns `nullptr` if the ID is missing" (`RTData.h:58`) | `Source/Graph/RTData.h:125` | Corrected | True. The line is `:125`, not `:58`. |
| 3 | "`TraversalDispatcher.cpp:721` calls `getTargetNode` without any prior null checks" | `Source/Audio/TraversalDispatcher.cpp:698-721` | Refuted | `:721` is reached only when `runtime.repeatCount < repeatValue`. `repeatValue` is above 1 only if `nodes.find(traversal.primary.target)` at `:698` succeeded against the same `nodes`, with no mutation in between. `:747` is guarded by the `find` at `:746`. This is the same argument as `ongoing-issues.md` #7. |
| 4 | "unconditional dereference … causes an immediate segmentation fault on the audio thread" (P0) | same | Refuted | No reachable path dereferences null today. The hazard is latent: `:721` is safe only because of how `repeatValue` is initialised. That is hygiene (#7), not a live crash. |
| 5 | `getRootNode` crashes when a node is deleted | `Source/Audio/TraversalLogic.cpp:531`, `.h:152` | Refuted | It has no callers anywhere in `Source/` or `Tests/`. It is dead code, already `ongoing-issues.md` #8. |
| 6 | Recommendation: delete both, callers `find` and null-check | — | Judged: sound | Deleting `getRootNode` is already planned (#8). Dissolving `getTargetNode` into its two sites with an explicit `find` makes #7 structurally impossible. Note that `ongoing-issues.md` #7/#8 still describe an `.at()` lookup. The code now uses `*find`, which turns a throw into undefined behaviour, so those entries need their mechanism and line numbers updated. |
| 7 | "`selectTreeJumpChild` is called exactly once in `TraversalLogic.cpp` (in `advance`)" | `:354`, `:412` | Refuted | It is called twice: in `advance` (`:354`) and in `peekNextTarget` (`:412`). The report cites `:412` as `advanceAlternative`, but `:412` is the second call it says does not exist. |
| 8 | "`ModulatorWalk::decide` … omit[s] this check entirely" | `:142-209` | Confirmed | `decide` never consults tree-jump connections. `isModulatorChild` (`:7-9`) also excludes `RootNode`, so a root child of a modulator is never chosen. `ConnectionOps::canBeTraversalArrow` (`Source/Input/ConnectionOps.cpp:73-82`) does not check the owner's type, so a modulator can own a Traversal arrow to another root. |
| 9 | "`advanceAlternative` omit[s] this check" and "an alternative node with a cross-tree jump arrow will silently fail" | `:229-302`, `:325-403`, `:438-511` | Corrected | The mechanism is misplaced. `advanceAlternative` is not a step. It picks which alternative voices the host, and the step always leaves from `primary.target`. The real gap is inside the primary walk: `advance` checks tree jumps only on `primary.target` (`:353-354`), while `peekCrossTreeNode` scans both `primary.target` and `primary.alternativeTarget` (`:506-509`). A cross-root arrow on the active alternative therefore fires, and a Traversal arrow on the same alternative never does. |
| 10 | Recommendation: replicate the jump check "exactly as it is done in `advance`" | — | Judged: not as written | In the modulator walk, what a jump would mean is undefined. It could relocate the modulator walk, the primary, or neither, so the owner must decide. For alternatives, the check belongs in `advance`, which would test `primary.alternativeTarget`'s connections alongside `targetNode`'s. It does not belong in `advanceAlternative`. |
| 11 | "`advanceAlternative` manually increments `SwitchCount`… fails to use `SwitchCandidate`" | `:266-281` | Confirmed (fact) | The inline hold exists, and it keys `SwitchCount` on the held alternative rather than on its chooser. |
| 12 | "…incorrectly assuming that `currentAltId` is always the node being held"; "flawed code path" | `:258-281` vs `:304-323` | Refuted | `currentAltId` is `ActiveAlternative[parentId]`, which is by definition the alternative being voiced, so it is the held node. Traced counts: the primary walk plays a freshly chosen child `switchCountLimit` times (1 fresh selection plus limit−1 holds through `selectSwitchNode`). An alternative is likewise voiced `switchCountLimit` times before the chain advances from it. `Count` is not incremented during a hold in either path. The hold semantics match. The report shows no graph shape where they diverge. |
| 13 | Recommendation: replace with `selectSwitchNode(nodes, currentAltId, chosen)` | — | Judged: harmful | This would hold `SwitchCandidate[currentAltId]`, the alternative's successor, instead of the alternative, which shifts the hold one step down the chain. When `currentAltId == parentId`, it would also read and increment `SwitchCandidate`/`SwitchCount[parentId]`, the slots `advance` uses for the host's own child hold (`:365`, `:372`). The primary walk's switch hold on the host would be corrupted. |
| 14 | "`TraversalSession` … maintains up to 128 active `TraversalLogic` instances" | `Source/Audio/TraversalPool` | Corrected | `TraversalPool` holds the 128 slots, and `TraversalSession` owns the pool. |
| 15 | "The codebase strictly enforces Key Design Rules" | — | Refuted | The report's own P3 contradicts it. `refactor.smell Source/UI` alone reports 57 wrappers and 8 accessors. |
| 16 | Accessors `ArrowManager::all()`, `NodeManager::all()`, `NodeCanvas::getApplicationContext()`, `getSelectedItemId()` | `ArrowManager.h:26`, `NodeManager.h:29`, `NodeCanvas.h:98`, `ItemSelector.h:30` | Confirmed | `refactor.smell Source/UI` lists all four among its 8 accessors. The other four are `ButtonPane::getSelectedButton` (`ButtonPane.h:79`), `FileLabel::isGrabbed`/`isSelected` (`FileLabel.h:26,29`) and `ItemSelector::getSelectedLabel` (`ItemSelector.h:32`). |
| 17 | "`ItemSelector::addItem` is a 1-line wrapper around `std::vector::push_back`" | `Source/UI/Menus/ItemSelector.cpp:35-38` | Confirmed | `refactor.smell` lists it (wrapper [42]). It has 3 call sites (`Titlebar.cpp:59`, `TraversalMenu.cpp:99`, `ArrowBindBar.cpp:25`). |
| 18 | Recommendation: delete `addItem`, call `items.push_back` at call sites | — | Corrected | This cannot be done as stated. `items` is private (`ItemSelector.h:60`), so `items` and `Item` must move to public scope first. Whether "add an item" is the selector's core purpose (a `core_purpose_api` entry) or a wrapper is for the owner to decide. |

Counts: Confirmed 4, Corrected 5, Refuted 6, Stale 0, Unverified 0 (plus 3 recommendations judged without a verdict: #6, #10, #13). Tests: `SequenceTree_Tests` and `SequenceTree_GraphTests` build and pass at `HEAD`, 50/50.

### Critique
- **Method.** The `TraversalLogic.cpp` line numbers (151, 412, 430, 520) match nothing at 7f59922, although the report claims that commit, and `Source/` is clean. The traversal findings read like pattern-matching on function names and an older layout: "called exactly once" is disproved by a grep. The P3 section, which came straight from `refactor.smell`, is the only accurate one. Tool output beat hand reading here.
- **Evidence vs conclusion.** The P0 says "Confidence: verified" but never traced how `repeatValue` gates `:721`. Tracing it is what separates a crash from a latent hazard. The switch-hold finding asserts "incorrectly" without a graph shape or a divergent sequence, which GEMINI.md §3 requires for a traversal bug. Both overstate their confidence.
- **Omissions.** The report did not read `ongoing-issues.md`: #7 and #8 already cover its P0, and #11 is the template for a parity finding done properly. It did not run the parity test in `Tests/TraversalTests.cpp`. It missed that tree jumps are checked in `peekNextTarget` too, and so missed the real asymmetry with `peekCrossTreeNode`. It also missed that no test covers an alternative with `switchCountLimit > 1`, a gap that is real whatever the verdict on claim 12.
- **Proportionality.** The recommendations are small, which is right. But "zero risk" and "simplifies code and restores traversal parity" were asserted, not shown. Recommendation #13 would have introduced a bug.
- **Fit with the rules.** The P3 recommendation must also account for private members being made public. Otherwise it fits. The report does not treat any rule as a defect.

### What Survives
- **P1: A Traversal arrow on the active alternative never fires, while a cross-root arrow on it does.** `advance` tests tree jumps only on `primary.target` (`Source/Audio/TraversalLogic.cpp:353-354`). `peekCrossTreeNode` scans `primary.alternativeTarget` too (`:506-509`). This breaks "every walk is the same traversal" inside the primary walk itself. A fix belongs in `advance`, and it needs a parity-test shape.
- **P2: Modulators can own cross-root and Traversal arrows, but the modulator walk honours neither.** `ModulatorWalk::decide` (`:142-209`) has no jump check, and `isModulatorChild` (`:7-9`) excludes roots. `ConnectionOps::canBeTraversalArrow` (`Source/Input/ConnectionOps.cpp:73-82`) permits drawing them. What a modulator's jump should do is an owner decision: forbid the arrow in `ConnectionOps`, or define the mechanic.
- **P3: `getTargetNode` is a latent null dereference, and `getRootNode` is dead.** `Source/Audio/TraversalLogic.cpp:530-531`. `TraversalDispatcher.cpp:721` is safe only through `repeatValue`'s initialisation. `ongoing-issues.md` #7/#8 still say `.at()` and give old line numbers, so update them.
- **P3: 8 accessors and the `ItemSelector::addItem` wrapper** per `refactor.smell Source/UI`. Sites are in ledger rows 16-17. Moving `ItemSelector::items` public is a prerequisite.
- **P3 (test gap): No test covers an alternative with `switchCountLimit > 1`.** `alternativeShape` (`Tests/TraversalTests.cpp:184-201`) leaves it at 1, so the hold semantics traced in claim 12 are unguarded.

### Questions for the Author
1. Re-read `TraversalLogic.cpp` at `HEAD` and re-cite every line. Why did the line numbers not match the commit you named?
2. For the alternative tree-jump gap, give a concrete graph shape (host, alternative, Traversal arrow to root B) and the expected versus actual step sequence.
3. What *should* a Traversal or cross-root arrow on a modulator do? Survey the existing modulator semantics (synced vs unsynced arrows, `findActiveModulatorRoot`) and propose options for the owner. Do not propose code.
4. Before filing a real-time finding, check `ongoing-issues.md` and trace every guard on the path from `processBlock`.
5. Of the 57 wrappers `refactor.smell Source/UI` reports, which are true forwarding wrappers and which are core-purpose candidates? Rank them instead of sampling one.

## Corrected Report

### Summary
- ~~`getTargetNode` and `getRootNode` crash the audio thread when a node is deleted (Source/Audio/TraversalLogic.cpp:151).~~
  > **Refuted:** both call sites of `getTargetNode` are guarded (`TraversalDispatcher.cpp:698-721`, `:746-747`), and `getRootNode` has no callers. This is latent hygiene, `ongoing-issues.md` #7/#8, at `TraversalLogic.cpp:530-531`.
- Cross-tree jumps are not honoured by every walk.
  > **Correction:** "Modulator walks and Alternative walks ignore cross-tree jumps (:520, :412)" → the primary walk checks Traversal arrows only on `primary.target` (`:353-354`) but checks cross-root arrows on the active alternative too (`:506-509`). The modulator walk honours neither kind (`:142-209`). `:412` is `peekNextTarget`'s tree-jump call.
- ~~`advanceAlternative` reimplements switch-hold logic incorrectly (Source/Audio/TraversalLogic.cpp:430).~~
  > **Refuted:** the inline hold at `:266-281` gives the same play count as `selectSwitchNode` (`:304-323`). The recommended replacement would corrupt the host's `SwitchCandidate`/`SwitchCount` slots.
- Multiple classes use accessors and 1-line wrappers (Source/UI/Canvas/ArrowManager.h:26 and others).

### How It Works Now
The audio thread orchestrates MIDI generation through `TraversalSession`, whose `TraversalPool` holds up to 128 `TraversalLogic` instances. Each `TraversalLogic` steps through a `NodeMap` with the primary walker (`advance`) and the modulator walker (`ModulatorWalk::decide`), and chooses the active alternative with `advanceAlternative`. Data crosses from the message thread to the audio thread by lock-free pointer publishing (`AudioSnapshotPublisher`), so the audio thread never allocates or blocks.
> **Correction:** "`TraversalSession` maintains up to 128" → the pool holds them, and the session owns the pool.
> **Refuted:** "The codebase strictly enforces Key Design Rules". `refactor.smell Source/UI` reports 57 wrappers and 8 accessors.

### Findings

#### [P3] `getTargetNode` and `getRootNode` dereference `find` without a check
- Where: `Source/Audio/TraversalLogic.cpp:530-531`, declared at `TraversalLogic.h:151-152` (at 7f59922)
- Kind: code fact
- Confidence: verified
- What happens: both execute `return *nodes.find(...)`. `NodeMap::find` (`Source/Graph/RTData.h:125`) returns `nullptr` for a missing ID.
  > **Correction:** "unconditionally dereference `nullptr` … immediate segmentation fault" → no reachable call dereferences null. `TraversalDispatcher.cpp:721` runs only when `repeatValue > 1`, which requires the `find` at `:698` to have succeeded against the same `nodes`. `:747` is guarded by the `find` at `:746`. `getRootNode` has no callers. The downgrade from P0 to P3 is because this is latent, and it duplicates `ongoing-issues.md` #7/#8. Those entries still describe `.at()`, so they are stale in mechanism.
- Recommendation: delete `getRootNode` (#8). Dissolve `getTargetNode` into its two call sites, with an explicit `find` and null check at each.

#### [P1] Traversal arrows are not honoured by every walk
- Where: `Source/Audio/TraversalLogic.cpp:353-354` (`advance`), `:506-509` (`peekCrossTreeNode`), `:142-209` (`ModulatorWalk::decide`) (at 7f59922)
- Kind: code fact
- Confidence: traced
  > **Correction:** "`selectTreeJumpChild` is called exactly once … absent from `decide` and `advanceAlternative`" → it is called in `advance` (`:354`) and `peekNextTarget` (`:412`). `advanceAlternative` is not a step, so it is the wrong place to look. The real gaps: (a) `advance` tests Traversal arrows only on `primary.target`, while `peekCrossTreeNode` scans `primary.alternativeTarget` too, so a Traversal arrow on the active alternative never fires but a cross-root arrow on it does; (b) the modulator walk honours neither kind, although `ConnectionOps::canBeTraversalArrow` (`Source/Input/ConnectionOps.cpp:73-82`) lets a modulator own one.
- Why it matters: this violates "every walk is the same traversal".
  > **Unverified:** the intended semantics of a jump from a modulator. The owner must decide whether to forbid the arrow or to define the mechanic. A parity-test shape with an alternative carrying a Traversal arrow would settle (a).
- Recommendation:
  > **Correction:** "replicate the check in `decide` and `advanceAlternative`" → for (a), extend the check in `advance` to the active alternative's connections. For (b), wait for the owner's decision.

#### ~~[P1] `advanceAlternative` reimplements switch holds incorrectly~~
> **Refuted:** the inline hold (`Source/Audio/TraversalLogic.cpp:266-281`) keys `SwitchCount` on the held alternative. `currentAltId` is by definition the voiced alternative, and it is voiced `switchCountLimit` times, the same count `selectSwitchNode` (`:304-323`) gives a held child. The proposed `selectSwitchNode(nodes, currentAltId, chosen)` would hold the alternative's successor. When `currentAltId == parentId`, it would also increment the host's `SwitchCount` and read its `SwitchCandidate`, both owned by `advance` (`:365`, `:372`). No graph shape was given. Remaining gap: no test covers an alternative with `switchCountLimit > 1`.

#### [P3] Accessors and 1-line wrappers violate Key Design Rules
- Where: `Source/UI/Canvas/ArrowManager.h:26`, `Source/UI/Canvas/NodeManager.h:29`, `Source/UI/Canvas/NodeCanvas.h:98`, `Source/UI/Menus/ItemSelector.h:30`, `Source/UI/Menus/ItemSelector.cpp:35` (at 7f59922)
- Kind: code fact
- Confidence: verified
- What happens: classes provide getters such as `all()`, `getApplicationContext()` and `getSelectedItemId()` instead of making the fields public. `ItemSelector::addItem` is a 1-line wrapper around `std::vector::push_back`.
  > **Correction:** `getSelectedItemId` is in `ItemSelector.h:30`, not `ItemSelector.cpp:35`. The full accessor list from `refactor.smell Source/UI` also includes `ButtonPane::getSelectedButton` (`ButtonPane.h:79`), `FileLabel::isGrabbed`/`isSelected` (`FileLabel.h:26,29`) and `ItemSelector::getSelectedLabel` (`ItemSelector.h:32`). The same run reports 57 wrappers, not "multiple".
- Recommendation: move `arrows`, `nodes`, `applicationContext`, `selectedItemId` (and the four fields above) to public scope and delete their accessors. For `addItem`:
  > **Correction:** "call `items.push_back` directly" → `items` (`ItemSelector.h:60`) and `Item` are private, so they must move public first. Alternatively, the owner may declare `addItem` core-purpose API.
- Cost: touches several UI headers and their callers. Structural only.
