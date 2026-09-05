# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure
cmake -B build

# Build (debug)
cmake --build build --config Debug

# Build (release)
cmake --build build --config Release
```

CMake is configured with `JUCE_COPY_PLUGIN_AFTER_BUILD ON`, so successful builds automatically install the plugin to system AU/VST3/VST/Standalone locations. The JUCE submodule must be initialized (`git submodule update --init`) before building.

Every `.cpp` must be listed explicitly in `CMakeLists.txt` — there is no glob. Adding a source file without registering it fails at link time, or silently does nothing.

There are no automated tests, and none should be added. Verify changes by building (`SequenceTree_Standalone` is the fastest full-link target) and by exercising the plugin manually in a host.

## Architecture Overview

SequenceTree is a JUCE plugin that generates MIDI by traversing a user-designed directed graph. Users create nodes, assign MIDI note data and a "count limit" to each, then the plugin walks the graph during playback — when a node's counter reaches its limit, traversal advances to matching children.

Three things about the model are easy to miss:

- **Arrow geometry is data.** A connection's note duration is derived from the vector between the two node centres (`RTGraphBuilder::fillDurationMap` → `arrowDurationFromDelta`). Dragging a node retimes the sequence, which is why `NodeMoved` triggers `updateDurationMap`.
- **Duration is derived, pitch is owned.** Duration is recomputed from geometry on every read; pitch is not. A pitch-bound arrow only ever *shifts* the pitch of the node it points at, and the semitone amount it has already contributed is stored on the arrow as `ArrowPitchOffset`. `GraphState::syncPitchBindings` applies the difference between that stored amount and the current geometry, so moving a node transposes it around whatever pitch the user last typed, and no arrow ever recomputes a node's pitch from its parent.
- **Transport is internal.** Playback runs off the plugin's own play button (`Titlebar` → `NodeCanvas::setProcessorPlayblack` → `SequenceTreeAudioProcessor::isPlaying`), not the host transport.

### Directory Responsibilities

**`Source/Plugin/`** — JUCE entry points
- `PluginProcessor` is the `AudioProcessor`: transport state, plugin state save/restore, and the per-block sequencing in `processBlock`. Traversal lifecycle lives in `TraversalSession`, snapshot publication in `AudioSnapshotPublisher`, not here.
- `AudioSnapshotPublisher` owns everything the audio thread reads: the `Snapshot` type, the published pointer, and the deferred reclamation. Every mutation goes `beginEdit()` → change one member → `publish()`, so the rule that all other members are carried forward lives in one function; `publishGraph`, `publishScript` and `publishActiveTraversalRule` are the three callers plus `RTGraphBuilder::updateDurationMaps`. It sits in `Source/Plugin/` because it depends on `Graph`, `Audio` and `Script` at once.
- `PluginEditor` constructs every UI component and populates the shared `ApplicationContext`.

**`Source/Audio/`** — Audio thread
- `TraversalSession` owns the `TraversalPool` and the traversal lifecycle: starting, restarting, syncing against graph edits, and stopping traversals. `processBlock` drives it through `silenceAllNotes` / `suspendActiveNotes` / `restartActiveTraversals` / `syncWithGraph` / `startTraversalsFromFirstRoot`.
- `TraversalPool` is a fixed-capacity slot array (128 traversals), preallocated in `prepare()` so acquiring a traversal never allocates. Each slot is an `Instance { TraversalLogic logic; TraversalRuntime runtime; }` — `runtime` carries spawn origin (flag / cross-tree), `pendingRemoval`, and `repeatCount`.
- `TraversalLogic` is one running traversal instance — pure graph walking over `NodeMap`, returning a `StepResult`. No MIDI, no UI.
- `NodeStateTable` is per-traversal node state (visit counts, trigger counts, active alternative, last chosen child) as a flat `vector<int>` indexed by slot × nodeId, capped at 1024 node IDs. Out-of-range IDs land in a sink rather than growing the table.
- `TraversalRule` is the child-selection strategy. `RuleContext::eligibleChild` does all the filtering (count limits, trigger limits, zero-duration arrows, per-traversal disables); the rule only picks among survivors. `NativeTraversalRule` is the built-in policy; `ScriptTraversalRule` executes a user script instead.
- `TraversalDispatcher` turns traversal steps into MIDI events and UI commands (chords, modulators, cross-tree jumps, traversal flags).
- `NoteScheduler` writes note-ons into the `MidiBuffer` and keeps each sounding note in `activeNotes`, counting down its remaining samples before sending the note-off. A `NoteVoicing` per note applies transpose, velocity scaling, and pitch or velocity overrides.
- `AudioUIBridge` is the only channel from audio to UI: four lock-free `juce::AbstractFifo`s (highlight, progress, count, arrow reset) carrying plain command structs.
- `EventManager` bundles bridge + scheduler + dispatcher and runs the per-block event loop.

**`Source/Script/`** — The traversal scripting language
- `RTScript` is the bytecode a traversal rule runs, and the compiler's only output: a stack VM with `maxLocals` 32, `maxStack` 64, and a `stepBudget` of 8192 — the budget is what stops a user-authored loop from hanging the audio thread. It lives here rather than in `Audio/` because everything that *produces* it is here; `Audio/` only executes it, through `ScriptTraversalRule`.
- The pipeline is one strictly linear pass — `Lexer` (source → tokens) → `Parser` (tokens → AST) → `Emitter` (AST → bytecode) — split across `ScriptLexer`, `ScriptParser` and `ScriptEmitter`, all inside `namespace script`. It never runs on the audio thread; only the compiled bytecode crosses over.
- `ScriptCompiler.{h,cpp}` is the public face: `ScriptDiagnostic`, `ScriptCompileResult`, and a `compileTraversalScript` that constructs each phase locally and destroys it after one `run()`. The phases are deliberately *not* members — a phase never outlives a compile, so stale per-run state stays impossible rather than becoming a reset you have to remember. `ScriptCompiler.h` names none of them, so the AST and the emitter's internals stay out of `PluginProcessor.h`.
- The language has `let`, `if` / `else`, `for … in`, `while`, `break`, `continue`, `return`, `none`, and `and` / `or` / `not`. Statements are terminated by `;`, and a `}` ends a block statement on its own — newlines are whitespace, so an expression may span as many lines as it likes. Comments run from `//` to either a closing `//` or the end of the line. Scripts read the graph through fixed field tables — `parent.{id,count,childCount,lastChild}`, `child.{id,eligible,limit,triggerLimit,triggerCount,visits,repeat,pitch,switchLimit,subLoopLimit}`, `children.count`, `traversal.id` — and return a child ID or `none`.
- Failures come back as `ScriptDiagnostic { message, line, column, length }`, so the editor can mark the offending line.
- `defaultTraversalScriptSource()` is the language-level equivalent of `NativeTraversalRule`.

**`Source/Graph/`** — Data model
- `GraphState` creates and mutates the graph's `ValueTree`: adds and removes all node types, connects and disconnects them, and writes node properties from argument structs (`setNodePosition`, `addMidiNote`). It is a `ValueTree::Listener` on its own `nodeMap`, and that listener is the only thing that maintains `nodeIndex` (id → node) and the public `parentIdsOf` (child id → parent ids). Because the index is fed by tree callbacks rather than by the mutators, undo, redo and paste keep it correct for free, and `getNode` / `getNodeParent` are constant time instead of scans. `getNodeParent` walks up *through* `TraversalFlagData` parents, so a node hanging off a flag chain still reports the note node the chain belongs to.
- `TraversalRuleState` owns the traversal rule scripts and which one is active — a separate document from the graph, sharing only the save file.
- `RTData.h` defines `RTNote` / `RTtraversal` / `RTNode` / `RTGraph` plus the `NodeMap` and `RTGraphs` aliases — plain structs safe to hand to the audio thread.
- `RTGraphBuilder` builds `RTGraph`s from the `ValueTree`: `makeRTGraph` rebuilds one tree, `rebuildAllGraphs` rebuilds all of them, `updateDurationMaps` recomputes arrow durations for given node ids without a rebuild. Finished graphs are held in `rtGraphs`, keyed by graph id, and published from there.
- `ValueTreeIdentifiers` lists every `juce::Identifier` used as a tree type or property key.

**`Source/Input/`** — Canvas gestures
- `NodeController` receives all canvas mouse events: Shift+Click creates a root node, Shift+Right-Click deletes, drag moves nodes and their descendants proportionally, plus arrow creation/selection and context menus. What a gesture means is held in one `DragState` enum, not in a set of booleans; `mouseDown`/`mouseDrag`/`mouseUp` dispatch on it to small named handlers. It owns the gesture and nothing else — the selection set belongs to `SelectionOps`, the grid and the update queue to `NodeCanvas`.
- `SelectionOps` owns the selection: `clearAll` and `deselectAllExcept` change which nodes are selected, and it copies, deletes and pastes them. Pasting builds a `PasteLayout` — new ids, remapped parents, orphans promoted to roots — then inserts the nodes, reconnects them, restores their dangling arrows and selects them.
- `ConnectionOps` connects and disconnects arrows, resolves which node owns a given arrow, and sets arrow type.
- `NodeCreationDispatcher::create` maps a `NodeCreationMode` (Node, Modulator, TraversalFlag), the parent's type and the alternative flag onto a node type identifier and the matching `NodeFactory::create*`.

**`Source/UI/`** — Components
- `NodeCanvas` collects `AsyncUpdate{type, nodeId, rootNodeId}` records through `enqueueAsyncUpdate` and applies them in `handleAsyncUpdate` on the message thread. `cancelPendingUpdatesFor` drops a node's queued updates when a drag-created node is undone, so the queue is only ever edited by the class that owns it. It owns the parts that do the work: `NodeManager` creates, positions and moves `Node`s, `ArrowManager` creates, removes and previews `Arrow`s, `AudioCommandDrainer` drains the bridge FIFOs, `CanvasHitTester` answers picking queries, `ValueField` renders paint mode. The snap grid is canvas state, so the math lives here too — `gridVisible` / `gridOrigin` / `gridSpacing` with `showGrid`, `hideGrid` and `snapPointToGrid`.
- `NodeCanvasTreeListener` listens on the graph `ValueTree` and turns child additions, child removals and property changes into `NodeCanvas::AsyncUpdate` records.
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

**`Source/Util/`** — `ApplicationContext` is a struct of pointers (`processor`, `canvas`, `lookAndFeel`, `undoManager`, `graphState`, `traversalRuleState`, `nodeController`, `rtGraphBuilder`) passed by reference into components and populated in `PluginEditor`'s constructor. It carries the current `NodeDisplayMode` and the node-selection listener list.

### Data Flow

**Node creation:** `NodeController::mouseDown` → `NodeCreationDispatcher` → `NodeFactory::create*` → mutates `GraphState` → `NodeCanvasTreeListener` → `AsyncUpdater` → `NodeManager` adds a `Node` component.

**GUI → audio:** `RTGraphBuilder::makeRTGraph()` builds an `RTGraph` from the `ValueTree`, then `AudioSnapshotPublisher::publishGraph()` copies the current `Snapshot`, merges the new graph in, prunes stale node IDs, and publishes.

**Script → audio:** `AudioSnapshotPublisher` reads its source from `TraversalRuleState`, and its `Snapshot` carries three members — `globalNodes`, `rtGraphs`, and `selectChildScript` — so a recompiled rule is published through exactly the same path as a graph edit, via `publishScript()`. `TraversalSession::setSelectChildScript` swaps it into `ScriptTraversalRule` each block, falling back to a compiled copy of the native rule when the script is empty. A bad script therefore degrades rather than silencing playback.

**Audio → GUI:** `TraversalDispatcher` / `EventManager` push commands into the `AudioUIBridge` FIFOs; `processBlock` calls `notifyUi`, which triggers `NodeCanvas::handleAsyncUpdate()` to drain them on the message thread.

### General Principles for Answering Questions
- Verify that you have fully applied each principle
- Carefully analyze the entire API the code affects
- Closely observe the structure of the API the code affects
- Understand the data flow of the relevant API section
- Verify that the relevant code follows all rules in the Key Design Rules section
- Analyze the broader API the smaller section of the API affects
- Closely observe the general design of the API and verify the relevant code follows it fully
- Reapply the same principles from the second step to the broader API and repeat 

### Key Design Rules

**THESE RULES ARE NON-NEGOTIABLE**

- Never mutate UI components from the audio thread — go through `AudioUIBridge` FIFOs, `AsyncUpdater`, or `MessageManager::callAsync`.
- `RTData` structs and `RTScript` are the only types crossing the audio boundary.
- **Never allocate or free on the audio thread.** Snapshots are published only via `AudioSnapshotPublisher::publish()`, which stores the raw pointer with `std::memory_order_release` and parks the outgoing `shared_ptr` in `retiredSnapshots`, freeing it only once `blocksCompleted` proves the audio thread has finished a block since. The audio thread can therefore never drop the last reference and trigger a deallocation in `processBlock`.
- Per-block scratch state lives in reserved member vectors (see `TraversalSession`), not in locals, for the same reason.
- `ApplicationContext` pointers are valid only after `PluginEditor` construction; do not touch them at static init time.
- This project uses **no code comments**. Express intent through naming.
- Never write functions that are 1-2 lines **do not write wrapper functions**  
- Never use ternary operators
- Always use {} for blocks
- Always avoid encapsulation on very small segments of code which repeat
- Make sure code fits the class's intended purpose, and generally sticks to a single area of concern
- Never use functions with the keyword `inline`
- Avoid using getter and setter functions, prefer public variable access when possible
- Avoid using namespaces
- Never use functions that perform a single operation (single if statement or boolean operation, etc.)