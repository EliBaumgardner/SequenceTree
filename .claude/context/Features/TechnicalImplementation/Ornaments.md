# Ornaments — Technical Implementation

> Status: Implemented
> Implemented 2026-10-09 at uncommitted (on d8880a5).
> Written 2026-10-09 at ecd9950. Revised 2026-10-09 with the owner's answers. Describes: FeatureDescriptions/Ornaments.md (part: Creation)
> Built from d8880a5; drift: none (only `Source/Input/ConnectionOps.h` changed since ecd9950, which no step touches)
> Revised 2026-10-09: the host is a mark on one node (`OrnamentHostId` on the figure), chosen with its own Host button. See *Revision — the host is a mark* under Implementation Notes.

## Summary

This plan builds the creation part of ornaments:

- any number of ornament libraries, one file each, all shown in the ornament window;
- the window itself, opened from a new ornament button in the bottom bar;
- the selection panel, with one section per library and named symbol tiles;
- the build panel's mini graph of ornament nodes, one of them marked as the host;
- the freehand symbol editor;
- publishing, where edits go live only when published and only while a node is marked as the host.

The recommended approach adds its own small document class, `OrnamentLibraries`, in `Source/Graph/`. It writes the same node and arrow identifiers the main graph uses. That lets the builder reuse the existing `Node` and `Arrow` components and `ArrowInfo`'s geometry functions without going through `GraphState`, `NodeCanvas` or `NodeController`.

Nothing reaches the audio thread in this part, and no project's saved state changes. The plan has six steps, each of which can be tried in the standalone app, and adds about 1,600 lines, mostly in a new `Source/UI/Ornaments/` folder.

## Components

| Component | One job | Owner | Directory |
|---|---|---|---|
| `OrnamentLibraries` | Owns every loaded library and does all ornament editing. See below. | `SequenceTreeAudioProcessor` (member `ornamentLibraries`) | `Source/Graph/` |
| `OrnamentStep` (struct) | One node of a figure, in play order: `kind`, `offset`, `velocityPercent`, `durationWeight`. Tests check this shape now, and deployment will expand it into notes. | `OrnamentLibraries.h` | `Source/Graph/` |
| `OrnamentNodeKind` (enum) | `Note`, `Host`: a step's role in `OrnamentStep`, read from the figure's `OrnamentHostId`. | `OrnamentLibraries.h` | `Source/Graph/` |
| `OrnamentStatus` (enum) | `Draft` (never published), `Published` (the live version matches the figure), `ChangesPending` (published, with edits not yet published). | `OrnamentLibraries.h` | `Source/Graph/` |
| `ornamentUndoManager` | Undo history for ornament edits, kept apart from the project's `undoManager`. | `SequenceTreeAudioProcessor` | `Source/Plugin/` |
| New identifiers | `OrnamentLibraries`, `OrnamentLibrary`, `LibraryUid`, `LibraryName`, `OrnamentData`, `OrnamentFigure`, `PublishedFigure`, `OrnamentNoteData`, `OrnamentHostId`, `OrnamentUid`, `OrnamentName`, `OrnamentSymbol`, `OrnamentOffset`, `OrnamentVelocity`, `ActiveOrnamentUid`, `NextFigureNodeId` | `ValueTreeIdentifiers` | `Source/Graph/` |
| `OrnamentWindow` | The window's root component. Lays out the build panel and the selection panel, and handles undo and redo keys. Sets its own look-and-feel, because it is a window root. | `BottomBar::ornamentLauncher` (`PopupWindowLauncher`) | `Source/UI/Ornaments/` |
| `OrnamentSelectionPanel` | One `OrnamentLibrarySection` per loaded library, stacked and scrollable. Title-bar buttons: New library, Add library…. | `OrnamentWindow` | `Source/UI/Ornaments/` |
| `OrnamentLibrarySection` | A library's header (name editor, add-ornament button; right-click: Reveal in Finder, Remove library…) and its grid of tiles. | `OrnamentSelectionPanel` | `Source/UI/Ornaments/` |
| `OrnamentTile` | One ornament: its symbol, a name tag, and a status marker. Right-click: Duplicate, Delete…. | `OrnamentLibrarySection` | `Source/UI/Ornaments/` |
| `OrnamentBuildPanel` | Shows the active ornament. Top bar: the Host button (its own button, not in a pane), symbol button, Publish, undo/redo. Its body is the figure canvas or the symbol editor (`enum class BuildView { Figure, Symbol }`). | `OrnamentWindow` | `Source/UI/Ornaments/` |
| `OrnamentCanvas` | Renders one figure: an `OrnamentNode` per figure node and an `Arrow` per connection and dangling arrow. Rebuilds them when the figure changes. | `OrnamentBuildPanel` | `Source/UI/Ornaments/` |
| `OrnamentFigureController` | The canvas's gestures: place the first node, grow the chain, move a node, set the last node's length, remove a node. Its `DragState` enum holds the gesture in progress, and its `ClickMode { Build, PickHost }` whether the next click picks the host. | `OrnamentBuildPanel` | `Source/UI/Ornaments/` |
| `OrnamentNode : Node` | A figure node. Shows its signed offset (as `Modulator` shows `ModAmount`) and a velocity-percent badge, and hides the count badges. `OrnamentCanvas` draws the host with the inner ring. | `OrnamentCanvas` | `Source/UI/Ornaments/` |
| `OrnamentSymbolEditor` | Freehand drawing. Each stroke is one undoable write. Has Clear. | `OrnamentBuildPanel` | `Source/UI/Ornaments/` |
| `CustomLookAndFeel::drawOrnamentSymbol` | Paints a stored symbol into any bounds. Called by the tile, the bottom-bar button and the symbol editor now, and by the ornamented node in deployment. | `CustomLookAndFeel`, in a new `CustomLookAndFeelOrnaments.cpp` | `Source/UI/Theme/` |
| Bottom-bar ornament button | One `IconButton` in a new `ornamentPane`. Its painter draws the active symbol, and clicking it opens the window. | `BottomBar` | `Source/UI/Bars/` |
| Library folder | `~/Library/Application Support/SequenceTree/Ornament Libraries/`. Each `*.ornaments` file in it is one library. `Ornaments.settings` beside it remembers the active ornament. | `OrnamentLibraries` | — |

`OrnamentLibraries` does the following:

- loads every library file in the library folder, and saves each changed one back;
- creates, imports and removes libraries;
- creates, duplicates and removes ornaments, and grows, edits and shrinks an ornament's one-chain figure;
- publishes;
- answers `figureSteps` and `statusOf`.

### The document

```
OrnamentLibraries                    ActiveOrnamentUid           (in memory; persisted in Ornaments.settings)
└── OrnamentLibrary                  LibraryUid, LibraryName     (one per *.ornaments file; the file holds this subtree)
    └── OrnamentData                 OrnamentUid, OrnamentName, OrnamentSymbol, NextFigureNodeId
        ├── OrnamentFigure           OrnamentHostId              (what the builder edits; -1 while no node is the host)
        │   ├── OrnamentNoteData     Id, XPosition, YPosition, OrnamentOffset, OrnamentVelocity
        │   │   └── NodeChildrenIds
        │   │       └── NodeId       Id, arrow-info properties
        │   └── OrnamentNoteData     … same shape, for the last node …
        │       └── DanglingArrows
        │           └── DanglingArrow  ArrowTipX, ArrowTipY, arrow-info properties
        └── PublishedFigure                                      (copy of OrnamentFigure at the last Publish; absent while Draft)
```

- **Several libraries, nothing monolithic.** The library folder holds any number of library files, and every one of them appears in the selection panel as its own section.
  - **Adding a shared library.** "Add library…" copies a chosen `.ornaments` file into the folder and loads it. A library whose `LibraryUid` is already loaded is refused with a message, so the same library can never show up twice. Nothing is ever replaced.
  - **Sending a library.** "Reveal in Finder" on a section shows the file to send.
  - **Where new ornaments go.** Each section's add button creates the ornament in that library. The first ornament a user ever makes creates a library called "My Ornaments".
- **Publishing keeps a live copy.**
  - `PublishedFigure` is what deployment will play. `OrnamentFigure` is what the builder edits.
  - Publish copies the figure into `PublishedFigure`, and only when the figure's `OrnamentHostId` names one of its nodes.
  - An edit that removes the host can be made but not published, so nodes keep the last published version.
  - `statusOf` compares the two trees (`isEquivalentTo`) to tell `Published` from `ChangesPending`.
- **The host is a property of the figure, not a node type.** Every figure node is `OrnamentNoteData`, and `OrnamentHostId` on `OrnamentFigure` names the host. One property makes "at most one host" true by construction, moving the host is a single undoable write, and the published copy carries it with the figure.
- **Connections use the main graph's identifiers**, so `Arrow`, `ArrowBindingOps::getArrowInfo` and `ArrowInfo::durationFromDelta` read them unchanged.
- **Identity is a `juce::Uuid` string** for both libraries and ornaments. Deployment's per-node reference and its missing-ornament mark need ids that survive sharing, and every user's library would otherwise number from 1. Duplicating an ornament mints a new uid.
- **`OrnamentSymbol` is a list of strokes joined by `|`**, each `<width> <juce::Path::toString()>`, normalised to a unit square so it draws at any size; the width is a fraction of the square. `OrnamentLibraries::readSymbol` parses it, and an entry with no width (a symbol saved before widths) reads at `defaultStrokeWidth`.

## Where It Lands

### API Surface

| Existing | Use | Callers today |
|---|---|---|
| `ArrowInfo::durationFromDelta` | A figure node's duration weight, from its outgoing or dangling arrow. | `RTGraphBuilder::fillDurationMap`, `Arrow::getDuration` |
| `ArrowInfo::pixelsPerGridSpace`, `semitonesPerGridSpace` | A figure node's grid row → offset shift. | `ArrowInfo::pitchOffsetFromDelta`, `NodeCanvas` grid |
| `ArrowBindingOps::setArrowInfo` / `getArrowInfo` (`static`) | Writing and reading the figure's arrow trees. | `GraphState::connectNodes`, `NodeFactory::createDanglingArrow`, `Arrow`, `RTGraphBuilder` |
| `ArrowBindingOps::syncPitchBindings` | **Not used.** It shifts a node through the arrow into it, and a figure's first node has no arrow into it. Height in the builder belongs to the node itself (see step 1). | `GraphState::setNodePosition`, `NodeFactory` |
| `Node` | Base of `OrnamentNode`. Its constructor needs only a `juce::UndoManager&`, and `Node.cpp` includes `NodeCanvas.h` without using it. `bindToTree` and `bindValueEditorForMode` are virtual, and `Modulator` already overrides them. | `NodeManager::instantiateFromTree`; `RootNode`, `Modulator`, `TraversalFlagNode`, `Encapsulator` |
| `NodeVisual::hasInnerRim` | The host's look. It is set today only for encapsulation, which never appears in the builder. | `EncapsulationView`, `Encapsulator` |
| `Arrow` | Figure connections and the last node's length. The dangling constructor's `valueEditor` (the main graph's count limit) is hidden by `OrnamentCanvas`. | `ArrowManager` |
| `CustomLookAndFeel::drawArrow`, `drawNode`, `drawRulesButton`, `drawAddIcon` | Painting, unchanged. | `Arrow::paint`, `Node::paint`, `TraversalRulesWindow`, `TraversalRulesWindow::RulesPanel` |
| `ButtonPane`, `IconButton` (`painter`, `onClick`, `onRightClick`, `setSelected`) | Host, symbol and Publish buttons, undo/redo pane, bottom-bar button. | `Titlebar::configureModePane`, `BottomBar` |
| `PopupWindowLauncher` / `PopupWindow` | The ornament window. In a plugin it stays on top, and closing it hides it rather than destroying it. | `MenuArea::settingsLauncher`, `TraversalMenu`, `NodeMenu`, `NodeController` |
| `Bar`, `ValueEditor` + `NumberFormat` (`showsPositiveSign`, `suffix`) + `TextFormat`, `ContextMenu` | Title bars, offset / velocity / name editors, right-click menus. | Throughout `UI/` |
| `BottomBar(NodeCanvas&, TraversalSession&, ValueTree colourPresets, UndoManager&)` | Gains `OrnamentLibraries&`, `juce::UndoManager& ornamentUndoManager`, `ornamentPane`, `ornamentLauncher`. | Only in `SequenceTreeAudioProcessorEditor`'s constructor |
| `SequenceTreeAudioProcessor` | Gains `ornamentUndoManager` and `ornamentLibraries`, loaded in its constructor. `getStateInformation` and `applyRestoredState` are not touched. | — |
| `ValueTreeIdentifiers.{h,cpp}`, `CMakeLists.txt` | New identifiers; every new `.cpp` in `target_sources(SequenceTree …)`; `Tests/OrnamentTests.cpp` in `SequenceTree_GraphTests`. | — |

### Structure

```
SequenceTreeAudioProcessor
├── ornamentUndoManager
└── ornamentLibraries : OrnamentLibraries          (Graph/; ValueTree::Listener on its own tree; Timer for saving)

SequenceTreeAudioProcessorEditor
└── bottomBar : BottomBar(…, p.ornamentLibraries, p.ornamentUndoManager)
    ├── ornamentPane → ornament button (painter → drawOrnamentSymbol)
    └── ornamentLauncher : PopupWindowLauncher
        └── OrnamentWindow(CustomLookAndFeel&, OrnamentLibraries&, UndoManager&)
            ├── OrnamentBuildPanel(OrnamentLibraries&, UndoManager&)
            │   ├── topBar : Bar → kindPane, symbolButton, publishButton, undoRedoPane
            │   ├── canvas : OrnamentCanvas → OrnamentNode…, Arrow…
            │   ├── controller : OrnamentFigureController(OrnamentCanvas&, OrnamentLibraries&, UndoManager&)
            │   └── symbolEditor : OrnamentSymbolEditor
            └── OrnamentSelectionPanel(OrnamentLibraries&, UndoManager&)
                └── OrnamentLibrarySection… → OrnamentTile…
```

- **Dependencies are passed by reference**, each in its own constructor parameter. Containers pass them down, as `MenuArea` does for `NodeMenu` → `TraversalRulesWindow`.
- **The canvas is split from its controller** the way `NodeCanvas` is from `NodeController`: the canvas holds components and the controller holds gestures. It is not a feature gate. Its four gestures are fixed and would grow only if branching ornaments were ever added.
- **`NodeController` and `NodeCanvas` are not touched in this part.**

### Data Flow

```
gesture in OrnamentCanvas / OrnamentSymbolEditor / selection panel      (message thread)
        │
        ▼
OrnamentLibraries mutator ── writes the tree under ornamentUndoManager
        │
        ├─► listeners: OrnamentCanvas rebuilds; sections add or remove tiles; OrnamentBuildPanel refreshes
        │   Publish and status; BottomBar repaints its button
        │
        └─► OrnamentLibraries' own listener marks the changed library dirty and restarts a 500 ms timer
              └─► timerCallback writes each dirty library to its own file (juce::TemporaryFile, atomic replace)

processor constructor ─► ornamentLibraries.load(folder) ─► one OrnamentLibrary per file; active uid from Ornaments.settings
window opens          ─► ornamentLibraries.rescan()     ─► picks up files added, changed or removed by another instance
```

- **Threads.** Everything happens on the message thread, and the audio thread never reads the libraries in this part.
- **Projects.** Projects save exactly what they saved before.

### Invariants

- **The audio thread is untouched**, so the allocation, UI-access and boundary-type rules are not in play. Deployment will flatten `PublishedFigure` into plain `RTData` on the snapshot path, never a `ValueTree`.
- **Arrow geometry is data.** Duration weights are derived from arrow geometry on every read, and dragging a figure node retimes the ornament.
- **Duration is derived, pitch is owned.** The offset is owned: the user types it. Height only *shifts* it, by the number of grid rows the node moved, as the main canvas shifts a typed pitch rather than recomputing it. Moving a node shifts that node only, and the first node shifts like any other.
- **Feature gates.** `NodeController` and `NodeCanvas` gain nothing.
- **Components and windows.** Every component takes its dependencies by reference. `OrnamentWindow` sets its own look-and-feel, as `TraversalRulesWindow` does. No constructor reads the theme.
- **Code style.** Structs live in headers; named states are enums (`OrnamentStatus` has three); there are no getters or setters and no `_` in names; definitions follow call-hierarchy order (`refactor.order`).
- **Nothing existing changes.** No current component, document or saved state changes, and the bottom bar gains one button.

## Approaches Considered

### A. A second `GraphState` / `NodeCanvas` / `NodeController` per open ornament

This reuses connect, disconnect and dangling-arrow code, but at a high cost:

- `NodeCanvas` needs `SequenceTreeAudioProcessor&` and `RTGraphBuilder&`, and `NodeController` needs `TraversalSession&`. A second instance would publish figures to the audio thread as trees.
- `GraphState` writes count, switch and traversal properties that mean nothing in a figure.
- Every main-canvas gesture would need guards against the ornament canvas.

It is high risk, and it works against the feature-gate rule and against narrow dependencies.

### B. A dedicated `OrnamentLibraries` document and a small canvas reusing `Node`, `Arrow` and `ArrowInfo` (recommended)

- **What it adds:** one document class that knows the one-chain, host and publish rules and nothing else, and a canvas with four gestures.
- **What it reuses:** node and arrow painting, duration labels and the geometry functions, because the document writes the same arrow identifiers.
- **Cost:** about 1,600 lines.
- **Risk:** main-canvas changes to `Node` or `Arrow` can show up in the builder.
- **Fit:** clean.

### C. A row of steps with proportions instead of a graph

This contradicts the owner's mini-graph decision, so it is ruled out.

**Recommendation: B.** It keeps ornament editing off the audio path and out of the feature gates, and it reuses exactly the pieces that make arrow geometry data on the main canvas.

## Plan

### Step 1 — The library document

- **Goal:** libraries and ornaments exist as data. They can be created, imported, shaped, published, saved and reloaded. This step has tests and no UI.
- **Changes:**
  - **`ValueTreeIdentifiers.{h,cpp}`:** the identifiers listed under *Components*.
  - **`Source/Graph/OrnamentLibraries.{h,cpp}`** (new). Declarations are grouped by return type, and the enums and `OrnamentStep` sit above the class:
    ```cpp
    enum class OrnamentNodeKind { Note, Host };
    enum class OrnamentStatus   { Draft, Published, ChangesPending };

    struct OrnamentStep
    {
        OrnamentNodeKind kind;
        int              offset;
        int              velocityPercent;
        int              durationWeight;
    };

    class OrnamentLibraries : public juce::ValueTree::Listener, private juce::Timer
    {
    public:
        OrnamentLibraries();
        ~OrnamentLibraries() override;

        void load(const juce::File& folder);
        void rescan();

        juce::ValueTree addLibrary       (const juce::String& name, juce::UndoManager* undoManager);
        juce::ValueTree importLibrary    (const juce::File& file);
        juce::ValueTree addOrnament      (juce::ValueTree library, juce::UndoManager* undoManager);
        juce::ValueTree duplicateOrnament(const juce::String& uid, juce::UndoManager* undoManager);
        juce::ValueTree findOrnament     (const juce::String& uid) const;
        juce::ValueTree addFigureNode    (juce::ValueTree ornament, int parentNodeId, juce::Point<int> position, juce::UndoManager* undoManager);

        void removeLibrary        (const juce::String& libraryUid);
        void removeOrnament       (const juce::String& uid, juce::UndoManager* undoManager);
        void removeFigureNode     (juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager);
        void toggleHost           (juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager);
        void setFigureNodePosition(juce::ValueTree ornament, int nodeId, juce::Point<int> position, juce::UndoManager* undoManager);
        void setLastNodeLength    (juce::ValueTree ornament, juce::Point<int> tipOffset, juce::UndoManager* undoManager);
        void addSymbolStroke      (juce::ValueTree ornament, const juce::Path& stroke, juce::UndoManager* undoManager);
        void publish              (juce::ValueTree ornament, juce::UndoManager* undoManager);

        std::vector<OrnamentStep> figureSteps(const juce::ValueTree& figure) const;

        OrnamentStatus statusOf(const juce::ValueTree& ornament) const;

        bool isPublishable(const juce::ValueTree& ornament) const;

        juce::ValueTree libraries;
        juce::File      folder;
        …
    };
    ```
  - **`addFigureNode`** keeps the figure one chain:
    - With `parentNodeId == -1`, it places the first node, and only into an empty figure. That node gets a dangling arrow two grid spaces long.
    - Otherwise the parent must be the chain's last node.
    - It connects parent → new node with `ArrowBindingOps::setArrowInfo` (`xBinding = DurationBind`, `yBinding = NoBind`, so an arrow's vertical extent never adds to its duration).
    - It removes the old last node's dangling arrow and gives the new node one equal to the parent → new-node delta, so the new last node takes the length of the arrow that reached it.
  - **`setFigureNodePosition`** writes the position and shifts the node's own `OrnamentOffset` by `round(oldY / pixelsPerGridSpace) − round(newY / pixelsPerGridSpace)` semitones (up is higher), clamped to ±48.
    - Both rows are rounded against the fixed grid, so a sequence of small moves never drifts.
    - A typed offset stays put until the node changes row.
    - No other node changes.
  - **`removeFigureNode`** joins the removed node's parent to its child. If the last node goes, its dangling arrow moves to the new last node. Removing the host sets `OrnamentHostId` to -1 in the same transaction, so undo restores the mark.
  - **`toggleHost`** marks a node as the host, moving the mark from any other node, or clears it when the node already is the host.
  - **`figureSteps(figure)`** walks the chain from the node no arrow points into.
    - Each weight is `ArrowInfo::durationFromDelta(ArrowBindingOps::getArrowInfo(arrow), dx, dy)` of the node's outgoing arrow, or of its dangling arrow for the last node.
    - Steps with weight 0 are dropped, as zero-duration arrows are ineligible on the main canvas.
  - **Publishing:**
    - `isPublishable` requires `OrnamentHostId` to name a node of the figure, at least one other node, and a total weight above 0.
    - `publish` copies `OrnamentFigure` into `PublishedFigure` only when it is publishable.
    - `statusOf` gives `Draft` with no `PublishedFigure`, `Published` when the two trees are equivalent, and `ChangesPending` otherwise.
  - **Saving:**
    - The listener finds the changed tree's `OrnamentLibrary` ancestor, adds its uid to `dirtyLibraryUids`, and restarts a 500 ms timer.
    - `timerCallback` writes each dirty library's `createXml()` to its file through `juce::TemporaryFile::overwriteTargetFileWithTemporary`, and records the file's modification time.
    - The file name is the library name plus `.ornaments`; a name collision gets a numeric suffix.
  - **Loading:**
    - `load(folder)` reads every `*.ornaments` file and skips a file whose `LibraryUid` is already loaded.
    - It reads `ActiveOrnamentUid` from `Ornaments.settings`, a `juce::PropertiesFile` in the same folder; if that ornament is gone, the first ornament becomes active.
  - **`importLibrary`** refuses a `LibraryUid` that is already loaded, copies the file into the folder, and loads it.
  - **`removeLibrary`** moves the file to the trash (`juce::File::moveToTrash`). It is not undoable, which is why the UI asks first.
  - **`Source/Plugin/PluginProcessor.{h,cpp}`:**
    - Members `juce::UndoManager ornamentUndoManager;` and `OrnamentLibraries ornamentLibraries;`, after `colourPresets`.
    - The constructor body calls `ornamentLibraries.load(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support").getChildFile(JucePlugin_Name).getChildFile("Ornament Libraries"));`, the folder `interfaceSettingsOptions` already uses.
  - **`CMakeLists.txt`:** `Source/Graph/OrnamentLibraries.cpp` in `target_sources(SequenceTree …)`, and `Tests/OrnamentTests.cpp` in `SequenceTree_GraphTests`.
  - **`Tests/OrnamentTests.cpp`** (new). Expected values come from `ArrowInfo`'s constants (50 px = one grid space = 250 ms = 1 semitone), not from running the code:
    - **"Lower grace":** a note at (0,0) with offset −1 and velocity 70, then a host at (100,0) with a dangling tip at +200. This gives steps `{Note, -1, 70, 500}` and `{Host, 0, 100, 1000}`, which split a 1000 ms note into 333 and 666.
    - **Growing the chain** gives the new last node a dangling arrow equal to the incoming delta.
    - **Refusals:** growing from a node that is not last, and a second first node.
    - **The host mark:** marking another node moves it, marking the host again clears it, and removing the host clears it until undo restores it.
    - **Removing the middle** of three nodes joins its neighbours.
    - **Height:** moving the note from y=0 to y=−100 shifts its offset from −1 to +1, and leaves the host's offset at 0.
    - **Publishing:**
      - `isPublishable` is false without a host, false with a host alone, and true for "Lower grace".
      - After `publish`, removing the host gives `ChangesPending`, `isPublishable` is false, and `PublishedFigure` still yields the two steps.
      - Re-adding the host and publishing gives `Published`.
    - **Duplicating** gives a new uid and an equal figure.
    - **Two libraries in one folder** both load. Importing a file whose `LibraryUid` is already loaded is refused. A save → `load` round trip gives equal trees.
- **Mechanical edits:** none.
- **Behaviour delta:**
  - Nothing is visible yet.
  - Library files are written only after the first edit.
  - Project save and restore are unchanged.
- **Invariants:** message thread only.
- **Verification:**
  - `cmake --build cmake-build-debug --target SequenceTree_Standalone SequenceTree_Tests SequenceTree_GraphTests`, then `ctest --output-on-failure`.
  - `design-rules.sh --file` and `readability.sh --file` on every touched file.
  - Manual: an existing project loads and sounds the same.
- **Rollback:** remove the new files, the processor members, the identifiers and the CMake lines. Nothing else depends on them yet.

### Step 2 — Window, selection panel, bottom-bar button

- **Goal:** the user opens the ornament window from the bottom bar and manages libraries and ornaments: new library, add a shared library, reveal and remove a library, and add, rename, duplicate and delete ornaments. They also choose the active ornament.
- **Changes:**
  - **`OrnamentWindow`** (new):
    - Constructor: `OrnamentWindow(CustomLookAndFeel&, OrnamentLibraries&, juce::UndoManager&)`.
    - It calls `setLookAndFeel(&lookAndFeel)`, and `setLookAndFeel(nullptr)` in the destructor.
    - It lays out `buildPanel` on the left and `selectionPanel` on the right.
    - `keyPressed` handles Cmd+Z, Cmd+Shift+Z and Cmd+Y, because the main editor's key listener sits on a different top-level window.
    - The constructor and `visibilityChanged` call `ornamentLibraries.rescan()` (step 6 fills it in).
  - **`OrnamentBuildPanel`** (new; a placeholder body for now): its top bar and the active ornament's name. It listens for `ActiveOrnamentUid`.
  - **`OrnamentSelectionPanel`** (new):
    - It listens on `libraries` and keeps one `OrnamentLibrarySection` per `OrnamentLibrary` inside a `juce::Viewport`.
    - Its title `Bar` holds **New library**, which calls `addLibrary("New Library")`.
    - It also holds **Add library…**, which opens a `juce::FileChooser` for `*.ornaments` and calls `importLibrary`. A refusal shows a `juce::AlertWindow` saying the library is already added.
  - **`OrnamentLibrarySection`** (new):
    - A header with a `ValueEditor` (`TextFormat`) bound to `LibraryName`, and an add `IconButton` (`drawAddIcon`) that calls `addOrnament(library)` and makes the new ornament active.
    - Header right-click opens a `ContextMenu`:
      - Reveal in Finder calls `File::revealToUser`.
      - Remove library… asks through `juce::AlertWindow::showAsync` (OK/Cancel), captures a `SafePointer`, and calls `removeLibrary` on OK.
    - Below the header sits the grid of `OrnamentTile`s.
  - **`OrnamentTile`** (new):
    - A symbol area (blank until step 5), a name tag `ValueEditor` bound to `OrnamentName`, and a status marker.
    - A click sets `ActiveOrnamentUid`.
    - Right-click offers Duplicate and Delete…; Delete asks first, as above, then calls `removeOrnament`.
  - **`BottomBar.{h,cpp}`:**
    - The constructor gains `OrnamentLibraries& ornamentLibraries, juce::UndoManager& ornamentUndoManager`.
    - New members: `ButtonPane ornamentPane` and `IconButton* ornamentButton`, plus `PopupWindowLauncher ornamentLauncher`, built like `MenuArea::settingsLauncher`. Its factory makes `OrnamentWindow(CustomLookAndFeel::get(*this), ornamentLibraries, ornamentUndoManager)` at `defaultWidth` × `defaultHeight`.
    - The button's click calls `ornamentLauncher.show()`.
    - `resized` places `ornamentPane` left of `toolPane`.
    - `BottomBar` becomes a `juce::ValueTree::Listener` on `ornamentLibraries.libraries`, and repaints the button when the active ornament or its symbol changes.
  - **`PluginEditor.cpp`:** passes `p.ornamentLibraries` and `p.ornamentUndoManager` to `BottomBar`.
  - **`CustomLookAndFeelOrnaments.cpp`** (new): `drawOrnamentButton(graphics, bounds, state)`, a tile with a placeholder glyph for when no ornament has a symbol yet.
  - **CMake:** register the new `.cpp` files.
- **Mechanical edits:** none.
- **Behaviour delta:**
  - An ornament button appears in the bottom bar and opens the window.
  - Libraries and ornaments can be managed as described, and they survive a relaunch.
  - Nothing on the main canvas changes.
- **Invariants:** `OrnamentWindow` is a window root that sets its own look-and-feel. The launcher reads `CustomLookAndFeel::get(*this)` only on click, when `BottomBar` already has a parent.
- **Verification:**
  - Build, test, and run the design-rule and readability checks.
  - Manual, standalone and Live:
    - Make two libraries with ornaments in each and rename things.
    - Delete an ornament, cancelling first and then confirming.
    - Reveal a library file, copy it elsewhere, remove the library, then add it back with Add library….
    - Add it again and check that it is refused.
    - Relaunch and check that everything is unchanged.
    - In Live, check that the window stays above the plugin window.
- **Rollback:** revert `BottomBar` and `PluginEditor`, and drop the new UI files.

### Step 3 — The figure canvas

- **Goal:** the user builds a figure.
  - They place nodes of either kind and grow the one chain.
  - They move nodes: horizontal distance sets duration, height shifts offset.
  - They set the last node's length, edit offset and velocity, and remove nodes.
- **Changes:**
  - **`OrnamentNode : Node`** (new):
    - `bindToTree` hides `countEditor`, `switchCountEditor` and `subLoopLimitEditor`. It binds a new `velocityEditor` badge to `OrnamentVelocity`, using `NumberFormat(1, 200)` with `suffix = "%"`.
    - `bindValueEditorForMode` binds `nodeValueEditor` to `OrnamentOffset` with a signed `NumberFormat(-48, 48)`, as `Modulator::bindValueEditorForMode` does.
    - `resized` puts `velocityEditor` in the `countEditor` slot, and `respondToClick` begins editing the clicked editor.
    - The host sets `hasInnerRim = true`.
  - **`OrnamentCanvas`** (new):
    - It holds the shown ornament, `std::unordered_map<int, std::unique_ptr<OrnamentNode>> nodes` and `juce::OwnedArray<Arrow> arrows`.
    - It listens on the ornament's `OrnamentFigure` and rebuilds everything on any change. A figure holds a handful of nodes, so a full rebuild is cheap and avoids the incremental bookkeeping `NodeManager` and `ArrowManager` need.
    - Dangling arrows get `valueEditor->setVisible(false)`.
    - `paint` fills the canvas and draws the grid at `ArrowInfo::pixelsPerGridSpace`.
  - **`OrnamentFigureController : juce::MouseListener`** (new), with `enum class DragState { Idle, MovingNode, GrowingChain, MovingDanglingTip }`:
    - Shift+Click on an empty figure places the first node, of the kind chosen in the kind pane.
    - Dragging from the last node shows a preview `Arrow`, and releasing calls `addFigureNode`. Dragging any other node moves it.
    - A plain drag on a node calls `setFigureNodePosition`, snapped to the grid.
    - Dragging the dangling tip calls `setLastNodeLength`.
    - Shift+Right-Click on a node calls `removeFigureNode`, the same delete gesture as on the main canvas.
    - Each gesture is one `undoManager.beginNewTransaction()`.
  - **`OrnamentBuildPanel`:**
    - It adds `kindPane` (`ButtonPane::Selection::Exclusive`), with Note and Host buttons labelled like `Titlebar`'s mode buttons. Host is disabled while the figure has a host.
    - It owns `canvas` and `controller` and calls `canvas.addMouseListener(&controller, true)`.
  - **CMake:** register the new `.cpp` files.
- **Mechanical edits:** none.
- **Behaviour delta:** "Lower grace" can be built exactly as the description walks through. Duration labels show each arrow's length.
- **Invariants:** durations are derived on every read, and offsets are shifted, never recomputed from the parent.
- **Verification:**
  - Build, test, and run the checks.
  - New test: moving the host from x=100 to x=150 changes the weights to 750 and 1000.
  - Manual:
    - Build "Lower grace" and drag the host sideways; both labels change.
    - Drag the first node up two grid spaces; −1 becomes +1.
    - Remove the middle of three nodes; the chain rejoins.
    - Undo and redo every gesture.
- **Rollback:** drop the three classes and restore the placeholder body.

### Step 4 — Publishing

- **Goal:** Publish is greyed out until a host is connected. Edits go live only when published. An edit without a host can't be published, and nodes keep the last published version.
- **Changes:**
  - **`OrnamentBuildPanel`:**
    - `publishButton` in the top bar, drawn with `drawRulesButton` and the text "Publish".
    - The figure listener refreshes `setEnabled(isPublishable(ornament) && statusOf(ornament) != OrnamentStatus::Published)`.
    - Clicking it calls `publish`.
  - **`OrnamentTile`:** the marker shows `statusOf`: "draft", none, or "unpublished changes".
- **Mechanical edits:** none.
- **Behaviour delta:**
  - Publish is enabled only when there is something valid to publish.
  - Tiles show where each ornament stands.
- **Invariants:** unchanged from step 3.
- **Verification:**
  - Build, test, and run the checks.
  - Step 1's publish tests cover the rule.
  - Manual:
    - With only a host, Publish stays greyed out.
    - Connect a note, publish, and the marker clears.
    - Remove the host: "unpublished changes" appears and Publish stays greyed out.
    - Add the host back and publish again.
- **Rollback:** remove the button and the marker binding.

### Step 5 — The symbol editor

- **Goal:** the user draws an ornament's symbol freehand, and it appears on its tile and on the bottom-bar button.
- **Changes:**
  - **`OrnamentSymbolEditor`** (new):
    - `mouseDown` starts a `juce::Path`, and `mouseDrag` adds a `lineTo` after every two pixels of movement.
    - `mouseUp` normalises the stroke to the unit square and calls `addSymbolStroke`, one transaction per stroke.
    - A Clear `IconButton` writes an empty `OrnamentSymbol`.
    - `paint` draws the stored symbol and the stroke in progress.
  - **`OrnamentLibraries::addSymbolStroke`** appends the stroke to the parsed stored path and writes `toString()` back.
  - **`CustomLookAndFeel::drawOrnamentSymbol(juce::Graphics&, juce::Rectangle<float>, const juce::String& symbol)`** in `CustomLookAndFeelOrnaments.cpp`:
    - It parses the path, fits it into the bounds while keeping its aspect, and strokes it with rounded joins scaled to the bounds.
    - It has three callers now and a fourth in deployment.
  - **`OrnamentBuildPanel`:** `symbolButton` toggles `BuildView` between `Figure` and `Symbol`.
  - **`OrnamentTile`** and the bottom-bar button painter call `drawOrnamentSymbol`.
- **Mechanical edits:** none.
- **Behaviour delta:** symbols can be drawn, cleared and undone stroke by stroke, and they show on tiles and on the button at any size.
- **Invariants:** the theme is read in `paint`, never in a constructor.
- **Verification:**
  - Build, test, and run the checks.
  - New test: two strokes survive a save and load.
  - Manual: draw a trill sign; the tile and the button match it, it scales cleanly with the window, and undo removes the last stroke.
- **Rollback:** remove the editor, the draw function and the toggle.

### Step 6 — Other instances and library files

- **Goal:** two plugin instances, or the plugin and the standalone, never silently lose each other's libraries or ornaments.
- **Changes:**
  - **`OrnamentLibraries::rescan`** compares the folder with what is loaded:
    - a new file is loaded;
    - a file whose modification time is newer than the one recorded at the last load or save is reloaded;
    - a library whose file is gone is unloaded.
  - When anything changed, it keeps `ActiveOrnamentUid` if that ornament still exists and clears `ornamentUndoManager`'s history, because the history may point at trees that are gone.
  - Saves never trigger a rescan.
- **Mechanical edits:** none.
- **Behaviour delta:**
  - Opening the window shows libraries and ornaments added in another instance, or dropped into the folder in Finder.
  - For edits to the same library at the same moment, the last save wins.
- **Invariants:** message thread only.
- **Verification:**
  - New tests:
    - write a library from a second `OrnamentLibraries` instance, then `rescan` and see it;
    - delete a file, then `rescan` and see the library gone.
  - Manual: run two instances in Live, add a library in one, and open the window in the other.
- **Rollback:** reduce `rescan` to a no-op.

## Risks

- **`Node` or `Arrow` change for the main canvas and break the builder.** This is likely over time. It would show up as stray badges or a visible count editor in the builder. The guard is that `OrnamentNode` and `OrnamentCanvas` use only virtuals and public members `Modulator` already relies on, and step 3's manual check catches it.
- **Two instances editing the same library at once.** This is unlikely. The last save wins, and the other's edits from the same half-second are lost. Step 6 keeps a window from showing a stale library, but merging is not attempted.
- **The same library file copied into the folder twice in Finder.** The second copy has the same `LibraryUid`, so `load` and `rescan` skip it, and it is never shown twice.
- **A corrupt or hand-edited file.** `load` keeps only well-formed `OrnamentData` that have a uid. The atomic replace means a crash in the middle of a save leaves the previous file intact.
- **Undo crossing windows.** The separate `ornamentUndoManager` means project undo never touches ornaments, and loading a project (`applyRestoredState` → `clearUndoHistory`) never wipes ornament history.
- **Removing a library can't be undone.** The file goes to the trash and can be recovered from there, and the UI asks before removing.
- **Zero-length arrows.** Steps with weight 0 are dropped, and publishing needs a total weight above 0, so nothing can divide by zero.

## Later Parts

Deployment is not built here. This plan leaves the following in place for it:

- **Stable ids.** Every ornament has a uid that survives sharing. A node will store only that uid, and a uid missing from every loaded library gives the missing-ornament mark.
- **The live figure.** `PublishedFigure` holds what deployment plays, and `figureSteps(PublishedFigure)` already returns it as plain values in play order.
  - `RTGraphBuilder` can flatten those values into a fixed-size `RTData` array on the node, so no `ValueTree` reaches the audio thread.
  - Splitting the note's actual duration by weight gives the stretch-on-retime rule, since `EventManager::followTempo` already rescales active notes.
- **Drawing.** `drawOrnamentSymbol` is ready to draw the symbol at the top of a node's area.
- **Canvas gestures.** Dragging from the ornament button and Shift+Right-Click on the symbol are canvas gestures. They will arrive as a class of their own that `NodeController` routes to, not as new `DragState` values in it. The project rule asks to be consulted before that, so deployment's plan will ask.

## Decisions for the Owner

All eight earlier decisions are answered and folded into the plan above:

1. Separate undo history.
2. Height shifts the offset.
3. Publishing needs a host, and nodes keep the last published version.
4. A removed middle node's neighbours join up.
5. Velocity goes up to 200%.
6. The host gets the inner ring. Since the revision, the host is marked with its own Host button rather than placed as a node kind.
7. A new last node takes the incoming arrow's length.
8. A user has several libraries, all shown, and sharing adds a library and never replaces one.

Nothing is open.

## Implementation Notes

### Step 1 — The library document: done (uncommitted)

- **Built:** 16 identifiers in `ValueTreeIdentifiers`; `Source/Graph/OrnamentLibraries.{h,cpp}`; `ornamentUndoManager` and `ornamentLibraries` on the processor, loaded in its constructor; `Tests/OrnamentTests.cpp` (9 cases) in `SequenceTree_GraphTests`; both `.cpp`s registered in `CMakeLists.txt`.
- **Where it differs from the plan:**
  - `removeLibrary` takes `juce::UndoManager&` and clears its history. JUCE undoes a child-add by index, so an older "add library" undone after a non-undoable removal would remove the wrong library. It is the same reason step 6 gives for clearing on rescan.
  - The destructor flushes pending saves, so an edit made within 500 ms of closing the plugin is not lost.
  - `timerCallback` is in the public section, so the tests can force a save without a message loop.
  - `statusOf` compares a copy of `PublishedFigure` retyped to `OrnamentFigure`, because `isEquivalentTo` compares tree types too.
  - `rescan` is not declared yet; step 6 adds it together with its body, and step 2's calls to it come in with step 6.
  - `addSymbolStroke` (step 5's body) is already written, because step 1's header declares it.
  - `addOrnament` with no library uses the first loaded library, and creates "My Ornaments" only when none exists.
  - The library listener keeps `ActiveOrnamentUid` valid on every add and remove, so removing the active ornament (or undoing its addition) moves the selection to the first remaining one, and the first ornament ever added becomes active.
  - A library removed from the tree by any route (Remove, or undoing its addition) has its file moved to the Trash.
- **Gates:** design-rules clean on every touched file. The readability gate flags `OrnamentLibraries.h` only for the blank lines between return-type groups and the `static` placed last, which is the owner's header-grouping preference (`ScriptRun.h` is flagged the same way). The `PluginProcessor.h:60` interleave finding is pre-existing.
- **Evidence:** expected values come from `ArrowInfo` (50 px = one grid space = 250 ms = one semitone). Flipping the sign of the row shift made the height test fail, and restoring it made it pass again. The GraphTests processors never wrote into the real library folder.
- **Manual check still owed:** an existing project loads and sounds the same.

### Step 2 — Window, selection panel, bottom-bar button: done (uncommitted)

- **Built:**
  - `Source/UI/Ornaments/`:
    - `OrnamentWindow`: a window root that sets its own look-and-feel and handles Cmd+Z, Cmd+Shift+Z and Cmd+Y.
    - `OrnamentBuildPanel`: a title bar with the active ornament's name, which can be edited, over a placeholder body.
    - `OrnamentSelectionPanel`: New library and Add library… in its title bar, over a scrolling stack of sections.
    - `OrnamentLibrarySection`: the library name, which you edit by double-clicking. It has an add button, which also makes the new ornament active, a right-click menu with Reveal in Finder and Remove library…, and the tile grid.
    - `OrnamentTile`: click to make it active, double-click to rename. Its right-click menu has Duplicate and Delete…, and Delete asks first.
  - `BottomBar`: an `ornamentPane` left of `toolPane`, and an `ornamentLauncher` built like `MenuArea::settingsLauncher`. Its tooltip names the active ornament.
  - `PluginEditor` passes the two new dependencies, and CMake registers the five `.cpp`s.
- **Where it differs from the plan:**
  - `drawOrnamentButton` and a new `drawImportIcon` (for Add library…) live in `CustomLookAndFeelButtons.cpp`, because both use that file's local `drawGlyph`. `CustomLookAndFeelOrnaments.cpp` arrives in step 5 with `drawOrnamentSymbol`.
  - The tile's status marker is left to step 4, where it gets something to show.
  - The `rescan()` calls are left to step 6.
  - `OrnamentLibraries::libraryFiles` and `LibraryFile` are public, so Reveal in Finder can reach the file. A library not yet saved reveals the folder instead.
  - Renaming is a double-click, both on a section header and on a tile. A single click activates a tile, and a right-click opens the menu.
  - The panel resyncs only when a library or ornament is added or removed, so figure edits never rebuild the panel.
  - The tile is a fixed size, so it lays itself out in its constructor and has no `resized`.
- **Gates:** design-rules clean on every touched file. Readability flags only the grouped headers (`OrnamentLibraries.h`, `OrnamentLibrarySection.h`), for the blank lines between return-type groups, which is the owner's decision.
- **Tests:** 101/101. No new test: this step is UI only, and the document behaviour under it is covered by step 1.
- **Manual checks still owed (standalone and Live):**
  - make two libraries with ornaments in each, and rename things;
  - delete an ornament, cancelling first and then confirming;
  - reveal a library file and copy it elsewhere, remove the library, then add it back with Add library…;
  - add it again and see it refused;
  - relaunch and check that everything is unchanged;
  - in Live, check that the window stays above the plugin window.
- **Follow-up:** `OrnamentLibrarySection.h` was later re-laid out by `refactor.order`. A single-function `int` group lost its blank line, which cleared the gate at no cost.

### Step 3 — The figure canvas: done (uncommitted)

- **Built:**
  - **`OrnamentNode : Node`:**
    - it hides the count, switch and sub-loop badges;
    - a `velocityEditor` badge takes the count badge's slot (`NumberFormat(1, 200)`, `%`);
    - its centre value is the signed `OrnamentOffset` (±48);
    - the host gets `hasInnerRim`;
    - `respondToClick` opens the velocity editor before falling back to `Node`'s.
  - **`OrnamentCanvas`:**
    - it draws the grid at `pixelsPerGridSpace` around a grid-aligned origin, with the origin row drawn stronger;
    - it rebuilds every node and arrow in `handleAsyncUpdate`;
    - it hides the dangling arrow's count editor;
    - it provides the preview arrow and `findNodeAt`.
  - **`OrnamentFigureController`:** `DragState { Idle, MovingNode, GrowingChain, MovingDanglingTip }`. Every gesture is snapped to the grid and is one undo transaction.
  - **`OrnamentBuildPanel`:**
    - the Note/Host pane, with Host disabled while the figure has one;
    - undo and redo buttons;
    - it hosts the canvas and the controller.
- **Where it differs from the plan:**
  - **Growing the chain is Shift+drag from the last node.** The plan said a plain drag, but that would leave the last node impossible to move. Shift+drag matches the main canvas, where Shift+drag from a node draws an arrow. A plain drag moves any node, and a click without a drag edits the badge or offset under the mouse.
  - **The canvas doesn't listen on the figure.** `OrnamentBuildPanel`'s listener on the libraries tree already receives every change, so it filters to the shown ornament and calls `canvas.triggerAsyncUpdate()`.
  - **The canvas's children take no mouse clicks.** The controller listens on the canvas alone and hit-tests nodes itself, so a rebuild in the middle of a drag never deletes the component the drag started on.
  - **No `+`/`−` buttons on figure nodes.** Nodes don't receive mouse moves, so those buttons never show. The offset and velocity are edited by clicking and typing.
- **Test:** moving the host from x=100 to x=150 gives weights 750 and 1000.
- **Manual checks still owed:**
  - build "Lower grace", then drag the host sideways; both labels change;
  - drag the first node up two grid spaces; −1 becomes +1;
  - remove the middle of three nodes; the chain rejoins;
  - undo and redo every gesture.

### Step 4 — Publishing: done (uncommitted)

- **Built:**
  - **Publish button:** in the title bar, drawn with `drawRulesButton`. It is enabled only when `isPublishable` holds and the status isn't `Published`, and publishing is one undo step.
  - **Tile status marker:** "draft", "unpublished", or nothing once published.
  - **Selection panel:** it repaints when a figure changes, and resyncs only when a library or ornament is added or removed.
- **Where it differs from the plan:** the marker reads "unpublished" rather than "unpublished changes", which doesn't fit a 72 px tile.
- **Manual checks still owed:**
  - with only a host, Publish stays greyed out;
  - connect a note and publish; the marker clears;
  - remove the host; "unpublished" appears and Publish stays greyed out;
  - add the host back and publish again.

### Step 5 — The symbol editor: done (uncommitted)

- **Built:**
  - **`OrnamentSymbolEditor`:**
    - it draws in a centred square, adding a point every 2 px;
    - on release, the stroke is normalised to the unit square and saved with `addSymbolStroke` (one transaction);
    - a Clear button empties the symbol.
  - **`CustomLookAndFeel::drawOrnamentSymbol`:** in the new `CustomLookAndFeelOrnaments.cpp`. It fits the unit square into the bounds, and its stroke width is `Theme::ornamentStrokeRatio` of the size. The caller sets the colour.
  - **Symbol toggle:** the Symbol button switches `BuildView { Figure, Symbol }`.
  - **Where symbols show:** the tile and the bottom-bar button draw the symbol. The button's painter falls back to the placeholder glyph when the active ornament has no symbol.
- **Where it differs from the plan:** the button's setup moved into `BottomBar::configureOrnamentButton` (extracted with `refactor.encap`), because the constructor had passed 80 lines.
- **Test:** two strokes survive a save and reload, and their bounds come back as the unit-square strokes.
- **Manual check still owed:** draw a trill sign. The tile and the button should match it and scale cleanly with the window, and undo should remove the last stroke.

### Step 6 — Other instances and library files: done (uncommitted)

- **Built:** `OrnamentLibraries::rescan(juce::UndoManager&)`:
  - it unloads a library whose file is gone;
  - it reloads a file that is newer than the copy last loaded or saved, unless this instance has unsaved edits to that library, in which case its next save wins;
  - it loads new files, skipping a second copy of a library already loaded;
  - when anything changed, it re-checks the active ornament and clears the ornament undo history.
- **Where it differs from the plan:**
  - **Where rescan runs:** the bottom-bar button calls it just before showing the window, rather than the window's constructor and `visibilityChanged`. `PopupWindow` hides the window and never its content, so `visibilityChanged` would not fire when the window reopens.
  - **Undo manager:** `rescan` takes the undo manager as a parameter, as `removeLibrary` does.
  - **Build panel:** it re-resolves the active ornament whenever a library is added or removed, so a reloaded library never leaves it showing the old tree.
- **Test:** a library written by a second instance appears after a rescan; a newer file replaces the loaded copy; a deleted file unloads its library. Making the unload check always pass made this test fail, and restoring it made it pass again.
- **Manual check still owed:** run two instances in Live, add a library in one, and open the window in the other.

### Whole feature

- **Tests:** 104/104. `[ornament]` covers 12 cases and 71 assertions.
- **Gates:**
  - design-rules is clean on every new and changed file.
  - Readability flags only:
    - `OrnamentLibraries.h`, for the blank lines between return-type groups, which is still the owner's choice;
    - `Tests/OrnamentTests.cpp`, a false positive because `refactor.order` reads `TEST_CASE` as a function;
    - `PluginProcessor.h:60`, which was already there.
- **Lines:** +2,754 / −8. That is 2,576 in new files and +178 / −8 in existing ones.

### Revision — the host is a mark: done (uncommitted)

- **Why:** the owner changed how the host is chosen. The user no longer places a host node. They place ornament nodes, then mark one as the host with a Host button of its own.
- **Built:**
  - **Model (`OrnamentLibraries`):**
    - `OrnamentHostData` is gone; every figure node is `OrnamentNoteData`.
    - `OrnamentFigure` carries `OrnamentHostId`, which new ornaments start at `noHostId` (-1).
    - `addFigureNode` lost its `kind` parameter.
    - `toggleHost` marks, moves or clears the host.
    - `removeFigureNode` clears the mark when it removes the host.
    - `figureSteps` and `isPublishable` read the host from `OrnamentHostId`.
  - **`OrnamentFigureController`:**
    - `ClickMode { Build, PickHost }`.
    - `switchClickMode` sets the circle cursor (an ellipse image, as `ValueField`'s brush cursor is) or restores the normal one, and fires `onClickModeChanged`.
    - In `PickHost`, any mouse-down ends the mode. A left-click on a node calls `toggleHost` first, as one transaction.
  - **`OrnamentBuildPanel`:**
    - The Note/Host `kindPane` is replaced by a standalone `hostButton`, drawn with `drawRulesButton` like Publish and Symbol, and selected while the mode is on.
    - Pressing it again cancels the mode, and so does switching the view or the active ornament.
    - `canvas`, `port` and `controller` are public, so the window can reach the controller.
  - **`OrnamentCanvas`:** it sets `hasInnerRim` on the node whose id is `OrnamentHostId` on every rebuild.
  - **`OrnamentWindow`:** Escape cancels the mode.
- **Tests:** 107/107. The existing cases now mark the host with `toggleHost`. A new case covers moving the host, clearing it by marking it again, and the mark coming back on undo after the host is deleted.
- **Not migrated:** a library saved before the revision keeps its `OrnamentHostData` node. It still renders and plays as a node, but has no host until the user marks one.
- **Manual checks owed:**
  - press Host and see the circle cursor; click a node, and it gets the ring and the cursor resets;
  - press Host and click another node; the ring moves;
  - press Host and click the host; the ring goes and Publish greys out;
  - press Host, then click empty canvas or press Escape; nothing changes;
  - delete the host; Publish greys out, and undo brings the ring back.

### Deployment — the audio side: done (uncommitted)

- **Built:**
  - **`RTData.h`:** `OrnamentNodeKind` and `OrnamentStep` moved here from `OrnamentLibraries.h`, so the figure is plain RT data. `RTNode` gains `ornament`, the node's published figure in play order.
  - **`RTGraphBuilder`:**
    - fills `ornament` from `figureSteps` of the `PublishedFigure` of the node's `OrnamentUid`. A draft, a deleted or a missing ornament gives an empty figure, so the node plays plain;
    - rebuilds a node's graph when its `OrnamentUid` changes;
    - listens on `ornamentLibraries.libraries` and rebuilds every graph when an ornament is published (its `PublishedFigure` is replaced), deleted, or a library is added, removed or reloaded. Figure edits that are not published are ignored, and a library event never reaches the main graph's branches.
  - **`NoteScheduler`:** an ornamented note stays one `ActiveNote` that re-voices itself at each step boundary. `OrnamentProgress` holds the step, the voiced base pitch and velocity, and the figure's samples still to come. Each step gets `remaining × weight ÷ remaining weight`, and the last step takes exactly what is left, so the figure always ends when the plain note would have.
  - **`EventManager`:** an expiring note with figure time left calls `continueOrnament` instead of stepping the traversal; `followTempo` scales the figure's remaining time with the note, so a tempo change stretches the figure.
- **What this keeps:** the node lights up for the whole figure and the walk steps exactly when it did plain, since the traversal only sees the note's final expiry. Chord voices are ornamented note by note, because each chord member is scheduled through the same `scheduleNote`. Offsets and velocity percentages apply after transpose, modulator, alternative and script voicing, clamped to MIDI range.
- **Test:** `Tests/GraphTests.cpp`, "an ornamented node plays its figure across its own duration, and the walk keeps its rhythm": "Lower grace" on the middle node of a chain plays the grace a semitone down at 70% velocity, the host a third of the way in, and every other note where it played plain. It failed on the engine before this change (8 notes where 9 were expected). `playChainWithScript`'s play loop became `playProcessor` so both tests share it.
- **Manual checks owed:** republish an ornament while playing and hear the change; change host tempo mid-figure; ornament a chord's second note; relocate the host playhead into the middle of a figure.
