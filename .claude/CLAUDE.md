# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Build (debug)
cmake --build cmake-build-debug

# Build (release)
cmake --build cmake-build-release
```

`cmake-build-debug` and `cmake-build-release` are CLion's build directories, and they are the only ones. Terminal builds, CLion's run buttons and `scripts/run-in-live.sh` all share them on purpose: `COPY_PLUGIN_AFTER_BUILD` installs over the system plugin, so a second build tree means the installed AU and VST3 silently come from different builds. Never configure a new one (`cmake -B build`) — build into these. The one exception is a RealtimeSanitizer tree (see Tests), which needs a different compiler, never installs the plugin, and belongs outside the repo.

They use Ninja, so the config is fixed when the directory is configured and `--config` on the build line does nothing. CLion creates them; only a fresh clone needs them configured by hand, and Ninja is not on `PATH`, so the bundled one must be named:

```bash
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_MAKE_PROGRAM=/Applications/CLion.app/Contents/bin/ninja/mac/aarch64/ninja
```

`juce_add_plugin` is called with `COPY_PLUGIN_AFTER_BUILD TRUE`, so successful builds automatically install the plugin to system AU/VST3/Standalone locations. It must stay a keyword argument to `juce_add_plugin`: JUCE declares the property `INHERITED` and defaults the global to `FALSE`, so a plain `set(JUCE_COPY_PLUGIN_AFTER_BUILD ON)` is silently ignored and builds stop installing. The JUCE submodule must be initialized (`git submodule update --init`) before building.

Every `.cpp` must be listed explicitly in `CMakeLists.txt` — there is no glob. Adding a source file without registering it fails at link time, or silently does nothing.

Verify changes by building (`SequenceTree_Standalone` is the fastest full-link target) and by exercising the plugin manually in a host. Prioritise exercising it manually for audio engine work.

## Tests

Catch2 v3 is a submodule at `Catch2/`, next to `JUCE/`, and `Tests/` holds two test targets that build into the same `cmake-build-debug` tree:

```bash
cmake --build cmake-build-debug --target SequenceTree_Tests SequenceTree_GraphTests
(cd cmake-build-debug && ctest --output-on-failure)
```

- `SequenceTree_Tests` compiles the traversal core and the script compiler directly — `TraversalLogic`, `TraversalRule`, `ScriptTraversalRule`, `NodeStateTable` and `Source/Script/*` are JUCE-free — so it builds in seconds and never touches the installed plugin. Tests build `NodeMap`s by hand. `TraversalTests.cpp` holds the walk-parity check the model section asks for: `mirrorAsModulators` turns a node tree into the same shape of modulators, and the modulator walk must unfold the same sequence as the node traversal. A new traversal mechanic gets a shape there. `ScriptCompilerTests.cpp` covers diagnostics, the step budget, and the default script agreeing with `NativeTraversalRule`.
- `SequenceTree_GraphTests` links the plugin's shared code and builds graphs the way the UI does, through `NodeFactory` and `GraphState`, then walks what `RTGraphBuilder` published. It is what covers geometry being data: arrow length → duration, a same-X child becoming a chord link, pitch bindings shifting around the typed pitch, and creation defaults. Its `main` holds the `ScopedJuceInitialiser_GUI`.
- `SequenceTree_RealtimeTests` exists only when `SEQUENCETREE_REALTIME_SANITIZER` is `ON`. The option builds everything with `-fsanitize=realtime`, and `processBlock` is declared `noexcept [[clang::nonblocking]]`, so any allocation, lock or blocking call reached from it aborts the test with a stack trace. It plays a graph through a fake host playhead while editing, retiming, relocating, resetting and stopping. Apple clang has no RealtimeSanitizer, so the option needs Homebrew LLVM in its own tree outside the repo — it turns `COPY_PLUGIN_AFTER_BUILD` off, so that tree can never install over the system plugin:

  ```bash
  cmake -S . -B <outside-the-repo> -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
    -DCMAKE_MAKE_PROGRAM=/Applications/CLion.app/Contents/bin/ninja/mac/aarch64/ninja \
    -DSEQUENCETREE_REALTIME_SANITIZER=ON
  cmake --build <outside-the-repo> --target SequenceTree_RealtimeTests && <outside-the-repo>/SequenceTree_RealtimeTests
  ```

  The option also passes `-fdelayed-template-parsing`, because upstream clang 23 rejects an uninstantiated constructor template in JUCE 8.0.10's `juce_AudioPluginInstance.h`. The test pre-sizes its `MidiBuffer` the way the AU and VST3 wrappers do.

Hand-built `RTNode`s must set `subLoopCountLimit` and `switchCountLimit` to 1, as `GraphState` does, and a `TraversalLogic` needs `nodeState.prepare()` before use — `RTNode`'s own zero defaults arm a sub-loop on every node, and an unprepared table drops every count into its sink. Test files follow the Key Design Rules like any other source, and every one is listed explicitly in `CMakeLists.txt`. The tests reach graph building and traversal walking only; MIDI timing, UI and plugin-format behaviour still need a host.

## Refactoring Commands

`refactor.encap`, `refactor.decap`, `refactor.replace`, `refactor.rewrite`, `refactor.const_exper`, `refactor.undo`, `refactor.smell`, `refactor.find` and `refactor.help` are the refactoring suite. They live in their own repository, `~/Documents/GitHub/refactor-tools` - changes to the tools are made there, never in this project - and are on `PATH` through an editable `pip install`, reading this project's settings (source tree, build target, test targets, gate scripts) from `.claude/refactor.toml`. `refactor.help` lists them and `refactor.help <command>` prints one command's arguments. Every one carries the `refactor.` prefix so that none shadows a command already on `PATH`. Lifting a selection into its own function, dissolving a function into its call sites, renaming a variable, and putting any of them back are performed by those commands, not by hand. A rename is never a search-and-replace: `refactor.replace` takes the selection written out again with the new names in it and asks clangd which symbol each name is, so the variable is renamed where it is that variable and nowhere else. `refactor.rewrite` is structural search and replace: a C++ pattern with `$NAME` placeholders (one node) and `$$$NAME` runs, matched against the syntax tree rather than the text and rewritten through a template, with `--where` narrowing by token, node kind or libclang type — a mechanical rewrite across many sites goes through it, not through a one-off script. `refactor.const_exper` (or `refactor.constexpr`) scans scoped files and automatically converts constants, defaulted/member-init constructors, comparison operators, and pure inline/static member functions into `constexpr`, verifying every candidate with the standalone build. `refactor.smell` and `refactor.find` only report: `refactor.smell` names the wrappers and accessors clang can see, and `refactor.find` lists the things of one kind that match a condition, as in `refactor.find function 'numlines > 10'` — quoted, or the shell reads the `>` as a redirect. See `~/Documents/GitHub/refactor-tools/REFACTORING.md` for what each command is and what it does.

### Reaching for them

```bash
refactor.smell Source/Audio                      # wrappers and accessors, AST-accurate
refactor.find function 'numlines > 80' Source/Audio
refactor.find repeating 'numlines > 3' Source/Audio          # identical tokens
refactor.find repeating shape 'numlines > 3' Source/Audio    # identical structure, any names
refactor.encap Source/Audio/Foo.cpp:112-147 <name>
refactor.decap Foo::bar                          # also deletes a function with no call sites
refactor.replace 'while (point < text.size()) {' # the selection retyped; renames the symbols, not the text
refactor.const_exper Source/Audio/TraversalLogic.h # automatically applies constexpr where valid
refactor.rewrite '($T) $X' 'static_cast<$T>($X)' all --dry-run   # structural, $X one node, $$$X a run
refactor.undo
```

Duplication questions go to `refactor.find repeating`, both likenesses, before reading files by hand — it reads the whole scope in about half a second and beats grepping for a remembered line.

Two things about it decide whether the answer is any good:

- **Start the threshold low and read upward.** `numlines > 3` first; a high threshold silently hides the shorter half of a finding, and there is no indication that it did. `count >= 3` asks the other question — what has been written three times over.
- **`shape` relaxes spelling, not structure.** `shape_of` in the tools' `refactor/reporting/find.py` spells every identifier as the one token `name`, so `spawnKey` is one token and `traversal.key` is three, and `obj.f(x)` and `f(x)` differ by a receiver. Both likenesses report *contiguous* runs, so two functions that do the same thing with different expressions plugged in come back as several short islands rather than one long finding. Read adjacent findings in the same pair of files as possibly one duplicate, and go read the sites before reporting a size.

## Analysis, Reviews and Proposals

Gemini is the project's research analyst: it audits the codebase and researches outside it, following the research-suite's analyst brief and `analysis` skill (`gemini/ANALYST.md` and `gemini/analysis/SKILL.md` in the plugin) plus this project's profile in `.gemini/GEMINI.md`, and writes reports but never code. The skill fixes the finding format `/review` checks against — location at a recorded commit, confidence level, mechanism, evidence, and what would settle an unverified claim. Claude is its advisor. Findings reach the code in four stages, and each stage reads only what the previous one produced:

```
.claude/context/
├── GeminiAnalysis/
│   ├── Unreviewed/{ProgramAudits,ResearchReports}/   Gemini writes here
│   └── Reviewed/{ProgramAudits,ResearchReports}/     /review writes here
└── Proposals/
    ├── Unimplemented/                                /propose writes here
    └── Implemented/                                  /implement files here
```

- The skills, the gates, the analyst brief and every hook live in the `research-suite` plugin, its own repository at `~/Documents/GitHub/research-suite` — changes to them are made there, never in this project — enabled for this project in `.claude/settings.json`. Plugin skills are namespaced, so the stages run as `/research-suite:review`, `/research-suite:propose`, `/research-suite:implement` and `/research-suite:research`. Hooks run the plugin's cached copy, so after changing the repository bump `version` in its `plugin.json` and run `claude plugin update research-suite@research-suite --scope project`.
- **`ProgramAudits`** hold findings about this codebase; **`ResearchReports`** hold outside research mapped back onto it. The split is the same on both sides of review.
- **`/review`** (the `review` skill) reads one report from `Unreviewed/` as an academic advisor would: it breaks the report into claims, verifies each against the code at `HEAD` under the systematic principles and the Key Design Rules, gives each a verdict (Confirmed, Corrected, Stale, Refuted, Unverified), critiques the method and reasoning, and writes the corrected report — advisor review first — to the matching folder in `Reviewed/`, then deletes the original. Its *What Survives* section is the only part of a report that later work builds on.
- **`/propose`** (the `proposal` skill) turns surviving findings into `Proposals/Unimplemented/<topic>.md`: the mechanism with real call sites, the current API surface, structure and data flow, the approaches considered, and a numbered plan whose steps each carry their changes, behaviour delta, real-time safety argument, verification and rollback. Open choices go under *Decisions for the Owner*.
- Nothing in `Unreviewed/` is trusted — Gemini's line numbers drift and some of its claims are wrong or already fixed. A proposal is never built on an unreviewed report.
- **`/implement`** (the `implement` skill) builds one proposal from `Unimplemented/`. Its job is implementation, not verification: `/review` and `/propose` already did that, so it only checks whether the files the plan touches have changed since the proposal was written, and asks if the code it depends on has changed shape. It then carries out the plan one step at a time, and when every step is done it moves the proposal to `Implemented/` with implementation notes. An implemented proposal is never implemented again.
- `/review` and `/propose` never edit `Source/`; only `/implement` does, and only after the owner has approved the proposal and answered its Decisions for the Owner.
- `.claude/notes/ongoing-issues.md` stays the record of confirmed and resolved problems; reviews cite it rather than rediscovering what it already holds.

## Architecture Overview

SequenceTree is a JUCE plugin that generates MIDI by traversing a user-designed directed graph. Users create nodes, assign MIDI note data and a "count limit" to each, then the plugin walks the graph during playback — when a node's counter reaches its limit, traversal advances to matching children.

Five things about the model are easy to miss:

- **Every walk is the same traversal.** The primary walker, the modulator walk (`TraversalLogic::ModulatorWalk`), alternatives and a tree that is stepped into all unfold by one mechanic: a node's count advances each time it is left, children are chosen by their count limits, a child's switch count holds its parent on that child for that many visits, and sub-loop and trigger limits work the same way. What differs is only what drives the step — the primary walker steps when its own note ends, the modulator walk steps once per primary note played under its host (or on the host itself when the modulator arrow is unsynced), each keeping its counts in its own slot (`Count`, `ModulatorCount`). So a modulator's counts depend on how often the other modulator nodes have been played, exactly as a node's depend on the other nodes. Any behaviour that exists in one walk and not another — a node picking itself as its own switch target, a limit honoured in one walk and ignored in the other — is a bug, never a feature of that walk; fix it for every walk, and verify a traversal change by running the same graph shape as a node traversal and as a modulator walk and comparing the sequences.
- **Arrow geometry is data.** A connection's note duration is derived from the vector between the two node centres (`RTGraphBuilder::fillDurationMap` → `arrowDurationFromDelta`). Dragging a node retimes the sequence, which is why `NodeMoved` triggers `updateDurationMap`.
- **Duration is derived, pitch is owned.** Duration is recomputed from geometry on every read; pitch is not. A pitch-bound arrow only ever *shifts* the pitch of the node it points at, and the semitone amount it has already contributed is stored on the arrow as `ArrowPitchOffset`. `ArrowBindingOps::syncPitchBindings`, run by `GraphState::setNodePosition`, applies the difference between that stored amount and the current geometry, so moving a node transposes it around whatever pitch the user last typed, and no arrow ever recomputes a node's pitch from its parent.
- **The standalone runs its own transport; the plugin follows the host's.** In the standalone app, playback runs off the play button (`Titlebar` → `NodeCanvas::setProcessorPlayblack` → `SequenceTreeAudioProcessor::isPlaying`). In AU and VST3, `followHostTransport` sets `isPlaying` from the host's play state every block and treats host position 0 as the start of the walk. When the host starts playing or its playhead moves — while playing (its PPQ differs from the one expected after the last block) or while stopped (its PPQ differs from where the walk sits) — `TraversalSession::beginReplay` restarts the walk from the first root and `continueReplay` silently runs the event loop forward to the new position — at most a quarter of each block's time per block, so a far jump catches up over several blocks — then delivers the canvas state the replay recorded. A note already in progress at the new position stays silent, so the first thing heard is the next node starting on time; only the note-ons emitted exactly at the landing point (the replay clears `replayMidi` after each chunk, so they are what is left in it) are sent, and only if the transport is playing — which is how a play from position 0 still sounds the root. A stopped host therefore still sees the canvas follow its playhead, with in-flight arrows frozen at the right fraction. Positions are compared in beats, and every duration scales with 1/BPM, so a replay lands on the same graph state as playing through, whatever the host tempo. Arrow timing also follows a tempo change mid-note: `EventManager::followTempo` rescales every active note and pending flag delay when the global tempo differs from the last block's.
- **The plugin is an instrument that emits only MIDI.** `IS_SYNTH TRUE` makes it an AU `aumu` and a VST3 `Instrument|Synth`, so it has a stereo output bus and no audio input — the `#if ! JucePlugin_IsSynth` guards in `PluginProcessor`'s constructor and in `isBusesLayoutSupported` compile the input bus and its matching layout check away. Nothing ever writes audio, so `processBlock` clears `buffer` and `midiMessages` once at the top, before the pending note-off flush and every other MIDI writer, making the block's MIDI output entirely freshly generated.

### Directory Responsibilities

**`Source/Plugin/`** — JUCE entry points
- `PluginProcessor` is the `AudioProcessor`: transport state, plugin state save/restore, and the per-block sequencing in `processBlock`. Traversal lifecycle lives in `TraversalSession`, snapshot publication in `AudioSnapshotPublisher`, not here.
- `AudioSnapshotPublisher` owns everything the audio thread reads: the `Snapshot` type, the published pointer, and the deferred reclamation. Every mutation goes `beginEdit()` → change one member → `publish()`, so the rule that all other members are carried forward lives in one function; `publishGraph`, `publishScript` and `publishActiveTraversalRule` are the three callers plus `RTGraphBuilder::updateDurationMaps`. It sits in `Source/Plugin/` because it depends on `Graph`, `Audio` and `Script` at once.
- `PluginEditor` constructs every UI component and populates the shared `ApplicationContext`.

**`Source/Audio/`** — Audio thread
- `TraversalSession` owns the `TraversalPool` and the traversal lifecycle: starting, restarting, syncing against graph edits, and stopping traversals. `processBlock` drives it through `silenceAllNotes` / `suspendActiveNotes` / `restartActiveTraversals` / `syncWithGraph` / `startTraversalsFromFirstRoot`, and through `beginReplay` / `continueReplay` for host relocations. While `playback` is `Replaying`, output goes to its own pre-sized `replayMidi` and the bridge records commands instead of delivering them.
- `TraversalPool` is a fixed-capacity slot array (128 traversals), preallocated in `prepare()` so acquiring a traversal never allocates. Each slot is an `Instance { TraversalLogic logic; TraversalRuntime runtime; }` — `runtime` carries spawn origin (flag / cross-tree), `pendingRemoval`, and `repeatCount`.
- `TraversalLogic` is one running traversal instance — pure graph walking over `NodeMap`, returning a `StepResult`. No MIDI, no UI.
- `NodeStateTable` is per-traversal node state (visit counts, trigger counts, active alternative, last chosen child) as a flat `vector<int>` indexed by slot × row. A `NodeRowMap` hands each node ID a row on its first write, so any ID works and the cap is 1024 distinct nodes per traversal between resets; past that, writes land in a sink rather than growing the table.
- `TraversalRule` is the child-selection strategy. `RuleContext::eligibleChild` does all the filtering (count limits, trigger limits, zero-duration arrows, per-traversal disables); the rule only picks among survivors. `NativeTraversalRule` is the built-in policy; `ScriptTraversalRule` executes a user script instead.
- `TraversalDispatcher` turns traversal steps into MIDI events and UI commands (chords, modulators, cross-tree jumps, traversal flags).
- `NoteScheduler` writes note-ons into the `MidiBuffer` and keeps each sounding note in `activeNotes`, counting down its remaining samples before sending the note-off. A `NoteVoicing` per note applies transpose, velocity scaling, and pitch or velocity overrides.
- `AudioUIBridge` is the only channel from audio to UI: three lock-free `juce::AbstractFifo`s (highlight, arrow, count) carrying plain command structs. Its push functions are the class's whole reason to exist, so they stay short by design and are declared in `core_purpose_api`. A trail reset is an `ArrowKind` on `ArrowCommand`, not a fourth queue, and `HighlightKind::ClearEveryNode` is how the audio thread blanks the canvas. `delivery` (`CommandDelivery::Deliver` / `Record`) is one switch in front of all three FIFOs. While a replay runs it is `Record`: `beginRecording` empties three fixed tables and each command folds into the net canvas state — highlights by node and run, arrows by parent, child and trail stamped with `recordClockMs`, counts by node — and `deliverRecording` pushes a clear, a trail reset and that state, each arrow carrying `elapsedMs` so `ArrowAnimation::startTrail` places it partway (measured from `pausedAtMs` while paused). The audio thread only ever writes these FIFOs and atomics — it never posts a message — and the editor polls them (see Audio → GUI).
- `EventManager` bundles bridge + scheduler + dispatcher and runs the per-block event loop.

**`Source/Script/`** — The traversal scripting language
- `RTScript` is the bytecode a traversal rule runs, and the compiler's only output: a stack VM with `maxLocals` 32, `maxStack` 64, and a `stepBudget` of 8192 — the budget is what stops a user-authored loop from hanging the audio thread. It lives here rather than in `Audio/` because everything that *produces* it is here; `Audio/` only executes it, through `ScriptTraversalRule`.
- The pipeline is one strictly linear pass — `Lexer` (source → tokens) → `Parser` (tokens → AST) → `Emitter` (AST → bytecode) — split across `ScriptLexer`, `ScriptParser` and `ScriptEmitter`, all inside `namespace script`. It never runs on the audio thread; only the compiled bytecode crosses over.
- `ScriptCompiler.{h,cpp}` is the public face: `ScriptDiagnostic`, `ScriptCompileResult`, and a `compileTraversalScript` that constructs each phase locally and destroys it after one `run()`. The phases are deliberately *not* members — a phase never outlives a compile, so stale per-run state stays impossible rather than becoming a reset you have to remember. `ScriptCompiler.h` names none of them, so the AST and the emitter's internals stay out of `PluginProcessor.h`.
- The language has `let`, `if` / `else`, `for … in`, `while`, `break`, `continue`, `return`, and `and` / `or` / `not`. Statements are terminated by `;`, and a `}` ends a block statement on its own — newlines are whitespace, so an expression may span as many lines as it likes. Comments run from `//` to either a closing `//` or the end of the line. Scripts read the graph through fixed field tables — `parent.{id,count,childCount,lastChild}`, `child.{id,eligible,limit,triggerLimit,triggerCount,visits,repeat,pitch,switchLimit,subLoopLimit}`, `children.count`, `traversal.id` — and return a child ID or `-1`.
- Failures come back as `ScriptDiagnostic { message, line, column, length }`, so the editor can mark the offending line.
- `defaultTraversalScriptSource()` is the language-level equivalent of `NativeTraversalRule`.

**`Source/Graph/`** — Data model
- `GraphState` creates and mutates the graph's `ValueTree`: adds and removes all node types, connects and disconnects them, and writes node properties from argument structs (`setNodePosition`, `addMidiNote`). It is a `ValueTree::Listener` on its own `nodeMap`, and that listener is the only thing that maintains `nodeIndex` (id → node) and the public `parentIdsOf` (child id → parent ids). Because the index is fed by tree callbacks rather than by the mutators, undo, redo and paste keep it correct for free, and `getNode` / `getNodeParent` are constant time instead of scans. `getNodeParent` walks up *through* `TraversalFlagData` parents, so a node hanging off a flag chain still reports the note node the chain belongs to.
- `GraphState` composes three parts, reached through it rather than forwarded by it — callers write `graphState.encapsulation.dissolve(...)`, and `GraphState` grows no wrapper for them:
  - `EncapsulationOps` creates, dissolves and removes encapsulation groups and answers `memberIds`. `GraphState::removeNode` dispatches into it for `EncapsulatorData` nodes and it calls `removeNode` back for each member, so the two are mutually recursive by design.
  - `ArrowBindingOps` is the pitch half of "arrow geometry is data": the `ArrowInfo` ↔ `ValueTree` marshalling (`setArrowInfo` / `getArrowInfo`, both `static`) plus `syncPitchBindings` and `clearArrowDurations`.
  - `TraversalState` owns the `traversalMap` document as `map`, creates `TraversalData`, and answers which traversal keys are equipped anywhere in the graph. It has no `replaceState`: restore interleaves both documents and `TraversalMenu`'s listener stays attached to `map` throughout, so `GraphState::replaceState` still drives the whole sequence.
- `TraversalRuleState` owns the traversal rule scripts and which one is active — a separate document from the graph, sharing only the save file.
- `RTData.h` defines `RTNote` / `RTtraversal` / `RTNode` / `RTConnection` and `NodeMap` — plain structs safe to hand to the audio thread. `NodeMap` holds every node by value in one vector, `sortedById`, and `find` returns a `const RTNode*` or `nullptr`. Anything that fills `sortedById` keeps it sorted: `RTGraphBuilder::freezeNodes` sorts, and `AudioSnapshotPublisher::publishGraph` merges a rebuilt graph into the published nodes with one sorted `set_union` that also drops the graph's stale nodes.
- `RTGraphBuilder` builds `RTGraph`s from the `ValueTree`: `makeRTGraph` rebuilds one tree, `rebuildAllGraphs` rebuilds all of them, `updateDurationMaps` recomputes arrow durations for given node ids without a rebuild. Each finished graph is frozen into a sorted `NodeMap` and handed to `AudioSnapshotPublisher::publishGraph`, which merges it into the published nodes. It is also the `ValueTree::Listener` on `nodeMap` and `traversals.map` that keeps the audio model in step with the graph: tree changes collect into `PendingChanges`, and its `handleAsyncUpdate` rebuilds or discards each touched root and refreshes duration maps. It never writes to the document: the derived writes a move owes — clearing arrow duration overrides and syncing pitch bindings — are made by `GraphState::setNodePosition` and `NodeFactory`'s dangling-arrow functions inside the gesture's own undo transaction, so undo and redo restore them exactly and never gain a step of their own. It is owned by the processor, so graph edits reach the audio thread whether or not an editor is open; the canvas never rebuilds on a tree change.
- `ValueTreeIdentifiers` lists every `juce::Identifier` used as a tree type or property key.

**`Source/Input/`** — Canvas gestures
- `NodeController` receives all canvas mouse events: Shift+Click creates a root node, Shift+Right-Click deletes, drag moves nodes and their descendants proportionally, plus arrow creation/selection and context menus. What a gesture means is held in one `DragState` enum, not in a set of booleans; `mouseDown`/`mouseDrag`/`mouseUp` dispatch on it to small named handlers. It owns the gesture and nothing else — the selection set belongs to `SelectionOps`, the grid and the update queue to `NodeCanvas`.
- `SelectionOps` owns the selection: `clearAll` and `deselectAllExcept` change which nodes are selected, and it copies, deletes and pastes them. Pasting builds a `PasteLayout` — new ids, remapped parents, orphans promoted to roots — then inserts the nodes, reconnects them, restores their dangling arrows and selects them.
- `ConnectionOps` connects and disconnects arrows, resolves which node owns a given arrow, and sets arrow type.
- `NodeCreationDispatcher::create` maps a `NodeCreationMode` (Node, Modulator, TraversalFlag), the parent's type and the alternative flag onto a node type identifier and the matching `NodeFactory::create*`.

**`Source/UI/`** — Components
- `NodeCanvas` collects `AsyncUpdate{type, nodeId, rootNodeId}` records through `enqueueAsyncUpdate` and applies them in `handleAsyncUpdate` on the message thread. `cancelPendingUpdatesFor` drops a node's queued updates when a drag-created node is undone, so the queue is only ever edited by the class that owns it. It owns the parts that do the work: `NodeManager` creates, positions and moves `Node`s, `ArrowManager` creates, removes and previews `Arrow`s, `AudioCommandDrainer` drains the bridge FIFOs, `CanvasHitTester` answers picking queries, `ValueField` renders paint mode. The snap grid is canvas state, so the math lives here too — `gridVisible` / `gridOrigin` / `gridSpacing` with `showGrid`, `hideGrid` and `snapPointToGrid`.
- `NodeCanvasTreeListener` listens on the graph `ValueTree` and turns child additions, child removals and property changes into `NodeCanvas::AsyncUpdate` records. They drive components only — rebuilding the audio graph is `RTGraphBuilder`'s own listener.
- `Theme` (`Theme/Theme.h`) is the palette and metrics, with no knowledge of any component. `CustomLookAndFeel` inherits it, so `CustomLookAndFeel::get(*this)` yields a `const Theme&` that components paint against.
- `DynamicPort` is a zoom/pan viewport; left-drag pans, Shift+Scroll or pinch zooms (0.1×–5.0×).
- `Bar` is the base class for every bar (`Titlebar`, `BottomBar`, `MenuBar`, `ArrowBindBar`, and the rules window's title bars). It owns the look-and-feel hookup, background paint, content inset, and separator drawing; a `Bar::Style` picks orientation, background, and inset. `paint` is `final` — subclasses lay out in `resized` off `getContentBounds()`, and decorate by overriding `paintOverBar`.
- `MenuArea` hosts `MenuBar` plus the `TraversalMenu` and `NodeMenu` panels.
- `Node` renders and edits one graph node: it holds the node's `ValueTree` and MIDI note tree, hosts the editors for the displayed value, count limit and switch count, and paints hover, selection and per-traversal highlights. `RootNode`, `TraversalFlagNode` and `Modulator` specialise it. `Arrow` renders every connection — dangling is a mode on it (`isDangling()`), not a separate class — with arrow length encoding note duration.
- `NodeFactory` assembles a new node out of `GraphState` calls: it adds the node, sets its position, and applies the creation defaults — a root gets a default note and the default traversal, a child inherits its parent's count limits, repeat value and MIDI notes.
- `TraversalRulesWindow` is the script editor: a `FilePage` over the active rule's source, recompiled on a 250 ms debounce, with diagnostics pushed back as per-line errors and a status bar showing the instruction count and whether this rule is live.

**`Source/UI/Editors/`** — Text and value editing
- `ValueEditor` is the base: it binds to a `ValueTree` property and delegates display/parse/commit to a `ValueFormat` strategy, so pitch names, decimals, multipliers, and dual values are formats rather than subclasses.
- `LineEditor` extends it into one line of code (caret access, indent, and the key handling that splits, merges, and moves between lines).
- `FileLine` is a gutter plus a `LineEditor`, and carries any compile error for that line. `FilePage` is the document — line list, zoom, focus movement, and the `onTextChanged` hook.

**`Source/Util/`** — `ApplicationContext` is a struct of pointers (`processor`, `canvas`, `lookAndFeel`, `undoManager`, `graphState`, `traversalRuleState`, `nodeController`, `rtGraphBuilder`) populated in `PluginEditor`'s constructor and passed into components as `const ApplicationContext&`. It is not a singleton — each editor owns one as a member. The pointers are non-owning: the processor owns `graphState`, `traversalRuleState`, `rtGraphBuilder` and `undoManager`, the editor owns the rest. Undo history therefore outlives the editor window. Only `PluginEditor` writes its fields; everything else receives it `const`, so the compiler rejects rebinding a pointer anywhere else, while the objects pointed at stay mutable. `canvas` and `nodeController` are filled in during construction, each right after its object is built, so `NodeCanvas`'s constructor must read neither and `NodeController`'s must not read `nodeController`.

### Data Flow

**Node creation:** `NodeController::mouseDown` → `NodeCreationDispatcher` → `NodeFactory::create*` → mutates `GraphState` → `NodeCanvasTreeListener` → `AsyncUpdater` → `NodeManager` adds a `Node` component. In parallel, `RTGraphBuilder`'s listener queues the node's root and its parents' roots for a rebuild.

**Graph → audio:** a `GraphState` change reaches `RTGraphBuilder`'s listener, whose `handleAsyncUpdate` calls `makeRTGraph()`, which builds an `RTGraph` from the `ValueTree`, then `AudioSnapshotPublisher::publishGraph()` copies the current `Snapshot`, merges the new graph in, prunes stale node IDs, and publishes.

**Script → audio:** `AudioSnapshotPublisher` reads its source from `TraversalRuleState`, and its `Snapshot` carries `globalNodes` and `selectChildScript` (plus a `generation` counter) so a recompiled rule is published through exactly the same path as a graph edit, via `publishScript()`. `TraversalSession::setSelectChildScript` swaps it into `ScriptTraversalRule` each block, falling back to a compiled copy of the native rule when the script is empty. A bad script therefore degrades rather than silencing playback.

**Audio → GUI:** `TraversalDispatcher` / `EventManager` push commands into the `AudioUIBridge` FIFOs, and `followHostTransport` sets `playbackStateChanged`. Nothing on the audio thread notifies anyone: `SequenceTreeAudioProcessorEditor`'s `audioCommandFrames` (a `VBlankAttachment`) polls once per display frame, applies a transport change to the `Titlebar`, and calls `NodeCanvas::handleAsyncUpdate()` to drain the FIFOs when they have commands. With no editor open, the FIFOs fill and overflow, and the drainer resyncs the canvas on its first drain. `triggerAsyncUpdate` is not realtime-safe — on macOS it takes a mutex in `juce::MessageQueue::post` — which is why the processor is no longer an `AsyncUpdater`.

### Rules

The general rules - *How to Systematically Solve Problems* and the *Key Design Rules* - come from the research-suite plugin (`rules/systematic.md` and `rules/style.md` in its repository) and are injected at the start of every session. The rules below are this project's own and bind alongside them. This project's `core_purpose_api` entries, kept small classes and machine checks for these rules live under `[suite]` in `.claude/refactor.toml`.

### Project Design Rules

**THESE RULES ARE NON-NEGOTIABLE**

- Never mutate UI components from the audio thread — the audio thread writes `AudioUIBridge` FIFOs and atomics only, and the message thread polls them. `AsyncUpdater::triggerAsyncUpdate` and `MessageManager::callAsync` lock or allocate, so neither is called from the audio thread.
- `RTData` structs and `RTScript` are the only types crossing the audio boundary.
- **Never allocate or free on the audio thread.** Snapshots are published only via `AudioSnapshotPublisher::publish()`, which stores the raw pointer `seq_cst` and then reads `blockEpoch`, a counter the audio thread bumps in `beginBlock` and `endBlock` so that it is odd while a block runs. An even epoch means no block holds any snapshot, so the outgoing `shared_ptr` is freed right there; an odd one parks it in `retiredSnapshots` until the epoch moves past. Both sides are `seq_cst`, which is what rules out a block loading the old pointer after the publisher read an even epoch, and a host that stops calling `processBlock` therefore leaves nothing parked. The audio thread can therefore never drop the last reference and trigger a deallocation in `processBlock`.
- Per-block scratch state lives in reserved member vectors (see `TraversalSession`), not in locals, for the same reason.
- `ApplicationContext` pointers are valid only after `PluginEditor` construction; do not touch them at static init time.
