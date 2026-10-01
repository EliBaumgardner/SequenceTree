# SequenceTree Codebase Audit & Scrutiny

> Reviewed 2026-09-30 at 2233a2a by Claude. The working tree has uncommitted edits to 28 files, and line numbers are cited as it stands. Where an uncommitted edit changes a verdict, the ledger says so. Source: Unreviewed/ProgramAudits/tracker.md (Antigravity / Gemini, written 2026-09-24).
> Verdict: Major revision

## Advisor Review

### Assessment
The report is a broad sweep across architecture, audio-thread safety, data layout, the script VM, JUCE idiom, C++20 usage and the project's own rules. It was written on 2026-09-24, and most of what it found has since been fixed. Many of those fixes came directly from reviewing an audit of the same era (`ongoing-issues.md` #1, #4, #7, #8, #13). So a large share of the ledger is Stale. That says the report once had value, not that it is wrong. Of what is still current, two of its four live "P0" claims are Refuted. The swap-and-pop removal in a backward loop is the correct, standard idiom and skips nothing. The `pendingNoteOffs` "race" ignores JUCE's threading contract. The two real-time claims that stand are small: a 256-entry scratch vector on the audio thread can reallocate when a graph has more than 256 arrows into root nodes, and the flag scheduler does a linear scan. §7 argues against the owner's non-negotiable design rules. That is out of scope for a findings report, and every item in it is either stale or already decided.

### Claim Ledger
| # | Claim (short, quoted) | Where (current file:line) | Verdict | Evidence / correction |
|---|---|---|---|---|
| 1 | "UI canvas components directly invoke `rtGraphBuilder->makeRTGraph()`" | `Source/Plugin/PluginProcessor.h:85` | Stale | `RTGraphBuilder` is a processor member, and its own `ValueTree::Listener` drives rebuilds. No file under `Source/UI` calls `makeRTGraph`, `rebuildAllGraphs` or `updateDurationMaps`. `ongoing-issues.md` #3 still reads as open and should be marked resolved. |
| 2 | "`detachStateListeners()` … audio thread continues playing stale snapshot data" | — | Stale | `detachStateListeners` no longer exists. Rebuilds happen whether or not an editor is open (CLAUDE.md, `RTGraphBuilder`). |
| 3 | `ApplicationContext` bundles raw pointers, and its construction order is brittle | `Source/Util/ApplicationContext.h`, `Source/Plugin/PluginEditor.cpp:21` | Confirmed | Accurate, and deliberate. CLAUDE.md documents the order: `canvas` and `nodeController` are filled in during construction. |
| 4 | Recommendation: narrow dependencies per component | — | Judged: already decided | The owner reviewed this in the 2026-09-04 ApplicationContext refactor and kept the context. It is not a finding. |
| 5 | "node IDs easily exceed 1023 … corrupted playback" | `Source/Audio/NodeStateTable.h:22-40` | Stale | `ongoing-issues.md` #4, resolved 2026-09-24. `NodeRowMap` hands each ID a dense row, so the cap is 1024 distinct nodes per traversal, not the highest ID. |
| 6 | "`isPlaying` is unconditionally overwritten … conflicts with CLAUDE.md" | `Source/Plugin/PluginProcessor.cpp:387` (`followHostTransport`) | Stale | CLAUDE.md now says the plugin follows the host's transport. The uncommitted working tree adds a "Sync To Host" parameter (`PluginProcessor.cpp:43`, read at `:406`), so following the host is optional. `ongoing-issues.md` #5 is superseded. |
| 7 | "determining which nodes are selected requires iterating … `node->isSelected`" | `Source/Input/SelectionOps.cpp:33-44`, `:53`, `:62`, `:71`, `:729` | Confirmed | The selection is derived from the `Node` components every time it is read. |
| 8 | "If the canvas rebuilds (`rebuildFromNodeMap`) … selection is instantly lost" | `Source/Plugin/PluginProcessor.cpp:232` | Corrected | True, but the only caller is state restore (`setStateInformation`). Losing the selection when a new document is loaded is expected behaviour, not a defect. |
| 9 | "Headless operations … cannot query or alter the selection" | — | Confirmed (inference) | True, but nothing headless needs the selection today. The profile asks for a concrete, present problem. |
| 10 | "`.at()` throws `std::out_of_range` on the audio thread" | — | Stale | There is no `.at(` anywhere in `Source/Audio` or `Source/Plugin`. `NodeMap::find` returns a pointer (`RTData.h:121-134`). `getTargetNode` and `getRootNode` were removed 2026-09-27 (#7, #8). `TraversalSession.cpp:396` dereferences a `find` for an ID that `findFirstUnlinkedRootId` just took from the same `nodes`. |
| 11 | "`peekNextTarget` … `nodeState.set(SwitchCandidate …)` MUTATES STATE" | `Source/Audio/TraversalLogic.cpp:415`, `.h:144` | Stale | #1, resolved 2026-09-24. `peekNextTarget` and `selectNextChild` are `const`. #13 later made peek honour the switch hold. |
| 12 | "`peekNextTarget` filters with `isAudibleChild` while `advance` filters with `isAdvanceableChild`" → desync | `TraversalLogic.cpp:19-26`, `:380`, `:444` | Unverified | The predicates do differ, by `NodeType::Modulator`. Creating a node never gives a note node a `Modulator` child: a Node parent in Modulator mode creates a `ModulatorRoot` (`NodeCreationDispatcher.cpp:27-40`). A desync therefore needs an arrow drawn from a Node onto an existing `Modulator`. **To settle it:** find whether `ConnectionOps` permits that arrow. If it does, add a parity shape where a node's only eligible child is a `Modulator`. |
| 13 | "`removeNote(i)` … The element swapped into index `i` is completely skipped!" (P0) | `Source/Audio/TraversalSession.cpp:423-441`, `NoteScheduler.cpp:88-92` | Refuted | The loop runs from the back. The element moved into `i` comes from `back()`, an index greater than `i` that the loop has already visited and kept. Skipping it is correct, and it is why backward iteration is the standard pairing with swap-and-pop. No note is left hanging. |
| 14 | "`handleOrphanNotes` … the element moved into `i` … is skipped, and vector bounds are mutated while iterating" (P0) | `Source/Audio/EventManager.cpp:5-47` | Refuted | The same argument as #13 applies to the removal. `pushNote` appends at the back, above every index still to visit, so the loop never reaches it. It is also inside the reserved capacity: `NoteScheduler.cpp:5` reserves `maxExpectedActiveNotes` = 1024, and `:20` refuses beyond that. `orphanedRunId` and `orphanedRole` are copied before `removeNote`, so the reference is not used after it dangles. |
| 15 | "`linkedRootScratch.push_back()` … heap allocation … when > 256" | `Source/Audio/TraversalSession.cpp:361-386`, `.h:69`, `.cpp:24` | Confirmed | It is reserved to `scratchCapacity` = 256 and pushed once per connection into a root node, duplicates included, with no bound. Reached from `processBlock` → `driveWalk` → `startTraversalsFromFirstRoot` (`PluginProcessor.cpp:376`), and from `beginReplay` (`TraversalSession.cpp:70`). The input that triggers it is a graph with more than 256 arrows into root nodes. |
| 16 | "`std::sort()` … O(N log N) on audio thread whenever `isIdle()`" | same, `:377` | Corrected | `std::ranges::sort` sorts in place and does not allocate, so it is a cost, not a real-time violation. It runs whenever the pool is empty while playing (`PluginProcessor.cpp:376`), which is every block only when no unlinked root exists. There is no `isIdle()`. |
| 17 | "O(K × N) … linear scan … 4.2 million loop iterations" | `Source/Audio/EventManager.cpp:67-110` | Stale | `processEvents` now keeps `activeNotes` as a min-heap (`std::ranges::push_heap` / `pop_heap` on `remainingSamples`), which is the report's own recommendation. |
| 18 | "`startNextDue` performs a full linear scan over all 64 `pendingStarts`" | `Source/Audio/FlagScheduler.cpp:81-93`, `.h:34` | Confirmed | It does: 64 entries per event iteration. The cost is bounded and unmeasured. Nobody has profiled it. |
| 19 | "Unsynchronized data race on `pendingNoteOffs`" (P1) | `Source/Plugin/PluginProcessor.cpp:118-131`, `:303-307` | Refuted | JUCE's contract is that `releaseResources` is not called while `processBlock` runs. JUCE's own `AudioProcessorPlayer` calls it under the same `ScopedLock` that guards its audio callback (`juce_AudioProcessorPlayer.cpp:377-380`). `releaseResources` also clears `activeNotes` under the same assumption, and the report did not flag that. |
| 20 | RCU commendation: "`memory_order_release` / `acquire` … `blocksCompleted` … deletion strictly deferred" | `Source/Plugin/AudioSnapshotPublisher.cpp:72-104` | Corrected | The mechanism has changed. Both sides are `seq_cst`, and the counter is `blockEpoch`, odd while a block runs. The outgoing snapshot is freed immediately when the epoch is even and parked only when it is odd. The commendation stands. |
| 21 | "retired snapshot remains held … indefinitely … until a subsequent graph edit" | same `:86-104`, `PluginProcessor.cpp:131` | Corrected | A snapshot is parked only when `publish` lands during a block. It is then freed by the next `publish` or by `releaseRetiredSnapshots` in `releaseResources`. At most one edit's worth is held, so this is a P3 at most. |
| 22 | "`using NodeMap = std::unordered_map<int, std::shared_ptr<const RTNode>>`" | `Source/Graph/RTData.h:121-134` | Stale | `NodeMap` is now a `std::vector<RTNode> sortedById`, holding values and searched with `lower_bound`. `RTNode` still owns five vectors (`RTData.h:91-101`). The "50–100 ns per hop" cost has never been measured. |
| 23 | Recommendation: flatten into a `FlatRTGraph` | — | Judged: disproportionate | No profile shows a traversal step on the hot path. Graphs are hand-drawn and small. |
| 24 | "`indexOf = slot × 1024 + nodeId` … strides 4096 bytes" | `Source/Audio/NodeStateTable.cpp:85-88` | Stale | It is already `row * slotCount + slot`, the interleaved layout the report recommends. |
| 25 | "128 × 45 KB = 5.76 MB" | `NodeStateTable.h:48-49`, `TraversalSession.h:70` | Confirmed | 11 slots × 1024 rows × 4 B = 45,056 B per table, times 128 pool slots, plus each `NodeRowMap` (about 16 KB). It is allocated once in `prepare()`, never on the audio thread. The footprint is real. Whether it matters is not established. |
| 26 | VM strengths: fixed stack and locals, step budget, `INT_MIN / -1` check | `Source/Script/RTScript.h`, `Source/Audio/ScriptTraversalRule.cpp:243` | Confirmed | All present. |
| 27 | Recommendation: a register VM, "40–50% fewer instructions" | — | Judged: disproportionate | The 8192-step budget already bounds the worst case. There is no measurement, and the 40–50% figure has no source. |
| 28 | "`throw EmitFailure{}` … risks scope leakage in `emitBlock`" | `Source/Script/ScriptEmitter.cpp:127`, `:133`, `:181-198` | Corrected | The exceptions exist, and the parser uses the same pattern (`ParseFailure`, `ScriptParser.cpp:95`, `:204`), which the report missed. There is no scope leak: the `catch` is per statement inside `emitSequence`, so `emitBlock` always reaches `closeScope`. All of it is message-thread code. |
| 29 | Recommendation: `std::expected<void, ScriptDiagnostic>` | — | Judged: not writable | `std::expected` is C++23. The project is C++20, and the profile asks for C++23-only advice to be marked as such. |
| 30 | "no separate semantic analysis pass" | `Source/Script/ScriptEmitter.cpp` | Confirmed (fact) | Names are resolved during emission. The recommendation shows no diagnostic that the current pipeline misses. |
| 31 | "`createParameterLayout()` creates a single dummy `"gain"` parameter" (P1) | `Source/Plugin/PluginProcessor.cpp:27-47` | Stale | The layout now has Tempo Multiplier, Velocity and Transpose, plus Sync To Host in the uncommitted tree. There is no `"gain"` anywhere in `Source/`. `ongoing-issues.md` #9 still says "confirmed" and should be marked resolved. |
| 32 | "`UndoManager` owned by the Editor" (P1) | `Source/Plugin/PluginProcessor.h:77` | Stale | The processor owns it, so undo history outlives the editor. |
| 33 | "`createXml()` … can introduce floating-point precision round-off" | `Source/Plugin/PluginProcessor.cpp:189-191`, `:242` | Corrected | It does serialise to XML. JUCE writes doubles through `serialiseDouble` with at most 15 significant digits (`juce_String.cpp:2286`), so a double can in principle lose its last bit or two. Nothing here stores values that need that precision. Switching to `writeToStream` would break loading every existing saved session, which the report does not weigh. |
| 34 | "`BlockScope` calls `triggerAsyncUpdate()` … 700+ wakeups/sec" (P1) | `Source/Plugin/PluginEditor.h:71`, `.cpp:65` | Stale | The processor is no longer an `AsyncUpdater`. The editor polls the FIFOs through a `VBlankAttachment`. `ongoing-issues.md` #6 describes the old mechanism and should be updated. |
| 35 | "`trimPathToFraction()` instantiates `PathFlatteningIterator` twice … third in `strokePath`" | `Source/UI/Theme/CustomLookAndFeel_Nodes.cpp:119-160`, `:219-222` | Confirmed | Already `ongoing-issues.md` #10, which is unmeasured. |
| 36 | "each individual `Arrow` … own `VBlankAttachment` … 30 separate … `repaint()` calls" | `Source/UI/Node/Arrow.cpp:439`, `:456`, `:479`, `:507`, `Node.cpp:176` | Confirmed (fact) | True, and `Node` has one for its pulse too, which the report missed. The implied cost is overstated, because `repaint` calls are merged before painting (see #37). |
| 37 | "`drainCounts()` … dozens of dirty rectangle repaint events flood the JUCE message queue" | `Source/UI/Canvas/AudioCommandDrainer.cpp:124-139` | Refuted (mechanism) | `Component::repaint` posts no message per call. On macOS the peer adds the area to `deferredRepaints`, a `RectangleList`, and paints the merged region once (`juce_NSViewComponentPeer_mac.mm:1082-1090`). |
| 38 | C++20 table: concepts, span, `erase_if`, ranges, views, `<=>`, `enum class`, `constexpr` | `AudioUIBridge.h:9,33`, `RTGraphBuilder.h:36`, `AudioSnapshotPublisher.cpp:103`, `TraversalSession.cpp:377,380`, `TraversalPool.h:61,66`, `RTData.h:32`, `NodeController.h:73-90`, `ArrowAnimation.h:25-30` | Stale | Every row has been done as recommended. |
| 39 | "There are no automated tests, and none should be added" | CLAUDE.md, *Tests* | Stale | Catch2 v3 suites `SequenceTree_Tests` and `SequenceTree_GraphTests` exist, plus an optional RealtimeSanitizer target. |
| 40 | §7B–E: allow comments, short helpers, accessors and ternaries | — | Judged: out of scope | These are the owner's non-negotiable rules. §7D's example, public `activeNotes` "allows external callers to mutate container bounds while the scheduler is executing", names no concurrent path. All mutation is on the audio thread. |

Counts: Confirmed 10, Corrected 6, Stale 14, Refuted 4, Unverified 1. Four recommendations were judged without a verdict (#4, #23, #27, #29), and §7B–E (#40) was judged out of scope. Those five are not counted. No test run was needed: no verdict depends on runtime behaviour, and #13–#14 are settled by the loop invariant. `design-rules.sh` was not run, because the report makes no design-rule claim about a specific file. §7 argues against the rules rather than applying them.

### Critique
- **Method.** The report is broad, and in places it read the code carefully. The swap-and-pop analysis quotes the right lines. But it pattern-matched on "mutating a container while iterating" without following the loop direction, and that turned a correct idiom into two P0s. The `pendingNoteOffs` race was asserted without checking JUCE's threading contract for `releaseResources`, which the profile tells the analyst to confirm in `JUCE/modules/`.
- **Evidence vs conclusion.** Five items are labelled P0 "CRITICAL". None came with a concrete input, meaning a graph shape or host action, as the profile requires. The two that are still live at `HEAD` (#13, #14) are false. The 4.2-million-iteration and 50–100 ns figures are arithmetic on worst cases with no measurement behind them.
- **Omissions.** It missed the parser's use of exceptions next to the emitter's, `Node`'s own `VBlankAttachment`, and the fact that `activeNotes` has a hard capacity guard. That guard is the fact that decides #14. On the one live real-time finding (#15), it did not say that only arrows into root nodes count, which is what makes the trigger rare.
- **Proportionality.** Several recommendations, a flattened DOD graph, a register VM and a semantic pass, are large rewrites with no measured problem. That runs against the solo-project guidance in the profile. The fixes that did get made (heap, interleaved table, dense rows) were the small ones.
- **Fit with the project's rules.** §7 treats four Key Design Rules as defects. An argument against a rule belongs in its own document addressed to the owner, not among findings. The `std::expected` recommendation cannot be written in C++20.
- **Staleness.** The report is six days older than `HEAD`, and 14 of its claims have since been resolved. The next report should read `ongoing-issues.md` and `Reviewed/` first, as the profile's "Existing Reports" section asks.

### What Survives
- **P2: `findFirstUnlinkedRootId` can reallocate on the audio thread.** `Source/Audio/TraversalSession.cpp:372` pushes one entry per arrow into a root node, duplicates included, into a vector reserved to 256 (`.h:69`, `.cpp:24`). It is reached from `processBlock` → `driveWalk` (`PluginProcessor.cpp:376`) and from `beginReplay` (`TraversalSession.cpp:70`). It breaks the no-allocation rule for any graph with more than 256 root-bound arrows. The result depends only on the published `NodeMap`, so the message thread could compute it at publish time. The RealtimeSanitizer target can confirm it with a 257-arrow graph.
- **P3: `ongoing-issues.md` is behind the code.** #3 (graph publishing from the UI), #5 (host transport), #6 (`triggerAsyncUpdate` wakeups) and #9 (dummy `"gain"` parameter) describe states that no longer exist (ledger #1, #6, #31, #34). Mark them resolved so the next audit stops rediscovering them.
- **P3: The flag scheduler's linear scan.** `Source/Audio/FlagScheduler.cpp:81-93` scans all 64 slots on every event iteration. It is bounded. Profile it before changing it.
- **P3: Selection lives on view components.** `Source/Input/SelectionOps.cpp:33-44` derives the selection from `Node::isSelected`. There is no present defect, because the only rebuild is state restore. Keep it in mind if anything headless ever needs the selection.
- **P3 (already tracked): trail drawing flattens each curve three times per frame, and every `Arrow` and `Node` has its own `VBlankAttachment`.** See `ongoing-issues.md` #10 and `Arrow.cpp:439`. Profile before changing.

### Questions for the Author
1. For #12: can a note node own an arrow to a `Modulator`? Read `ConnectionOps`' connect path and give the shape, or withdraw the claim.
2. Before flagging a removal inside a loop, state the loop direction and which indices remain to be visited. Why did #13 and #14 not follow `i` downward?
3. For every threading claim, cite the framework's contract (the header or doc comment in `JUCE/modules/`) before calling it a race.
4. Read `ongoing-issues.md`, `Reviewed/` and `Proposals/Implemented/` before writing, and mark stale items as resolved rather than restating them.
5. Put performance claims behind a measurement: a profile, or a test that times a representative graph. Label arithmetic worst cases as such.
6. Move arguments against the design rules into a separate document addressed to the owner.

## Corrected Report

### 1. Architecture & System Design

#### A. Inverted Coupling: The UI Drives the Audio Model
> **Resolved:** `RTGraphBuilder` is owned by the processor (`PluginProcessor.h:85`) and listens to the graph tree itself. No UI file calls `makeRTGraph`, `rebuildAllGraphs` or `updateDurationMaps`, and `detachStateListeners` no longer exists. `ongoing-issues.md` #3 is out of date.

#### B. The Service Locator God Object (ApplicationContext)
- `ApplicationContext` bundles non-owning pointers to the processor, canvas, look-and-feel, undo manager, graph state, rules, controller and graph builder, and is passed into most UI components. `canvas` and `nodeController` are filled in during `PluginEditor` construction, so their constructors must not read them.
- Recommendation: narrow component dependencies.
  > **Correction:** "brittle … obscures dependencies" → the construction order is documented in CLAUDE.md, and the owner deliberately kept the context in the 2026-09-04 refactor. This is a settled decision, not a finding.

#### C. ID Allocation: Monotonic Increment vs. Hardcoded 1024 Bounds
> **Resolved:** `ongoing-issues.md` #4 (2026-09-24). `NodeRowMap` maps IDs to dense rows (`NodeStateTable.h:22-40`), so IDs can grow without limit.

#### D. Host Transport vs. Internal Transport Inconsistency
> **Resolved:** CLAUDE.md now specifies that the plugin follows the host's transport (`followHostTransport`, `PluginProcessor.cpp:387`). The uncommitted working tree adds a "Sync To Host" parameter that makes it optional. `ongoing-issues.md` #5 is superseded.

#### E. Misplaced UI State: Selection Owned by Visual View Components
- `SelectionOps::selectedNodeIds` (`Source/Input/SelectionOps.cpp:33-44`) and its siblings (`:53`, `:62`, `:71`, `:729`) derive the selection by iterating `canvas->nodeManager.all()` and reading `node->isSelected`.
- If the canvas rebuilds, the selection is lost.
  > **Correction:** "instantly lost" on rebuild → `rebuildFromNodeMap` is called only from state restore (`PluginProcessor.cpp:232`), where dropping the selection is expected.
- Headless operations cannot query the selection. True, but no headless consumer exists.
- Recommendation: a `SelectionModel`. Defer it until something needs the selection without a canvas.

### 2. Real-Time Safety & Threading (Audio Thread)

#### A. Exception Throwing via `.at()` on the Audio Thread
> **Resolved:** `ongoing-issues.md` #7 and #8 (2026-09-27, `Proposals/Implemented/traversal_target_lookup_hygiene.md`). There is no `.at(` in `Source/Audio` or `Source/Plugin`. `getTargetNode` and `getRootNode` are gone.

#### B. Lookahead State Mutation (`peekNextTarget`)
> **Resolved:** `ongoing-issues.md` #1 (2026-09-24) and #13 (2026-09-25). `peekNextTarget` is `const` (`TraversalLogic.h:144`) and honours the switch hold.
- `peekNextTarget` filters with `isAudibleChild` while `advance` filters with `isAdvanceableChild`.
  > **Unverified:** the predicates differ only by `NodeType::Modulator` (`TraversalLogic.cpp:19-26`). Node creation never gives a note node a `Modulator` child (`NodeCreationDispatcher.cpp:27-40`). The claim can be settled by finding whether `ConnectionOps` lets an arrow be drawn from a Node onto an existing `Modulator`, and if it does, by adding a parity shape.

#### C. Container Mutation Skips in `stopTraversalNotes`
> **Refuted:** the loop at `TraversalSession.cpp:423-441` runs backward. `removeNote` (`NoteScheduler.cpp:88-92`) moves `back()`, an index already visited and kept, into `i`. Nothing unvisited is skipped, and no note hangs.

#### D. Container Mutation While Iterating in `handleOrphanNotes`
> **Refuted:** `EventManager.cpp:5-47` iterates backward. The swapped-in element was already visited, and `pushNote` appends above every index still to visit, inside the capacity reserved at `NoteScheduler.cpp:5` and guarded at `:20`. The fields used after the removal are copied first.

#### E. Memory Allocation on the Audio Thread: `linkedRootScratch`
- `findFirstUnlinkedRootId` (`Source/Audio/TraversalSession.cpp:361-386`) clears `linkedRootScratch`, which is reserved to `scratchCapacity` = 256 (`.h:69`, `.cpp:24`). It pushes one entry per connection whose child is a root node (`:372`), with no bound and no deduplication. More than 256 such arrows reallocate on the audio thread. It is reached from `processBlock` → `driveWalk` → `startTraversalsFromFirstRoot` (`PluginProcessor.cpp:376`) and from `beginReplay` (`TraversalSession.cpp:70`).
- The function also sorts on the audio thread.
  > **Correction:** "O(N log N) … whenever `traversalSession.isIdle()`" → `std::ranges::sort` (`:377`) is in place and does not allocate, so it is a cost rather than a violation. It runs while playing with an empty pool, which is every block only when no unlinked root exists.
- Fix: the first unlinked root depends only on the published `NodeMap`, so it can be computed on the message thread when the snapshot is published. Bounding the scratch vector is the alternative.

#### F. Event Loop Complexity in `processEvents`
> **Resolved:** `EventManager::processEvents` (`EventManager.cpp:67-110`) keeps `activeNotes` as a min-heap on `remainingSamples`, as recommended.

#### G. Linear Scanning in `FlagScheduler`
- `startNextDue` (`Source/Audio/FlagScheduler.cpp:81-93`) scans all `maxPendingStarts` = 64 slots on every event iteration.
  > **Unverified:** the cost has not been measured. A profile of a flag-heavy graph would settle whether it matters.

#### H. Race Condition on `pendingNoteOffs`
> **Refuted:** under JUCE's contract, `releaseResources` never runs while `processBlock` does. `AudioProcessorPlayer` calls it under the audio callback's own lock (`juce_AudioProcessorPlayer.cpp:377-380`).

#### I. Architectural Strength: AudioSnapshotPublisher (RCU Pattern)
- The audio thread reads a raw `const Snapshot*`, never touches `shared_ptr`, and never frees.
  > **Correction:** "`memory_order_release` / `acquire` … `blocksCompleted` … deletion strictly deferred" → both sides are `seq_cst` and the counter is `blockEpoch`, odd during a block (`AudioSnapshotPublisher.cpp:72-104`). The outgoing snapshot is freed immediately when the epoch is even and parked only when it is odd.

#### J. Lazy Snapshot Reclamation Memory Hold
> **Correction:** "held … indefinitely" → only a snapshot retired during a block is parked. It is freed by the next `publish` or by `releaseRetiredSnapshots` in `releaseResources` (`PluginProcessor.cpp:131`). At most one edit's snapshot is held.

### 3. Data-Oriented Design (DOD) & Cache Locality

#### A. Pointer-Chasing `NodeMap`
> **Resolved (partly):** `NodeMap` is now a sorted `std::vector<RTNode>` held by value (`RTData.h:121-134`). `RTNode` still owns vectors for traversals, notes, connections, dangling arrows and disabled traversals (`:91-101`).
> **Unverified:** the cache-miss cost has not been measured. Flattening further is not justified without a profile.

#### B. `NodeStateTable` Striding and Footprint
> **Resolved:** the index is `row * slotCount + slot` (`NodeStateTable.cpp:85-88`), which is the interleaved layout recommended here.
- The footprint claim is confirmed: 11 × 1024 × 4 B = 45,056 B per table × 128 pool slots (`TraversalSession.h:70`) ≈ 5.8 MB, plus each slot's `NodeRowMap`. It is allocated in `prepare()`, not on the audio thread.

### 4. Embedded Scripting Language, Compiler & Bytecode VM

#### A. Strengths of the Script Subsystem
- Confirmed as stated: compilation happens only on the message thread, the VM has a fixed stack, fixed locals and an 8192-step budget, and division is guarded (`ScriptTraversalRule.cpp:243`).

#### B. Stack Machine Overhead vs. Register VM
- `ScriptInstruction` is `{ ScriptOpcode, int }` (`RTScript.h:66-71`).
  > **Unverified:** "reduces instruction count by 40–50%" has no source and no measurement. The step budget already bounds the worst case.

#### C. Exception-Driven Compiler Control Flow
- `Emitter::fail` throws `EmitFailure` (`ScriptEmitter.cpp:127`, `:133`), caught per statement in `emitSequence` (`:181-190`). The parser does the same with `ParseFailure` (`ScriptParser.cpp:95`, `:204`, caught at `:12`, `:260`).
  > **Correction:** "risks scope leakage in `emitBlock`" → the `catch` sits inside `emitSequence`, so `emitBlock` always reaches `closeScope` (`:192-198`). There is no leak.
  > **Correction:** "Replace with `std::expected`" → `std::expected` is C++23, and the project is C++20.

#### D. Lack of Static Semantic Analysis Pass
- Confirmed: name resolution happens during emission. The report names no diagnostic that the current pipeline misses.

### 5. JUCE Framework Leverage, UI & Idiomatic Patterns

#### A. Dummy APVTS Parameters
> **Resolved:** `createParameterLayout` (`PluginProcessor.cpp:27-47`) registers Tempo Multiplier, Velocity and Transpose, plus Sync To Host in the uncommitted tree. There is no `"gain"`. `ongoing-issues.md` #9 is out of date.

#### B. `UndoManager` Belongs to AudioProcessor
> **Resolved:** `PluginProcessor.h:77`.

#### C. ValueTree Serialization: XmlElement vs Binary Stream
- `getStateInformation` uses `createXml` and `copyXmlToBinary` (`PluginProcessor.cpp:189-191`), and restore uses `getXmlFromBinary` (`:242`).
  > **Correction:** "precision round-off" → JUCE serialises doubles with at most 15 significant digits (`juce_String.cpp:2286`). Nothing here stores values that need more. Changing the format would also break loading every existing saved session.

#### D. Idle Message-Thread Wakeups
> **Resolved:** the processor is no longer an `AsyncUpdater`, and the editor polls the FIFOs with a `VBlankAttachment` (`PluginEditor.h:71`, `.cpp:65`). `ongoing-issues.md` #6 describes the old mechanism.

#### E. Graphics & VBlank Attachment Overhead
- `trimPathToFraction` (`CustomLookAndFeel_Nodes.cpp:119-160`) flattens the path twice, and `strokePath` flattens it a third time (`:219-222`). This duplicates `ongoing-issues.md` #10.
  > **Unverified:** it has not been profiled.

#### F. Proliferation of Independent `juce::VBlankAttachment` Instances
- Each `Arrow` attaches one while animating (`Arrow.cpp:439`, `:456`, `:479`, `:507`), and so does each `Node`'s pulse (`Node.cpp:176`).
  > **Unverified:** the cost has not been measured. The repaints are merged by the peer (see G).

#### G. Repaint Storms in `drainCounts`
> **Refuted:** `Component::repaint` posts no message per call. The macOS peer accumulates each area into `deferredRepaints` and paints the merged region once (`juce_NSViewComponentPeer_mac.mm:1082-1090`).

### 6. Modern C++ Language Feature Leverage
> **Resolved:** every row of the table has been done: `std::invocable` (`AudioUIBridge.h:9`, `:33`), `std::span` (`RTGraphBuilder.h:36`), `std::erase_if` (`AudioSnapshotPublisher.cpp:103`), `std::ranges::sort` / `binary_search` (`TraversalSession.cpp:377`, `:380`), views in `TraversalPool.h:61`, `:66`, a defaulted `<=>` (`RTData.h:32`), `enum class` (`NodeController.h:73-90`), and `static constexpr` (`ArrowAnimation.h:25-30`).

### 7. Critique of Repository Meta-Rules & Development Philosophy
#### A. The "No Automated Tests" Mandate
> **Resolved:** the Catch2 suites `SequenceTree_Tests` and `SequenceTree_GraphTests` exist, and a RealtimeSanitizer target is optional.
#### B–E. Comments, Short Functions, Accessors, Ternaries
> **Condensed:** arguments against the owner's non-negotiable Key Design Rules. They make no checkable claim about the code, except §7D's claim that external callers mutate `activeNotes` "while the scheduler is executing". That is refuted: every mutation of `activeNotes` happens on the audio thread, and no path runs concurrently with the scheduler.

### 8. Comprehensive Summary of Critical Action Items
| Priority | Category | File & Location | Issue | Status at review |
| :---: | :--- | :--- | :--- | :--- |
| P2 | RT Safety | `Source/Audio/TraversalSession.cpp:372` | `linkedRootScratch` grows past its 256 reserve on the audio thread | Confirmed |
| P3 | Hygiene | `.claude/notes/ongoing-issues.md` #3, #5, #6, #9 | Entries describe code that no longer exists | Confirmed |
| P3 | Performance | `Source/Audio/FlagScheduler.cpp:81` | Linear scan over 64 pending starts | Confirmed, unmeasured |
| P3 | Performance | `Source/UI/Theme/CustomLookAndFeel_Nodes.cpp:119` | Triple flattening per trail per frame | Confirmed (#10), unmeasured |
| P3 | Architecture | `Source/Input/SelectionOps.cpp:33` | Selection derived from view components | Confirmed, no present defect |
| — | RT Safety | `TraversalLogic.cpp:512`, `TraversalSession.cpp:317` | `.at()` throws | Resolved (#7, #8) |
| — | RT Safety | `TraversalLogic.cpp:385` | `peekNextTarget` mutates state | Resolved (#1, #13) |
| — | RT Safety | `TraversalSession.cpp:423-441` | Swap-and-pop skips notes | Refuted |
| — | RT Safety | `EventManager.cpp:5-47` | `handleOrphanNotes` mutation while iterating | Refuted |
| — | RT Safety | `PluginProcessor.cpp:118-131` | `pendingNoteOffs` race | Refuted |
| — | Architecture | `NodeCanvas.cpp`, `PluginEditor.cpp` | Graph building driven by the UI | Resolved |
| — | Architecture | `NodeStateTable.h`, `GraphState.cpp` | ID cap at 1024 | Resolved (#4) |
| — | Performance | `EventManager.cpp` | Linear scan for the next note-off | Resolved (min-heap) |
| — | Performance | `RTData.h`, `NodeStateTable.h` | Pointer-chasing map; strided table | Resolved (sorted vector; interleaved) |
| — | JUCE Idiom | `PluginProcessor.cpp` | Dummy `"gain"` parameter | Resolved |
| — | JUCE Idiom | `PluginEditor.h` | `UndoManager` in editor | Resolved |
| — | JUCE Idiom | `PluginProcessor.cpp` | `triggerAsyncUpdate` wakeups | Resolved (VBlank polling) |
| — | Architecture | `PluginProcessor.cpp` | Host transport vs. CLAUDE.md | Resolved (CLAUDE.md updated; Sync To Host) |
| — | Compiler | `ScriptEmitter.cpp` | Exceptions for diagnostics | Confirmed fact; `std::expected` not available in C++20 |
| — | Reliability | CLAUDE.md | No automated tests | Resolved |
