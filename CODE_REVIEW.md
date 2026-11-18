# SequenceTree — Code Review

**Repo:** https://github.com/elibaumgardner/SequenceTree
**Commit reviewed:** `507ad4c` — "Drive the encapsulation and selection-box gesture tests through NodeController's mouse events…". The first draft was written against `4600d21` and re-checked at `cd0ec6a` on 2026-11-17. This update re-checks every finding against `507ad4c` and adds Day 2's findings. All of Day 2's fixes landed in `fdefb93`; `507ad4c` itself only reworks the gesture tests.
**Date:** 2026-11-18
**Scope:** Daily recurring review. Each day adds new findings, refines old ones, and re-checks earlier ones against the current code.

## At a glance

- 199 C++ files in `Source/` and `Tests/`, 35,508 lines. 90 `.cpp` files in `Source/`, 8 test files, 182 `TEST_CASE`s.
- 134 findings in total: 130 raised on Day 1, 3 new on Day 2, and 1 new unchecked item. Day 2 fixed 23 Day 1 findings — including the only High — and confirmed 2 of the 3 items Day 1 had not yet checked. No new High or Medium findings.

| Status | Count |
|---|---|
| Open: High | 0 |
| Open: Medium | 3 |
| Open: Low (includes documentation fixes) | 39 |
| Needs a test or the owner's decision | 8 |
| Settled by the owner, or not a defect | 9 |
| Withdrawn: did not survive re-checking | 29 |
| Suggestions, not defects | 16 |
| Duplicates (merged into another finding) | 4 |
| Fixed | 24 |
| Not yet checked | 2 |

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

None open. G-M16, the only High, was fixed in `fdefb93` (see Fixed / re-checked).

## Open — Medium

### S-H1 — `and` / `or` are not short-circuited
**Medium; the owner decides between the two fixes.** `emitBinary` emits the left operand, then the right, then one `LogicalAnd` / `LogicalOr` (`Source/Core/Script/ScriptEmitter.cpp:1034-1044`), so the right side always runs. The original examples do not compile: `advance` returns a `Node`, and `and` / `or` take numbers only (`Source/Core/Script/ScriptEmitter.cpp:1054`). What is affected is a helper with side effects on the right side (one that assigns a member, or calls `playNote` from `main`), as in `ready and bump()`. The user guide says nothing about evaluation order (`documentation/ScriptingLanguage.md:104`).
Fix: either short-circuit (jump past the right operand on `JumpIfFalse` / `JumpIfTrue`), which changes what existing scripts with such helpers do, or document eager evaluation. Test: `0 and bump()` leaves the member `bump` writes unchanged.

### G-H6 — Copying one member of a group does not copy the group, contrary to `Clipboard.h`
**Medium; the owner decides which side is right.** The header promises that copy first adds every member of any group one of the ids is in (`Source/Processor/Document/Clipboard.h:18-19`). `withEncapsulatedMembers` asks `memberIds` of each selected id (`Source/Processor/Document/Clipboard.cpp:354-372`), which returns members only when the id is the group node itself (`Source/Processor/Document/EncapsulationOps.cpp:131-143`). Copying one member therefore copies that node alone, and paste does not recreate its group.
Fix: either widen through the member's `EncapsulatorId`, or change the header to say that only a copied group node brings its members. Test: copy one member of a three-member group, paste, and check the result against whichever contract is kept.

### A-H2 — Preview with a high count repeats rule selection for each alternative host on the audio thread
**Medium, Suspected: undecided until measured.** In Preview mode `begin()` replays `advanceAlternative` up to `visitCount - 1` times for every node that has an alternative (`Source/Core/Traversal/TraversalLogic.cpp:63-77`), and the count field allows up to 9,999 (`Source/Editor/Bars/BottomBar.cpp:22`). Each pass runs one rule selection. The cost was not measured.
The original fix (clamp the count to 64) is rejected: the seeded counts decide which alternative is sounding, and counts are compared against limits up to 9,999, so a clamp changes what preview plays.
Settle it: time one preview at count 9,999 with 1, 10 and 50 alternative hosts. Only if that is too slow for one block, look for a way to seed the counts that gives the same alternatives.

## Open — Low

### Documentation gaps (all confirmed; fix in `documentation/ScriptingLanguage.md`)
- **S-L5** — The trailing `;` half, now checked: the class need not end with `;`. `Parser::run` skips terminators after the class body (`Source/Core/Script/ScriptParser.cpp:23-25`), and `;` lexes as a terminator (`Source/Core/Script/ScriptParser.cpp:301-303`), so `}` alone compiles. The guide still presents the class as "closed with `};`" (`documentation/ScriptingLanguage.md:19`). The base-class half is fixed: the guide now says the name is free and `: defaultTraversal` is optional.

### Script language
- **S-M3** — `int x = 1; int x = 2;` in one block compiles: `emitDeclare` and `declareLocal` never check the current scope (`Source/Core/Script/ScriptEmitter.cpp:223-256`, `Source/Core/Script/ScriptEmitter.cpp:785-790`). Fix: report "'x' is already declared" for a binding made since `scopeMarks.back()`; shadowing in a nested block stays legal.
- **S-M2** — The ban on `advance` / `playNote` inside `advance` checks only the function named `advance` (`Source/Core/Script/ScriptEmitter.cpp:931`). A helper bypasses it, but the nested call then returns `none` or does nothing, so this is a missing diagnostic, not a safety problem.
- **S-M7** — `-2147483648` cannot be written, because the lexer rejects `2147483648` (`Source/Core/Script/ScriptLexer.cpp:100-118`) before `parseUnary` folds the minus (`Source/Core/Script/ScriptParser.cpp:793-822`).
- **S-L2** — Checked on Day 2: a function body missing its `}` produces two diagnostics at the same place. `parseBlock` fails with "expected '}'" at the end of the file (`Source/Core/Script/ScriptParser.cpp:432`), the failure propagates out of `parseClassMember`, `recover()` stops at once because it is already at `End` (`Source/Core/Script/ScriptParser.cpp:311-315`), and `parseClassBody` then adds "expected '}' to close the class" at the same token (`Source/Core/Script/ScriptParser.cpp:160-163`). The guide's "every error is reported at once" does not promise one diagnostic per error, but two for one missing brace is noise. Fix: in `parseClassBody`, do not add the second diagnostic when the failure that just happened was already an "expected '}'".
- **S-L4** — Checked on Day 2: the stack high-water over-counts by one slot at each call site. `emit` raises the high water from `stackDelta(Call)`, which is +1 (`Source/Core/Script/ScriptEmitter.cpp:1149-1150`, `Source/Core/Script/ScriptEmitter.cpp:759-762`), and only afterwards does `emitCall` subtract the parameter count (`Source/Core/Script/ScriptEmitter.cpp:982`). At run time the call pops its arguments before anything is pushed, so the peak the emitter records (arguments + 1) never occurs. The over-count is conservative — it can only falsely reject a deeply nested expression, never overflow the real stack — so this stays Low. Fix: apply the parameter-count subtraction before the high-water update, or give `Call` a delta that knows the parameter count.
- **S-L8** — A member named `current` is refused (`Source/Core/Script/ScriptEmitter.cpp:34-37`), but a local of that name is not, and the same holds for `traversal`. Checked on Day 2: a member or local named `traversal` shadows the context value — `emitField` only treats `traversal.id` as the traversal id when no local, member or global of that name exists (`Source/Core/Script/ScriptEmitter.cpp:871-873`) — and a local named `current` shadows the context node the same way, since `emitName` checks locals first. Nothing about this is documented. Fix: refuse `traversal` as a member name like `current`, and either refuse both as locals or document that they shadow the context.
- **S-L3** — There is no cap on the size of a compiled script; only execution is capped (`Emitter::emit` appends with no limit, `Source/Core/Script/ScriptEmitter.cpp:767-772`).
- **S-L9** — The VM reads 0 for an out-of-range field, context or convert operand instead of faulting (`Source/Core/Traversal/ScriptRun.cpp:294`, `Source/Core/Traversal/ScriptRun.cpp:306`). Only hand-built bytecode can reach it; the compiler never emits one. (B-H5 merged here.)

### Audio and real-time
- **A-H1** — The script budget is per call (`Source/Core/Traversal/ScriptRun.cpp:27-60`), so one note can run a few budgets (`main`, `advance`, a trial peek, a modulator decision). The original's "64 trial runs per step" is wrong: `skipAhead` never runs a script, because the scripted path returns first (`Source/Core/Traversal/TraversalLogic.cpp:122-126`). Reaching a block overrun needs a script that spends its whole budget on every call, on many notes ending in one block.
- **A-H3** — The event cap is a count (`Source/Processor/Playback/EventManager.cpp:29`), and notes left over when it is hit fire at sample 0 of the next block (`Source/Processor/Playback/EventManager.cpp:71-73`). Needs over 4,096 events in one block.
- **A-M3** — `scheduleNote` drops notes past 1,024 sounding at once (`Source/Processor/Playback/NoteScheduler.cpp:21-23`). A drop counter would be cheap.
- **A-M7** — Pool and connection lookups are linear scans over small arrays, at most 128 slots (`Source/Core/Traversal/TraversalPool.h:81-100`). The 1,024-node cap of `NodeStateTable` is documented (`Source/Core/Traversal/NodeStateTable.h:20-22`).
- **A-M8** — Overflow of `previewRequests` (16 entries) and of the virtual-MIDI FIFO is set but never read (`Source/Processor/Channels/CommandFifo.h:42`). Needs 16 preview clicks within one block.
- **A-L1** — The orphan scan runs every block (`handleOrphanNotes`, `Source/Processor/Playback/EventManager.cpp:76`) and the flag scan every event (`startNextDue`, `Source/Processor/Playback/FlagScheduler.cpp:76`), even when nothing changed. Cheap to skip.
- **A-L3** — Record mode in `AudioUIBridge` drops entries past capacity without a flag and dedups linearly (`send`, `Source/Processor/Channels/AudioUIBridge.cpp:46`).

### Graph, traversal and state
- **G-H2** — `NodeStateTable` does not check its slot argument (`Source/Core/Traversal/NodeStateTable.h:30-44`); no caller passes an invalid one.
- **G-H5** — One corrupt child makes `StateJson` reject the whole document, with no error detail (`Source/Processor/Document/StateJson.cpp:60-68`). All-or-nothing is a safe failure; reachable only from a corrupt file. Refined on Day 2: the refusal is now deliberate and the user is told the patch could not be read (`Source/Processor/PluginProcessor.cpp:216`), but there is still no per-child detail — a patch with one bad node and a patch with a hundred both report the same sentence.
- **G-H10** — When a host and its active alternative both have an arrow to the same root, that root's cross-tree count advances twice in one step (`Source/Core/Traversal/TraversalLogic.cpp:174-257`). Two arrows from one host to one root cannot exist (`connectionFor` dedups by child id, `Source/Processor/AudioModel/RTGraphBuilder.cpp:769-781`), and pushing the root on every step of its switch phase is the switch mechanism.
- **G-H13** — Entry points in `GraphState` and `NodeFactory` check their arguments with `jassert` only (for example `Source/Processor/Document/Graph/GraphState.cpp:55-56`); every caller passes a valid parent.
- **G-H17** — `rebuildAllGraphs` rebuilds root graphs only (`Source/Processor/AudioModel/RTGraphBuilder.cpp:47-53`), so after an ornament change modulator-root graphs keep their old data. Whether a modulator node can carry an ornament was not checked.
- **G-H19** — `pasteAt` trusts `contents` (`Source/Processor/Document/Graph/Clipboard.cpp:65-83`); only `copy` fills it, with distinct ids.
- **G-M6** — The selection random value advances once per step (`Source/Core/Traversal/TraversalLogic.cpp:431-433`), so two weighted picks in one step with the same span pick the same rank.
- **G-M10** — An async update can publish twice: `updateDurationMaps`, then `publishQueuedGraphs` (`Source/Processor/AudioModel/RTGraphBuilder.cpp:250`, `Source/Processor/AudioModel/RTGraphBuilder.cpp:256`). Cost only.
- **G-M12** — `rememberOwners` drops a change whose tree has no owner, silently (`Source/Processor/AudioModel/RTGraphBuilder.cpp:706-729`).
- **G-M17** — Paste recreates a group only when every member was pasted (`Source/Processor/Document/Graph/Clipboard.cpp:398-403`).
- **G-M18** — `removeMember` does not reset the removed node's own `EncapsulatorId` (`Source/Processor/Document/Graph/EncapsulationOps.cpp:116-129`); harmless for its only caller, `removeNode`.
- **G-L4** — Note values are copied without range checks; the editor limits them. The original's "`fallbackValue` is unused" is wrong: it is used at `Source/Processor/Playback/NoteScheduler.cpp:32-33` and `Source/Processor/Playback/NoteScheduler.cpp:165`.
- **G-L6** — New on Day 2. `findNodeMapProblem` refuses an unknown node type and a duplicated id, but it accepts a node id that is missing, 0, negative, or `INT_MAX` (`Source/Processor/Document/Graph/GraphState.cpp:567-586`). A node with no `Id` property reads as id 0 (a `juce::var` with no value converts to 0; the JUCE submodule is empty in this clone, so that conversion was not re-read from source), and 0 is also the "no parent" sentinel, so the node is indexed under an id that means "no node". A node with id `INT_MAX` loads, and `replaceState` then sets `nodeIdIncrement` to it (`Source/Processor/Document/Graph/GraphState.cpp:387-395`), so the next node creation overflows `nodeIdIncrement + 1` (`Source/Processor/Document/Graph/GraphState.cpp:23`), which is signed overflow. Reachable only from a hand-edited patch. Fix: refuse ids below 1 in `findNodeMapProblem` (and, for completeness, above a sane maximum). Test: a patch with a node whose `Id` is `0`, `-5`, or `2147483647` is refused with a reason in `loadError`.

### UI, build and files
- **U-H3** — Paint buffers grow with the canvas, but sizes are in points. Corrected on Day 2: it is not true that only the active layer is allocated — `paintDensity` is one vector per layer (`Source/Editor/Paint/ValueField.h:80`), each sized lazily when its layer is first painted (`Source/Editor/Paint/ValueField.cpp:401`) and never freed, so painting in all three layers leaves all three allocated alongside `strokeMask`. The 132 MB figure is still overstated.
- **U-M3** — `makeNode` returns `nullptr` for an unknown tree type with no assertion (`Source/Editor/Canvas/NodeCanvas.cpp:435-470`).
- **U-M5** — `DisplayMode::Duration` is not offered by the display menu (`Source/Editor/Bars/Titlebar.cpp:185-190`), so its case in `Node` never runs. The real finding is that the mode is dead; its missing rebind is not reachable.
- **U-L6** — New on Day 2. Editor commands die at the OrnamentWindow instead of falling through to the editor. `EditorCommandManager::getFirstCommandTarget` returns the focused component's target for any command id without asking whether that target lists the command (`Source/Editor/EditorCommandManager.cpp:8-17`), `OrnamentWindow::perform` returns false for everything but undo and redo (`Source/Editor/Ornaments/OrnamentWindow.cpp:84-101`), and `OrnamentWindow::getNextCommandTarget` returns `nullptr` (`Source/Editor/Ornaments/OrnamentWindow.cpp:109-112`). So Space, Delete, Copy, Paste and Select All pressed while focus is inside the OrnamentWindow do nothing, where the same keys in the main window toggle playback or edit the graph. This is not a regression — before `fdefb93` no key listener was registered on that window at all, so those keys did nothing there either — but the new routing now receives the keys and drops the ones the window does not handle instead of letting them reach the editor. Fix: have `getFirstCommandTarget` skip a target that does not list the command, or have `OrnamentWindow::getNextCommandTarget` return the editor.
- **U-M9** — Node colour resolution is written three times (`Source/Editor/Canvas/NodeCanvas.cpp:117-121`, `Source/Editor/Canvas/NodeCanvas.cpp:207-210`, `Source/Editor/Nodes/Node.cpp:215-225`). The fourth site, in `AudioCommandDrainer`, resolves a traversal colour and is not the same code.
- **U-M10** — `Node::mouseMove` and `mouseExit` have the same body (`Source/Editor/Nodes/Node.cpp:191-211`).
- **U-M11** — The VBlank callback marks the frosted backdrop stale every frame (`Source/Editor/PluginEditor.cpp:64-74`), and some refreshes repaint the whole canvas. Cost not measured.
- **B-H6** — `createDirectory` results are unchecked (`Source/Processor/PluginDocument.cpp:80`, `Source/Processor/Document/Ornaments/OrnamentLibraries.cpp:33`).
- **B-M1** — Partly fixed in `fdefb93`: `SequenceTree_Tests` and `SequenceTree_GraphTests` now guard `-Wall -Wextra` with `if(MSVC)` / `/W4` (`CMakeLists.txt:194-197`, `CMakeLists.txt:212-216`), but `SequenceTree_RealtimeTests` still passes `-Wall -Wextra` with no MSVC guard (`CMakeLists.txt:230`), so that target would still break on an MSVC build. Fix: give it the same guard; it is the only test target without one.
- **B-L6** — New on Day 2. The patch autosave write is unchecked too: `getStateInformation` calls `patchFile.replaceWithData(...)` and ignores the result (`Source/Processor/PluginProcessor.cpp:197`), so a patch that fails to save — a read-only folder, a full disk — is silent, and the user is left believing the patch file is current. The state itself is still in the host's own save; only the `.sqtree` copy is at stake. Fix: report the failure the same way `loadError` reports a failed load, or at least `DBG` it. (Related to B-H6, which is the same omission on `createDirectory`.)

## Needs a test or the owner's decision

- **G-H12** — Plausible: a switch repeat returns before the group-loop diversion (`Source/Core/Traversal/ChildSelector.cpp:107-133`). Test: a group whose exit child has a switch limit above 1.
- **G-H16** — Children under a traversal flag are never built as RT nodes (`Source/Processor/AudioModel/RTGraphBuilder.cpp:487-489`), and the flag then starts nothing (`Source/Processor/Playback/FlagScheduler.cpp:182-186`). Open question: can the canvas grow a node out of a flag? `addChildNode` does not refuse one (`Source/Editor/Canvas/GraphDocument.cpp:23-38`).
- **G-M5** — The "invalid start node" half is wrong. The "begin twice" half holds: `prepareTraversal` reuses an ended run without resetting it (`Source/Processor/Playback/TraversalDispatcher.cpp:241-266`), called from `FlagScheduler` (`Source/Processor/Playback/FlagScheduler.cpp:192-205`) and from `startCrossTreeTraversal` (`Source/Processor/Playback/TraversalDispatcher.cpp:845-858`), so its counts and `subRootNode` carry over. Is that intended?
- **G-M11** — A child with two parents can get `parentId` 0 when its first parent is built after it (`Source/Processor/Document/GraphState.cpp:142-175`, `Source/Processor/AudioModel/RTGraphBuilder.cpp:462-484`). Test with a shared child under a modulator.
- **G-H8** — The `insertNodeAfter` half cannot happen (it is only given a newly created child). The `create` half can only happen if a node already in a group can be a span's first click (`Source/Processor/Document/EncapsulationOps.cpp:28-38`). Test that gesture.
- **G-M3** — After a traversal-arrow jump, Live mode sets the loop limit to endless instead of the new tree's limit (`Source/Core/Traversal/TraversalLogic.cpp:550-552`). The owner's call.
- **U-M7** — Partly answered on Day 2. The Rules window never registers the editor's command key mappings — only the main window (`Source/Editor/PluginEditor.cpp:130-142`) and the OrnamentWindow (`Source/Editor/Ornaments/OrnamentWindow.cpp:23`) add `commandManager.getKeyMappings()` as a key listener, and `TraversalRulesWindow` adds no key listener at all — so no editor shortcut can fire while focus is in the Rules window; Space types a space in its code editor. What still needs the runtime test is whether the code editor's own Cmd+Z undoes text there rather than the graph: that is JUCE's `CodeEditorComponent` behaviour, and the JUCE submodule is empty in this clone, so it was not re-read from source.
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
- **G-M13** — The node-map listener fires only for trees attached under `nodeMap`, so the parent chain it reads is fixed (`Source/Processor/AudioModel/RTGraphBuilder.cpp:90-143`). The `isAudioProperty` half was checked on Day 2 and is sound: every identifier in `NodePropertyTable::rows` is a property of a node tree (`Source/Processor/Document/TypedProperties.cpp:6-19`), and the one that also lives on a note tree, `MidiChannel`, still resolves to the right node because `rememberOwners` climbs parents until it finds a tree with a `RootNodeId` (`Source/Processor/AudioModel/RTGraphBuilder.cpp:709-729`).
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

- **S-L10** (parser leniencies). Also the `%`-on-`bool` half of S-L6, and the counts in B-L1 to B-L5.
- Whether a patch whose JSON starts with a UTF-8 byte-order mark loads: `StateJson::read` trims leading whitespace and then requires `{` (`Source/Processor/Document/StateJson.cpp:9-15`), and whether `trimStart` strips a BOM is JUCE behaviour the empty submodule in this clone could not answer.

## Suggested fix order

1. The owner's decisions: S-H1 (short-circuit or document), G-H6 (widen or change the header), and the questions under "Needs a test or the owner's decision".
2. S-M3: the duplicate-local diagnostic.
3. A-H2: measure before deciding.
4. G-L6: refuse out-of-range node ids at the load boundary, with its test.
5. U-L6: let editor commands fall through from the OrnamentWindow, or document that they do not.
6. Documentation: the trailing-`;` wording in S-L5, and the `traversal` / `current` shadowing in S-L8.
7. Everything else under Open: Low, while touching those files anyway.

## Fixed / re-checked

- **B-M4** — The test target no longer lists plugin sources by hand: the `SequenceTreeCore` library, added in `cd0ec6a`, is linked by both the plugin and `SequenceTree_Tests` (`CMakeLists.txt:52`, `CMakeLists.txt:194`).

Fixed in `fdefb93` (Day 2). Behaviour fixes each have a test added in the same commit or in `507ad4c`; documentation fixes are verified against the guide text.
- **G-M16** — `connectClipboardNodes` now copies the source arrow's `pitchOffset` when it is set (`Source/Processor/Document/Graph/Clipboard.cpp:137-147`), so a pasted pitch-bound arrow transposes on its first move exactly as the source does. Test: "a pasted pitch-bound arrow keeps its pitch offset, so moving the pasted child transposes it as it does the original" (`Tests/Integration/GraphTests.cpp`).
- **C1** — `createRTNodes` now skips an invalid child with `continue` instead of asserting (`Source/Processor/AudioModel/RTGraphBuilder.cpp:493-497`), as the two sibling passes do.
- **S-H2** — A trial run that outgrows its write table now faults: `writeState` sets `trial.overflowed` (`Source/Core/Traversal/ScriptRun.cpp:583`) and `call` returns -1 when it is set (`Source/Core/Traversal/ScriptRun.cpp:62`), and `peekNextTarget` treats that -1 as "no peek" via `nodes.find` returning null (`Source/Core/Traversal/TraversalLogic.cpp:329-336`). A committed run is unaffected. Test: "a trial run that writes more nodes than its table holds faults, and a committed run does not" (`Tests/Unit/ScriptCompilerTests.cpp`).
- **G-H9** — `WeightedPick::tally` and `accepts` clamp a negative probability to 0 (`Source/Core/Traversal/WeightedPick.cpp:8`, `Source/Core/Traversal/WeightedPick.cpp:38`). Tests: the parity walk test and "a negative probability weighs the same as a probability of 0" (`Tests/Unit/TraversalTests.cpp`).
- **G-L2** — `NodeStateTable::increment` stops at `INT_MAX` instead of overflowing (`Source/Core/Traversal/NodeStateTable.cpp:62-68`). Test: "incrementing a count already at the largest int leaves it there" (`Tests/Unit/TraversalTests.cpp`).
- **G-M9** — `Choices::Fixed` is gone: the enum, the `choices` member and the branch in `acquire` were removed (`Source/Core/Traversal/TraversalPool.h:115-118`, `Source/Core/Traversal/TraversalPool.h:200-214`), so there is no dead mode left. Correction to Day 1's reasoning on the duplicate-`runId` half: `acquire` does not check for an existing runId — it takes the first free slot (`Source/Core/Traversal/TraversalPool.h:103-120`) — so the Day 1 claim that `acquire` covers it was wrong. No caller can produce a duplicate in practice: `prepareTraversal` finds by runId before acquiring (`Source/Processor/Playback/TraversalDispatcher.cpp:243-247`), and the other callers draw fresh ids from `nextRunId()` (`Source/Core/Traversal/TraversalPool.h:189`, `Source/Processor/Playback/TraversalSession.cpp:446`, `Source/Processor/Playback/TraversalSession.cpp:525`).
- **G-M14** — `StateJson::toElement` now rejects a `properties` value that is not an object and a `children` value that is not an array (`Source/Processor/Document/StateJson.cpp:53-57`). Test: "a state whose properties or children are not an object and an array is not read" (`Tests/Integration/GraphTests.cpp`).
- **G-M19** — `getArrowInfo` range-checks the stored type and bindings and falls back to the defaults for out-of-range values (`Source/Processor/Document/Graph/ArrowBindingOps.cpp:143-165`). Test: "an arrow type or binding outside its enum reads as the default" (`Tests/Integration/GraphTests.cpp`).
- **G-M20** — `StateJson::read` measures the JSON nesting depth before parsing and refuses anything deeper than 256 (`Source/Processor/Document/StateJson.cpp:13-17`, `Source/Processor/Document/StateJson.cpp:83-114`), so `toElement`'s recursion is bounded. Tests: the 300-level deep state is refused, the 10-level one reads, and brackets inside string values are not counted as nesting (`Tests/Integration/GraphTests.cpp`). Note: the pre-check covers the JSON form only; the legacy binary form still goes through JUCE's XML parser, whose depth handling was not checked (JUCE submodule empty).
- **S-M6** — The lexer now reports a decimal too large for `double`: `strtod`'s result is checked with `std::isinf` (`Source/Core/Script/ScriptLexer.cpp:127-128`). Test: "a decimal literal too large for a double is reported" (`Tests/Unit/ScriptCompilerTests.cpp`).
- **U-M8** — A failed load is no longer `DBG`-only: `setStateInformation` leaves the reason in `loadError` (`Source/Processor/PluginProcessor.cpp:203-254`) and opening a patch shows it in an alert (`Source/Editor/Bars/PatchControls.cpp:107-108`). Tests: refused states leave their reason in `loadError`, a later good state clears it, and a refused file leaves the open patch untouched (`Tests/Integration/GraphTests.cpp`).
- **U-H4** — The README badge now says C++23 (`README.md:9`).
- **S-M4** — The user guide now says members and out-of-class variables can take starting values, that they run in declaration order with globals first, and that reading a variable not set up yet gives 0 or `none` (`documentation/ScriptingLanguage.md:40-48`).
- **S-M1** — The guide now says `advance(n)` above 64 counts as 64 and below 1 stays put (`documentation/ScriptingLanguage.md:127`).
- **S-M9** — The guide now says a step plays one note and the last `playNote` wins (`documentation/ScriptingLanguage.md:128`).
- **S-L6** — `bool` is in the Types table (`documentation/ScriptingLanguage.md:63`).
- **S-L7** — The guide now says a decimal is digits, a point, then more digits, and that `1.` and `1e10` are not decimals (`documentation/ScriptingLanguage.md:70`).
- **S-M8** — The guide now says each run of `main` or `advance` gets at most 8,192 instructions, counting every helper it calls (`documentation/ScriptingLanguage.md:166`).
- **S-L1** — The guide now says every error inside the class is reported at once, and that an unknown import or a malformed class header stops the check at that point (`documentation/ScriptingLanguage.md:175`).
- **A-M4** — `Architecture.md` now says the virtual MIDI port is best effort: messages are stamped with the wall-clock time their block started, not a sample position (`documentation/Architecture.md`, section 2.4).
- **C4** — A saved state is now validated at the load boundary before `replaceState` runs: `setStateInformation` refuses an unreadable state, a newer schema version, a non-patch, and a node map with an unknown type or duplicated ids, leaving the reason in `loadError` (`Source/Processor/PluginProcessor.cpp:203-254`, `Source/Processor/Document/Graph/GraphState.cpp:567-586`). `replaceState` itself still does not validate, but its only production caller is `PluginDocument::restore` from `applyRestoredState`, which only ever sees a state that passed that boundary.
- **G-H14** — An unknown node type in a saved patch is now refused at the load boundary (`Source/Processor/Document/Graph/GraphState.cpp:574-578`), so it can no longer reach `kindOf`'s map-to-`Node` fallback. Test: "a state with a node type that does not exist is not loaded" (`Tests/Integration/GraphTests.cpp`).
- **G-H15** — Two nodes with one id in a saved patch are now refused at the load boundary (`Source/Processor/Document/Graph/GraphState.cpp:580-582`), so `indexNode` can no longer be fed a duplicate from a file. Test: "a state with two nodes sharing an id is not loaded" (`Tests/Integration/GraphTests.cpp`).

## Daily log

- **2026-11-17 (Day 1, commit `4600d21`):** First review. Five areas reviewed (audio and real-time, script, graph and traversal, UI and docs, build and tests); 130 findings, 7 of them Critical.
- **2026-11-17 (re-check, commit `cd0ec6a`):** Every finding re-checked against the code and re-cited at `cd0ec6a`. No Critical survives; one High (G-M16) and five Medium findings remain open. 29 findings are withdrawn and 9 are settled, each with the code that answers it. Four of the original fixes would have broken behaviour: C3's validation (refuses every new node), U-H4's drop to C++20 (breaks `std::expected`), A-H2's clamp (changes what preview plays) and U-L4's `PRIVATE` (splits the JUCE settings).
- **2026-11-18 (Day 2, commit `507ad4c`):** Re-checked every finding against `507ad4c`. 23 Day 1 findings are fixed, all in `fdefb93`, each with a test: the only High (G-M16, pasted pitch offset), the load-boundary validation (C4, G-H14, G-H15, G-M14, G-M20, U-M8), the trial fault (S-H2), the clamps (G-H9, G-L2), the range checks (G-M19), the decimal-overflow diagnostic (S-M6), the dead code (G-M9), and the documentation (U-H4, S-M4, S-M1, S-M9, S-L6, S-L7, S-M8, S-L1, A-M4, plus the base-class half of S-L5). B-M1 is only partly fixed — `SequenceTree_RealtimeTests` still has no MSVC guard. 3 new Low findings: G-L6 (the validator accepts a missing, zero, negative or `INT_MAX` node id), U-L6 (editor commands die at the OrnamentWindow), B-L6 (the patch autosave write is unchecked). 2 items Day 1 had not checked are now confirmed (S-L2, S-L4); S-L10 and the BOM question remain unchecked. No High findings are open; 3 Medium remain (S-H1, G-H6, A-H2). Two Day 1 claims were corrected after Greptile's review of PR #1, each verified against the code: the G-M9 duplicate-`runId` reasoning (`acquire` takes the first free slot without checking; duplicates are prevented by the callers, not by `acquire`) and the U-H3 layer-allocation claim (all three `paintDensity` vectors stay allocated once painted).
