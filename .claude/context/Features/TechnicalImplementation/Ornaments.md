# Ornaments — Technical Implementation

> Status: Draft
> Written 2026-10-09 at ecd9950. Describes: FeatureDescriptions/Ornaments.md (part: Creation)

## Summary

This plan builds the creation part of ornaments:

- the ornament library, saved in one file per user;
- the ornament window, opened from a new ornament button in the bottom bar;
- the selection panel of named symbol tiles;
- the build panel's mini graph, made of ornament nodes and a host node;
- the freehand symbol editor;
- the Publish rule.

The recommended approach gives the library its own small document class, `OrnamentLibrary`, in `Source/Graph/`. That document writes the same node and arrow identifiers the main graph uses, so the builder can reuse the existing `Node` and `Arrow` components and `ArrowInfo::durationFromDelta` without going through `GraphState`, `NodeCanvas` or `NodeController`. Nothing reaches the audio thread in this part, and no project's saved state changes.

The plan has six steps, each of which can be tried in the standalone app. It adds roughly 1,400 lines, most of them in a new `Source/UI/Ornaments/` folder.

## Components

| Component | One job | Owner | Directory |
|---|---|---|---|
| `OrnamentLibrary` | Owns the library `ValueTree`. Creates, duplicates and removes ornaments. Grows, edits and shrinks an ornament's one-chain figure. Answers `figureSteps`. Loads the library file and saves it back. | `SequenceTreeAudioProcessor` (member `ornamentLibrary`) | `Source/Graph/` |
| `OrnamentStep` (struct) | One node of a figure, in play order: `kind`, `offset`, `velocityPercent`, `durationWeight`. It is the shape tests check now and that deployment will expand into notes. | Declared in `OrnamentLibrary.h` | `Source/Graph/` |
| `OrnamentNodeKind` (enum) | `Note`, `Host`. | `OrnamentLibrary.h` | `Source/Graph/` |
| `OrnamentStatus` (enum) | `Draft`, `Published`. Both states are named, so this is an enum rather than a bool. | `OrnamentLibrary.h` | `Source/Graph/` |
| `ornamentUndoManager` | Undo history for library edits, kept apart from the project's `undoManager` so project undo never reaches into the user-wide library. | `SequenceTreeAudioProcessor` | `Source/Plugin/` |
| New identifiers | `OrnamentLibrary`, `OrnamentData`, `OrnamentFigure`, `OrnamentNoteData`, `OrnamentHostData`, `OrnamentUid`, `OrnamentName`, `OrnamentStatus`, `OrnamentSymbol`, `OrnamentOffset`, `OrnamentVelocity`, `ActiveOrnamentUid`, `NextFigureNodeId` | `ValueTreeIdentifiers` | `Source/Graph/` |
| `OrnamentWindow` | The window's root component. Lays out the build panel and the selection panel, and handles undo and redo keys. Sets its look-and-feel explicitly, because it is a window root. | `BottomBar::ornamentLauncher` (a `PopupWindowLauncher`) | `Source/UI/Ornaments/` |
| `OrnamentSelectionPanel` | Grid of `OrnamentTile`s plus an add button. Right-click offers Duplicate and Delete; Delete asks first. Selecting a tile sets `ActiveOrnamentUid`. | `OrnamentWindow` | `Source/UI/Ornaments/` |
| `OrnamentTile` | One ornament: its symbol, a name tag (`ValueEditor` bound to `OrnamentName`), and a draft marker. | `OrnamentSelectionPanel` | `Source/UI/Ornaments/` |
| `OrnamentBuildPanel` | Shows the active ornament. Its top bar holds the node-kind pane (Note / Host), the symbol button, Publish, and undo/redo. Its body shows either the figure canvas or the symbol editor (`enum class BuildView { Figure, Symbol }`). | `OrnamentWindow` | `Source/UI/Ornaments/` |
| `OrnamentCanvas` | Renders one figure: owns an `OrnamentNode` per figure node and an `Arrow` per connection and per dangling arrow, and rebuilds them when the figure tree changes. | `OrnamentBuildPanel` | `Source/UI/Ornaments/` |
| `OrnamentFigureController` | The canvas's mouse gestures: place the first node, grow the chain, move a node, set the last node's length, remove a node. Its `DragState` enum holds the gesture. | `OrnamentBuildPanel` | `Source/UI/Ornaments/` |
| `OrnamentNode : Node` | A figure node. Shows its signed offset in the centre (as `Modulator` does with `ModAmount`) and a velocity-percent badge. Hides the count, switch and sub-loop badges. Marks the host with a distinct rim. | `OrnamentCanvas` | `Source/UI/Ornaments/` |
| `OrnamentSymbolEditor` | Freehand drawing. Each stroke becomes one undoable write of `OrnamentSymbol`. Has Clear. | `OrnamentBuildPanel` | `Source/UI/Ornaments/` |
| `CustomLookAndFeel::drawOrnamentSymbol` | Paints a stored symbol into any bounds. Called by the tile, the bottom-bar button and the symbol editor now, and by the ornamented node in deployment. | `CustomLookAndFeel` (new file `CustomLookAndFeelOrnaments.cpp`, one file per kind of thing drawn) | `Source/UI/Theme/` |
| Bottom-bar ornament button | One `IconButton` in a new `ornamentPane`. Its painter draws the active symbol; its click opens the window. | `BottomBar` | `Source/UI/Bars/` |
| Library file | `~/Library/Application Support/SequenceTree/Ornaments.xml`, next to `Interface.settings`. Sharing the library means sharing this file. | `OrnamentLibrary` | — |

### The library document

```
OrnamentLibrary            ActiveOrnamentUid
└── OrnamentData           OrnamentUid, OrnamentName, OrnamentStatus, OrnamentSymbol, NextFigureNodeId
    └── OrnamentFigure
        ├── OrnamentNoteData   Id, XPosition, YPosition, OrnamentOffset, OrnamentVelocity
        │   └── NodeChildrenIds
        │       └── NodeId     Id, ArrowType, ArrowXBinding, ArrowYBinding, … (written by ArrowBindingOps::setArrowInfo)
        └── OrnamentHostData   … same shape …
            └── DanglingArrows
                └── DanglingArrow  ArrowTipX, ArrowTipY, arrow-info properties
```

- **Node kind is the tree type** (`OrnamentNoteData` / `OrnamentHostData`), as the main graph uses `NodeData` / `ModulatorData`. `OrnamentNodeKind` is the API's name for the two types.
- **Connections and dangling arrows use the main graph's identifiers.** `Arrow::getDuration`, `Arrow::getDurationLabel` and `ArrowBindingOps::getArrowInfo` therefore read them unchanged.
- **Identity is a `juce::Uuid` string** (`OrnamentUid`), not an incrementing int. Two users' libraries both number from 1, so deployment's per-node reference and its missing-ornament mark need an id that survives sharing. Duplicating an ornament mints a new one.
- **Figure node ids are local to their ornament** (`NextFigureNodeId`).
- **`OrnamentSymbol` is a `juce::Path::toString()`** of strokes normalised to a unit square, so the symbol scales cleanly anywhere it is drawn.

## Where It Lands

### API Surface

Existing code the plan touches or reuses, with its callers:

| Existing | Use | Callers today |
|---|---|---|
| `ArrowInfo::durationFromDelta` (`Util/ArrowInfo.h`) | Each figure node's duration weight, from the connection or dangling-arrow delta. | `RTGraphBuilder::fillDurationMap`, `Arrow::getDuration` |
| `ArrowBindingOps::setArrowInfo` / `getArrowInfo` (both `static`) | Writing and reading the figure's connection trees. | `GraphState::connectNodes`, `NodeFactory::createDanglingArrow`, `Arrow`, `RTGraphBuilder` |
| `Node` (`UI/Node/Node.h`) | Base of `OrnamentNode`. Its constructor needs only a `juce::UndoManager&`, and `Node.cpp` includes `NodeCanvas.h` without using it. `bindToTree` and `bindValueEditorForMode` are virtual and already overridden by `Modulator`. | `NodeManager::instantiateFromTree`, subclasses `RootNode`, `Modulator`, `TraversalFlagNode`, `Encapsulator` |
| `Arrow` (`UI/Node/Arrow.h`) | Figure connections (`Arrow(start, end, undo)`) and the last node's length (`Arrow(start, tipOffset, undo)`). The dangling constructor's `valueEditor` is the main graph's count limit, so `OrnamentCanvas` hides it. | `ArrowManager` |
| `CustomLookAndFeel::drawArrow`, `drawNode` | Painting, unchanged. | `Arrow::paint`, `Node::paint` |
| `ButtonPane` (`Selection::Exclusive`), `IconButton` (`painter`, `onClick`, `onRightClick`) | Node-kind pane, symbol and Publish buttons, bottom-bar ornament button. | `Titlebar::configureModePane`, `BottomBar` |
| `PopupWindowLauncher` / `PopupWindow` | Opens the ornament window (always on top in a plugin, hidden rather than destroyed on close). | `MenuArea::settingsLauncher`, `TraversalMenu`, `NodeMenu`, `NodeController::allowedTraversalsLauncher` |
| `Bar` | The build panel's top bar and the window's panel title bars. | `TraversalRulesWindow::RulesTitlebar`, every bar |
| `ValueEditor` + `NumberFormat` (`showsPositiveSign`, `suffix`) + `TextFormat` | Offset (signed), velocity (`%`), ornament name. | `Node`, `Modulator::bindValueEditorForMode`, `FileLabel` |
| `ContextMenu` | Tile right-click: Duplicate, Delete. | `BottomBar::showAxisMenu`, `NodeController` |
| `BottomBar(NodeCanvas&, TraversalSession&, ValueTree colourPresets, UndoManager&)` | Gains `OrnamentLibrary&` and `juce::UndoManager& ornamentUndoManager` parameters, an `ornamentPane` and an `ornamentLauncher`. | Constructed once, in `SequenceTreeAudioProcessorEditor`'s constructor |
| `SequenceTreeAudioProcessor` | Gains `ornamentUndoManager` and `ornamentLibrary`, loaded in its constructor. `getStateInformation` / `applyRestoredState` are **not** touched: ornaments are not project state. | — |
| `ValueTreeIdentifiers.{h,cpp}` | New identifiers, listed above. | Everywhere |
| `CMakeLists.txt` | Every new `.cpp` registered in `target_sources(SequenceTree …)`. `Tests/OrnamentTests.cpp` is added to `SequenceTree_GraphTests`. | — |

### Structure

```
SequenceTreeAudioProcessor
├── ornamentUndoManager
└── ornamentLibrary : OrnamentLibrary              (Graph/, ValueTree::Listener on its own tree, Timer for saving)

SequenceTreeAudioProcessorEditor
└── bottomBar : BottomBar(…, p.ornamentLibrary, p.ornamentUndoManager)
    ├── ornamentPane : ButtonPane → ornament button (painter → drawOrnamentSymbol)
    └── ornamentLauncher : PopupWindowLauncher
        └── OrnamentWindow(CustomLookAndFeel&, OrnamentLibrary&, UndoManager&)
            ├── OrnamentBuildPanel(OrnamentLibrary&, UndoManager&)
            │   ├── topBar : Bar → kindPane, symbolButton, publishButton, undoRedoPane
            │   ├── canvas : OrnamentCanvas(OrnamentLibrary&, UndoManager&) → OrnamentNode…, Arrow…
            │   ├── controller : OrnamentFigureController(OrnamentCanvas&, OrnamentLibrary&, UndoManager&)
            │   └── symbolEditor : OrnamentSymbolEditor(OrnamentLibrary&, UndoManager&)
            └── OrnamentSelectionPanel(OrnamentLibrary&, UndoManager&) → OrnamentTile…
```

- **Dependencies are passed by reference, each in its own constructor parameter.** `OrnamentWindow` passes the library and the undo manager down to panels that need them, the way `MenuArea` passes them to `NodeMenu` and on to `TraversalRulesWindow`.
- **The canvas and its controller are split like `NodeCanvas` and `NodeController`.** The canvas holds components; the controller is a `juce::MouseListener` that holds the gestures. The ornament canvas is not a feature gate: it has four fixed gestures, and they grow only if branching ornaments are ever added.
- **`NodeController` and `NodeCanvas` are not touched in this part.** Dragging an ornament onto a node is a deployment gesture, and it arrives there as a class of its own (see Later Parts).

### Data Flow

```
mouse gesture in OrnamentCanvas / OrnamentSymbolEditor / OrnamentTile
        │  (message thread)
        ▼
OrnamentLibrary mutator ── writes library ValueTree under ornamentUndoManager
        │
        ├─► ValueTree listeners: OrnamentCanvas rebuilds its figure; OrnamentSelectionPanel adds or removes tiles;
        │                        OrnamentBuildPanel re-checks Publish; BottomBar repaints the ornament button
        │
        └─► OrnamentLibrary's own listener restarts its save timer ─► after 500 ms, writes Ornaments.xml
                                                                     through juce::TemporaryFile (atomic replace)

processor constructor ─► OrnamentLibrary::load(file) ─► library tree; ids resolved; undo history empty
```

- **Threads.** Everything happens on the message thread. The audio thread never reads the library in this part.
- **Saving and restoring.** The library is saved to its own file, never to the plugin state. A project saved now is byte-for-byte what it was.
- **Several plugin instances** each load the file when they are constructed. `OrnamentWindow` asks the library to reload when it opens if the file has changed since this instance last read or wrote it (step 6).

### Invariants

- **Never touch UI from the audio thread / never allocate there.** Nothing in this part runs on the audio thread. `processBlock`, `AudioSnapshotPublisher` and `RTData` are untouched.
- **`RTData` and `RTScript` are the only types crossing the audio boundary.** Unchanged. Deployment will flatten a figure into plain data on the snapshot path (see Later Parts), never pass the `ValueTree`.
- **Arrow geometry is data.** Kept and reused: a figure node's duration weight is `ArrowInfo::durationFromDelta` of its outgoing arrow, recomputed on every read, so dragging a figure node retimes the ornament.
- **Duration is derived, pitch is owned.** The offset is owned (`OrnamentOffset`, typed by the user). Figure arrows are written with `yBinding = NoBind`, so geometry never sets an offset unless the owner chooses otherwise (Decision 2).
- **Feature gates.** `NodeController` and `NodeCanvas` gain nothing in this part.
- **Components take dependencies by reference; window roots set their look-and-feel.** `OrnamentWindow` takes `CustomLookAndFeel&` and calls `setLookAndFeel`, like `TraversalRulesWindow`. No constructor reads the theme.
- **Structs only in headers; enums for named states; no getters or setters; no `_`; definitions in call-hierarchy order** (`refactor.order`).
- **Nothing existing changes.** No current component, document or saved state is altered. The bottom bar gains one button.

## Approaches Considered

### A. A second `GraphState` per open ornament, edited by a second `NodeCanvas` / `NodeController`

- **Adds:** `GraphState`'s indexing, connect, disconnect and dangling-arrow code, for free.
- **Costs:**
  - `NodeCanvas` takes `SequenceTreeAudioProcessor&` and `RTGraphBuilder&`, and `NodeController` takes `TraversalSession&` and `RTGraphBuilder&`. A second instance would publish ornament figures to the audio thread as if they were trees.
  - `GraphState::addChildNode` writes count, switch and sub-loop limits, and `addRootNode` writes traversal children. None of these mean anything in a figure.
  - Every `NodeController` gesture (modulators, flags, encapsulation, paint, span, quaver) would need guards against the ornament canvas.
- **Risk:** high. Ornament edits could reach playback, and the main feature gates would grow states for a second canvas.
- **Rules:** works against the feature-gate rule and against narrow constructor dependencies.

### B. A dedicated `OrnamentLibrary` document and a small ornament canvas reusing `Node`, `Arrow` and `ArrowInfo` (recommended)

- **Adds:**
  - One document class that knows the one-chain rule and the host rule, and nothing else.
  - A canvas that only does the four figure gestures.
  - Reuse of the node and arrow components, their painting, their duration labels and `durationFromDelta`, because the document writes the same arrow identifiers.
- **Cost:** about 1,400 new lines, mostly UI.
- **Risks:**
  - `Arrow` and `Node` carry main-graph extras (the dangling arrow's count `valueEditor`; the count badges). `OrnamentCanvas` and `OrnamentNode` hide them.
  - A later change to `Arrow` aimed at the main canvas could show up in the builder.
- **Rules:** fits. New classes in their own folder, nothing added to a feature gate, and dependencies passed by reference.

### C. A non-graph editor: a row of steps with proportions

- **Adds:** the least code.
- **Problem:** it contradicts the owner's settled decision. The figure is a mini graph whose arrow lengths set the durations.
- **Verdict:** listed only to rule it out.

**Recommendation: B.** It is the only approach that keeps ornament editing out of the audio path and out of the two feature gates. It also reuses the exact pieces that make "arrow geometry is data" true on the main canvas.

## Plan

### Step 1 — The library document

- **Goal:** ornaments exist as data that can be created, shaped, published, saved and reloaded, with tests and no UI.
- **Changes:**
  - **`ValueTreeIdentifiers.{h,cpp}`:** the thirteen identifiers listed under *Components*.
  - **`Source/Graph/OrnamentLibrary.h/.cpp`** (new). These declarations are grouped by return type, and `OrnamentNodeKind`, `OrnamentStatus` and `OrnamentStep` are declared at file scope above the class:
    ```cpp
    enum class OrnamentNodeKind { Note, Host };
    enum class OrnamentStatus   { Draft, Published };

    struct OrnamentStep
    {
        OrnamentNodeKind kind;
        int              offset;
        int              velocityPercent;
        int              durationWeight;
    };

    class OrnamentLibrary : public juce::ValueTree::Listener, private juce::Timer
    {
    public:
        OrnamentLibrary();
        ~OrnamentLibrary() override;

        void load(const juce::File& file);
        void reloadIfChangedOnDisk();

        juce::ValueTree addOrnament      (juce::UndoManager* undoManager);
        juce::ValueTree duplicateOrnament(const juce::String& uid, juce::UndoManager* undoManager);
        juce::ValueTree findOrnament     (const juce::String& uid) const;
        juce::ValueTree addFigureNode    (juce::ValueTree ornament, OrnamentNodeKind kind, int parentNodeId,
                                          juce::Point<int> position, juce::UndoManager* undoManager);

        void removeOrnament   (const juce::String& uid, juce::UndoManager* undoManager);
        void removeFigureNode (juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager);
        void setFigureNodePosition(juce::ValueTree figureNode, juce::Point<int> position, juce::UndoManager* undoManager);
        void setLastNodeLength(juce::ValueTree ornament, juce::Point<int> tipOffset, juce::UndoManager* undoManager);
        void addSymbolStroke  (juce::ValueTree ornament, const juce::Path& stroke, juce::UndoManager* undoManager);
        void publish          (juce::ValueTree ornament, juce::UndoManager* undoManager);

        std::vector<OrnamentStep> figureSteps(const juce::ValueTree& ornament) const;

        bool isPublishable(const juce::ValueTree& ornament) const;

        juce::ValueTree library;
        juce::File      libraryFile;
        …
    };
    ```
  - **The one-chain rule lives in `addFigureNode`:**
    - With `parentNodeId == -1`, it places the first node, and only into an empty figure.
    - Otherwise the parent must be the chain's last node.
    - It refuses a second host (`jassert` and an invalid tree).
    - On success it connects parent → new node with `ArrowBindingOps::setArrowInfo` (`xBinding = DurationBind`, `yBinding = NoBind`). It then removes the old last node's dangling arrow and gives the new node one whose tip offset equals the parent → new-node delta, so the new last node starts with the length of the arrow that reached it (Decision 7).
    - A first node gets a dangling arrow of two grid spaces (`2 * ArrowInfo::pixelsPerGridSpace` to the right).
  - **`removeFigureNode`** reconnects the removed node's parent to its child, so the figure stays one chain (Decision 4). If the last node is removed, its dangling arrow moves to the new last node.
  - **`figureSteps`:**
    - It walks from the node no arrow points into, along `NodeChildrenIds`.
    - Each node's weight is `ArrowInfo::durationFromDelta(ArrowBindingOps::getArrowInfo(arrow), dx, dy)`, where `arrow` is the outgoing connection or, for the last node, its dangling arrow.
    - Steps whose weight is 0 are dropped, as zero-duration arrows are ineligible on the main canvas.
  - **`isPublishable`:** the figure holds a host, at least one other node, and a total weight above 0.
  - **`publish`:** sets `OrnamentStatus::Published` only when `isPublishable`.
  - **Leaving Published:** `OrnamentLibrary`'s own `valueTreeChildRemoved` returns a published ornament to Draft the moment its host is removed (Decision 3).
  - **Saving:** `valueTreePropertyChanged` / `ChildAdded` / `ChildRemoved` restart a 500 ms `juce::Timer`. `timerCallback` writes `library.createXml()` through `juce::TemporaryFile::overwriteTargetFileWithTemporary` and records the file's modification time.
  - **Loading:** `load` reads the XML when the file exists, keeps only `OrnamentData` children that have a `OrnamentUid`, and sets `ActiveOrnamentUid` to the first ornament when the stored one is missing.
  - **`Source/Plugin/PluginProcessor.h/.cpp`:** members `juce::UndoManager ornamentUndoManager;` and `OrnamentLibrary ornamentLibrary;`, declared after `colourPresets`. The constructor body calls `ornamentLibrary.load(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support").getChildFile(JucePlugin_Name).getChildFile("Ornaments.xml"));`, which is the folder `interfaceSettingsOptions` already uses.
  - **`CMakeLists.txt`:** `Source/Graph/OrnamentLibrary.cpp` in `target_sources(SequenceTree …)`; `Tests/OrnamentTests.cpp` in `SequenceTree_GraphTests`.
  - **`Tests/OrnamentTests.cpp`** (new). Expected values come from `ArrowInfo` constants (50 px per grid space, 250 ms per grid space), not from running the code:
    - **"Lower grace":** a note at (0,0), a host at (100,0), and a dangling tip at (+200,0) gives steps `{Note, -1, 70, 500}`, `{Host, 0, 100, 1000}`. Applied to 1000 ms, that splits into 333 and 666.
    - **Growing the chain** removes the old last node's dangling arrow and gives the new last node one equal to the incoming delta.
    - **Refusals:** a second host is refused, and growing from a node that is not last is refused.
    - **Removing a middle node** reconnects its neighbours.
    - **Publishing:** `isPublishable` is false without a host, false with a host alone, and true for the figure above. Removing the host returns a published ornament to Draft.
    - **Duplicating** gives a new `OrnamentUid` and an equal figure.
    - **Round trip:** save to a `juce::TemporaryFile`, then `load` gives an equal tree.
- **Mechanical edits:** none.
- **Behaviour delta:**
  - Nothing visible yet.
  - The library file is written only after the first edit, so a user who never opens the builder gets no file.
  - Project save and restore are unchanged.
- **Invariants:** message thread only, and no audio-thread code is touched.
- **Verification:**
  - `cmake --build cmake-build-debug --target SequenceTree_Standalone SequenceTree_Tests SequenceTree_GraphTests`, then `ctest --output-on-failure`.
  - `design-rules.sh --file` and `readability.sh --file` on every touched file.
  - Manual: launch the standalone, then check that an existing project loads and sounds the same.
- **Rollback:** remove the two files, the processor members, the identifiers and the CMake lines. No other code depends on them yet.

### Step 2 — The ornament window, the selection panel and the bottom-bar button

- **Goal:** the user opens the ornament window from the bottom bar and manages the library: create, rename, duplicate and delete (with confirmation), and choose the active ornament.
- **Changes:**
  - **`Source/UI/Ornaments/OrnamentWindow.{h,cpp}`** (new):
    - Constructor `OrnamentWindow(CustomLookAndFeel& lookAndFeel, OrnamentLibrary& ornamentLibrary, juce::UndoManager& undoManager)`.
    - It calls `setLookAndFeel(&lookAndFeel)` (and `setLookAndFeel(nullptr)` in the destructor) and lays out `buildPanel` on the left and `selectionPanel` on the right.
    - `keyPressed` handles Cmd+Z and Cmd+Shift+Z / Cmd+Y on `undoManager`, because the main editor's key listener is attached to a different top-level window.
    - `defaultWidth` 640, `defaultHeight` 400.
  - **`OrnamentBuildPanel`** (new, a placeholder in this step): its top bar and an empty body labelled with the active ornament's name. It is a `juce::ValueTree::Listener` on `library` for `ActiveOrnamentUid`.
  - **`OrnamentSelectionPanel` + `OrnamentTile`** (new):
    - The panel listens on `library` and keeps one tile per `OrnamentData`, laid out in a grid sized from the panel width.
    - Its title `Bar` holds an add `IconButton` (`drawAddIcon`) that calls `addOrnament` and selects the new ornament.
    - A tile click sets `ActiveOrnamentUid`.
    - A tile right-click opens a `ContextMenu` with Duplicate and Delete.
    - Delete calls `juce::AlertWindow::showAsync` with OK/Cancel and runs `removeOrnament` in the callback when OK is pressed. The callback captures a `SafePointer` to the panel.
    - The tile's name tag is a `ValueEditor` with `TextFormat` bound to `OrnamentName`. Its symbol area is blank until step 5. A Draft tile shows a small "draft" marker.
  - **`BottomBar.{h,cpp}`:**
    - Constructor gains `OrnamentLibrary& ornamentLibrary, juce::UndoManager& ornamentUndoManager`.
    - New members `ButtonPane ornamentPane`, `IconButton* ornamentButton`, and `PopupWindowLauncher ornamentLauncher { "Ornaments", [this]() { … make_unique<OrnamentWindow>(CustomLookAndFeel::get(*this), ornamentLibrary, ornamentUndoManager) … } }`, built the way `MenuArea::settingsLauncher` is.
    - The button's click calls `ornamentLauncher.show()`.
    - `resized` places `ornamentPane` left of `toolPane`, using `idealWidth`.
    - `BottomBar` becomes a `juce::ValueTree::Listener` on `ornamentLibrary.library` and repaints the button when `ActiveOrnamentUid` or the active ornament's symbol changes.
  - **`PluginEditor.cpp`:** `std::make_unique<BottomBar>(*nodeCanvas, p.traversalSession, p.colourPresets, p.undoManager, p.ornamentLibrary, p.ornamentUndoManager)`.
  - **`CustomLookAndFeelOrnaments.cpp`** (new): `drawOrnamentButton(graphics, bounds, state)`, a tile plus a placeholder glyph, used until the active ornament has a symbol.
  - **CMake:** register the five new `.cpp` files.
- **Mechanical edits:** none.
- **Behaviour delta:**
  - The bottom bar has an ornament button, and clicking it opens the window.
  - Ornaments can be added, renamed, duplicated and deleted (after OK), and their names survive a relaunch.
  - Nothing on the main canvas changes.
- **Invariants:**
  - `OrnamentWindow` is a window root that sets its own look-and-feel.
  - The launcher's factory reads `CustomLookAndFeel::get(*this)` only when the user clicks, when `BottomBar` already has a parent.
- **Verification:**
  - Build and test as in step 1, plus the design-rule and readability checks.
  - Manual, in the standalone and in Live:
    - open, close and reopen the window;
    - add three ornaments and rename one;
    - delete one and cancel, then delete it and confirm;
    - relaunch and confirm the library is unchanged.
    - In Live, check that the window stays above the plugin window.
- **Rollback:** revert `BottomBar` and `PluginEditor` and drop the new UI files. Step 1 stands on its own.

### Step 3 — The figure canvas

- **Goal:** the user builds a figure: places nodes of either kind, grows the one chain, moves nodes to set durations, sets the last node's length, edits offset and velocity, and removes nodes.
- **Changes:**
  - **`OrnamentNode : Node`** (new):
    - `bindToTree` hides `countEditor`, `switchCountEditor` and `subLoopLimitEditor`, and binds a new `velocityEditor` badge to `OrnamentVelocity` (`NumberFormat(1, 200)` with `suffix = "%"`; Decision 5).
    - `bindValueEditorForMode` binds `nodeValueEditor` to `OrnamentOffset` with a signed `NumberFormat(-48, 48)`, as `Modulator::bindValueEditorForMode` does.
    - `resized` places `velocityEditor` where `countEditor` sits.
    - `respondToClick` begins editing whichever editor was clicked.
    - A host sets `hasInnerRim = true` (Decision 6).
  - **`OrnamentCanvas`** (new):
    - It holds the ornament being shown (`juce::ValueTree ornament`), `std::unordered_map<int, std::unique_ptr<OrnamentNode>> nodes` and `juce::OwnedArray<Arrow> arrows`.
    - It listens on the ornament's `OrnamentFigure` and rebuilds everything on any change. A figure holds a handful of nodes, so a full rebuild costs nothing and avoids the incremental bookkeeping `NodeManager` and `ArrowManager` need.
    - Dangling `Arrow`s get `valueEditor->setVisible(false)`.
    - Its `paint` fills the canvas colour and draws the snap grid at `ArrowInfo::pixelsPerGridSpace`.
  - **`OrnamentFigureController : juce::MouseListener`** (new), with `enum class DragState { Idle, MovingNode, GrowingChain, MovingDanglingTip }`:
    - **Shift+Click on an empty figure** places the first node, of the kind chosen in the build panel's kind pane.
    - **Dragging from the last node** shows a preview `Arrow` to the cursor. Releasing calls `addFigureNode(ornament, kind, lastId, dropPoint)`. Dragging from a node that is not last moves it instead.
    - **Plain drag on a node** moves it (`setFigureNodePosition`, snapped to the grid). Dragging retimes the figure, because weights are derived.
    - **Dragging the dangling tip** calls `setLastNodeLength`.
    - **Shift+Right-Click on a node** calls `removeFigureNode`. This matches the main canvas's delete gesture.
    - Each gesture is one `undoManager.beginNewTransaction()`.
  - **`OrnamentBuildPanel`:**
    - Its top bar gains `kindPane` (`ButtonPane::Selection::Exclusive`, a Note button and a Host button labelled like `Titlebar`'s mode buttons). The Host button is disabled while the figure already has a host.
    - It owns `canvas` and `controller` and calls `canvas.addMouseListener(&controller, true)`.
  - **CMake:** register the three new `.cpp` files.
- **Mechanical edits:** none.
- **Behaviour delta:**
  - "Lower grace" can be built exactly as the description's walkthrough describes, and duration labels show each arrow's share.
  - The main canvas is untouched.
- **Invariants:**
  - Duration is derived from geometry on every read; nothing stores a computed duration.
  - Offsets are owned, and no arrow writes one (`yBinding = NoBind`).
- **Verification:**
  - Build and test, plus the checks.
  - New `OrnamentTests` case: `figureSteps` after `setFigureNodePosition` moves the host from x=100 to x=150 gives weights 750 and 1000.
  - Manual: build "Lower grace", drag the host and watch both labels change, remove the middle of a three-node chain and confirm it rejoins, undo and redo every gesture.
- **Rollback:** drop the three classes and restore the step-2 placeholder body.

### Step 4 — Publishing

- **Goal:** the Publish rule. The button is greyed out until a host is connected, a published ornament can still be edited, and removing the host returns it to Draft.
- **Changes:**
  - **`OrnamentBuildPanel`:** `publishButton` in the top bar. Its `setEnabled(ornamentLibrary.isPublishable(ornament))` is refreshed from the panel's figure listener, and its click calls `publish`.
  - **`OrnamentTile`:** the draft marker follows `OrnamentStatus`.
  - **`CustomLookAndFeelButtons.cpp`:** none; the Publish button uses `drawRulesButton` with text, as `TraversalRulesWindow`'s play button does.
- **Mechanical edits:** none.
- **Behaviour delta:**
  - Publish is enabled only once the host is connected to at least one other node.
  - The tile drops its draft marker once published.
- **Invariants:** unchanged from step 3.
- **Verification:**
  - Build and test.
  - Step 1's publish tests already cover the rule.
  - Manual:
    - Publish stays greyed out with only a host;
    - it becomes enabled once a note connects;
    - after publishing, removing the host shows the draft marker again.
- **Rollback:** remove the button and the marker binding.

### Step 5 — The symbol editor

- **Goal:** the user draws an ornament's symbol freehand, and it appears on the tile and on the bottom-bar button.
- **Changes:**
  - **`OrnamentSymbolEditor`** (new):
    - `mouseDown` starts a `juce::Path` stroke, and `mouseDrag` adds points with `lineTo` once the cursor has moved two pixels.
    - `mouseUp` normalises the stroke into the unit square of the editor's drawing area and calls `addSymbolStroke` (one transaction per stroke, so undo removes one stroke).
    - A Clear `IconButton` writes an empty `OrnamentSymbol`.
    - `paint` draws the stored symbol and the stroke in progress.
  - **`OrnamentLibrary::addSymbolStroke`** appends the stroke to the parsed stored path and writes `toString()` back.
  - **`CustomLookAndFeel::drawOrnamentSymbol(juce::Graphics&, juce::Rectangle<float>, const juce::String& symbol)`** in `CustomLookAndFeelOrnaments.cpp`:
    - It parses the path, fits it into the bounds while keeping its aspect, and strokes it with a rounded `PathStrokeType` scaled to the bounds.
    - It has three callers in this part (tile, button, editor) and gains a fourth in deployment (the node), so it is a draw function, not a glyph helper with too few callers.
  - **`OrnamentBuildPanel`:** `symbolButton` toggles `BuildView` between `Figure` and `Symbol`, swapping `canvas` and `symbolEditor`.
  - **`OrnamentTile` and `BottomBar`'s button painter** call `drawOrnamentSymbol`.
- **Mechanical edits:** none.
- **Behaviour delta:**
  - Symbols can be drawn, cleared and undone stroke by stroke.
  - Tiles and the bottom-bar button show them, and they look the same at every size.
- **Invariants:** no theme read in a constructor; painting reads the theme in `paint`.
- **Verification:**
  - Build and test.
  - New test: `addSymbolStroke` twice, then a reload, keeps both strokes.
  - Manual:
    - draw a trill sign;
    - check that the tile and the bottom-bar button match it;
    - resize the window and confirm the symbol scales without blur;
    - undo removes the last stroke.
- **Rollback:** remove the editor, the draw function, and the button that toggles the view.

### Step 6 — Several instances and the library file

- **Goal:** two plugin instances, or the plugin and the standalone, never silently lose each other's ornaments.
- **Changes:**
  - **`OrnamentLibrary::reloadIfChangedOnDisk`:**
    - It compares the file's modification time with the one recorded at the last load or save.
    - If the file is newer, it reloads, keeps `ActiveOrnamentUid` when that ornament still exists, and clears `ornamentUndoManager`'s history, because the history refers to trees that are gone.
    - It is called from `OrnamentWindow`'s constructor and from its `visibilityChanged` when it becomes visible.
  - **Saves never reload**, so an instance never undoes its own edits.
- **Mechanical edits:** none.
- **Behaviour delta:**
  - Opening the window shows ornaments made in another instance since this one last read the file.
  - The last instance to save wins for edits made at the same moment.
- **Invariants:** message thread only.
- **Verification:**
  - New test: write the file from a second `OrnamentLibrary`, call `reloadIfChangedOnDisk`, and check that the new ornament is present.
  - Manual: two instances in Live; add an ornament in one, open the window in the other, and confirm it is listed.
- **Rollback:** remove the function and its two call sites.

## Risks

- **`Node` and `Arrow` change for the main canvas and break the builder.** Likely over time. A change to `Node::resized` or `Arrow`'s dangling editor would show up as misplaced badges or a stray count editor in the builder. The guard is that `OrnamentNode` and `OrnamentCanvas` touch only virtuals and public members `Modulator` already relies on, and step 3's manual check is the canary.
- **Two instances editing at once.** Unlikely, but possible in a live set. The last save wins, and the other instance's unsaved edits in the same half-second are lost. Step 6 keeps a window from showing a stale library. Full merging is not attempted.
- **A corrupt or hand-edited library file.** Unlikely. `load` keeps only well-formed `OrnamentData` with a uid, and the atomic replace means a crash mid-save leaves the previous file intact.
- **Undo history crossing windows.** If the ornament window shared the project's `undoManager`, Cmd+Z in the main window would undo ornament edits, and loading a project (`applyRestoredState` → `clearUndoHistory`) would wipe them. The separate `ornamentUndoManager` rules that out.
- **Zero-length arrows.** A figure node dragged straight onto its parent's column has weight 0. `figureSteps` drops it, and `isPublishable` needs a total above 0, so the figure can never divide by zero.

## Later Parts

Deployment is not built by this plan. What this plan leaves for it:

- **`OrnamentUid`** is stable across sharing, ready for a node to store as its reference. A project then keeps the uid only, so a missing uid gives the missing-ornament mark, and a library edit reaches every project.
- **`figureSteps`** already returns the figure as plain values in play order, with proportional weights. Deployment's `RTGraphBuilder` change can flatten those values into a fixed-size `RTData` array on the node, with no `ValueTree` on the audio thread. The scheduler then splits the note's actual duration by weight, which also gives the stretch-on-retime rule, because `EventManager::followTempo` already rescales active notes.
- **`drawOrnamentSymbol`** is ready to draw the symbol at the top of a node's area.
- **The drag from the ornament button and Shift+Right-Click on the symbol** are canvas gestures. They arrive as a class of their own that `NodeController` routes to, never as new `DragState` values in it. That is the moment the project rule asks to be consulted, so deployment's plan will ask first.
- **The drag source is the bottom-bar button.** JUCE's `DragAndDropContainer` on the editor is the natural carrier for that drag, and the button is already in place.

## Decisions for the Owner

1. **Undo history.** Should ornament edits have their own undo history, separate from the project's? Then Cmd+Z in the main window never undoes an ornament edit, and loading a project never wipes the builder's history. *Recommended: yes, separate* (the plan assumes it).
2. **Can the vertical position of a figure node set its offset?** On the main canvas, the Y-axis of an arrow is pitch-bound by default.
   - *(a)* Offsets are typed only, and height is just layout.
   - *(b)* Moving a figure node up one grid space raises its offset by a semitone, as on the main canvas.

   *Recommended: (a)*, because your walkthrough types the −1. (b) can be added later by turning on the arrow's Y binding.
3. **What happens to a published ornament when its host is removed?** It goes back to Draft, and nodes carrying it play plain until it is published again. The alternative is that it stays published and the Host button is disabled for removal. *Recommended: back to Draft.*
4. **Removing a node from the middle of the chain.** Should its neighbours join up, or should removal be allowed only at the ends? *Recommended: neighbours join*, so the figure is always one chain and nothing is left floating.
5. **Velocity range.** Should the velocity percentage go above 100% (an accented ornament note), up to 200%, clamped to MIDI 127? Or should it stop at 100%? *Recommended: up to 200%.*
6. **How the host looks in the builder.** Should it have a second inner ring, the existing inner-rim look, so no new drawing is needed? Or a different colour? *Recommended: the inner ring.*
7. **The default length of a new last node.** Should it take the length of the arrow that just reached it? Or always two grid spaces (half a beat at the default scale)? *Recommended: the incoming arrow's length*, so growing a trill keeps an even rhythm.
8. **Sharing the library.** In this part, sharing is sending `~/Library/Application Support/SequenceTree/Ornaments.xml`, which replaces the receiver's file. Should the selection panel get "Import library…" / "Export library…" buttons that merge by uid instead? *Recommended: add them in the deployment pass.* Uids already make merging safe.
