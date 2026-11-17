# SequenceTree — Code Review

**Repo:** https://github.com/elibaumgardner/SequenceTree
**Commit reviewed:** `4600d21` — "Move playback into PlaybackEngine and the saved document into PluginDocument, split ornament figure and symbol editing out of OrnamentLibraries, add BindPane and OrnamentPane to the bottom bar, add WeightedPick, move AudioSnapshotPublisher into Graph, publish the active rule only when its id or source changes, and add a fixture for state saved before the schema version"
**Date:** 2026-11-17
**Scope:** Daily recurring review. This is Day 1. Each day adds new findings to this file, refines old ones, and re-checks previous findings against the current code. Items marked **UNVERIFIED** / **suspected** are not proven bugs — they need a caller, a JUCE source check, or a run to settle.

## At a glance

- 198 C++ files in `Source/` + `Tests/`, 34,785 lines (Source+Tests). Whole repo ~36k lines including design/docs.
- 90 Source `.cpp` files, 8 test files, 169 `TEST_CASE`s (492 `CHECK`s). No `TODO`/`FIXME`/`HACK`, no `malloc`/`strcpy`/`std::rand`, no raw `delete`, 2 raw `new`s (both JUCE-required factory returns).
- C++ standard is **23 in CMake** (`cxx_std_23` on all 4 targets) but **C++20 on the README badge** — see H-UI-4 / B-5.
- Submodules (`JUCE/`, `Catch2/`, `ArsNovaUI/`) were empty in the clone used for this review, so JUCE internals, ArsNovaUI behaviour, and any build/run could **not** be verified here. README does document `git submodule update --init`, but CMake has no guard for a missing submodule.
- What is genuinely good: real-time design is deliberate (pre-allocated pools, lock-free FIFOs, snapshot publish, script step budget), listener hygiene in UI code is consistent, documentation is unusually deep, and traversal/script test coverage is deep (45 traversal cases, 44 script-compiler cases).

## How this file is maintained

- New findings are appended / merged in, never silently replacing old ones. When a finding is fixed, it moves to "Fixed / re-checked" with the commit that fixed it.
- Severity: Critical = data loss / crash / silent wrong playback; High = wrong behaviour or data loss under plausible input; Medium = correctness trap, silent failure, or significant perf/doc risk; Low = style / consistency / defence-in-depth.
- Every finding should cite file + line as read at commit `4600d21`. Line numbers drift as the code changes — re-check before acting on an old entry.

## Self-check — corrections made while writing this report

These are places where an initial finding did **not** survive re-checking the code. They are recorded here so the daily run does not re-report them as facts.

1. **Modulo-by-zero (`% countLimit`) — DOWNGRADED from Critical/High to Low / defence-in-depth.** `ChildSelector.cpp:202` and `TraversalRule.cpp:95,113` do evaluate `parentCount % child->countLimit` before a local `> 0` check. But both sites are only reached after `RuleContext::eligibleChild()` (`TraversalRule.cpp:24-30`), which returns `nullptr` when `child.countLimit <= 0`. Every other `% countLimit` site in the codebase guards `<= 0` first (`FlagScheduler.cpp:32`, `ModulatorWalk.cpp:259-263`, `TraversalDispatcher.cpp:651`, `TraversalLogic.cpp:52-53`, `TraversalRule.cpp:75`). `RTNode::countLimit` does default to `0` (`RTData.h:122`), so the guard belongs at the modulo sites too for defence-in-depth, but there is **no proven crash path** via the two cited sites. A `countLimit = 0` unit test is still worth adding.
2. **Ornament zero-weight division — NOT a finding.** `NoteScheduler.cpp:209-213` divides by `remainingWeight`, but `OrnamentFigure::walkChain()` only pushes steps with `durationWeight > 0`, so the divisor cannot be 0 for a published figure. Left as a defence-in-depth note only.
3. **Script VM stack / arithmetic — checked, no finding.** `ScriptRun.cpp` bounds stack (512), call depth (8), step budget (8192), guards `INT_MIN / -1` and div/mod-by-zero. Jump targets outside `[0, instructionCount)` exit the call loop safely.

---

## Critical

### C1 — Dangling arrow creates a phantom RTNode with id 0
`Source/Graph/RTGraphBuilder.cpp:494-497` (`createRTNodes`)
For every child id it does `childDataTree = graphState.getNode(childId); jassert(childDataTree.isValid()); stack.push_back(childDataTree);` — `jassert` is debug-only, no release guard. The next iteration reads `Id` from the invalid tree (= 0) and creates `RTNode{nodeID=0, graphID=0}` in `tempNodeMap[0]` / `builtNodes[0]`. Sibling passes `createRTNodeConnections` (522-551) and `fillDurationMap` (552-625) *do* `continue` on invalid children.
Fix: `if (!childDataTree.isValid()) continue;` before `push_back`, plus log/count it. Validate at write time (see C3).

### C2 — Adding/removing a MIDI note never rebuilds the RT graph
`Source/Graph/RTGraphBuilder.cpp:672-704` (`rememberStructureChange`); `Source/Graph/GraphState.cpp:281-298` (`addMidiNote`)
`rememberStructureChange` handles only `NodeChildrenIds`, `TraversalChildrenIds` / `DisabledTraversalIds`, `DanglingArrows` / `DanglingArrow`. No branch for `MidiNotesData` / `MidiNoteData`, so add/remove triggers `triggerAsyncUpdate()` with empty `PendingChanges` and `handleAsyncUpdate` rebuilds nothing. `addMidiNote` also sets properties before `addChild`, so the property-change path never fires either.
Fix: add `else if (parent.getType() == MidiNotesData) graphOwner = parent.getParent();` in `rememberStructureChange`.

### C3 — `connectNodes` has no validation — invalid, duplicate, self-loop and cyclic arrows are persisted
`Source/Graph/GraphState.cpp:196-231` (`connectNodes`), `:233-248` (`disconnectNodes`)
Unconditionally appends a `NodeId` child before checking `childNode.isValid()` (which only skips the colour copy). Never checks parent exists, child exists, `parent != child`, duplicate pair, or cycle.
Consequences: (a) missing child leaves a permanent dangling arrow → C1 at RT-build; (b) duplicate connection leaves a duplicate `NodeId` entry, but `disconnectNodes` removes only the first match, and `linkParent` dedupes `parentIdsOf`, so index and document disagree; (c) self-loop/cycle persisted silently, RT builder only tolerates it via `tempNodeMap` dedup, dropping the back-edge with no diagnostic.
Fix: return early (assert/log) if `!parentNode.isValid() || !childNode.isValid() || parentId == childId`, if `getConnection(parent,child).isValid()`, or if the child already reaches the parent; make `connectNodes` return `bool`.

### C4 — `replaceState` is destructive and unvalidated — corrupt/duplicate data is loaded, old document already gone
`Source/Graph/GraphState.cpp:373-397`
Clears `traversals.map` and `nodeMap` *first*, then copies restored children with no checks: no duplicate-`Id`, missing-`Id`, arrow-target, root, cycle, or traversal-map validation. `nodeIdIncrement` is recomputed from top-level `Id`s only. No undo manager (`nullptr`).
Consequences: empty/corrupt restore unrecoverably loses the previous graph; duplicate ids make `indexNode` overwrite the first node, which becomes unreachable via `getNode` while still in `nodeMap`; missing/0 id makes the next `addRootNode` / `addChildNode` reuse an existing id; arrows to missing nodes survive and trigger C1.
Fix: validate into a temporary structure *before* clearing (unique non-zero ids, every `NodeChildrenIds` entry refers to a present node, no self-loops), only then swap in, and rebuild `nodeIdIncrement` from *all* ids.

### C5 — `removeNode` deletes one node and orphans its whole subtree
`Source/Graph/GraphState.cpp:250-279`; `:611-622` (`unindexNode`)
Removes only the named node, disconnects incoming arrows, calls `encapsulation.removeMember`. Never touches the removed node's children: `unindexNode` unlinks them from `parentIdsOf`, but they stay in `nodeMap` with their old `rootNodeId`, unreachable from any root.
Fix: decide/document the contract — cascade-delete the reachable subtree (collect via `NodeChildrenIds` before removal) or reparent children to the removed node's parent(s); at minimum assert/log when removal would orphan children.

### C6 — Replay discards the MIDI it is supposed to deliver at the relocation point
`Source/Audio/TraversalSession.cpp:208-223, 248-292`
`beginReplay()` starts traversals into `replayMidi`, then `continueReplay()` loops: `processEvents(chunkSamples, replayContext)` (writes note-ons into `replayMidi`), then `replayMidi.clear()` at line 268 on *every* chunk, including the last. After the loop, line 285: `context.midiMessages.addEvents(replayMidi, 0, -1, 0)` — `replayMidi` is always empty at that point in the normal case, so this is dead code. Line 288 clears it again.
Consequence: a note that should be sounding at the host seek target never gets a note-on in the live buffer. Its `ActiveNote` *does* remain in `NoteScheduler::activeNotes`, so it will later get a note-off for a note that was never turned on, and playback after a seek is silent until the next traversal step.
Fix: do not clear `replayMidi` after the final chunk; or, at replay end, re-emit note-ons at sample 0 for every `ActiveNote` with `remainingSamples > 0` (using its current ornament step pitch), then clear. Add a regression test: seek, then assert the live `MidiBuffer` contains a note-on for the sounding note.

### C7 — Replay can never catch up online, and is unbounded offline
`Source/Audio/TraversalSession.cpp:248-282`, `Source/Audio/PlaybackEngine.cpp:89-113,170-178`, `TraversalSession.h:97-98`
`continueReplay()`: budget is 25% of block wall-time (`replayShareOfBlock = 0.25`). While `playing`, lines 275-277 add `numSamples` back to `replayRemainingSamples` every block. If traversal/script work per musical sample is slower than ~4× real-time, the gap grows every block and playback stays in `Playback::Replaying` forever — no live output, only recorded bridge commands.
Offline (`renderingOffline == true`) the time check is bypassed entirely: the `while (replayRemainingSamples >= 1.0 …)` loop runs to completion in one `processBlock`, with no iteration/sample cap. `pendingRelocationSamples` is an absolute position (`PlaybackEngine.cpp:111-113`), so a jump to e.g. 2 hours is ~350M samples / ~340k chunks of 1024, each `processEvents()` doing up to 4096 events.
Fix: cap total replay per offline block (max samples *and* max events, then resume live from the target with traversals re-seeded at the root); online, if replay has not finished after N blocks, abandon the silent walk and start live at the target. Never use wall-clock `getMillisecondCounterHiRes()` as the only bound on the audio thread — add a chunk/event counter.

---

## High — Audio / real-time / performance

### A-H1 — Script step budget is per-call, not per-block: one block can run ~100M+ script instructions
`Source/Audio/Traversal/ScriptRun.cpp:27-55`, `Source/Script/RTScript.h:142-159` (`stepBudget = 8192`, `maxCallDepth = 8`), `Source/Audio/Traversal/TraversalLogic.cpp:326-380,594-609`, `Source/Audio/EventManager.cpp:29,41` (`maxEventsPerBlock = 4096`)
Each `ScriptRun::call()` gets a fresh 8192-instruction budget. One note expiry can trigger several independent runs: `stepTraversal()` → script `main`, `advanceByScript()` → `advance`, `peekNextTarget()` → a Trial run, `decideNextModulator()` → another run, and `skipAhead()` calls `peekNextTarget()` up to `maxAdvanceSteps = 64` times — each peek is a full Trial script run. `EventManager::processEvents()` allows 4096 events per block with no wall-time check. Worst case is therefore 4096 events × several runs × 8192 instructions.
Fix: carry a single shared instruction/time budget across all `ScriptRun`s in one `processBlock` (e.g. a budget object in `DispatchContext`, decremented by each run, aborting further script calls at 0), and cap `skipAhead` peeks that invoke scripts (cache the peek result per step).

### A-H2 — Note preview with count up to 9,999 does O(visitCount × hosts) rule selections on the audio thread
`Source/Audio/Traversal/TraversalLogic.cpp:28-82`, `Source/Audio/TraversalSession.cpp:523`, `Source/Input/Gestures/NotePreviewGestures.cpp:20`, `Source/Util/NodeInfo.h:50-51` (`maximumCountLimit = 9999`)
`startPreview()` sets `previewTraversals.previewVisitCount = jmax(1, request.visitCount)`; the UI count field allows 1–9999. `TraversalLogic::begin()` in Preview mode seeds up to 1024 nodes, then for *each* host node with an alternative runs `for (visit = 1; visit < previewVisitCount; ++visit) { set(Count,…); advanceAlternative(…); }` — `advanceAlternative()` → `selectNextChild()` → `TraversalRule::selectChild()` iterates all connections twice. At 9,999 visits × even a handful of hosts this is tens/hundreds of thousands of rule evaluations inside `playPreview()`, on the audio thread, from a single click.
Fix: clamp `previewVisitCount` on the audio thread (e.g. `jmin(request.visitCount, 64)` — the seeded state only needs to reproduce the *count modulo* behaviour), or compute the seeded counts analytically instead of simulating every visit.

### A-H3 — `EventManager` event cap is a count, not a time bound, and late events drift by a whole block
`Source/Audio/EventManager.cpp:25-78`, `Source/Audio/EventManager.h:41`
Loop exits after 4096 events, but per-event work is unbounded (A-H1, plus `pushNote` → chord walk, cross-tree, flags, script). A dense block can therefore still overrun the block by a large factor; there is no wall-time / deadline check. When the cap is hit, lines 70-78 still subtract `numSamples` from every remaining note, so unprocessed expired notes go negative and fire at sample 0 of the *next* block. Under sustained overload this lateness compounds.
Fix: add a time budget to the loop (in addition to the count cap), and when the cap is hit, preserve the original expiry sample for the deferred notes instead of letting them all collapse to sample 0.

### A-M1 — Snapshot publish / retired-snapshot free is unsynchronised (suspected race)
`Source/Graph/AudioSnapshotPublisher.cpp:25-50,99-102,176-189`, `Source/Plugin/PluginProcessor.cpp:34-41`
`publish()` mutates `publishedSnapshot`, `retiredSnapshots` (a `std::vector`), `snapshotGeneration` and `currentSnapshot` with no lock and no atomic for the `shared_ptr`/vector. It relies on an unenforced "message thread only" convention. `releaseResources()` calls `releaseRetiredSnapshots()` (`retiredSnapshots.clear()`), and the host thread for `releaseResources()` was **UNVERIFIED** here. If a graph edit publishes while the host is stopping playback, the vector is mutated from two threads.
Fix: assert `isThisTheMessageThread()` in `publish()` / `releaseRetiredSnapshots()`, or guard `retiredSnapshots` / `publishedSnapshot` with a mutex the audio thread never takes. Note `getPublished()` returns a raw pointer to the non-atomic `publishedSnapshot` — it has no callers in `Source/`, so do not expose it to the audio thread.

### A-M2 — Every graph publish deep-copies the whole graph (message thread, but expensive)
`Source/Graph/AudioSnapshotPublisher.cpp:58-92`, `Source/Graph/RTGraphBuilder.cpp:145-204`
`publishGraphs()`: `std::ranges::set_union(replacementNodes, oldNodes | filter, …)` copies every `RTNode` — each copy deep-copies its 5 vectors (`traversals`, `notes`, `connections`, `danglingArrows`, `ornament`). `updateDurationMaps()` line 157 does `make_shared<NodeMap>(*edit->globalNodes)` — another full deep copy — just to change a few arrow durations.
Fix: share unchanged per-graph `NodeMap`s (copy-on-write per graph, not per whole graph set) and move, rather than copy, `replacementNodes` into the merged map; make `updateDurationMaps()` copy only the affected nodes.

### A-M3 — `NoteScheduler` silently drops notes past 1024
`Source/Audio/NoteScheduler.cpp:11-50`, `Source/Audio/NoteScheduler.h:116` (`maxExpectedActiveNotes = 1024`)
`scheduleNote()` returns silently if `activeNotes.size() >= 1024`. The traversal has already advanced, so audible output and traversal state diverge with no log, no counter, and no note-off to clean up.
Fix: when at capacity, steal the oldest non-sounding / shortest-remaining note (send its note-off first), or at minimum count drops in an atomic the UI can display.

### A-M4 — Virtual MIDI output timing is wall-clock + 1 ms timer, not sample-accurate
`Source/Audio/ThreadMessaging/VirtualMidiOutput.cpp:44-76`, `Source/Audio/ThreadMessaging/VirtualMidiOutput.h:47-52`
`mirrorBlock()` stamps messages with `Time::getMillisecondCounterHiRes()` taken *after* `processBlock`, plus `samplePosition / sampleRate` — wall-clock at mirror time, not the host's block-start time, so host buffer latency is baked in as an offset. `hiResTimerCallback()` (1 ms) drains one message at a time. Hi-res timer jitter (typically several ms) is added on top, and messages with `numBytes > 3` are dropped silently.
Fix: stamp from the host playhead / block start if available, or document the virtual port as "best-effort, ± few ms"; batch messages with the same timestamp into one `sendBlockOfMessages()` call. **UNVERIFIED** whether `MidiOutput::sendBlockOfMessages` itself is lock-free — JUCE source absent.

### A-M5 — `processBlock` is `noexcept [[clang::nonblocking]]` but calls host/JUCE APIs whose RT-safety was not verifiable here
`Source/Plugin/PluginProcessor.cpp:64-92`, `Source/Audio/PlaybackEngine.cpp:89-134`
`processBlock()` calls `getPlayHead()->getPosition()` (returns `Optional<PositionInfo>` by value), `buffer.clear()`, `midiMessages.clear()`, and `playback.processBlock()` → many `MidiBuffer::addEvent()` calls. Whether `getPosition()` or `MidiBuffer::addEvent()` allocates/locks is host- and JUCE-version-dependent and **UNVERIFIED** in this clone (JUCE submodule empty).
Fix: verify against the pinned JUCE 8 source; pre-size the host `MidiBuffer` if the host allows it; do not add further playhead queries in the hot path. Do not present this as a confirmed allocation.

### A-M6 — Host ppq→seconds uses instantaneous BPM; relocation tolerance is coarse
`Source/Audio/PlaybackEngine.cpp:89-134`, `PlaybackEngine.h:41-44` (`relocationToleranceBeats = 1/64`)
`hostSeconds = hostBeats * 60 / hostBpm` uses the *current* BPM, with no tempo map. Under tempo automation the relocation target in samples is wrong. Tolerance is 1/64 beat (~7.8 ms at 120 BPM): a host jump smaller than that is ignored and playback stays out of sync by up to the tolerance.
Fix: if the host provides a tempo map, use it; otherwise clamp the relocation target to the nearest note boundary and tighten the tolerance (e.g. 1/256 beat) or compare in samples, not beats.

### A-M7 — Pool / state lookups are linear scans in the hot path; >1024 nodes silently corrupt counts
`Source/Audio/Traversal/TraversalPool.h:81-100,232-244`, `Source/Audio/Traversal/NodeStateTable.cpp:71-130`, `Source/Audio/Traversal/NodeRowMap.cpp:31-70`
`TraversalPool::find()` / `findSlotIndex()` linearly scan up to 128 slots, and are called for *every* note. `RTNode::findConnection()` linearly scans connections per call, and `TraversalRule::selectChild()` calls `eligibleChild()` (which itself does `findConnection` + `nodes.find` + `ranges::find` over `disabledTraversals`) twice per child selection.
`NodeStateTable` caps at 1024 distinct nodes. Past that, `claim()` returns −1 and `ref()` returns a *shared* per-slot `outOfRangeSinks` entry — two different out-of-range nodes using the same slot alias each other's counts, and `get()` / `set()` silently return/drop. Only a debug `assert` fires.
Fix: index the pool by run-id, and make the 1024-node overflow loud in release (clamp + atomic overflow counter, or spill to a slower fallback map) instead of silent aliasing. See also G-H1.

### A-M8 — `previewRequests` and Virtual-MIDI FIFO overflow are never observed
`Source/Audio/ThreadMessaging/CommandFifo.h:42,69`, `Source/Audio/TraversalSession.h:62`, `Source/Audio/ThreadMessaging/VirtualMidiOutput.h:52`
`CommandFifo::overflowed` is set on drop and, for `previewRequests` (capacity 16) and Virtual-MIDI `messages` (4096), is never read or cleared anywhere (only highlights/arrows/counts are cleared in `AudioCommandDrainer.cpp:19-27`). Rapid preview clicks or a >4096-message block therefore lose requests/messages silently.
Fix: check `previewRequests.overflowed` in `playPreview()` and coalesce (keep the latest Start/Stop); check the MIDI fifo overflow in the timer callback and log/count it.

### A-L1 — Per-block orphan scan and per-event flag scan are wasted work in the common case
`Source/Audio/EventManager.cpp:25-29,80-122`, `Source/Audio/FlagScheduler.cpp:82-110`
`handleOrphanNotes()` runs every `processEvents()` call and does a binary `nodes.find()` per active note, even when the graph generation has not changed. `FlagScheduler::startNextDue()` linearly scans all 64 `pendingStarts` on *every* loop iteration, even when none are active.
Fix: skip the orphan scan when `snap.generation` is unchanged since the last scan; keep a count of active pending starts and skip `startNextDue()` when it is 0.

### A-L2 — `ModulatorWalk::isDescendantOf` guard is 10,000 steps per note; rule selection does two full passes
`Source/Audio/Traversal/ModulatorWalk.cpp:280-305` (`maximumAncestorSteps = 10000`), `Source/Audio/Traversal/TraversalRule.cpp:82-115`
`isDescendantOf()` walks parent pointers with a 10,000-step guard, doing a binary `nodes.find()` per step, on every `followMainWalk()` call. `NativeTraversalRule::selectChild()` iterates all connections twice, re-running `eligibleChild()` each time.
Fix: lower the guard to the graph depth (or detect a cycle with a visited set), and make `selectChild()` a single pass that tallies and then picks from the tallied list.

### A-L3 — AudioUIBridge record mode drops state silently and dedups in O(n) per command
`Source/Audio/ThreadMessaging/AudioUIBridge.cpp:18-110`, `AudioUIBridge.h:78-85` (capacities 256/512/256)
In `Record` mode, `Show` highlights / arrows / counts past capacity are dropped without a flag, so a long replay can deliver an incomplete final canvas state. Each `send()` also linearly scans the recorded arrays to dedup.
Fix: when a recorded array is full, set a `recordOverflowed` flag and have `deliverRecording()` fall back to a full resync.

### A-L4 — Fragile coupling around silence / reset / suspend
`Source/Audio/TraversalSession.cpp:60-100`, `Source/Audio/PlaybackEngine.cpp:20-60`
`silenceAllNotes()` does not clear `flagScheduler.pendingStarts` itself; every current caller happens to clear them via `clearTraversals()` / `restartActiveTraversals()`. `resetRequested` during `Playback::Replaying` is swallowed at `PlaybackEngine.cpp:24-28` — a reset pressed during a host relocation does nothing visible. `pendingNoteOffs` (`PlaybackEngine.h:69`) is written in `releaseNotes()` and read in `processBlock()` with no synchronisation — host thread for `releaseResources()` **UNVERIFIED**.
Fix: make `silenceAllNotes()` clear pending flag starts itself; handle reset-during-replay explicitly; document or synchronise the `releaseNotes()` / `processBlock()` hand-off.

---

## High — Script language

### S-H1 — `and` / `or` are not short-circuited; side-effecting RHS always runs
`Source/Script/ScriptEmitter.cpp:1026-1090` (`emitBinary`), `:1129-1130`
`emitBinary` emits the left operand, then the right operand, then a single `LogicalAnd` / `LogicalOr` opcode. There is no jump. `ScriptRun.cpp` just combines two already-computed values.
Consequence: C-like users expect `false and advance(1)` / `true or playNote(...)` to skip the RHS. Here `advance()`, `playNote()` and any mutating helper on the RHS execute unconditionally. `node.pitch` on `none` is safe (returns 0), so the common `child != none and child.pitch > 60` case does not crash — but walk movement and note playing are silently wrong.
Fix: in `emitBinary`, for `And` emit left, `JumpIfFalse` to a patched end (leaving `0` on the stack, or popping left first), then right; mirror for `Or` with `JumpIfTrue`. Adjust `stackDelta` / high-water accounting for the jump path, and add tests: `false and advance(1)` must not move, `true or advance(1)` must not move.

### S-H2 — Trial node-state writes are silently dropped after 64 distinct writes
`Source/Audio/Traversal/ScriptRun.h:142,163` (`trialWriteCapacity = 64`); `Source/Audio/Traversal/ScriptRun.cpp:561-582` (`writeState`)
In `ScriptWrites::Trial` mode, a write to a new `(slot, nodeId)` pair is stored only `if (trialWriteCount < trialWriteCapacity)`; otherwise the `if` simply falls through and the write vanishes, with no fault and no diagnostic.
Consequence: trial runs are how `peekNextTarget` / modulator decisions preview a user `advance()` (`TraversalLogic.cpp:329,402`). A script that touches >64 distinct node counts in one preview previews a different state than the committed run will produce, so the peeked target can be wrong — silently.
Fix: either grow the table, or return `Progress::Faulted` from `writeState` / `executeNodeOpcode` when full so the trial call returns `-1`, or make `writeState` return `bool` and fault the caller. Add a test with >64 distinct trial writes.

### S-M1 — `advance(n)` is silently capped at 64 steps; doc says it moves `n` nodes
`Source/Audio/Traversal/TraversalLogic.h:105` (`maxAdvanceSteps = 64`); `TraversalLogic.cpp:567` (`std::min(steps, maxAdvanceSteps)`), `:596`
`advance(100)` reaches user `advance` as `64`, and the built-in walk likewise skips at most 64. `Dispatcher::advance` (`TraversalDispatcher.cpp:20-35`) also treats `steps < 1` as "stay on current node" — `advance(0)` / `advance(-5)` are undocumented no-ops returning `current`.
Fix: document the 64 cap and the `<=0` behaviour in `ScriptingLanguage.md`, or emit a compile-time diagnostic for a literal `advance(n)` with `n > 64` / `n < 0`.

### S-M2 — The `advance` / `playNote` ban inside `advance` is bypassed by a helper
`Source/Script/ScriptEmitter.cpp:931,963` (`insideAdvance = currentFunction->name == "advance"`)
Only the function literally named `advance` is checked. `advance` → helper `foo()` → `advance(1)` compiles, because `currentFunction` is `foo` when `foo` is emitted.
Bounded impact: this does *not* corrupt the walk — user `advance` is run with `host == nullptr` (`TraversalLogic.cpp:569`), so the nested `Advance` opcode returns `-1` and `PlayNote` is a no-op. The failure is diagnostic, not safety: the script compiles but the call silently yields `none` / nothing instead of the documented compile error.
Fix: build the call graph after `declareFunctions` and reject `advance` / `playNote` in any function reachable from `advance`.

### S-M3 — Same-scope local redeclaration is silently accepted
`Source/Script/ScriptEmitter.cpp:223-256` (`emitDeclare`); `:785-789` (`declareLocal` just pushes a new binding); contrast `:135` where duplicate *parameters* are diagnosed.
`int x = 1; int x = 2;` in one block allocates two slots; `findLocal` (`:667`) searches backwards, so the second silently shadows the first and the first slot is leaked toward the 32-local cap.
Fix: in `emitDeclare`, check `findLocal` for a binding declared in the *current* scope (compare against `scopeMarks.back()`) and report "'x' is already declared", mirroring the parameter check. Nested-block shadowing must stay legal.

### S-M4 — Member/global initialisers are order-dependent and contradict the doc
`Source/Script/ScriptEmitter.cpp:689-750` (`emitInitializer`); `Source/Audio/Traversal/TraversalLogic.cpp:278-320` (`makeScriptContext` runs `globalInitFunction` before `memberInitFunction`)
The doc (`ScriptingLanguage.md`, "Members") says "you cannot give a member a starting value in its declaration" — but the emitter and tests (`ScriptCompilerTests.cpp`, "members and variables outside the class start at any value") *do* allow `int steps = base + 1;`. Worse, `int a = b; int b = 5;` compiles and gives `a == 0`, because `b` is still at its default when `a` is initialised; a global initialiser reading a member likewise reads the pre-init default.
Fix: fix the doc first (initialisers *are* allowed), then either reject an initialiser that references a variable declared later / a member from a global initialiser, or specify and test declaration-order semantics explicitly. See also UI-H5 — the two docs contradict each other on this point.

### S-M5 — `node.eligible` is always `1`, for any node, eligible or not
`Source/Audio/Traversal/ScriptRun.cpp:448` (`case ScriptField::Eligible: return 1;`)
`readField` never consults `context.isEligible` / `RuleContext::eligibleChild` (`TraversalRule.h:46`). `current.eligible`, or `.eligible` on a node whose count/trigger limit is exhausted, returns `1`.
Fix: compute eligibility in `readField` via the context predicate, or remove `eligible` from `nodeFieldTable` and document that `for`-loop children are already filtered. At minimum, document that `.eligible` is `1` for every real node.

### S-M6 — Decimal literal overflow silently becomes `inf`; integer overflow is diagnosed
`Source/Script/ScriptLexer.cpp:112,126` (`overflowed = false;` on seeing `.`, then `std::strtod(...)` with no `errno` / range check)
`99999999999.5` clears the integer-overflow flag, and a decimal too large for `double` yields `HUGE_VAL` / `inf` with no diagnostic — while `99999999999` (integer) *is* reported as "number is too large".
Fix: after `strtod`, check `errno == ERANGE` or `std::isinf(decimalValue)` and report "number is too large".

### S-M7 — `INT_MIN` cannot be written: `-2147483648` is rejected
`Source/Script/ScriptLexer.cpp:100-118`; `Source/Script/ScriptParser.cpp:793-822` (`parseUnary` folds `-literal` by negating the already-clamped value)
The only way to spell `INT_MIN` is `-2147483648`, which the lexer rejects before the parser can fold the minus (literal `2147483648` > `INT_MAX` → diagnostic, value clamped to `2147483647`).
Fix: in `parseUnary`, when the operand is a literal whose text is `2147483648`, allow the folded value `-2147483648`; or teach the lexer to accept `2147483648` when it is the operand of unary minus.

### S-M8 — Step budget does not bound host work per step
`Source/Audio/Traversal/ScriptRun.cpp:27-58`; `TraversalLogic.cpp:329,402,564`
A `while` loop calling `advance(1)` 8,192 times does 8,192 full traversal steps in one audio callback, and each note additionally re-runs user `advance` outside `main`'s budget for peeking/modulation. The doc's "each run of a function may take at most 8,192 instructions" is also inaccurate: nested calls share the *top-level* `call()`'s budget, they do not each get 8,192.
Fix: document the real bound (per top-level `call`, plus peek/modulator re-runs), and consider counting host `Advance` / `PlayNote` invocations against the budget or capping `advance()` calls per `main` run. See A-H1.

### S-M9 — `playNote` called twice: the last call silently wins
`Source/Audio/TraversalDispatcher.cpp:81-87` (`playNote` overwrites `mainRun.note`); `:368-380` (`stepTraversal`)
Nothing diagnoses or documents multiple `playNote` calls in one `main` run; earlier notes are discarded, not played as a chord and not an error.
Fix: document "last `playNote` wins, at most one note per step" in `ScriptingLanguage.md`, or add a runtime/compile diagnostic for a second call in the same `main`.

### S-L1 — Parser aborts early on some errors, breaking "every error is reported at once"
`Source/Script/ScriptParser.cpp:34-60` (`parseImports`: unknown library returns failure immediately); `Parser::run` returns the declaration at once on import/class-header failure.
Fix: record the diagnostic, skip to a recovery point, and continue parsing instead of returning early.

### S-L2 — Missing `}` produces two diagnostics for one mistake
`Source/Script/ScriptParser.cpp:432` (`parseBlock`); `:155-175` (`parseClassBody`)
Fix: propagate the `parseBlock` failure without re-adding the class-level diagnostic when the current token is already `End`.

### S-L3 — No cap on total compiled instructions / script size
`Source/Script/ScriptEmitter.cpp:114-165`
A very large script compiles to an unbounded `instructions` / `constants` vector (recompiled 250 ms after typing, per the doc). Only per-call *execution* is capped.
Fix: add an `RTScript::maxInstructions` (e.g. 64k) and reject in `emit()` once exceeded.

### S-L4 — Stack high-water over-counts `Call` by 1; temp slots count toward the 32-local limit
`Source/Script/ScriptEmitter.cpp:1154-1185` (`stackDelta(Call) == +1`); `emitCall` (`:920`); `emitFor` (`:444`), `emitAssignField` (`:344`)
Both errors are in the safe (conservative) direction, but a borderline script — e.g. 32 locals plus one `for` loop, or a 64-deep call nest — can be spuriously rejected as "too deep / too many variables" even though the VM (`stackCapacity = 64*8`) would fit it.
Fix: make `Call`'s delta `1 - parameterCount` atomically, and track temporary slots separately from `highWaterSlot`.

### S-L5 — Class header, base class and trailing `;` are all optional, contrary to the doc
`Source/Script/ScriptParser.cpp:115-145` (`parseClassHeader`); `Parser::run`
`class Foo { void main() {} }` (no base, any name, no `};`) compiles, while the doc presents `class Traversal : defaultTraversal { … };` as *the* structure.
Fix: either enforce the documented form or update the doc to say the base class and trailing semicolon are optional and the class name is free.

### S-L6 — `bool` is a full type in code/tests but absent from the doc; `%` accepts `bool`
`ScriptingLanguage.md` Types table lists only `int/float/double/Node`; `ScriptLexer.h` / `ScriptEmitter.cpp: commonType` treat `Bool` as a number, so `true % 2` compiles.
Fix: add `bool` / `true` / `false` to the doc's Types and Operators sections, and either reject `%` on `bool` in `emitBinary` or document it.

### S-L7 — Decimal syntax gaps: `1.` and exponents are rejected/confusing
`Source/Script/ScriptLexer.cpp:108-118`
`1.` lexes as `1` + `.`, then fails in the parser as a field access; `1e10` lexes as `1` + identifier `e10` and fails at the following `;` check. Neither is documented.
Fix: support `1.` and `1e10` / `1.5e-3` in `readNumber`, or state in the doc that only `digits.digits` is a decimal.

### S-L8 — Built-in names can be shadowed inconsistently
`Source/Script/ScriptEmitter.cpp:22-50`; `emitName` / `emitField`
`int current = 1;` as a *local* is allowed and shadows the walk node; a member named `traversal` makes `traversal.random` a (failing) node-field lookup instead of the context value. Behaviour depends on which scope the name is in.
Fix: reject `current`, `traversal`, `children` as declared names in all scopes, or document the shadowing rules.

### S-L9 — VM silently accepts out-of-range opcode operands (defence-in-depth only)
`Source/Audio/Traversal/ScriptRun.cpp: readContext`, `readField`, `convert`, `pushValue` / `popValue`
A hand-built/corrupt `RTScript` with an invalid field/context/convert operand silently yields `0` instead of faulting.
Fix: validate the operand range in `pushValue` / `readField` / `readContext` / `convert` and return `Faulted` on an unknown value; assert `frameCount > 0` in `popValue` / `pushValue`.

### S-L10 — Minor parser leniencies (document or tighten)
`Source/Script/ScriptParser.cpp:669-700` (`parseFor` uses `parsePostfix`, so `for child in (children)` is accepted); `parseBlock` skips stray `;`, so empty statements are accepted.
Fix: document both, or parse the iterable with the same strictness as the doc states.

---

## High — Graph / traversal / state / serialization

### G-H1 — `NodeStateTable::ref()` out-of-range sink aliases across nodes
`Source/Audio/Traversal/NodeStateTable.cpp:80-101`; `NodeStateTable.h:118`
On unprepared table, negative `nodeId`, or full table (`rows.claim()` returns -1), `ref()` returns `outOfRangeSinks[slot]` — one shared `std::array<int,slotCount>` for **all** nodes, reset to default first. `ref(Count, badA)` and `ref(Count, badB)` are the same `int`; a second `ref()` for the same slot silently resets a reference a caller still holds; `get()` (uses `find()`, never the sink) still returns default, so increments via `ref()` are lost. Once the table is full (>1024 nodes) or an id is invalid, per-node counts stop being per-node.
Fix: don't return a shared mutable sink — return `std::optional<std::reference_wrapper<int>>`, fail loudly, or key the sink by `(slot,nodeId)`; make `get()` / `ref()` agree.

### G-H2 — `NodeStateTable` has no slot validation — `Total` / invalid slot corrupts another row or goes out of bounds
`Source/Audio/Traversal/NodeStateTable.h:30-44,92-95`; `NodeStateTable.cpp:48-118`
`increment/get/ref/set` accept any `NodeStateSlot` unchecked; `indexOf(slot,row)=row*slotCount+slot`, `slotCount=11` (`Total`). Passing `Total` writes row `r`'s slot 11 = row `r+1`'s `Count` (for `r=1023`, one past `values`); a cast value like `99` indexes far out of `values` and out of `outOfRangeSinks[slot]` — UB/crash. Trigger **UNVERIFIED**: no in-scope caller passes `Total` / invalid.
Fix: `assert(slot>=Count && slot<Total)` in debug *and* a release guard, plus a `default:` / `unreachable` in `defaultNodeStateSlotValue()`.

### G-H3 — `TraversalState::unusedInstance()` returns an instance already in use when all are taken
`Source/Graph/TraversalState.cpp:81-101`, esp. `:96-100`; `RTData.h:52` (`maxInstances = 26`)
`while (instance < TraversalKey::maxInstances - 1 && usedInstances.count(instance)>0) ++instance; return instance;` — the loop never tests the last instance. If instances 0..25 are all used it returns 25, which is used.
Fix: `while (instance < TraversalKey::maxInstances && used.count(instance)>0) ++instance;` and return a documented sentinel (`maxInstances` or `-1`) meaning "none free"; callers must check it.

### G-H4 — `TraversalRuleState::replaceState()` leaves a stale/dangling `activeRuleId` across loads
`Source/Graph/TraversalRuleState.cpp:238-282`, esp. `:255-259`
`activeRuleId` is only overwritten `if (!restoredActiveRuleId.isUsingDefault())`. If the restored tree has no `ActiveRuleId` property, the previous (pre-load) id is kept — it may name a rule that no longer exists, or a *different* restored rule with the same numeric id. Even when present, it is copied without checking a rule with that id is in the restored set.
Fix: always reset in `replaceState()`: set `activeRuleId` to the restored id **only if** a rule with that id is in the restored set, else the first restored rule's id, else `noActiveRuleId`; call `ensureDefaultRule()` when the set is non-empty.

### G-H5 — `StateJson`: one corrupt child discards the whole document, with no error distinction
`Source/Graph/StateJson.cpp:60-68` (`toElement`), `:3-18` (`read`)
If any recursive `toElement(children[i])` returns `nullptr`, the parent returns `nullptr`; `read()` returns `nullptr` for invalid JSON, wrong top-level type, *and* any bad descendant alike.
Fix: return a structured result (`std::expected` / error enum: `NotJson`, `LegacyBinary`, `CorruptChild(path)`) instead of bare `nullptr`; or skip-and-log the bad subtree and preserve the rest. Add tests: bad child, bad `properties`, bad top-level type. See also S-2 (unbounded recursion, no depth/size cap) and G-M21.

### G-H6 — `Clipboard::copy()` does not expand a single encapsulated member, contradicting its own header
`Source/Graph/Clipboard.cpp:354-372` (`withEncapsulatedMembers`), `:10-12`, `:413-451`; `Clipboard.h:18-19`; `EncapsulationOps.cpp:131-143`
`withEncapsulatedMembers` calls `encapsulation.memberIds(nodeId)` per selected id, but `memberIds` only returns members when its argument *is* the encapsulator; for a regular member it returns empty. `encapsulatorsCovering` then requires *all* members in the covered set, so the group is also not copied.
Fix: in `withEncapsulatedMembers`, resolve the member's group first (`NodeProperties(node).encapsulatorId`, then `memberIds(encapsulatorId)`) and expand to all members. Test: copy one member of a 3-member group, paste, expect all 3 + recreated group.

### G-H7 — `EncapsulationOps::create()` has only debug assertions; in release, empty/invalid/duplicate input mutates live state
`Source/Graph/EncapsulationOps.cpp:9-49`, esp. `:11-19,28-38`
`jassert(!memberNodeIds.empty())` and `jassert(firstMember.isValid())` are debug-only. In release, an empty span calls `memberNodeIds.front()` (UB); an invalid member gets its `encapsulatorId` set while the encapsulator may never be added to `nodeMap`. No upfront validation, duplicate check, or rollback.
Fix: validate first — return an invalid `ValueTree` if the span is empty, any member is invalid, or any id repeats — before mutating anything.

### G-H8 — `create()` / `insertNodeAfter()` don't remove a node from its previous group
`Source/Graph/EncapsulationOps.cpp:28-38` (`create`), `:83-114` (`insertNodeAfter`); `EncapsulationOps.h:9-13`
Both set `memberProperties.encapsulatorId` to the new group without removing the node from its old group's `EncapsulatedIds` list. The header promises "a group's list and its members' `EncapsulatorId` always agree". `dissolve(oldGroup)` then resets that member's `encapsulatorId`, silently un-grouping it from its *new* group.
Fix: at the start of `create()` / `insertNodeAfter()`, if the member already has an `encapsulatorId` pointing at a different live group, call `removeMember(memberId, oldGroupId, undoManager)` first (or reject).

### G-H9 — WeightedPick: negative `probability` corrupts the draw and can suppress a positive-weight child
`Source/Audio/Traversal/WeightedPick.cpp:4-13` (`tally`), `:15-30` (`draw`), `:32-41` (`accepts`); `TraversalRule.cpp:140,158`
No clamping/validation anywhere. Max-limit children with probabilities `-50, 40` → `totalWeight=-10` → `draw` returns `false` → `selectChild` returns `-1`, even though a 40-weight child exists. `100, -20, 20` → `totalWeight=100`, `runningWeight` after child 1 is already `100`, so `pick<100` is always true and the first child absorbs 100% — later children unreachable.
Fix: in `tally` / `accepts` use `std::max(0, probability)`, or reject `probability<0` in `RuleContext::eligibleChild`. Unit test with mixed negative/positive weights. Whether the UI/builder can produce a negative probability was **UNVERIFIED**.

### G-H10 — `advanceCrossTreeCounts`: same child double-counted and pushed twice per step; switch-phase child pushed on every step
`Source/Audio/Traversal/TraversalLogic.cpp:174-257`, esp. `:222-248`, `:252-256`
No visited/dedup set; two `isCrossRoot` connections from one host to the same root, or the same root reachable from both host and its alternative, advance `CrossTree` by 2 and push the id twice in one call. In the `switchCount>0` branch the id is pushed on *every* call until the switch limit is hit.
Fix: track a local visited set of child ids already processed in this call and skip repeats; push a child at most once per call.

### G-H11 — `advanceByScript` bypasses every eligibility gate and all count/trigger/sub-loop state
`Source/Audio/Traversal/TraversalLogic.cpp:564-592`, esp. `:571-591`
`chosenId=run.call(advanceFunction,…)`; only null/self checks, then `primary.target=chosenId; advanceAlternative(…)`. No `eligibleChild`, `chooseChild`, `registerTrigger`, `armSubLoop`, no `Count` increment, no `skipAhead`, no `primary.target==primary.last` check.
Consequence: a script can move to *any* node — not connected, `countLimit<=0`, trigger limit exhausted, `duration==0`, disabled for this traversal, wrong kind — all gates enforced by `TraversalRule.cpp:47-96` for the native path. `Count` is frozen in script mode, so scripts reading `Count` see a stale value and trigger/sub-loop state never advances. Whether scripts are intentionally trusted to jump anywhere was **UNVERIFIED**.
Fix: after null/self checks, require `eligibleChild(chosenId)!=nullptr` (or at least a real connection + trigger/count checks) unless the script path is explicitly documented as unconstrained; then do the same bookkeeping as `advance()`: increment `Count`, `registerTrigger`, `armSubLoop`, clear/set `SwitchCandidate`.

### G-H12 — `chooseChild`: `SwitchCandidate` / `LastNode` stored before encapsulation diversion; switch-repeat path bypasses trigger, encapsulation and `Count`
`Source/Audio/Traversal/ChildSelector.cpp:101-133`, esp. `:107-118`, `:124-131`; `:249-269`
`selectSwitchNode(…); if (chosen!=-1) return chosen; count=increment(Count,targetId); chosen=selectNextChild(…); set(SwitchCandidate,targetId,chosen); set(LastNode,targetId,chosen);` *then* `encapsulationLoopTarget` may divert `chosen` to the entry. Next step, `selectSwitchNode` returns that bypassed child directly, so the switch repeat plays the wrong node and re-enters without sub-loop accounting. The early return also skips the `Count` increment and `registerTrigger`.
Fix: apply the encapsulation diversion *before* storing `SwitchCandidate` / `LastNode`, and store the actually-entered id. In the `selectSwitchNode` early-return path, call `registerTrigger`.

### G-H13 — All creation/entry points validate with `jassert` only — release builds create orphan nodes with `rootNodeId == 0`
`Source/Graph/GraphState.cpp:55-56`, `:91-93`, `:118-120`, `:253`/`:285`/`:312`/`:360`/`:406`; `NodeFactory.cpp:103-138` (`create`)
Invalid parent / wrong kind / non-note parent are all `jassert`-only. In release, `addChildNode` proceeds with `rootId = parentProperties.rootNodeId` (= default `0` for an invalid tree), creates the node, and indexes it. `NodeFactory::create` maps an invalid parent via `kindOf` (see G-H14) to `Kind::Node` and proceeds. The result is a node with `rootNodeId == 0` that `RTGraphBuilder::makeRTGraph` silently drops, so it is never published and never cleaned up.
Fix: replace `jassert`s on public entry points with real guards returning `{}` / `false`, and validate `rootNodeId != 0` before indexing.

### G-H14 — `kindOf` silently maps invalid/unknown types to `Kind::Node`
`Source/Graph/GraphState.cpp:532-565`
Every unrecognised `Identifier` — including an invalid `ValueTree` — falls through to `return NodeInfo::Kind::Node`.
Fix: add an explicit invalid/unknown result (or return `std::optional<Kind>`), and check it at call sites.

### G-H15 — Duplicate ids silently overwrite the index; removing one duplicate unindexes the other
`Source/Graph/GraphState.cpp:598-622` (`indexNode` / `unindexNode`)
`indexNode` does `nodeIndex[nodeId] = node;` with no duplicate check. `unindexNode` does `nodeIndex.erase(nodeId);` unconditionally. With two nodes sharing an id, `getNode` returns only the last-indexed one; erasing one removes the index entry for the still-present other node.
Fix: `jassert` / log and reject on duplicate id in `indexNode`; make `unindexNode` erase only if the stored tree is the same tree being removed.

### G-H16 — Traversal-flag children are never built as RT nodes, yet `flagTargetId` points at them
`Source/Graph/RTGraphBuilder.cpp:488-499`, `:648-670` (`fillTraversalFlag`)
Children of a `TraversalFlag` node are not pushed onto the traversal stack, so they are never added to `builtNodes` — unless also reachable by another path. `fillTraversalFlag` still sets `flagTargetId` to the first child's id, or to `parentId` if there are no children.
Fix: either push flag children in `createRTNodes` (and decide whether they are playable nodes or only targets), or document that flag targets must also be reachable normally and validate that at `connectNodes` time. Audio-side handling of a missing target is **UNVERIFIED**.

### G-H17 — `rebuildAllGraphs` never rebuilds modulator-root graphs
`Source/Graph/RTGraphBuilder.cpp:33-56`
Only calls `makeRTGraph` for children of type `RootNodeData`. Modulator roots (`ModulatorRootData`, whose `rootNodeId` is themselves) are in `builtGraphIds` and are built by the incremental path, but are skipped here. `rebuildAllGraphs` is the path taken when ornaments change (`handleAsyncUpdate`, `:206-257`). After an ornament change, every modulator-root graph keeps stale ornament steps / node data until some other edit rebuilds it.
Fix: rebuild every id in `builtGraphIds` whose node is valid (or every node whose `rootNodeId == its own id`), not just `RootNodeData`.

### G-H18 — Traversal changes only published for roots/flags; deleted traversals silently replaced by defaults
`Source/Graph/RTGraphBuilder.cpp:259-289` (`buildRTtraversal`); `:350-385` (`rebuildGraphsForTraversal`); `:206-257`
`buildRTtraversal` looks up the traversal by `key.typeId`; if not found it returns an `RTtraversal` with default tempo `1`, channel `1`, transpose `0`, velocity `1.0` and the *requested* key — no error, no skip. `rebuildGraphsForTraversal` only collects roots whose own `TraversalChildrenIds` contains the id, or flags with `abs(flagValue) == traversalId`; it never looks at non-root nodes' `TraversalChildrenIds`, even though `createRTNodes` reads traversals from *every* node.
Fix: in `rebuildGraphsForTraversal`, scan *all* nodes' `TraversalChildrenIds` (or maintain a traversal→roots index). In `buildRTtraversal`, return `std::optional<RTtraversal>` and skip missing traversals, or log the missing id.

### G-H19 — `pasteAt()` performs no validation of clipboard contents; duplicate/missing IDs can create duplicate node IDs
`Source/Graph/Clipboard.h:45` (`contents` is `public`); `Clipboard.cpp:65-83,513-526,85-115`
`contents` is a public `ValueTree` anyone can mutate; `pasteAt()` trusts it. `allocatePastedIds()` keys `idMap` by the source `Id` property; a missing `Id` reads as `0`, so several nodes collapse to one `idMap[0]` entry, and `insertClipboardNodes()` then assigns them all the same (last) new ID. Duplicate source IDs do the same.
Fix: validate in `buildPasteLayout()` / `pasteAt()`: every clipboard node must have a distinct, positive `Id`; abort (return `{}`) on violation. Make `contents` private with a validated setter, or at least document it as untrusted input.

### G-H20 — `StateJson`: no schema version, no newer-version rejection, no migration in this layer
`Source/Graph/StateJson.h:27-39`; `StateJson.cpp:3-18,42-71`
`read()` accepts any JSON object with a non-empty `"type"`; `toVar()` / `toElement()` copy tag/properties/children verbatim. No version key, `SchemaVersion` check, field rename/migration, or unknown-tag rejection. A v2 file loads silently as if v1.
Fix: keep `StateJson` generic but add a version contract at the load boundary — in `PluginDocument`, read `PluginState/SchemaVersion`, default missing to `0`, reject `> schemaVersion` with an error (not silent `nullptr`), run a per-version migration before `ValueTree::fromXml`. Add a test loading `savedBeforeSchemaVersion.sqtree` through that boundary, not just through `StateJson::read`.

### G-M1 — `ModulatorWalk::activate()` unconditionally discards a live modulator walk (suspected)
`Source/Audio/Traversal/ModulatorWalk.cpp:6-14,82-88,144-202`
`activate()` resets `gate.repeatCount=0`, `walker={.target=rootId}`, `decidedTarget=-1`, `decidedRestart=false`. `followMainWalk()` calls it whenever `findActiveModulatorRoot()` returns a root, with no check that a *different* modulator is already active. Whether synced re-trigger is meant to restart or continue is **UNVERIFIED**.
Fix: in `followMainWalk()`, if `isActive()` and the triggered root/host equal the active gate, don't call `activate()`; if they differ, explicitly `deactivate()` the old walk first and/or log the pre-emption.

### G-M2 — `ModulatorWalk::decide()` — script choice bypasses modulator eligibility and skips sub-loop arming; invalid script id silently becomes a restart
`Source/Audio/Traversal/ModulatorWalk.cpp:24-67`
With `scriptChoice`, the only check is `nodes.find(*scriptChoice)!=nullptr`. It bypasses `chooseChild(..., &isModulatorChild)`, so no check the node is a modulator child/descendant, eligible, or within count limits. If the id doesn't exist, `chosen` stays `-1` and execution falls into the restart branch with `decidedRestart=true, decidedTarget=gate.activeRootId` — indistinguishable from "no child available". On the chosen path, `armSubLoop()` is called only for non-script choices; on the restart path it *is* called.
Fix: validate a script choice the same way as a chosen child; on an invalid id, return without setting `decidedRestart` / target; call `armSubLoop()` for script choices too.

### G-M3 — `handleTreeJump` in Live mode discards the jumped-to tree's loop limit (suspected)
`Source/Audio/Traversal/TraversalLogic.cpp:516-553`, esp. `:547-552`
`loop.active=true; loop.count=0; if (mode==Live) { loop.limit=0; }` — `0` means "never ends". The jump path never reads `jumpTargetNode->graphLoopLimit`, though `begin()` sets `loop.limit=graphLoopLimit`. Preview keeps the old limit (asymmetry). Intent **UNVERIFIED**.
Fix: `loop.limit = jumpTargetNode->graphLoopLimit;` (both modes, or at least Live), or document infinite-after-jump as intended.

### G-M4 — `advanceAlternative` host case uses the un-incremented count (suspected off-by-one)
`Source/Audio/Traversal/ChildSelector.cpp:3-82`, esp. `:61-67`; contrast `TraversalLogic.cpp:324` (`get(Count)+1`), `ChildSelector.cpp:182` (`get(Count)+1`), `ChildSelector.cpp:113` (increment before `selectNextChild`)
With `count==0` on first entry, `0 % child->countLimit == 0` for *every* limit, so the first alternative pick is eligible for all limits and the max-limit rule just picks the highest-limit child — a limit-2 alternative is wrongly eligible on visit 1, while the primary path (count incremented to 1 first) would not pick it. Intended 0-based vs 1-based semantics **UNVERIFIED**.
Fix: use `get(countSlot,parentId)+1` in the host branch, or document 0-based alternative counting and adjust callers consistently.

### G-M5 — `begin()` does not validate the start node, does not arm a sub-loop for it, and does not clear stale walk state on re-entry
`Source/Audio/Traversal/TraversalLogic.cpp:28-81`
No `nodes.find(startNodeId)` check, no `armSubLoop`, no clearing of `primary.last/subRootNode/alternativeTarget`, `nodeState`, `pendingJumpTargetId`, or `referenceTargetId` (only cleared in `reset`). Invalid `startNodeId` leaves `state=Active` with a dangling target; the next `advance()` goes to `Reset`, and if `graphLoopLimit==0` (infinite) it cycles Reset forever, never playing a node. Calling `begin()` twice without `reset()` leaks the previous walk's `subRootNode`.
Fix: in `begin()`, return early / set `state=End` if `nodes.find(startNodeId)==nullptr`; call `armSubLoop(primary, *startNode, mode)`; either document "`reset()` must precede `begin()`" or clear the walk/node state inside `begin()`.

### G-M6 — RNG is advanced once per *step*, so every weighted pick in one step shares the same draw
`Source/Audio/Traversal/TraversalLogic.cpp:425-433` (`stepActive`); `ChildSelector.cpp:168,187`
`advance(nodes,steps); selectionRandom ^= …;` — the tree-jump check, primary choice, alternative choice and every `skipAhead` peek all use the identical `randomValue`. If two of those picks have the same `selectionSpan`, they pick the same rank — perfectly correlated, not independent draws.
Fix: advance the xorshift (or derive a fresh draw) inside `selectNextChild` / each `WeightedPick::draw` call, or pass a per-pick salt.

### G-M7 — `skipAhead` moves the walk without any of `chooseChild`'s bookkeeping, and peeks with a different predicate
`Source/Audio/Traversal/TraversalLogic.cpp:594-609`; `:322-390` (`peekNextTarget`); `ChildSelector.cpp:101-133`
No `Count` increment, no `registerTrigger`, no `selectSwitchNode`, no `armSubLoop`, no `SubRootCount` increment. `peekNextTarget` selects with `isAudibleChild` while `advance` / `chooseChild` use `isAdvanceableChild` — a skipped-to node may be one the normal walk could never choose, and vice versa.
Fix: either route skipped steps through `chooseChild` (or a shared "enter node" helper), or document that skipped nodes intentionally don't count and make `peekNextTarget` use the same predicate as the path it serves.

### G-M8 — `peekNextTarget` (supposedly side-effect-free) can run script init functions with committed writes
`Source/Audio/Traversal/TraversalLogic.cpp:322-340` and `:278-320` (`makeScriptContext`)
`peekNextTarget` builds a `Trial` context and calls the advance function. But `makeScriptContext:299-317` runs `globalInitFunction` / `memberInitFunction` with `ScriptWrites::Commit`, even when the caller asked for `Trial`.
Fix: in `makeScriptContext`, run the init functions with `Trial` writes when the requested `writes==Trial`, or defer initialisation out of `peekNextTarget`.

### G-M9 — `TraversalPool`: duplicate `runId` acquire, `prepare()` clearing live traversals, `Fixed` RNG is the absorbing `0` state
`Source/Audio/Traversal/TraversalPool.h:53-69` (`prepare`), `:103-132` (`acquire`), `:118-120` (`Fixed`)
`prepare`: calling with the same capacity mid-playback still calls `nodeState.prepare()` on every slot, wiping counts of active traversals, without bumping `epoch`. `acquire` scans for the first inactive slot, no check that `runId` is already active — re-acquire creates two live slots with the same id. `Fixed`: `selectionRandom=0`; xorshift on `0` stays `0` forever, so `Fixed` traversals always draw `pick=0` — the first max-limit child in connection order, every time.
Fix: in `acquire`, return `nullptr` (or the existing instance) if `findSlotIndex(runId)!=-1`. In `prepare`, skip `nodeState.prepare()` for active slots (or bump `epoch` and document that prepare resets live state), and reject `capacity<0`. For `Fixed`, use a fixed non-zero seed (e.g. `1`).

### G-M10 — `updateDurationMaps` publishes a separate snapshot before the queued graph rebuilds
`Source/Graph/RTGraphBuilder.cpp:145-204`; `:206-257` (`handleAsyncUpdate`)
`handleAsyncUpdate` first queues graph rebuilds, then calls `updateDurationMaps(...)`, which does `snapshots.publish(...)` immediately on the *old* published `globalNodes`, and only afterwards calls `publishQueuedGraphs()`. Two publishes per async update; the first publish is wasted work when the same nodes are about to be rebuilt anyway.
Fix: apply duration refreshes into the *queued* rebuilt graphs when they exist, and publish once at the end of `handleAsyncUpdate`.

### G-M11 — Shared / DAG children get an arbitrary single parent in the RT graph
`Source/Graph/GraphState.cpp:142-175` (`getNodeParent` — BFS, returns the first non-`TraversalFlag` parent); `RTGraphBuilder.cpp:462-484`
`getNodeParent` returns the first parent found in `parentIdsOf` order. `createRTNodes` sets `rtNode.parentId` only if that parent is already in `builtNodes` (DFS order, stack LIFO, so child order is reversed and first-visit wins); otherwise `parentId` stays `0`.
Fix: decide whether shared children are legal. If yes, make `parentId` deterministic (e.g. lowest parent id) and document it; if no, reject the second parent in `connectNodes`.

### G-M12 — `rememberOwners` can silently drop a rebuild
`Source/Graph/RTGraphBuilder.cpp:706-730`
It walks up via `getParent()` while `rootNodeId.isUsingDefault()`. If the walk reaches the `nodeMap` root (or an invalid tree) without finding a `rootNodeId`, `graphOwner` becomes invalid and nothing is inserted into `pending.rebuildNodeIds` — but `triggerAsyncUpdate()` is still called, so the change is silently swallowed.
Fix: log when the walk fails to find an owner, and fall back to `rebuildAllGraphs()` for unowned structural changes.

### G-M13 — `valueTreePropertyChanged` assumes fixed parent chains and has no handling for several audio-relevant properties
`Source/Graph/RTGraphBuilder.cpp:90-144`
Several branches do `tree.getParent().getParent().getProperty(Id)` with no validity check on either parent; a detached or restructured tree yields id `0`, which is pushed into `pending.durationRefreshNodeIds` and later skipped harmlessly — but the real change is lost. Whether `isAudioProperty` covers `traversalFlagValue`, `traversalInstance`, `encapsulatorId`, `ornamentUid`, `rootNodeId`, and `Id` itself is **UNVERIFIED**.
Fix: guard each `getParent()` chain, and add explicit branches (or a test) for `TraversalFlagValue`, `TraversalInstance`, `EncapsulatorId`, and `OrnamentUid`.

### G-M14 — `StateJson`: wrong-typed `properties` / `children` silently dropped, not rejected
`Source/Graph/StateJson.cpp:52-58`
If `properties` is not a `DynamicObject` the `if` is skipped and the element is created property-less; if `children` is not an array, `children.size()` yields no children and the subtree is silently dropped. No error branch exists.
Fix: in `toElement`, return `nullptr` (with path) when `properties` is present but not an object, or `children` is present but not an array.

### G-M15 — `Clipboard`: arrows to uncopied nodes are dropped, not converted to dangling arrows
`Source/Graph/Clipboard.cpp:117-137`, `:139-155`
For each copied parent, only children present in `idMap` are reconnected; a child outside the copy is skipped. `restoreDanglingArrows` only re-adds `DanglingArrows` children that already existed on the source.
Fix: decide explicitly — either convert a severed `NodeChildrenIds` entry into a `DanglingArrows` entry (preserving its `ArrowInfo`, tip at the child's old position), or document the drop.

### G-M16 — `Clipboard`: `getArrowInfo` / `setArrowInfo` round-trip is lossy, so pasted arrows lose `pitchOffset`
`Source/Graph/Clipboard.cpp:133`; `Source/Graph/ArrowBindingOps.cpp:111-156`
`setArrowInfo` writes only type, X/Y binding, X/Y multiplier, sync, duration override; it never writes `pitchOffset`, and `getArrowInfo` never reads it either.
Fix: add `pitchOffset` (and any other `ArrowProperties` field that belongs on a connected arrow) to both `getArrowInfo` and `setArrowInfo`, and add a round-trip test asserting all fields survive paste.

### G-M17 — `Clipboard`: whole encapsulation group dropped if any single member was discarded
`Source/Graph/Clipboard.cpp:374-411`, esp. `:398-403`
`createPastedEncapsulators` recreates a group only if *every* listed member was pasted; otherwise it `continue`s silently.
Fix: recreate the group around the pasted subset (preserving order) when at least two members survive, or surface the drop to the user.

### G-M18 — `EncapsulationOps::removeMember()` does not clear the removed node's own `encapsulatorId`
`Source/Graph/EncapsulationOps.cpp:116-129`
It removes the `NodeId` child from the group's list and dissolves the group if empty, but never resets the removed node's `encapsulatorId` property. The only in-repo caller is `GraphState::removeNode`, where the node has already been removed, so the stale property is currently harmless there — but the public API is broken for any keep-the-node caller.
Fix: `properties.encapsulatorId.resetToDefault(undoManager)` for the removed node.

### G-M19 — `ArrowBindingOps::getArrowInfo()` casts stored ints to enums with no range check
`Source/Graph/ArrowBindingOps.cpp:133-156`, esp. `:143-149`
`arrowType` / `xBinding` / `yBinding` are read as ints and `static_cast` to enums with no validation. A corrupt/old/future value becomes an out-of-range enum.
Fix: validate against the known enum ranges in `getArrowInfo()` and return default `Settings` (or a documented fallback) for out-of-range values; log/flag the arrow.

### G-M20 — `StateJson`: unbounded recursion; no depth/size limit
`Source/Graph/StateJson.cpp:31-33` (`toVar`), `:60-68` (`toElement`)
Both directions recurse with no depth cap and `read()` imposes no size cap. A deeply nested (malicious or corrupt) file can stack-overflow the load.
Fix: pass a depth counter (e.g. max 256) and fail cleanly above it; reject absurd `sizeInBytes` in `read()`.

### G-L1 — `TraversalState::collectKeys()` — `INT_MIN` flag negation is UB; output vector is not cleared; instance is not range-checked
`Source/Graph/TraversalState.cpp:42-79`
`if (flagValue<0) flagValue=-flagValue;` is UB for `INT_MIN`; the sign is also discarded, so `+x` / `-x` flags collapse to the same key. `collectKeys()` appends to the caller-supplied vector without clearing it. Instances are collected without checking `TraversalKey::maxInstances`.
Fix: use a safe absolute value, clear `keys` at the top, and filter instances to `[0,maxInstances)`.

### G-L2 — `NodeStateTable::increment()` signed-overflow if a count is set near `INT_MAX`
`Source/Audio/Traversal/NodeStateTable.cpp:48-63`
`set()` accepts any `int`; a script setting a count to `INT_MAX` makes the next `increment()` signed overflow (UB).
Fix: saturate in `increment()` (`if (v<INT_MAX) ++v;`).

### G-L3 — `RTNode.parentId == 0` is ambiguous — root, missing parent, and the phantom node all read as `0`
`Source/Graph/RTData.h:94-146`; `RTGraphBuilder.cpp:400-520`
Unlike `alternativeRootId = -1`, `encapsulationEntryId = -1`, `flagTargetId = -1`, the parent id has no "none" sentinel — `0` means "root / no parent", "parent not yet built", and "invalid/phantom node" alike.
Fix: use `-1` as the no-parent sentinel for `RTNode::parentId`, and update audio-side checks accordingly.

### G-L4 — RT note values are not validated — only the MIDI channel is clamped
`Source/Graph/RTGraphBuilder.cpp:503-515`; `RTData.h:30-37`; `GraphState.cpp:281-298` (`addMidiNote`, no range checks)
`pitch`, `velocity`, and `duration` are copied verbatim; only `midiChannel` is clamped. `RTNote::fallbackValue = 63` is defined but never used anywhere in these files.
Fix: clamp `pitch` / `velocity` to `0-127` and `duration` to `>= 0` (or use `fallbackValue` for a missing pitch) at both `addMidiNote` and RT-build time.

### G-L5 — Dead-end + `loop.limit==0` never terminates; `steps` is only upper-clamped
`Source/Audio/Traversal/TraversalLogic.cpp:98-172`, `:564-567`, `:594-597`
With `loop.limit==0` (explicit infinite, or after a Live tree jump per G-M3), a permanently dead-ended graph resets forever — each `handleNodeEvent` returns `LoopedToRoot` with no progress. This is a musical infinite loop, not a CPU spin, but there is no "dead-end while looping forever" guard. `advanceByScript` passes `std::min(steps,maxAdvanceSteps)` with no lower clamp, so `steps<=0` reaches script code as a negative argument.
Fix: track consecutive dead-end resets with no node advance and end the traversal after a small bound (e.g. 2) even when `loop.limit==0`; clamp `steps` to `>=1` at the top of `advance()` / `advanceByScript`.

---

## High — UI / JUCE / readability / docs

### U-H1 — `getStateInformation` silently overwrites the user's patch file on every host save
`Source/Plugin/PluginProcessor.cpp`, `getStateInformation()` (~L164-200)
After serialising state, if live and the graph is non-empty or a patch file exists, it calls `patchFile.replaceWithData(destData.getData(), destData.getSize())`. A host autosave / project save therefore overwrites `~/Music/SequenceTree/Patches/<name>.sqtree` with no backup, no confirmation, and no mention in `README.md`'s Patches section. A corrupted/empty in-memory state can destroy the on-disk patch. Also: `createXml()` result is dereferenced unchecked, and `replaceWithData`'s `bool` return is ignored.
Fix: only write the patch file on an explicit user Save action, or write to a temp file + atomic rename + keep one `.bak`. At minimum document the side effect. Add a null check for `createXml()`.

### U-H2 — Paint tool writes are un-undoable and fire a ValueTree write per node per drag step
`Source/UI/Canvas/ValueField.cpp:447`, `ValueField.h` (header comment admits it)
`note.setProperty(valueId, paintedValue, nullptr)` — `nullptr` undo manager — inside `applyPaintToNodes()`, called from every `mouseDrag` via `NodeController.cpp:mouseDrag`. Paint strokes cannot be undone (unlike every other graph edit), and each write fires `ValueTree` listeners / `RTGraphBuilder` rebuild machinery.
Fix: batch a stroke into one undo transaction on `endStroke()` (capture before/after values), or document prominently that paint is destructive. Coalesce writes so a node is written at most once per stroke.

### U-H3 — Paint buffers are uncapped per-pixel vectors
`Source/UI/Canvas/ValueField.cpp:ensurePaintBuffers()` (~L470-485), `ValueField.h:paintDensity`
`paintDensity[3]` + `strokeMask` are `std::vector<float>` sized `canvasWidth * canvasHeight`, re-`assign`ed whenever the canvas size changes. No cap. At 1366×768 that's ~16 MB; at 4K (3840×2160) it's ~132 MB (3 layers + mask), plus `fieldWeightedSum/TotalWeight/CoverageProd` in `render()`.
Fix: cap the density resolution (e.g. max 2048×2048, or reuse the `maximumFieldSide = 1024` cap already used for the glow image in `ValueField.h`), or store density at field resolution, not screen resolution.

### U-H4 — README says C++20, CMake says C++23 (all targets)
`README.md:9` (`C++20` badge) vs `CMakeLists.txt:34,183,198,212` (`cxx_std_23` for `SequenceTree`, `SequenceTree_Tests`, `SequenceTree_GraphTests`, `SequenceTree_RealtimeTests`)
A contributor / employer checking the badge, or a compiler without C++23 support, gets the wrong requirement. No C++23-only feature was found in the sampled code — it uses C++20 features (`std::span`, ranges, `std::erase_if`).
Fix: pick one. If C++23 is intentional, update the badge and `Architecture.md` / `Development.md`; if not, drop to `cxx_std_20`.

### U-H5 — Scripting docs directly contradict each other on member initialisers (and on `bool`)
`documentation/ScriptingLanguage.md:40` vs `documentation/Development.md:89`
User guide says "you cannot give a member a starting value in its declaration"; developer guide says "A class member … may take any expression as its starting value (`int steps = base + 1;`)" and describes `memberInitFunction` / `globalInitFunction`. Same paragraph: user guide's Types table has no `bool`; developer guide says value types are `int, float, double and bool`. The user guide also never mentions out-of-class (global/shared) variables, which `Development.md:89` describes.
Fix: decide the truth from the compiler, fix the wrong document, and add globals to the user guide. **UNVERIFIED which side is correct from docs alone** — the emitter *does* allow initialisers (see S-M4), so the user guide is the wrong one on that point.

### U-H6 — JUCE 8 claim and the whole build cannot be verified from this clone; submodules are empty
`JUCE 1b460fe…`, `Catch2 317ac1e…`, `ArsNovaUI 5bfe324…`; on disk `JUCE/`, `Catch2/`, `ArsNovaUI/` are empty. `CMakeLists.txt:16,19` does `add_subdirectory(JUCE)` / `add_subdirectory(${ARSNOVAUI_PATH})` with no existence/version check. `compile_commands.json` is a dangling symlink to `cmake-build-debug/compile_commands.json` (directory absent), so `.clangd` is also broken here.
Fix: add a guard in CMake (`if(NOT EXISTS JUCE/CMakeLists.txt) message(FATAL_ERROR …submodule…)`), and record the JUCE version in `README.md` / `Development.md`. README "Building" also omits the submodule step in the copy checked here — add `git submodule update --init` there (CI works only because `tests.yml` uses `submodules: true`).

### U-M1 — `.clang-tidy` enables only 3 checks; CI never runs it
`.clang-tidy:1-10`: `Checks: -* , readability-braces-around-statements, readability-misleading-indentation, readability-function-size` (threshold 80). No `modernize-*`, `bugprone-*`, `performance-*`, `cppcoreguidelines-*`. `.github/workflows/tests.yml` only builds/tests/pluginval — no clang-tidy or clang-format step.
Fix: add at least `modernize-use-auto, modernize-loop-convert, bugprone-*, performance-*` and a CI job running `clang-tidy` on `Source/`.

### U-M2 — `.clang-format` disables the two guards that would catch drift
`.clang-format:31` `ColumnLimit: 0` (no limit), `:43` `SortIncludes: Never`.
Evidence: `Source/UI/Bars/Titlebar.cpp:1-6` includes its own header last; `Source/Plugin/PluginEditor.cpp:1-2` includes `PluginProcessor.h` before `PluginEditor.h`.
Fix: set `ColumnLimit: 120` and `SortIncludes: CaseSensitive`, then run `clang-format` once.

### U-M3 — `NodeCanvas::makeNode` silently returns `nullptr` for an unknown tree type
`Source/UI/Canvas/NodeCanvas.cpp:435-469`
`std::unique_ptr<Node> node;` + if/else chain over 7 types, then `return node;` — no `else`, no `jassert`, no log.
Fix: add a final `else { jassertfalse; DBG("Unknown node type: " << treeType); return nullptr; }` — or return a fallback `Node`.

### U-M4 — `TraversalRulesWindow::openRule` reads properties before validating the tree
`Source/UI/Menus/TraversalRulesWindow.cpp:151-158`: `const RuleProperties properties(rule);` is constructed *before* `if (! rule.isValid()) return;`.
Fix: move the `isValid()` check above the `RuleProperties` construction. **Suspected, not confirmed as a crash** — `TypedProperties` invalid-tree behaviour was not read here.

### U-M5 — `Node` Duration display mode never rebinds its editor
`Source/UI/Node/Node.cpp:302-309` (`DisplayMode::Duration`): sets a `NumberFormat` and tooltip and `editable = false`, but — unlike Pitch/Velocity/CountLimit/Channel/Repeat/Probability (L258-300) — never calls `bindEditor()`.
Fix: bind it to the arrow-duration property (or explicitly `unbind` / clear it) in the Duration case. **Suspected** — `ValueEditor::bindEditor` on an unbound editor was not traced.

### U-M6 — `Node` constructor binds to an empty `midiNoteData`
`Source/UI/Node/Node.cpp:29`: `nodeValueEditor.bindEditor(midiNoteData, MidiPitch)` runs in the constructor, but `midiNoteData` is only assigned later in `NodeCanvas::nodeCreated()` (`NodeCanvas.cpp:120`), which then calls `bindToTree()` + `setDisplayMode()` (which rebinds).
Fix: remove the constructor bind, or assign `midiNoteData` via a constructor parameter.

### U-M7 — Editor-level shortcuts can swallow text editing (UNVERIFIED)
`Source/Plugin/PluginEditor.cpp:keyPressed()` (~L144-185): Space toggles playback, Cmd+Z/Y/C/V/A call document undo/redo/copy/paste/select-all, with the `originatingComponent` parameter ignored and no check for a focused text/code editor. `TraversalRulesWindow.h` explicitly says typing undo is the `FilePage`'s own history, yet Cmd+Z at editor level calls `document.undoManager.undo()`.
Fix: return `false` when the focused component is a `TextEditor` / `CodeEditorComponent` / `ValueEditor`, or route shortcuts through `ApplicationCommandManager`. **UNVERIFIED** — JUCE key-event delivery order was not verified here; test by typing Space/Cmd+Z in the Rules window.

### U-M8 — Silent state-load failures (debug-only `DBG`)
`Source/Plugin/PluginProcessor.cpp:213,217,222`: invalid state data, invalid state tree, and newer-schema state all `return` after only `DBG(...)` — invisible in Release, no user feedback, and a newer-schema patch is dropped while `pendingRestoreState` is left stale.
Fix: surface a message (status bar / dialog / log) and clear `pendingRestoreState` on the newer-schema path. Same pattern in `PatchControls.cpp:95-97`.

### U-M9 — Colour resolution duplicated in 4 places
`Source/UI/Canvas/NodeCanvas.cpp:117-121` and `:207-210`, `Source/UI/Node/Node.cpp:215-225`, `Source/UI/Canvas/AudioCommandDrainer.cpp:156` (`getTraversalColour`, different fallback — white).
Fix: one helper, e.g. `juce::Colour resolveNodeColour(const NodeProperties&, const Theme&)` in `TypedProperties` / `Util`, used by all four.

### U-M10 — `Node::mouseMove` / `mouseExit` are byte-identical
`Source/UI/Node/Node.cpp:191-213`
Fix: extract `updateIncrementButtonVisibility(const MouseEvent&)` and call it from both.

### U-M11 — Per-frame / per-mouse-move fan-out with no dirty-region discipline
- `Source/Plugin/PluginEditor.cpp:64-65`: VBlank lambda sets `lookAndFeel.frostedBackdropStale = true` on *every* frame, even idle.
- `Source/UI/Canvas/NodeCanvas.cpp:245`: `setActiveTool()` calls whole-canvas `repaint()`; `ValueField.cpp:29,82`: `nodeCanvas.repaint()` (whole canvas) on layer change/refresh, while `refreshAround()` correctly repaints only the affected rect.
- `Source/Input/NodeController.cpp:mouseMove` → `NodeCanvas::fadeFlagArrows()` loops *all* arrows on every mouse move.
Fix: only mark the backdrop stale when the theme/canvas actually changed; use the `refreshAround` rect path for paint refreshes; cache flag arrows (or only run the proximity test when the cursor moved > a threshold).

### U-M12 — `std::atomic<float>&` parameter refs dereference without a null check
`Source/Plugin/PluginProcessor.h:102-105`: `*valueTreeState.getRawParameterValue(tempoParameterId)` etc. in member initialisers. Declaration order is correct and the IDs match `createParameterLayout()` (`PluginProcessor.cpp:280+`), so this works today — but a typo'd ID would be UB at construction, not a compile error.
Fix: add a `jassert` in the constructor body, or look the parameters up once in the constructor and store pointers with a null check.

### U-M13 — Public mutable editor members break the documented encapsulation
`Source/Plugin/PluginEditor.h:129-130`: `nodeCanvas` and `titleBar` are `public std::unique_ptr<…>`, while every other child is `private`. `PluginProcessor::applyRestoredState()` reaches `editor->nodeCanvas` directly — documented in `Architecture.md` §1 as the one exception, but the `public unique_ptr` lets *any* caller reset/replace the canvas.
Fix: keep them private and add a narrow `rebuildCanvasFromNodeMap()` / `getNodeCanvas()` accessor on the editor.

### U-M14 — macOS-only assumptions are real, even though the README is honest about them
`Source/Plugin/PluginEditor.cpp:18` `osxLibrarySubFolder = "Application Support"`; `documentation/Development.md:21,48` hard-codes `/Applications/CLion.app/Contents/bin/ninja/mac/aarch64/ninja`; CI is `macos-latest` only, using `sysctl -n hw.ncpu`, `auval`, `killall -9 AudioComponentRegistrar`. `README.md:181,213` does disclose "Windows and Linux are untested".
Fix (when portability matters): guard the CLion/ninja path behind `APPLE`, use `cmake --build --parallel` instead of `sysctl`, and add a Windows/Linux CI job — even a configure-only one. Low priority while macOS-only is intentional.

### U-M15 — Duplicated "loop over every node/arrow" fan-out in `NodeCanvas`
`Source/UI/Canvas/NodeCanvas.cpp`: `setDisplayMode`, `setInterceptsClicks`, `clearHighlights`, `clearOutlines`, `equipRootTraversals` (~L270-365) each hand-roll a `for (auto& [id, node] : nodeManager.all())` loop with slightly different casts/null checks.
Fix: one private `forEachNode(auto&&)` / `forEachArrow(auto&&)` template.

### U-M16 — Largest files mix responsibilities; cursor drawing lives in the canvas
`ValueField.cpp` 617 lines, `RuleTreeView.cpp` 594, `TraversalRulesWindow.cpp` 569, `NodeCanvas.cpp` 542. In particular `NodeCanvas::notePreviewCursor()` (~L470-542) is ~70 lines of bezier/path drawing for a mouse cursor inside a canvas class.
Fix: move cursor-image construction to a small `Cursors` helper (or ArsNovaUI), and split `ValueField` (field rendering vs. brush/stroke state) when it next changes.

### U-L1 — Dead field: `Node::interiorLeftInset`
`Source/UI/Node/Node.h:102`, default `0`, never assigned elsewhere; only use is `Node.cpp:65` `withTrimmedLeft(0)`. Remove it or wire it up.

### U-L2 — Naming inconsistency / O(n²) dedup
Class `Titlebar` (`Source/UI/Bars/Titlebar.h:36`) vs member `titleBar` (`PluginEditor.h:130`); `std::vector<int> changedValueIds` + linear `std::ranges::find` in `NodeCanvas::handleAsyncUpdate()` (`NodeCanvas.cpp:81`) — O(n²) dedup; use a `std::unordered_set` or sort+unique.

### U-L3 — Build guidance is confusing across docs
`README.md` tells a fresh clone `cmake -B build`, while `Development.md` says "Never configure a new one (`cmake -B build`) — build into [cmake-build-debug/release]" and then parenthetically reconciles it.
Fix: one canonical build section, with the fresh-clone path clearly labelled as the exception.

### U-L4 — `target_compile_definitions(... PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)` leaks JUCE config
`CMakeLists.txt:47-50` — leaks to everything linking `SequenceTree` (notably `SequenceTree_GraphTests`). Make them `PRIVATE` unless a consumer genuinely needs them. Also `-Wno-unused-parameter` (`CMakeLists.txt:39,184,199,213`) is applied to all targets, hiding unused-parameter warnings; prefer per-function `[[maybe_unused]]` / `juce::ignoreUnused`.

### U-L5 — `TraversalRulesWindow::activePage` is a raw non-owning pointer
`TraversalRulesWindow.h`, `ArsNovaUI::FilePage* activePage`. It is carefully nulled in `syncWithRuleState()` (`TraversalRulesWindow.cpp:250-253`), so no dangling bug was found — but an id (`viewedRuleId`, already present) or a `WeakReference` would make the invariant structural instead of conventional.

---

## High / Medium — Build / tests / safety / modern C++

### B-H1 — `StateJson::toElement` / `toVar` recurse with no depth limit on untrusted input
`Source/Graph/StateJson.cpp:42-69` (`toElement`), `:19-40` (`toVar`); entry `read()` at `:3-17`
A corrupt/malicious `.sqtree` patch (loaded via `PatchControls.cpp:84-102` → `PluginProcessor::setStateInformation`) with deeply nested `children` arrays causes unbounded recursion → stack overflow. No size/depth cap, no `children.isArray()` check before `children[index]` at `:61-64`.
Fix: add a `depth` parameter, return `nullptr` above e.g. 64, validate `children.isArray()`, and add a test with a 1,000-deep JSON state. **UNVERIFIED:** whether the host caps state size before `setStateInformation`.

### B-H2 — Invalid / newer-schema state is silently dropped (Debug-only `DBG`)
`Source/Plugin/PluginProcessor.cpp:212-224`; `PatchControls.cpp:95-97`
In Release `DBG` compiles out, so a user opening a corrupt/newer patch gets no feedback and an empty graph.
Fix: log via `juce::Logger` / show a dialog in `PatchControls::chooseFileToOpen` and `setStateInformation` on the invalid/newer-schema paths. See U-M8.

### B-H3 — `reinterpret_cast` on bitmap bytes; zero-length segment divided before its guard
`Source/UI/Canvas/ValueField.cpp:201` — `juce::PixelARGB* pixel = reinterpret_cast<juce::PixelARGB*>(pixelBytes);` where `pixelBytes = line + bitmapColumn * pixels.pixelStride`. The image is created as `ARGB`, so stride is 4 in practice, but alignment/stride are never asserted. `Source/Graph/OrnamentSymbol.cpp:53` — `direction = along / length;` computed **before** the `length > 0.0f` guard at `:63`; a zero-length segment makes `direction` NaN, which then feeds `alongCentre` / `acrossCentre` / `gap` before the guard discards the result.
Fix: `jassert(pixels.pixelStride == sizeof(juce::PixelARGB));`; in `OrnamentSymbol::erase`, compute `direction` only inside `if (length > 0.0f)`, else `continue`.

### B-H4 — `NoteScheduler` ornament step can divide by a zero `remainingWeight` (precondition UNVERIFIED)
`Source/Audio/NoteScheduler.cpp:197-214` — `stepSamples = progress.remainingSamples * step.durationWeight / remainingWeight;`
`OrnamentFigure::isPublishable` requires `totalWeight > 0`, but it was **not** verified that every figure reaching `continueOrnament` / `startOrnamentStep` passes through `isPublishable`.
Fix: `if (remainingWeight <= 0) stepSamples = progress.remainingSamples;`, plus a test with an all-zero-weight ornament.

### B-H5 — Unchecked enum casts from script bytecode are silently 0, not faults
`Source/Audio/Traversal/ScriptRun.cpp:294,306,621`
`readContext` and `readField` both have a `return 0` fall-through, so an out-of-range operand from a corrupt/hand-crafted `RTScript` silently reads 0 instead of faulting.
Fix: validate `operand` against the enum range and return `Progress::Faulted` if out of range.

### B-H6 — File-I/O return values inconsistently checked
`Source/Graph/OrnamentLibraries.cpp:33` — `folder.createDirectory();` unchecked, vs `:175,225,403` which check `.wasOk()`; `:445,456` — `deleteFile()` return ignored. `Source/Plugin/PluginDocument.cpp:81` — `patchFolder.createDirectory();` unchecked.
Fix: check `.wasOk()` at `:33` and in `PluginDocument.cpp:81` and bail/log on failure.

### B-M1 — Test targets pass `-Wall -Wextra` unconditionally — breaks / misbehaves on MSVC
`CMakeLists.txt:184,199,213` — `target_compile_options(… PRIVATE -Wall -Wextra -Wno-unused-parameter)` with no `if(MSVC)` guard, unlike the plugin target at `:37-41` (`/W4` on MSVC). `-Wno-unused-parameter` is also GCC/Clang-only.
Fix: wrap in `if(MSVC) … /W4 … else() … -Wall -Wextra … endif()` for all three test targets.

### B-M2 — CI is macOS-only; no ASan/UBSan; no `-Werror`; realtime tests are not in the default test job
`.github/workflows/tests.yml` — all three jobs `runs-on: macos-latest`; `tests` job builds only `SequenceTree_Tests SequenceTree_GraphTests` then `ctest`; `realtime` job builds `SequenceTree_RealtimeTests` only with `-DSEQUENCETREE_REALTIME_SANITIZER=ON` (needs upstream clang 20+). No Linux/Windows build, no Address/UndefinedBehaviour sanitizer job, and no `-Werror`, so warnings are never forced to fail CI. If the `realtime` job is skipped, realtime safety is untested.
Fix: add a Linux ASan+UBSan job, add `-Werror` (at least for CI), and consider running `RealtimeTests` in the default job when the compiler supports it.

### B-M3 — Coverage is deep in traversal/script, thin everywhere else; 78/90 Source files have no dedicated unit test
Only 8 test files exist: Traversal (45 cases), ScriptCompiler (44), Graph (40), Ornament (19), Gesture (11), Canvas (4), Editor (3), Realtime (3). `SequenceTree_Tests` directly compiles only 12 Source files (traversal + script); the other 78 are only exercised indirectly through `GraphTests` / `RealtimeTests` linking the whole plugin.
No dedicated tests for `PlaybackEngine`, `TraversalSession` / `TraversalDispatcher`, `EventManager` / `NoteScheduler` / `FlagScheduler` (only via integration walks), `StateJson` (only 4 GraphTests state cases), `PluginProcessor::get/setStateInformation`, `PatchControls`, `OrnamentLibraries` file-error paths, `ValueField`, or most UI (only 3 EditorTests smoke cases).
Fix: add unit tests first for: `countLimit = 0` traversal, malformed/truncated/deep-nested JSON state, zero-weight ornament, `WeightedPick` with zero/negative weights, and `CommandFifo` / `AudioUIBridge` overflow. No fuzzing / negative-input tests for the two untrusted-input parsers (script text, `.sqtree` state) — add a libFuzzer/AFL harness for the script compiler and for `StateJson::read`, plus fixtures for empty, truncated, newer-schema, and deeply-nested states. RealtimeTests are only 3 cases and only run in the sanitizer job.

### B-M4 — Test source lists are hand-duplicated
`CMakeLists.txt:166-188` — `SequenceTree_Tests` manually lists 12 `Source/…cpp` files that are also in the plugin target (`:48-164`); `SequenceTree_GraphTests` / `RealtimeTests` instead link the whole `SequenceTree` target.
Fix: put the traversal+script sources in an `OBJECT` library or static library and link it into both the plugin and `SequenceTree_Tests`.

### B-L1 — `[[nodiscard]]` / `noexcept` are inconsistent outside the RT core
E.g. `Source/Script/RTScript.h` — `bool isEmpty() const { return instructions.empty(); }` has neither; many UI/Graph getters lack `[[nodiscard]]` while `NodeRowMap` / `NodeStateTable` have it on every accessor.
Fix: add `[[nodiscard]]` to pure getters and `noexcept` to non-allocating RT/UI accessors; enable `modernize-use-nodiscard` in clang-tidy.

### B-L2 — Manual index loops where ranges would be clearer (style, not a bug)
66 `for (int i …` loops and index loops such as `StateJson.cpp:26,61`, `OrnamentLibraries.cpp:588`. Prefer range-for / `std::views::iota` / `std::ranges::find` in new code; do not churn existing correct loops.

### B-L3 — 24 `dynamic_cast`s for UI downcasting; null-check audit not completed
`Source/Input/NodeController.cpp:53,65,75,97`, `Source/UI/Canvas/NodeCanvas.cpp:142,165,214,373,422`, `Source/UI/Canvas/EncapsulationView.cpp:34,52,123,202,224`, etc. Sites sampled check the result, but **not all 24 sites were read end-to-end**, so whether every result is null-checked is **UNVERIFIED**.
Fix: audit the 24 sites; where a cast is repeated for the same component, hoist it into a helper returning a typed pointer.

### B-L4 — Error handling uses `bool` / `nullptr` / `DBG` instead of `std::expected` / `std::optional`
`StateJson::read` returns `unique_ptr` or `nullptr`; `OrnamentLibraries::readLibraryFolder/File` return an invalid `ValueTree` on error. Callers cannot distinguish "empty file" from "malformed JSON" from "newer schema" — which is why B-H2 is silent.
Fix: introduce a small `enum class StateLoadError` / `std::expected<ValueTree, StateLoadError>` at the `StateJson` / `PatchControls` boundary only.

### B-L5 — 452 `static_cast`s, mostly `int`↔`size_t` in RT code
E.g. `ScriptRun.cpp` (100+), `AudioUIBridge.cpp:21-129`, `NodeStateTable.cpp:19-117`. The casts are explicit (good — 0 C-style casts), but the volume signals `int` / `size_t` mixing throughout the RT layer.
Fix: prefer `std::size_t` indices in RT containers or a named `checked_narrow` helper.

---

## Suggested fix order (Day 1)

1. C1 + C3 + G-H13 + G-H14 + G-H15 (validate at the graph boundary — one fix class removes the phantom node, duplicate-id, and orphan-node families), then C2 (MIDI add/remove rebuild), C4 + C5 (load/delete validation).
2. C6 + C7 (replay MIDI / replay bounds), A-H1 + A-H2 + A-H3 (script budget per block, preview clamp, event time budget).
3. U-H1 (patch overwrite on host save), U-H2 + U-H3 (paint undo/memory), U-H4 + U-H5 + S-M4 (doc contradictions).
4. S-H1 + S-H2 (short-circuit `and`/`or`, trial-write cap), G-H1 + G-H2 + G-H9 + G-H11 + G-H12 (traversal counting/eligibility).
5. B-H1 + B-M2 + B-M3 (JSON recursion, ASan/UBSan + `-Werror`, test coverage for the untested areas above).
6. Everything else in Medium/Low — readability/performance refactors, best done while touching those files anyway.

## Fixed / re-checked

_None yet — Day 1. When a fix lands, move the item here with the fixing commit._

## Daily log

- **2026-11-17 (Day 1, commit 4600d21):** First substantial review. 5 areas reviewed in parallel (audio/RT, script, graph/traversal, UI/JUCE/docs, build/tests). 7 Critical, ~35 High, ~45 Medium, ~25 Low (after deduplication; counts are approximate because several findings overlap across areas). Top self-check: modulo-by-zero downgraded — see "Self-check" above. Report prepared as `CODE_REVIEW.md` for a PR; GitHub push was blocked — no GitHub login is saved for Muse, so the PR could not be opened from here without Eli connecting GitHub.
