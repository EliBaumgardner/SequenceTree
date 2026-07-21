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

No automated tests exist — testing is done manually through a plugin host.

## Architecture Overview

SequenceTree is a JUCE plugin that generates MIDI by traversing a user-designed directed graph. Users create nodes, assign MIDI note data and a "count limit" to each, then the plugin walks the graph during playback — when a node's counter reaches its limit, traversal advances to matching children.

### Directory Responsibilities

**`Source/Plugin/`** — JUCE entry points
- `PluginProcessor` is the `AudioProcessor`: transport state, plugin state save/restore, and publishing graph snapshots to the audio thread. Traversal lifecycle lives in `TraversalSession`, not here.
- `PluginEditor` constructs every UI component and populates the shared `ApplicationContext`.

**`Source/Audio/`** — Audio thread
- `TraversalSession` owns the `TraversalMap` (instanceId → `TraversalLogic`) and the traversal lifecycle: starting, restarting, syncing against graph edits, and stopping traversals. `processBlock` drives it through `silenceAllNotes` / `restartActiveTraversals` / `syncWithGraph` / `startTraversalsFromFirstRoot`.
- `TraversalLogic` is one running traversal instance — pure graph walking over `NodeMap`, returning a `StepResult`. No MIDI, no UI.
- `TraversalDispatcher` turns traversal steps into MIDI events and UI commands (chords, modulators, cross-tree jumps, traversal flags).
- `NoteScheduler` tracks active notes and their expiry.
- `AudioUIBridge` is the only channel from audio to UI: four lock-free `juce::AbstractFifo`s (highlight, progress, count, arrow reset) carrying plain command structs.
- `EventManager` bundles bridge + scheduler + dispatcher and runs the per-block event loop.

**`Source/Graph/`** — Data model
- `ValueTreeState` is the source of truth for the graph, wrapping JUCE `ValueTree` with typed accessors. One instance per processor (`SequenceTreeAudioProcessor::graphState`), reached everywhere else through `ApplicationContext::valueTreeState`.
- `RTData.h` defines `RTNote` / `RTtraversal` / `RTNode` / `RTGraph` plus the `NodeMap` and `RTGraphs` aliases — plain structs safe to hand to the audio thread.
- `RTGraphBuilder` converts `ValueTree` state into an `RTGraph` and publishes it.
- `ValueTreeIdentifiers` lists every `juce::Identifier` used as a tree type or property key.

**`Source/Input/`** — `NodeController` handles all canvas mouse events: Shift+Click creates a root node, Shift+Right-Click deletes, drag moves nodes and their descendants proportionally, plus arrow creation/selection and context menus. What a gesture means is held in one `DragState` enum, not in a set of booleans; `mouseDown`/`mouseDrag`/`mouseUp` dispatch on it to small named handlers.

**`Source/UI/`** — Components
- `NodeCanvas` owns the node map (int→`Node*`) and the `NodeArrow` array. It delegates FIFO draining to `AudioCommandDrainer` and all dangling-arrow state to `DanglingArrowLayer`.
- `Theme` (`Theme/Theme.h`) is the palette and metrics, with no knowledge of any component. `CustomLookAndFeel` inherits it, so `CustomLookAndFeel::get(*this)` yields a `const Theme&` that components paint against.
- `DynamicPort` is a zoom/pan viewport; left-drag pans, Shift+Scroll or pinch zooms (0.1×–5.0×).
- `MenuArea` hosts `MenuBar` plus the `TraversalMenu` and `NodeMenu` panels.
- `Node` (and `RootNode`, `TraversalFlagNode`, `Modulator`) renders one graph node; `NodeArrow` / `DanglingArrow` render connections, with arrow length encoding note duration.

**`Source/Util/`** — `ApplicationContext` is a struct of pointers (`processor`, `canvas`, `lookAndFeel`, `undoManager`, `valueTreeState`, `nodeController`, `rtGraphBuilder`) passed by reference into components and populated in `PluginEditor`'s constructor.

### Data Flow

**Node creation:** `NodeController::mouseDown` → `NodeFactory::create*` → mutates `ValueTreeState` → `NodeCanvasTreeListener` → `AsyncUpdater` → adds a `Node` component.

**GUI → audio:** `RTGraphBuilder::makeRTGraph()` builds an `RTGraph` from the `ValueTree`, then `SequenceTreeAudioProcessor::setNewGraph()` copies the current `AudioSnapshot`, merges the new graph in, and calls `publishAudioSnapshot()`, which swaps it in with `std::atomic_exchange`.

**Audio → GUI:** `TraversalDispatcher` / `EventManager` push commands into the `AudioUIBridge` FIFOs; `processBlock` calls `notifyUi`, which triggers `NodeCanvas::handleAsyncUpdate()` to drain them on the message thread.

### Key Design Rules

- Never mutate UI components from the audio thread — go through `AudioUIBridge` FIFOs, `AsyncUpdater`, or `MessageManager::callAsync`.
- `RTData` structs are the only types crossing the audio boundary.
- **Never allocate or free on the audio thread.** Snapshots are published only via `publishAudioSnapshot()`, which keeps retired snapshots alive in `retiredSnapshots` so the audio thread can never drop the last reference and trigger a deallocation in `processBlock`.
- `ApplicationContext` pointers are valid only after `PluginEditor` construction; do not touch them at static init time.
- This project uses **no code comments**. Express intent through naming and small functions.

### Known Architectural Debt

- `CustomLookAndFeel` still forward-declares ~20 component classes for nodes, arrows, and toolbar buttons. Menus, panes, and bars already paint themselves against `Theme`; the rest should follow the same pattern, one widget at a time.
- Reset (`TraversalSession::silenceAllNotes`) sends note-off on channel 1 regardless of the channel the note started on, unlike `stopTraversalNotes`. Notes started on other channels can hang.
- There are no automated tests. `TraversalLogic` and `RTGraphBuilder` are pure functions over plain structs and are the natural place to start.
