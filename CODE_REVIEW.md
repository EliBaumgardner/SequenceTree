# SequenceTree — Code Review

**Repo:** https://github.com/elibaumgardner/SequenceTree
**Commit reviewed:** `cd0ec6a` — "Organise Source into Core, Processor and Editor". The first draft was written against `4600d21`. On 2026-11-17 every finding was re-checked against the code and re-cited at `cd0ec6a`, the commit this file lands on.
**Date:** 2026-11-17
**Scope:** Daily recurring review. Each day adds new findings, refines old ones, and re-checks earlier ones against the current code.

## At a glance

- 197 C++ files in `Source/` and `Tests/`, 34,785 lines. 90 `.cpp` files in `Source/`, 8 test files, 169 `TEST_CASE`s.
- 130 findings were raised on Day 1. After re-checking, none of the 7 Criticals holds as Critical, and one finding holds as High.

| Status | Count |
|---|---|
| Open: High | 1 |
| Open: Medium | 5 |
| Open: Low (includes documentation fixes) | 54 |
| Needs a test or the owner's decision | 8 |
| Settled by the owner, or not a defect | 9 |
| Withdrawn: did not survive re-checking | 29 |
| Suggestions, not defects | 16 |
| Duplicates (merged into another finding) | 4 |
| Fixed | 1 |
| Not yet checked | 3 |

- What is genuinely good: the real-time design is deliberate (pre-allocated pools, lock-free FIFOs, epoch-retired snapshots, a per-call script step budget, and a RealtimeSanitizer test of `processBlock`), listener hygiene in UI code is consistent, the documentation is unusually deep, and traversal and script test coverage is deep.

## How this file is maintained

- New findings are added, never silently replacing old ones. When a finding is fixed, it moves to "Fixed / re-checked" with the commit that fixed it.
- Every finding cites `file:line` at the commit named above. Line numbers drift: re-check a finding before acting on it.
- **Severity follows what a user can actually reach.** High: wrong output or data loss through normal use. Medium: behaviour a user can hit that is wrong, undocumented, or contradicts the code's own documentation. Low: needs a corrupt or hand-edited file, hand-built bytecode, a pathological script or an unrealistic load; or a cheap clean-up. A finding that needs a corrupt file is never above Low.
- **Settled** findings record the owner's decisions about deliberate behaviour. **Withdrawn** findings did not survive re-checking. Do not re-raise either unless the code they cite changes.
- A finding marked **Suspected** names the test or measurement that would settle it.

## Self-check — corrections made while writing this report

1. **Modulo-by-zero (`% countLimit`): Low, defence-in-depth.** `Source/Core/Traversal/ChildSelector.cpp:202` and the max-limit picks in `TraversalRule.cpp` evaluate `parentCount % child->countLimit`, but both are reached only through `RuleContext::eligibleChild`, which refuses `countLimit <= 0` (`Source/Core/Traversal/TraversalRule.cpp:32`). Confirmed on re-check.
2. **Ornament zero-weight division: not a finding.** `OrnamentFigure` only pushes steps with a positive weight (`Source/Processor/Document/Ornaments/OrnamentFigure.cpp:389`), so the divisor in `NoteScheduler::startOrnamentStep` cannot be 0 for a published figure. Confirmed on re-check (see B-H4, withdrawn).
3. **Script VM stack and arithmetic: no finding.** Consistent with the code read on re-check.

---

## Open — High

### G-M16 — Pasted arrows lose their pitch offset, so the first move after a paste does not shift the pitch
**High.** `connectClipboardNodes` copies each arrow between copied nodes with `getArrowInfo` / `setArrowInfo` (`Source/Processor/Document/Clipboard.cpp:133`), and neither reads nor writes `ArrowPitchOffset` (`Source/Processor/Document/ArrowBindingOps.cpp:111-156`). A pasted pitch-bound arrow therefore starts with its offset at the default, and `applyArrowPitchOffset` treats a default offset as "record it, don't shift" (`Source/Processor/Document/ArrowBindingOps.cpp:171-175`). The first move of a pasted child records the new offset and leaves its pitch unchanged, where the same move on the source arrow shifts the pitch.
Fix: copy `ArrowPitchOffset` from the source arrow in `connectClipboardNodes`. Test: copy a pitch-bound parent and child, paste, move the pasted child, and check that its pitch changes as the source's does.

## Open — Medium

### S-H1 — `and` / `or` are not short-circuited
**Medium; the owner decides between the two fixes.** `emitBinary` emits the left operand, then the right, then one `LogicalAnd` / `LogicalOr` (`Source/Core/Script/ScriptEmitter.cpp:1034-1044`), so the right side always runs. The original examples do not compile: `advance` returns a `Node`, and `and` / `or` take numbers only (`Source/Core/Script/ScriptEmitter.cpp:1054`). What is affected is a helper with side effects on the right side (one that assigns a member, or calls `playNote` from `main`), as in `ready and bump()`. The user guide says nothing about evaluation order (`documentation/ScriptingLanguage.md:104`).
Fix: either short-circuit (jump past the right operand on `JumpIfFalse` / `JumpIfTrue`), which changes what existing scripts with such helpers do, or document eager evaluation. Test: `0 and bump()` leaves the member `bump` writes unchanged.

### S-M4 — Member and global initialisers run in declaration order, and the user guide says they are not allowed
**Medium (documentation).** The user guide says a member cannot take a starting value (`documentation/ScriptingLanguage.md:40`). The developer guide says it can (`documentation/Development.md:91`), the emitter compiles initialisers (`emitInitializer`, `Source/Core/Script/ScriptEmitter.cpp:689-739`), and a test covers them (`Tests/Unit/ScriptCompilerTests.cpp:423`). Initialisers run in declaration order, globals before members (`Source/Core/Traversal/TraversalLogic.cpp:278-320`), so `int a = b; int b = 5;` leaves `a` at 0. The user guide also never mentions out-of-class variables.
Fix: correct the user guide (initialisers and globals are allowed, run in declaration order, and a variable declared later still reads 0 or `none`). Rejecting a forward reference is optional.

### G-H6 — Copying one member of a group does not copy the group, contrary to `Clipboard.h`
**Medium; the owner decides which side is right.** The header promises that copy first adds every member of any group one of the ids is in (`Source/Processor/Document/Clipboard.h:18-19`). `withEncapsulatedMembers` asks `memberIds` of each selected id (`Source/Processor/Document/Clipboard.cpp:354-372`), which returns members only when the id is the group node itself (`Source/Processor/Document/EncapsulationOps.cpp:131-143`). Copying one member therefore copies that node alone, and paste does not recreate its group.
Fix: either widen through the member's `EncapsulatorId`, or change the header to say that only a copied group node brings its members. Test: copy one member of a three-member group, paste, and check the result against whichever contract is kept.

### A-H2 — Preview with a high count repeats rule selection for each alternative host on the audio thread
**Medium, Suspected: undecided until measured.** In Preview mode `begin()` replays `advanceAlternative` up to `visitCount - 1` times for every node that has an alternative (`Source/Core/Traversal/TraversalLogic.cpp:63-77`), and the count field allows up to 9,999 (`Source/Editor/Bars/BottomBar.cpp:22`). Each pass runs one rule selection. The cost was not measured.
The original fix (clamp the count to 64) is rejected: the seeded counts decide which alternative is sounding, and counts are compared against limits up to 9,999, so a clamp changes what preview plays.
Settle it: time one preview at count 9,999 with 1, 10 and 50 alternative hosts. Only if that is too slow for one block, look for a way to seed the counts that gives the same alternatives.

### U-H4 — The README badge says C++20; the code needs C++23
**Medium (documentation).** `README.md:9` shows a C++20 badge, while every target sets `cxx_std_23` (`CMakeLists.txt:34`, `CMakeLists.txt:67`). C++23 is required, not incidental: the script compiler uses `std::expected` (`Source/Core/Script/ScriptParser.h:4`, `Source/Core/Script/ScriptEmitter.h:4`). The original fix's alternative, dropping CMake to C++20, would break the build.
Fix: change the badge to C++23.

## Open — Low

### Documentation gaps (all confirmed; fix in `documentation/ScriptingLanguage.md`)
- **S-M1** — `advance(n)` moves at most 64 nodes (`Source/Core/Traversal/TraversalLogic.h:105`), and `advance(n)` with `n < 1` stays on the current node (`Source/Processor/Playback/TraversalDispatcher.cpp:31-33`). Neither is documented (`documentation/ScriptingLanguage.md:118`).
- **S-M8** — Nested calls share the top-level call's 8,192-instruction budget (`Source/Core/Traversal/ScriptRun.cpp:27-60`); the guide says each run of a function gets its own (`documentation/ScriptingLanguage.md:157`). The "host work per step" half is bounded by that same budget.
- **S-M9** — A second `playNote` in one `main` run replaces the first (`Source/Processor/Playback/TraversalDispatcher.cpp:81-87`). The guide should say one note per step, and the last call wins (`documentation/ScriptingLanguage.md:119`).
- **S-L5** — The base class is optional and the class name is free (`Source/Core/Script/ScriptParser.cpp:123-144`); the guide presents one fixed form. The trailing `;` half was not checked.
- **S-L6** — `bool`, `true` and `false` are missing from the Types table (`documentation/ScriptingLanguage.md:54-59`). The `%`-on-`bool` half was not checked.
- **S-L7** — `1.` and `1e10` are not decimals (`Source/Core/Script/ScriptLexer.cpp:92-134`); the guide should say that only `digits.digits` is.

### Script language
- **S-M3** — `int x = 1; int x = 2;` in one block compiles: `emitDeclare` and `declareLocal` never check the current scope (`Source/Core/Script/ScriptEmitter.cpp:223-256`, `Source/Core/Script/ScriptEmitter.cpp:785-790`). Fix: report "'x' is already declared" for a binding made since `scopeMarks.back()`; shadowing in a nested block stays legal.
- **S-H2** — In a trial run, writes to more than 64 distinct node-state pairs are dropped silently (`Source/Core/Traversal/ScriptRun.cpp:561-582`). Needs a script that touches over 64 nodes' state in one peek. Fix: fault the trial call when the table is full.
- **S-M2** — The ban on `advance` / `playNote` inside `advance` checks only the function named `advance` (`Source/Core/Script/ScriptEmitter.cpp:931`). A helper bypasses it, but the nested call then returns `none` or does nothing, so this is a missing diagnostic, not a safety problem.
- **S-M6** — A decimal literal too large for `double` becomes `inf` with no diagnostic: `strtod` is unchecked and the integer-overflow flag is reset at the `.` (`Source/Core/Script/ScriptLexer.cpp:112`, `Source/Core/Script/ScriptLexer.cpp:126`).
- **S-M7** — `-2147483648` cannot be written, because the lexer rejects `2147483648` (`Source/Core/Script/ScriptLexer.cpp:100-118`) before `parseUnary` folds the minus (`Source/Core/Script/ScriptParser.cpp:793-822`).
- **S-L1** — An unknown import or a bad class header stops parsing (`Source/Core/Script/ScriptParser.cpp:34-60`), so not every error is reported at once as the guide says (`documentation/ScriptingLanguage.md:166`).
- **S-L3** — There is no cap on the size of a compiled script; only execution is capped (`Emitter::emit` appends with no limit, `Source/Core/Script/ScriptEmitter.cpp:767-772`).
- **S-L8** — A member named `current` is refused (`Source/Core/Script/ScriptEmitter.cpp:34-37`), but a local of that name is not. The `traversal` member half was not checked.
- **S-L9** — The VM reads 0 for an out-of-range field, context or convert operand instead of faulting (`Source/Core/Traversal/ScriptRun.cpp:294`, `Source/Core/Traversal/ScriptRun.cpp:306`). Only hand-built bytecode can reach it; the compiler never emits one. (B-H5 merged here.)

### Audio and real-time
- **C1** — `createRTNodes` asserts on an invalid child instead of skipping it (`Source/Processor/AudioModel/RTGraphBuilder.cpp:494-497`), unlike the two sibling passes. Editing cannot leave an arrow to a missing node: `removeNode` disconnects every parent (`Source/Processor/Document/GraphState.cpp:274-276`), and `canConnect` refuses duplicates (`Source/Editor/Canvas/GraphDocument.cpp:159-168`). Only a corrupt or hand-edited state file reaches it. Fix: `continue`, as the siblings do.
- **A-H1** — The script budget is per call (`Source/Core/Traversal/ScriptRun.cpp:27-60`), so one note can run a few budgets (`main`, `advance`, a trial peek, a modulator decision). The original's "64 trial runs per step" is wrong: `skipAhead` never runs a script, because the scripted path returns first (`Source/Core/Traversal/TraversalLogic.cpp:122-126`). Reaching a block overrun needs a script that spends its whole budget on every call, on many notes ending in one block.
- **A-H3** — The event cap is a count (`Source/Processor/Playback/EventManager.cpp:29`), and notes left over when it is hit fire at sample 0 of the next block (`Source/Processor/Playback/EventManager.cpp:71-73`). Needs over 4,096 events in one block.
- **A-M3** — `scheduleNote` drops notes past 1,024 sounding at once (`Source/Processor/Playback/NoteScheduler.cpp:21-23`). A drop counter would be cheap.
- **A-M4** — The virtual MIDI port is stamped with wall-clock time and drained by a 1 ms timer on its own thread, not the audio thread (`Source/Processor/Channels/VirtualMidiOutput.cpp:44-76`). Document it as best effort.
- **A-M7** — Pool and connection lookups are linear scans over small arrays, at most 128 slots (`Source/Core/Traversal/TraversalPool.h:81-100`). The 1,024-node cap of `NodeStateTable` is documented (`Source/Core/Traversal/NodeStateTable.h:20-22`).
- **A-M8** — Overflow of `previewRequests` (16 entries) and of the virtual-MIDI FIFO is set but never read (`Source/Processor/Channels/CommandFifo.h:42`). Needs 16 preview clicks within one block.
- **A-L1** — The orphan scan runs every block (`handleOrphanNotes`, `Source/Processor/Playback/EventManager.cpp:76`) and the flag scan every event (`startNextDue`, `Source/Processor/Playback/FlagScheduler.cpp:76`), even when nothing changed. Cheap to skip.
- **A-L3** — Record mode in `AudioUIBridge` drops entries past capacity without a flag and dedups linearly (`send`, `Source/Processor/Channels/AudioUIBridge.cpp:46`).

### Graph, traversal and state
- **C4** — `replaceState` clears the document and copies the restored one with no validation (`Source/Processor/Document/GraphState.cpp:373-396`). Reachable only from a file that parses but is corrupt. The "ids from top-level children only" point is wrong: every node is a direct child of `nodeMap`.
- **G-H2** — `NodeStateTable` does not check its slot argument (`Source/Core/Traversal/NodeStateTable.h:30-44`); no caller passes an invalid one.
- **G-H5** — One corrupt child makes `StateJson` reject the whole document, with no error detail (`Source/Processor/Document/StateJson.cpp:60-68`). All-or-nothing is a safe failure; reachable only from a corrupt file.
- **G-H9** — `WeightedPick` does not clamp a negative probability (`Source/Core/Traversal/WeightedPick.cpp:4-13`). The editor allows 0 to 100, so only a hand-edited file reaches it.
- **G-H10** — When a host and its active alternative both have an arrow to the same root, that root's cross-tree count advances twice in one step (`Source/Core/Traversal/TraversalLogic.cpp:174-257`). Two arrows from one host to one root cannot exist (`connectionFor` dedups by child id, `Source/Processor/AudioModel/RTGraphBuilder.cpp:769-781`), and pushing the root on every step of its switch phase is the switch mechanism.
- **G-H13** — Entry points in `GraphState` and `NodeFactory` check their arguments with `jassert` only (for example `Source/Processor/Document/GraphState.cpp:55-56`); every caller passes a valid parent.
- **G-H14** — `kindOf` maps an unknown type to `Kind::Node` (`Source/Processor/Document/GraphState.cpp:532-565`); only a corrupt file has one.
- **G-H15** — `indexNode` overwrites a duplicate id, and `unindexNode` erases it unconditionally (`Source/Processor/Document/GraphState.cpp:598-622`); only a corrupt file has duplicate ids.
- **G-H17** — `rebuildAllGraphs` rebuilds root graphs only (`Source/Processor/AudioModel/RTGraphBuilder.cpp:47-53`), so after an ornament change modulator-root graphs keep their old data. Whether a modulator node can carry an ornament was not checked.
- **G-H19** — `pasteAt` trusts `contents` (`Source/Processor/Document/Clipboard.cpp:65-83`); only `copy` fills it, with distinct ids.
- **G-M6** — The selection random value advances once per step (`Source/Core/Traversal/TraversalLogic.cpp:431-433`), so two weighted picks in one step with the same span pick the same rank.
- **G-M9** — The real point: `Choices::Fixed` is tested (`Source/Core/Traversal/TraversalPool.h:118`) but never set anywhere, since `choices` starts as `Random` and nothing assigns it (`Source/Core/Traversal/TraversalPool.h:214`), so that mode is dead code. The duplicate-`runId` half is covered by `acquire` resetting the walk (`Source/Core/Traversal/TraversalPool.h:113`).
- **G-M10** — An async update can publish twice: `updateDurationMaps`, then `publishQueuedGraphs` (`Source/Processor/AudioModel/RTGraphBuilder.cpp:250`, `Source/Processor/AudioModel/RTGraphBuilder.cpp:256`). Cost only.
- **G-M12** — `rememberOwners` drops a change whose tree has no owner, silently (`Source/Processor/AudioModel/RTGraphBuilder.cpp:706-729`).
- **G-M14** — `StateJson` drops wrong-typed `properties` / `children` instead of rejecting them (`Source/Processor/Document/StateJson.cpp:52-58`); corrupt file only.
- **G-M17** — Paste recreates a group only when every member was pasted (`Source/Processor/Document/Clipboard.cpp:398-403`).
- **G-M18** — `removeMember` does not reset the removed node's own `EncapsulatorId` (`Source/Processor/Document/EncapsulationOps.cpp:116-129`); harmless for its only caller, `removeNode`.
- **G-M19** — `getArrowInfo` casts stored ints to enums without a range check (`Source/Processor/Document/ArrowBindingOps.cpp:143-149`); corrupt file only.
- **G-M20** — `StateJson` recurses with no depth limit (`Source/Processor/Document/StateJson.cpp:31-33`, `Source/Processor/Document/StateJson.cpp:60-68`); corrupt file only. A depth cap in `StateJson` would not help on its own, because `juce::JSON::parse` recurses first. (B-H1 merged here.)
- **G-L2** — `increment` can overflow if a script set a count near `INT_MAX` (`Source/Core/Traversal/NodeStateTable.cpp:48-63`).
- **G-L4** — Note values are copied without range checks; the editor limits them. The original's "`fallbackValue` is unused" is wrong: it is used at `Source/Processor/Playback/NoteScheduler.cpp:32-33` and `Source/Processor/Playback/NoteScheduler.cpp:165`.

### UI, build and files
- **U-H3** — Paint buffers grow with the canvas, but sizes are in points and only the active layer is allocated (`Source/Editor/Canvas/ValueField.cpp:452-463`); the 132 MB figure is overstated.
- **U-M3** — `makeNode` returns `nullptr` for an unknown tree type with no assertion (`Source/Editor/Canvas/NodeCanvas.cpp:435-470`).
- **U-M5** — `DisplayMode::Duration` is not offered by the display menu (`Source/Editor/Bars/Titlebar.cpp:185-190`), so its case in `Node` never runs. The real finding is that the mode is dead; its missing rebind is not reachable.
- **U-M8** — A state that fails to load is reported only with `DBG`, so a release build gives no message (`Source/Processor/PluginProcessor.cpp:213`, `Source/Processor/PluginProcessor.cpp:217`, `Source/Processor/PluginProcessor.cpp:222`). (B-H2 merged here.)
- **U-M9** — Node colour resolution is written three times (`Source/Editor/Canvas/NodeCanvas.cpp:117-121`, `Source/Editor/Canvas/NodeCanvas.cpp:207-210`, `Source/Editor/Nodes/Node.cpp:215-225`). The fourth site, in `AudioCommandDrainer`, resolves a traversal colour and is not the same code.
- **U-M10** — `Node::mouseMove` and `mouseExit` have the same body (`Source/Editor/Nodes/Node.cpp:191-211`).
- **U-M11** — The VBlank callback marks the frosted backdrop stale every frame (`Source/Editor/PluginEditor.cpp:64-74`), and some refreshes repaint the whole canvas. Cost not measured.
- **B-H6** — `createDirectory` results are unchecked (`Source/Processor/PluginDocument.cpp:80`, `Source/Processor/Document/Ornaments/OrnamentLibraries.cpp:33`).
- **B-M1** — The test targets pass `-Wall -Wextra` with no MSVC guard (`CMakeLists.txt:193`, `CMakeLists.txt:207`, `CMakeLists.txt:221`), unlike the plugin and core targets.

## Needs a test or the owner's decision

- **G-H12** — Plausible: a switch repeat returns before the group-loop diversion (`Source/Core/Traversal/ChildSelector.cpp:107-133`). Test: a group whose exit child has a switch limit above 1.
- **G-H16** — Children under a traversal flag are never built as RT nodes (`Source/Processor/AudioModel/RTGraphBuilder.cpp:487-489`), and the flag then starts nothing (`Source/Processor/Playback/FlagScheduler.cpp:182-186`). Open question: can the canvas grow a node out of a flag? `addChildNode` does not refuse one (`Source/Editor/Canvas/GraphDocument.cpp:23-38`).
- **G-M5** — The "invalid start node" half is wrong. The "begin twice" half holds: `prepareTraversal` reuses an ended run without resetting it (`Source/Processor/Playback/TraversalDispatcher.cpp:241-266`), called from `FlagScheduler` (`Source/Processor/Playback/FlagScheduler.cpp:192-205`) and from `startCrossTreeTraversal` (`Source/Processor/Playback/TraversalDispatcher.cpp:845-858`), so its counts and `subRootNode` carry over. Is that intended?
- **G-M11** — A child with two parents can get `parentId` 0 when its first parent is built after it (`Source/Processor/Document/GraphState.cpp:142-175`, `Source/Processor/AudioModel/RTGraphBuilder.cpp:462-484`). Test with a shared child under a modulator.
- **G-H8** — The `insertNodeAfter` half cannot happen (it is only given a newly created child). The `create` half can only happen if a node already in a group can be a span's first click (`Source/Processor/Document/EncapsulationOps.cpp:28-38`). Test that gesture.
- **G-M3** — After a traversal-arrow jump, Live mode sets the loop limit to endless instead of the new tree's limit (`Source/Core/Traversal/TraversalLogic.cpp:550-552`). The owner's call.
- **U-M7** — Unverified: editor-level shortcuts may take keys from a focused text editor (`Source/Editor/PluginEditor.cpp:143-190`). Test by typing Space and Cmd+Z in the Rules window.
- **A-M1** — Suspected: `publish` and `releaseRetiredSnapshots` assume the message thread (`Source/Processor/AudioModel/AudioSnapshotPublisher.cpp:25-47`, `Source/Processor/AudioModel/AudioSnapshotPublisher.cpp:99-102`), and `releaseResources` calls the latter (`Source/Processor/PluginProcessor.cpp:34-39`). Settle it by asserting the message thread there and running under each host.

## Settled by the owner, or not a defect — do not re-raise

- **C6** — Replay after a host jump is silent until the next note starts, by the owner's decision (2026-11-13), like clipping a note's start in a DAW's MIDI editor. The original is also wrong on its own terms: the `addEvents` at `Source/Processor/Playback/TraversalSession.cpp:285` is live when the jump target is 0.
- **C7** — Overstated. Online, replay gets a quarter of each block's time (`Source/Processor/Playback/TraversalSession.h:98`); it only falls behind for good if walking the graph costs more than four times real time, and then live playback would overrun too. Offline, running replay to completion in one block is the accurate result for a bounce, which has no deadline, and is documented (`Source/Processor/Playback/TraversalSession.h:19`).
- **G-H3** — When every instance is taken, a new root shares the last one (`Source/Processor/Document/TraversalState.cpp:81-101`). Intended (owner, 2026-09-24).
- **U-H1** — `getStateInformation` writes the patch file (`Source/Processor/PluginProcessor.cpp:196-198`). Patch autosave stays (owner, 2026-11-13).
- **U-H2** — Paint has no undo by design (`Source/Editor/Canvas/ValueField.h:17`). The "write per node per drag step" half is wrong: a write happens only when the value changes (`Source/Editor/Canvas/ValueField.cpp:446`).
- **G-M7** — `skipAhead` does not count skipped nodes, which the guide documents (`documentation/ScriptingLanguage.md:118`), and peeking with `isAudibleChild` is correct (owner, 2026-11-13: only a modulator or a modulator root has a modulator child).
- **A-M2** — Each publish copies the graph on the message thread (`Source/Processor/AudioModel/AudioSnapshotPublisher.cpp:58-97`); an earlier measurement put it at about 88 µs for 1,024 nodes.
- **A-M6** — Note lengths scale with the host tempo (`Source/Processor/Playback/PlaybackEngine.cpp:138-152`), so the walk runs in beats and converting the jump target at the current BPM is consistent. The 1/64-beat tolerance only filters jitter.
- **G-L5** — A loop limit of 0 means endless by design, and `advance(n)` with `n < 1` stays put (`Source/Processor/Playback/TraversalDispatcher.cpp:31-33`).

## Withdrawn — did not survive re-checking

- **C2** — MIDI notes are added only while a node is being created (`setDefaultNodeNote` and `inheritFromParent`, `Source/Processor/Document/NodeFactory.cpp:186-196`, `Source/Processor/Document/NodeFactory.cpp:215-244`), before it is attached, and attaching the node triggers the rebuild (`Source/Processor/AudioModel/RTGraphBuilder.cpp:60-66`).
- **C3** — `connectNodes` runs before the child is in the map on purpose: `addChildNode` connects, then adds (`Source/Processor/Document/GraphState.cpp:83-84`). The proposed "child must be valid" check would refuse every new node. Duplicates and modulator-to-root arrows are refused by `canConnect` (`Source/Editor/Canvas/GraphDocument.cpp:159-168`), the canvas cannot draw an arrow from a node to itself (`ArsNovaUI/ArsNovaUI/Canvas/ArrowGestures.cpp:185,188` at the pinned submodule), and arrows back to a node's own root are a feature (`Source/Processor/AudioModel/RTGraphBuilder.cpp:759-766`).
- **C5** — Not a defect. Deleting a node leaves its children on the canvas, unattached, which is the state disconnecting an arrow leaves a child in; the header names a traversal flag as the one child disconnect removes instead (`Source/Editor/Canvas/GraphDocument.h:19-20`). Whether delete should cascade is a product question, not a bug.
- **A-M5** — The realtime test runs `processBlock` under RealtimeSanitizer (`Tests/Integration/RealtimeTests.cpp:40`), which reports any allocation or lock on that path. `MidiBuffer::addEvent` can grow only a host buffer that has too little room (`JUCE/modules/juce_audio_basics/midi/juce_MidiBuffer.cpp:155` at the pinned submodule), which is JUCE's contract with the host.
- **A-L2** — The 10,000-step guard only bounds a cycle; the parent walk stops at the root, so it costs the tree's depth (`Source/Core/Traversal/ModulatorWalk.cpp:270-296`). The two-pass `selectChild` is a small design idea, not a defect.
- **A-L4** — A reset during replay clears the expected host position, so the next block re-syncs (`Source/Processor/Playback/PlaybackEngine.cpp:24-28`). Every caller of `silenceAllNotes` already clears pending flag starts, as the original says.
- **S-M5** — `node.eligible` being 1 is documented (`documentation/ScriptingLanguage.md:149`).
- **G-H1** — The only caller of `ref()` takes two different slots (`Source/Core/Traversal/TraversalLogic.cpp:222-223`), each slot has its own sink (`Source/Core/Traversal/NodeStateTable.cpp:80-101`), and the 1,024-node cap is documented (`Source/Core/Traversal/NodeStateTable.h:20-22`).
- **G-H4** — `replaceState` removes every property of the rule state, so the old `activeRuleId` does not survive (`Source/Processor/Document/TraversalRuleState.cpp:243`), and `PluginDocument::restore` then calls `ensureDefaultRule` (`Source/Processor/PluginDocument.cpp:134`), which is the proposed fix.
- **G-H7** — Every caller of `EncapsulationOps::create` passes a non-empty span of valid nodes (`Source/Editor/Input/Gestures/EncapsulationGestures.cpp:20-85`).
- **G-H11** — A script's `advance` owning the choice and the counting is documented (`documentation/ScriptingLanguage.md:43`).
- **G-H18** — Only roots and flags carry traversal lists, and `rebuildGraphsForTraversal` scans both (`Source/Processor/AudioModel/RTGraphBuilder.cpp:350-385`); traversals are never deleted.
- **G-H20** — Newer schema versions are already refused at the load boundary (`Source/Processor/PluginProcessor.cpp:219-225`).
- **G-M1** — A walk already running on an unsynced arrow continues (`Source/Core/Traversal/ModulatorWalk.cpp:195-199`).
- **G-M2** — Documented: a script's choice owns the modulator step, as G-H11.
- **G-M4** — On the first visit `advanceAlternative` returns before choosing (`Source/Core/Traversal/ChildSelector.cpp:36-42`), so the host branch never runs with the count still at 0.
- **G-M8** — The init functions (`Source/Core/Traversal/TraversalLogic.cpp:299-317`) set the same values whether they first run in a peek or in the committed run; running them earlier has no effect.
- **G-M13** — The node-map listener fires only for trees attached under `nodeMap`, so the parent chain it reads is fixed (`Source/Processor/AudioModel/RTGraphBuilder.cpp:90-143`). The `isAudioProperty` half was not checked.
- **G-M15** — Documented: only arrows between copied nodes are reconnected, and a node whose parent was not copied becomes a root (`Source/Processor/Document/Clipboard.h:20-24`).
- **G-L1** — A flag's sign is not part of the traversal it names, so dropping it when collecting keys is right (`Source/Processor/Document/TraversalState.cpp:42-79`); a flag value is a traversal id and cannot be `INT_MIN`.
- **G-L3** — Node ids start at 1 (`Source/Processor/Document/GraphState.cpp:23`), so `parentId` 0 means no parent. The "parent not built yet" case is G-M11.
- **U-H6** — The README has the submodule step (`README.md:186`); the empty submodules were the review clone's state. `compile_commands.json` is no longer tracked since `cd0ec6a`.
- **U-M4** — `RuleProperties` over an invalid tree reads defaults, and nothing uses them before the validity check (`Source/Editor/Rules/TraversalRulesWindow.cpp:149-157`).
- **U-M6** — The constructor's bind (`Source/Editor/Nodes/Node.cpp:29`) is replaced when `NodeCanvas::nodeCreated` rebinds (`Source/Editor/Canvas/NodeCanvas.cpp:120`); harmless.
- **U-M12** — The parameter references are bound to ids that `createParameterLayout` defines (`Source/Processor/PluginProcessor.h:102-105`); works as written.
- **U-L1** — `interiorLeftInset` is set in `RootNode` (`Source/Editor/Nodes/RootNode.cpp:18`) and read in `OrnamentMarks` (`Source/Editor/Ornaments/OrnamentMarks.cpp:122`).
- **U-L4** — `PUBLIC` is what JUCE's CMake template uses; `PRIVATE` would split the JUCE settings between the plugin and the test targets that link it (`CMakeLists.txt:47-50`).
- **B-H3** — The NaN in `OrnamentSymbol` (`Source/Processor/Document/Ornaments/OrnamentSymbol.cpp:53`) is computed and then discarded by the length guard (`Source/Processor/Document/Ornaments/OrnamentSymbol.cpp:62`); not undefined behaviour. The bitmap is created as ARGB, so the cast matches its stride.
- **B-H4** — Contradicts self-check 2: only positive weights are pushed (`Source/Processor/Document/Ornaments/OrnamentFigure.cpp:389`).

## Suggestions, not defects

These are opinions about style, tooling or structure. They may be worth doing, but nothing is wrong without them.

- **U-M1** — Enable more clang-tidy checks and run clang-tidy in CI.
- **U-M2** — Set a `.clang-format` column limit and include sorting.
- **U-M13** — `nodeCanvas` and `titleBar` are public editor members (`Source/Editor/PluginEditor.h:128-130`); `applyRestoredState` reaching `nodeCanvas` is the documented one exception.
- **U-M14** — macOS-only paths and CI; the README says Windows and Linux are untested.
- **U-M15** — Several `NodeCanvas` loops over every node could share one helper.
- **U-M16** — Large files (`ValueField`, `RuleTreeView`, `TraversalRulesWindow`, `NodeCanvas`) could split when they next change.
- **U-L2** — `Titlebar` versus `titleBar`; linear dedup in `NodeCanvas::handleAsyncUpdate`.
- **U-L3** — One canonical build section across `README.md` and `Development.md`.
- **U-L5** — `TraversalRulesWindow::activePage` is a raw pointer, nulled correctly.
- **B-M2** — Linux and Windows CI, ASan / UBSan, `-Werror`. Its claim that the realtime tests do not run in CI is wrong: they run in their own sanitizer job.
- **B-M3** — Coverage outside traversal and script is thin. The file counts were not re-checked.
- **B-L1** — `[[nodiscard]]` / `noexcept` consistency.
- **B-L2** — Index loops where ranges read better.
- **B-L3** — The 24 `dynamic_cast` sites have not all been read.
- **B-L4** — `std::expected` at the state-load boundary.
- **B-L5** — `int` / `size_t` casts in the real-time layer.
- **U-L4** (second half) — `-Wno-unused-parameter` on every target.

## Duplicates

- **U-H5** is S-M4. **B-H1** is G-M20. **B-H2** is U-M8. **B-H5** is S-L9.

## Not yet checked

- **S-L2** (two diagnostics for a missing `}`), **S-L4** (stack high-water over-count), **S-L10** (parser leniencies). Also the unchecked halves named in S-L5, S-L6, S-L8 and G-M13, and the counts in B-L1 to B-L5.

## Suggested fix order

1. G-M16: copy `ArrowPitchOffset` on paste, with its test.
2. The owner's decisions: S-H1 (short-circuit or document), G-H6 (widen or change the header), and the questions under "Needs a test or the owner's decision".
3. Documentation: U-H4, S-M4, and the gaps listed under Open: Low.
4. S-M3: the duplicate-local diagnostic.
5. A-H2: measure before deciding.
6. C1: `continue` on an invalid child, as the sibling passes do (defensive).
7. Everything else under Open: Low, while touching those files anyway.

## Fixed / re-checked

- **B-M4** — The test target no longer lists plugin sources by hand: the `SequenceTreeCore` library, added in `cd0ec6a`, is linked by both the plugin and `SequenceTree_Tests` (`CMakeLists.txt:52`, `CMakeLists.txt:194`).

## Daily log

- **2026-11-17 (Day 1, commit `4600d21`):** First review. Five areas reviewed (audio and real-time, script, graph and traversal, UI and docs, build and tests); 130 findings, 7 of them Critical.
- **2026-11-17 (re-check, commit `cd0ec6a`):** Every finding re-checked against the code and re-cited at `cd0ec6a`. No Critical survives; one High (G-M16) and five Medium findings remain open. 29 findings are withdrawn and 9 are settled, each with the code that answers it. Four of the original fixes would have broken behaviour: C3's validation (refuses every new node), U-H4's drop to C++20 (breaks `std::expected`), A-H2's clamp (changes what preview plays) and U-L4's `PRIVATE` (splits the JUCE settings).
