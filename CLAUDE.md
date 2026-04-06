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

CMake is configured with `JUCE_COPY_PLUGIN_AFTER_BUILD ON`, so successful builds automatically install the plugin to system AU/VST3/Standalone locations. The JUCE submodule must be initialized (`git submodule update --init`) before building.

No automated tests exist — testing is done manually through a plugin host.

## Architecture Overview

SequenceTree is a JUCE audio plugin that generates MIDI by traversing a user-designed directed graph. Users create nodes, assign MIDI note data and a "count limit" to each node, then the plugin traverses the graph during playback — when a node's counter reaches its count limit, traversal advances to matching children.

### Layer Responsibilities

**`Source/Core/`** — Audio thread logic
- `PluginProcessor` hosts two nested classes: `TraversalLogic` (advances the graph) and `EventManager` (tracks active notes, schedules MIDI events, triggers UI highlights via async callbacks).
- `PluginEditor` constructs all UI components and the shared `ComponentContext`.

**`Source/Logic/`** — User input handling
- `NodeController` handles all mouse events: Shift+Click creates a root node, Shift+Right-Click deletes, drag moves nodes and their descendants proportionally.
- `DynamicPort` is a zoom/pan viewport; left-drag pans, Shift+Scroll or pinch zooms (0.1×–5.0×).

**`Source/UI/`** — Visual components
- `NodeCanvas` owns the node map (int→Node\*), listens to ValueTree changes, and builds `RTGraph` objects that are handed off to the audio thread. All UI mutations from the audio thread go through `AsyncUpdater`.
- `NodeArrow` renders connections between nodes; its length encodes note duration with a snap animation.
- `Node` (and subclasses `Counter`, `Connector`) renders a single graph node with hover/selected/highlighted states and editable fields.

**`Source/Util/`** — Data and context
- `ValueTreeState` is the single source of truth for the graph. It wraps JUCE `ValueTree` and provides typed accessors for node positions, MIDI values, and tree structure.
- `RTData.h` defines `RTNode` / `RTGraph` — plain structs safe to pass across the audio boundary via `std::shared_ptr` and atomics.
- `PluginContext` is a global namespace of static pointers (`processor`, `canvas`, `undoManager`, `lookAndFeel`, `valueTreeState`, `nodeController`) initialized once in `PluginEditor`.
- `ValueTreeIdentifiers.h` lists every JUCE `Identifier` constant used as a property key.

### Data Flow

**Node creation:** `NodeController::mouseDown` → `NodeFactory::create*` → mutates `ValueTreeState` → `NodeCanvas::valueTreeChildAdded` listener → `AsyncUpdater` → adds `Node` component to canvas.

**GUI → audio:** `NodeCanvas::makeRTGraph()` builds an `RTGraph` from the current `ValueTree` state and stores it in `PluginProcessor::rtGraphs` (an atomic shared_ptr map).

**Audio → GUI:** `EventManager::highlightNode()` posts a `MessageManager` callback to update node highlight state without blocking the audio thread.

### Key Design Rules

- Never mutate UI components directly from the audio thread — always use `AsyncUpdater` or `MessageManager::callAsync`.
- `RTData` structs are the only types that cross the audio boundary; use atomics and `shared_ptr` for hand-off.
- `ComponentContext` static pointers are valid only after `PluginEditor` construction; do not access them at static init time.