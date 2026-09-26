# Ongoing Issues

Problems confirmed by reading the code on 2026-09-24, while reviewing an external architectural audit. Claims from that audit that turned out false or overstated are left out. Each entry says what was verified and what still needs checking.

## 1. `peekNextTarget` writes traversal state

- **Where:** `peekNextTarget` in `Source/Audio/TraversalLogic.cpp`, called from `Source/Audio/TraversalDispatcher.cpp:209`.
- **Problem (corrected):** the slot it wrote was `SwitchCandidate`, not `LastNode`. It wrote it through `selectNextChild`, plus a direct `-1` write when a tree jump was due. When the parent then advanced, `selectSwitchNode` read the peeked candidate as if a real step had chosen it. So a switch hold started one visit early, the parent's `Count` lagged behind (visible in the count display, `parent.count` in scripts, and modulator-root eligibility), and siblings that tie on count limit were held on the wrong child. Neither the modulator walk nor alternatives peek, so the primary walk was the only one that diverged.
- **Status:** resolved 2026-09-24. `selectNextChild` and `peekNextTarget` are `const`. Each real selection (`advance`, `ModulatorWalk::decide`) records `SwitchCandidate` next to `LastNode`. Since #12, `advanceAlternative` no longer records `SwitchCandidate`, and writes `LastNode` only for an alternative. The jump-time `SwitchCandidate = -1` moved into `advance`, ahead of the switch hold, so a due tree jump still wins over a hold. Covered by "peeking at the next target leaves the walk unchanged".

## 2. `peekCrossTreeNode` is misnamed

- **Where:** `Source/Audio/TraversalLogic.cpp:410-481`, called once from `TraversalDispatcher::dispatchCrossTree` (`TraversalDispatcher.cpp:390`).
- **Problem:** it advances the `CrossTree` and `CrossTreeSwitch` counters. It is the only place those counters advance, so the behaviour is intended, but the name says it only inspects.
- **Status:** naming only, no behaviour change needed. Rename it with `refactor.replace`.

## 3. Graph publishing only happens from the UI

- **Where:** every `makeRTGraph` / `rebuildAllGraphs` / `updateDurationMaps` call lives in `UI/Canvas/NodeCanvas.cpp`, `UI/Canvas/NodeManager.cpp` and `UI/Canvas/ArrowManager.cpp`.
- **Problem:** the audio thread only gets a new graph when an editor exists to drive the rebuild. The data model depends on the view to reach the audio thread.
- **Still to check:** whether any path edits the graph with no editor open, such as host state restore or undo with the window closed. If one does, the audio thread keeps playing a stale graph.
- **Open design question:** where the rebuild trigger belongs. `GraphState` is one candidate, but it is the data model. Decide before writing anything.

## 4. Node IDs are never reused, and the audio state table caps them at 1024

- **Where:** `nodeIdIncrement` in `Source/Graph/GraphState.cpp:133`, `:167` and `Source/Graph/EncapsulationOps.cpp:15`. It is recomputed on load as the highest ID in use (`GraphState.cpp:109-115`). The cap is `NodeStateTable::maxNodeIds` (`Source/Audio/NodeStateTable.h`).
- **Problem:** IDs only go up, and deleted nodes' IDs are never reclaimed, even across reloads. The limit applies to the highest ID, not to the number of nodes, so a long editing session can pass it with a small graph.
- **Consequence:** debug builds assert. Release builds silently send traversal state for any ID ≥ 1024 to a sink (`NodeStateTable::isAddressable`), so sequencing breaks with no visible sign. `TraversalDispatcher.cpp:17` guards on the same limit.
- **Also affected:** `TraversalDispatcher::markChordVisited` indexed `chordVisitStamps` by ID, so a chord member with an ID ≥ 1024 was silently never played.
- **Status:** resolved 2026-09-24 by mapping IDs to dense rows inside the tables, not in `RTGraphBuilder`. `NodeRowMap` (in `NodeStateTable.h`) is a fixed-capacity, preallocated ID → row map. `NodeStateTable` claims a row on the first write to an ID and frees every row in `clear()`. The chord visited set is a second `NodeRowMap`, cleared at the start of each chord. Graph IDs stay monotonic, so a new node never inherits a deleted node's counts. The limit is now 1024 distinct nodes written by one traversal between resets, or 1024 nodes in one chord, not the highest ID. Covered by "node ids past the state table size walk like small ones".

## 5. Host transport overrides the internal transport

- **Where:** `Source/Plugin/PluginProcessor.cpp:280-295`.
- **Problem:** whenever the plugin isn't standalone, `isPlaying` is overwritten with the host's transport state on every block. Inside a DAW, pressing the plugin's play button while the host is stopped gets undone on the next block.
- **Conflict:** CLAUDE.md's Architecture Overview says "Transport is internal … not the host transport." That is only true standalone.
- **Open question:** inside a host, should the plugin follow the host transport (update CLAUDE.md) or run off its own play button (fix the code)?

## 6. Idle message-thread wakeups when the editor is closed

- **Where:** `BlockScope::~BlockScope` in `Source/Plugin/PluginProcessor.cpp:254-264`, and `SequenceTreeAudioProcessor::handleAsyncUpdate` (`:363-378`).
- **Problem:** with no editor, `handleAsyncUpdate` returns without draining the `AudioUIBridge` FIFOs. Once they fill, `hasPendingCommands()` stays true. When playback then stops, the audio thread keeps calling `triggerAsyncUpdate` at block rate, and the message thread keeps waking to do nothing while the plugin is idle.
- **Not a problem:** reopening the editor doesn't replay stale commands. `AudioCommandDrainer`'s constructor discards whatever is queued when the canvas is built, and `drainAll` resyncs after an overflow. (Resolved 2026-09-25: it used to discard on the first drain instead, which in the standalone was the first play, so the root's highlight and arrow were dropped and playback looked like it started on the second node.)
- **Status:** confirmed, low priority. A fix has to make the FIFOs empty. Only checking for an editor before triggering would stop the wakeups but leave the FIFOs full.

## 7. `getTargetNode` uses a throwing lookup on the audio thread

- **Where:** `Source/Audio/TraversalLogic.cpp:502` (`nodes.at(primary.target)`), called at `TraversalDispatcher.cpp:738` and `:764`.
- **Current state:** can't throw today. `:764` is guarded by a `find`. `:738` is reached only when `repeatValue > 1`, and `repeatValue` is only read from a target that was found (`:715-731`), so a deleted target forces it to 1 and skips the branch.
- **Problem:** `:738` is safe only because of how `repeatValue` happens to be initialized, and a later edit could remove that without anyone noticing. `std::out_of_range` on the audio thread would take down the host.
- **Related:** `removeDeletedTraversals` (`Source/Audio/TraversalSession.cpp:159-190`) checks only the home root, so a running traversal can outlive its current target.
- **Status:** hygiene, not a live crash. Make the lookup impossible to throw.

## 8. `getRootNode` is dead code

- **Where:** `Source/Audio/TraversalLogic.cpp:503`, declared at `TraversalLogic.h:149`.
- **Problem:** no callers, and it uses the same throwing `.at()` lookup.
- **Status:** delete it with `refactor.decap TraversalLogic::getRootNode`.

## 9. The only plugin parameter is a dummy

- **Where:** `SequenceTreeAudioProcessor::createParameterLayout` (`Source/Plugin/PluginProcessor.cpp:380-392`).
- **Problem:** it registers one `"gain"` parameter that nothing reads. The host sees one automatable control that does nothing.
- **Status:** confirmed. Remove it, or replace it with real controls.

## 10. Arrow trail drawing flattens each curve three times per trail per frame

- **Where:** `trimPathToFraction` and `drawArrowProgress` in `Source/UI/Theme/CustomLookAndFeel_Nodes.cpp:119-228`.
- **Problem:** for every active trail on every frame, the shaft is copied and translated, flattened once to measure its length (`:131`), flattened again to trim it (`:148`), and flattened a third time inside `g.strokePath`.
- **Possible fix:** the per-trail offset is a pure translation (`:215-217`), so one arc-length table per shaft, rebuilt when the geometry changes, would serve every trail and remove the first two passes.
- **Status:** unmeasured. Profile during playback before changing it.

## 11. The modulator walk ignores trigger limits

- **Where:** `registerTrigger` is called only from `TraversalLogic::advance`. `ModulatorWalk::decide` never calls it.
- **Problem:** a spent trigger limit makes a node ineligible in the primary walk, but a modulator with the same limit is played forever. This breaks the rule that every walk is the same traversal.
- **Evidence:** the "trigger limit" section of the parity test in `Tests/TraversalTests.cpp` fails at HEAD (7141024) as well as after the #1 fix: `ctest` reports 21/22.
- **Status:** resolved 2026-09-24. `decide` now calls `registerTrigger` for a newly selected child, the same place `advance` does: inside the fresh-selection branch, so a switch hold doesn't count as a trigger.

## 12. Alternatives overwrite their host's switch candidate

- **Where:** `TraversalLogic::advanceAlternative` in `Source/Audio/TraversalLogic.cpp`. It wrote `SwitchCandidate[currentAltId]` and `LastNode[currentAltId]` after every alternative selection.
- **Problem:** when a host `H` picked its first alternative `A`, it wrote `SwitchCandidate[H] = A`. That slot is `H`'s own child switch hold, which `selectSwitchNode` reads in both `advance` and `ModulatorWalk::decide`. So adding an alternative cut short the hold on `H`'s children. When `A.switchCountLimit > 1`, the walker stepped onto `A` as a primary target, and the modulator walk diverged from the node walk. The same write set `LastNode[H] = A` (or `-1` when no alternative was eligible), so a script's `parent.lastChild` on a host could return an alternative instead of a child.
- **Evidence:** `alternativeShape` with the first alternative's switch count at 3 gave `1 2 1 2 10 10 1 2 10 10 11 …`. A host with a held child (count limit 2, switch count 3), a sibling and a default alternative gave `1 2 4 1 2 10 3 1 2 3 1 2 10 4 …`, so the hold on 3 was cut to 2.
- **Status:** resolved 2026-09-25 (`Proposals/Implemented/alternative_switch_candidate_collision.md`). The `SwitchCandidate` write is gone, and `LastNode` is written only when `currentAltId != parentId`. Covered by "an alternative's switch count holds only that alternative", "an alternative leaves its host's switch hold on its children intact", the "held alternatives" and "host hold beside an alternative" parity sections, and "a host's last child is never one of its alternatives".

## 13. The primary arrow and note length ignore the switch hold

- **Where:** `TraversalLogic::peekNextTarget` in `Source/Audio/TraversalLogic.cpp`, read by `TraversalDispatcher::dispatchNode` (`TraversalDispatcher.cpp:198`) for `resolveDuration` and `dispatchPrimaryArrow`.
- **Problem:** `peekNextTarget` checked for a tree jump and then made a fresh `selectNextChild` pick. `advance` applies the switch hold (`selectSwitchNode`) between those two. So on every held visit after the first, the arrow animated toward the fresh pick, usually the sibling, and the note took that arrow's length, while the walker then entered the held child. The modulator walk was unaffected: it draws from `decideNextModulator`, which runs the real decision.
- **Status:** resolved 2026-09-25. `peekNextTarget` reads `SwitchCandidate` and `SwitchCount` after the tree-jump check and returns the held child when the hold continues, without writing. Covered by "the peeked target is the node the walk enters next". A scratch sweep of 1944 shapes shows no mismatches on advanced steps, against 102,713 at 7f59922.

## 14. The primary arrow ignores an encapsulated group's loop back to its entry

- **Where:** `TraversalLogic::peekNextTarget` against `advance`'s `encapsulationLoopTarget` redirect.
- **Problem:** when a member's fresh pick leaves its group before the group's sub-loop is done, `advance` enters the group's entry instead, but `peekNextTarget` returns the exit child. The arrow and the note length follow the exit arrow, then the walker jumps back to the entry. Shape: root 1 → entry 2 (sub-loop 2) → member 3 → exit 4. At 3 on the first pass, peek gives 4 and the walk enters 2.
- **Status:** confirmed 2026-09-25, not yet fixed.
