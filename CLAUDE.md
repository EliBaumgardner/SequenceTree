# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Build (debug)
cmake --build cmake-build-debug

# Build (release)
cmake --build cmake-build-release
```

`cmake-build-debug` and `cmake-build-release` are CLion's build directories, and they are the only ones. Terminal builds, CLion's run buttons and `scripts/run-in-live.sh` all share them on purpose: `COPY_PLUGIN_AFTER_BUILD` installs over the system plugin, so a second build tree means the installed AU and VST3 silently come from different builds. Never configure a new one (`cmake -B build`) — build into these.

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

Hand-built `RTNode`s must set `subLoopCountLimit` and `switchCountLimit` to 1, as `GraphState` does, and a `TraversalLogic` needs `nodeState.prepare()` before use — `RTNode`'s own zero defaults arm a sub-loop on every node, and an unprepared table drops every count into its sink. Test files follow the Key Design Rules like any other source, and every one is listed explicitly in `CMakeLists.txt`. The tests reach graph building and traversal walking only; MIDI timing, UI and plugin-format behaviour still need a host.

## Refactoring Commands

`refactor.encap`, `refactor.decap`, `refactor.replace`, `refactor.undo`, `refactor.smell`, `refactor.find` and `refactor.help` are the refactoring suite, on `PATH` and driven by `.claude/refactor/refactor.py`. `refactor.help` lists them and `refactor.help <command>` prints one command's arguments. Every one carries the `refactor.` prefix so that none shadows a command already on `PATH`. Lifting a selection into its own function, dissolving a function into its call sites, renaming a variable, and putting any of them back are performed by those commands, not by hand. A rename is never a search-and-replace: `refactor.replace` takes the selection written out again with the new names in it and asks clangd which symbol each name is, so the variable is renamed where it is that variable and nowhere else. `refactor.smell` and `refactor.find` only report: `refactor.smell` names the wrappers and accessors clang can see, and `refactor.find` lists the things of one kind that match a condition, as in `refactor.find function 'numlines > 10'` — quoted, or the shell reads the `>` as a redirect. See `.claude/refactor/REFACTORING.md` for what each command is and what it does.

### Reaching for them

```bash
refactor.smell Source/Audio                      # wrappers and accessors, AST-accurate
refactor.find function 'numlines > 80' Source/Audio
refactor.find repeating 'numlines > 3' Source/Audio          # identical tokens
refactor.find repeating shape 'numlines > 3' Source/Audio    # identical structure, any names
refactor.encap Source/Audio/Foo.cpp:112-147 <name>
refactor.decap Foo::bar                          # also deletes a function with no call sites
refactor.replace 'while (point < text.size()) {' # the selection retyped; renames the symbols, not the text
refactor.undo
```

Duplication questions go to `refactor.find repeating`, both likenesses, before reading files by hand — it reads the whole scope in about half a second and beats grepping for a remembered line.

Two things about it decide whether the answer is any good:

- **Start the threshold low and read upward.** `numlines > 3` first; a high threshold silently hides the shorter half of a finding, and there is no indication that it did. `count >= 3` asks the other question — what has been written three times over.
- **`shape` relaxes spelling, not structure.** `shape_of` in `find.py` spells every identifier as the one token `name`, so `spawnKey` is one token and `traversal.key` is three, and `obj.f(x)` and `f(x)` differ by a receiver. Both likenesses report *contiguous* runs, so two functions that do the same thing with different expressions plugged in come back as several short islands rather than one long finding. Read adjacent findings in the same pair of files as possibly one duplicate, and go read the sites before reporting a size.

## Architecture Overview

SequenceTree is a JUCE plugin that generates MIDI by traversing a user-designed directed graph. Users create nodes, assign MIDI note data and a "count limit" to each, then the plugin walks the graph during playback — when a node's counter reaches its limit, traversal advances to matching children.

Five things about the model are easy to miss:

- **Every walk is the same traversal.** The primary walker, the modulator walk (`TraversalLogic::ModulatorWalk`), alternatives and a tree that is stepped into all unfold by one mechanic: a node's count advances each time it is left, children are chosen by their count limits, a child's switch count holds its parent on that child for that many visits, and sub-loop and trigger limits work the same way. What differs is only what drives the step — the primary walker steps when its own note ends, the modulator walk steps once per primary note played under its host (or on the host itself when the modulator arrow is unsynced), each keeping its counts in its own slot (`Count`, `ModulatorCount`). So a modulator's counts depend on how often the other modulator nodes have been played, exactly as a node's depend on the other nodes. Any behaviour that exists in one walk and not another — a node picking itself as its own switch target, a limit honoured in one walk and ignored in the other — is a bug, never a feature of that walk; fix it for every walk, and verify a traversal change by running the same graph shape as a node traversal and as a modulator walk and comparing the sequences.
- **Arrow geometry is data.** A connection's note duration is derived from the vector between the two node centres (`RTGraphBuilder::fillDurationMap` → `arrowDurationFromDelta`). Dragging a node retimes the sequence, which is why `NodeMoved` triggers `updateDurationMap`.
- **Duration is derived, pitch is owned.** Duration is recomputed from geometry on every read; pitch is not. A pitch-bound arrow only ever *shifts* the pitch of the node it points at, and the semitone amount it has already contributed is stored on the arrow as `ArrowPitchOffset`. `GraphState::syncPitchBindings` applies the difference between that stored amount and the current geometry, so moving a node transposes it around whatever pitch the user last typed, and no arrow ever recomputes a node's pitch from its parent.
- **Transport is internal.** Playback runs off the plugin's own play button (`Titlebar` → `NodeCanvas::setProcessorPlayblack` → `SequenceTreeAudioProcessor::isPlaying`), not the host transport.
- **The plugin is an instrument that emits only MIDI.** `IS_SYNTH TRUE` makes it an AU `aumu` and a VST3 `Instrument|Synth`, so it has a stereo output bus and no audio input — the `#if ! JucePlugin_IsSynth` guards in `PluginProcessor`'s constructor and in `isBusesLayoutSupported` compile the input bus and its matching layout check away. Nothing ever writes audio, so `processBlock` clears `buffer` and `midiMessages` once at the top, before the pending note-off flush and every other MIDI writer, making the block's MIDI output entirely freshly generated.

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
- `AudioUIBridge` is the only channel from audio to UI: three lock-free `juce::AbstractFifo`s (highlight, arrow, count) carrying plain command structs. Its push functions are the class's whole reason to exist, so they stay short by design and are declared in `core_purpose_api`. A trail reset is an `ArrowKind` on `ArrowCommand`, not a fourth queue, and `HighlightKind::ClearEveryNode` is how the audio thread blanks the canvas.
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

**`Source/Util/`** — `ApplicationContext` is a struct of pointers (`processor`, `canvas`, `lookAndFeel`, `undoManager`, `graphState`, `traversalRuleState`, `nodeController`, `rtGraphBuilder`) populated in `PluginEditor`'s constructor and passed into components as `const ApplicationContext&`. It is not a singleton — each editor owns one as a member. The pointers are non-owning: the processor owns `graphState`, `traversalRuleState` and `rtGraphBuilder`, the editor owns the rest. Only `PluginEditor` writes its fields; everything else receives it `const`, so the compiler rejects rebinding a pointer anywhere else, while the objects pointed at stay mutable. `canvas` and `nodeController` are filled in during construction, each right after its object is built, so `NodeCanvas`'s constructor must read neither and `NodeController`'s must not read `nodeController`.

### Data Flow

**Node creation:** `NodeController::mouseDown` → `NodeCreationDispatcher` → `NodeFactory::create*` → mutates `GraphState` → `NodeCanvasTreeListener` → `AsyncUpdater` → `NodeManager` adds a `Node` component.

**GUI → audio:** `RTGraphBuilder::makeRTGraph()` builds an `RTGraph` from the `ValueTree`, then `AudioSnapshotPublisher::publishGraph()` copies the current `Snapshot`, merges the new graph in, prunes stale node IDs, and publishes.

**Script → audio:** `AudioSnapshotPublisher` reads its source from `TraversalRuleState`, and its `Snapshot` carries three members — `globalNodes`, `rtGraphs`, and `selectChildScript` — so a recompiled rule is published through exactly the same path as a graph edit, via `publishScript()`. `TraversalSession::setSelectChildScript` swaps it into `ScriptTraversalRule` each block, falling back to a compiled copy of the native rule when the script is empty. A bad script therefore degrades rather than silencing playback.

**Audio → GUI:** `TraversalDispatcher` / `EventManager` push commands into the `AudioUIBridge` FIFOs; `processBlock` calls `notifyUi`, which triggers `NodeCanvas::handleAsyncUpdate()` to drain them on the message thread.

### How to Systematically Solve Problems 
**These are negotiable principles for solving problems in the project but generally should always be adhered to**

- Verify that you have fully applied of these principles after applying them
- Carefully analyze the entire API the code affects
- Closely observe the structure of the API the code affects
- Understand the data flow of the relevant API section
- Verify that the relevant code follows all rules in the Key Design Rules section
- Verify that the relevant code does not change behavior unexpectantly, or introduce new bugs
- Analyze the broader API the smaller section of the API affects (a class is composed in another class, both should be understood)
- Closely observe the general design of the broader API and verify the relevant code follows it fully
- Reapply the same principles from the second step to the broader API and repeat 

### Key Design Rules

**THESE RULES ARE NON-NEGOTIABLE**

- Never mutate UI components from the audio thread — go through `AudioUIBridge` FIFOs, `AsyncUpdater`, or `MessageManager::callAsync`.
- `RTData` structs and `RTScript` are the only types crossing the audio boundary.
- **Never allocate or free on the audio thread.** Snapshots are published only via `AudioSnapshotPublisher::publish()`, which stores the raw pointer with `std::memory_order_release` and parks the outgoing `shared_ptr` in `retiredSnapshots`, freeing it only once `blocksCompleted` proves the audio thread has finished a block since. The audio thread can therefore never drop the last reference and trigger a deallocation in `processBlock`.
- Per-block scratch state lives in reserved member vectors (see `TraversalSession`), not in locals, for the same reason.
- `ApplicationContext` pointers are valid only after `PluginEditor` construction; do not touch them at static init time.
- This project uses **no code comments**. Express intent through naming.
- Never write functions that are 1-2 lines **do not write wrapper functions**. A function whose body is a single forwarding call is a wrapper however many callers it has - caller count is a floor, not a warrant, and avoiding duplication is not by itself a reason to extract. If you think a short function is justified because it names a larger process, **ask before writing it**; do not grant yourself that exception.
- A small function is permitted when it **is** the class's core purpose. When the thing a class exists to do is one small operation per kind of thing it handles - `AudioUIBridge::highlightNode`, `pushProgress`, `pushArrowReset`, `pushCount` - those functions are the class's functionality, not wrappers around it, and dissolving them into their call sites destroys the vocabulary the class exists to provide. The test is whether the function names something the class is *for*: a bridge pushes commands, so its push functions stay however short they are. A function that merely forwards to another class's API is still a wrapper. This exception is **declared, not self-certified** - add the entry to `core_purpose_api` in `.claude/design-rules.sh` so the gate exempts it and the list stays readable as the class's sanctioned API, and **ask before adding one**.
- Never keep a class that is too small to name what it owns. A subclass whose body is a constructor - no overrides, no members of its own - is constructor arguments pretending to be a type; dissolve it into its call site. The same goes for a standalone class holding one function and nothing else. Plain data aggregates and abstract interfaces are not covered by this.
- Never let a virtual be a shell. When a caller reaches a class polymorphically, that virtual is the one function the hierarchy is allowed - the work goes inside each override, not one hop further down.
- Use an enum whenever a property has two or more named states, and whenever two or more terms name the kinds a type comes in. A boolean is for a plain yes/no fact and nothing else - the moment the second state has a name of its own, the states belong in an enum rather than in a bool, an int or a string. Enums describe and label; that is a different job from what a subclass does, which is to modify data, so an enum is never an argument against a subclass family and a type may well want both.
- Never use ternary operators
- Always use {} for blocks
- Always avoid encapsulation on very small segments of code which repeat
- Make sure code fits the class's intended purpose, and generally sticks to a single area of concern
- Never use functions with the keyword `inline`
- Avoid using getter and setter functions, prefer public variable access. `private` and `protected` are fine for anything you can guarantee stays inside the class, and choosing them for organisation is legitimate. But the moment a member needs an accessor to be reached, it is not internal: move the member to public scope and delete the accessor.
- Avoid using namespaces
- Never use functions that perform a single operation (single if statement or boolean operation, etc.)
- It is better to declare an unused variable if it still represents some part of the class's immediate functionality
- If you are about to flag a rule deviation in your report, **stop and ask instead**. A disclosed violation is still a violation; reporting it is not permission, and the design-rule hook only machine-checks some of these rules - its silence on the rest is coverage, not a verdict. 