# Audio Concatenation Tool Launch

Coordination document for building a new, independent JUCE plugin: a corpus-based audio concatenator built on FluCoMa, reusing SequenceTree's UI through a shared library. Started 2026-10-12.

## What the tool is

- The user loads a single sample or a folder of samples. This set of samples is the **corpus**.
- The tool cuts the corpus into slices, describes each slice by how it sounds, and lays the slices out on a 2D map, with similar-sounding slices placed near each other.
- The user plays it by **browsing the corpus**: moving through the map plays the slices nearest to the current position (CataRT-style). Target matching / mosaicing is not part of this tool.
- JUCE provides the plugin, sample loading, playback and the whole interface: the loading UI, a panel for the concatenation parameters, and a graph showing the slices.
- FluCoMa provides the analysis: slicing, descriptors, normalisation, nearest-neighbour search and the 2D layout.

## Decisions made

| Decision | Choice |
|---|---|
| Concatenation mode | Corpus browsing |
| Analysis timing | Analysis runs to completion on a background thread before the tool uses it. Nothing partially analysed ever reaches the audio thread. |
| Real-time safety | All FluCoMa work stays off the audio thread. The audio thread only reads a finished, published corpus. |
| Independence | The concatenator is its own project and its own repository. Neither SequenceTree nor the concatenator contains the other. |
| Drawing code | All of `CustomLookAndFeel`'s drawing is kept and shared, including graphs, arrows, nodes, buttons and icons (see *UI library*). |

## FluCoMa facts

- Repository: `flucoma/flucoma-core`, BSD-3-Clause, so it can ship in a closed or commercial plugin.
- Language: C++17. SequenceTree builds with C++20 (`target_compile_features(... cxx_std_20)`).
- Platforms: macOS, Windows (MSVC) and Linux (GCC), Intel and Apple Silicon.
- Dependencies, downloaded by its CMake by default: Eigen 3.4, HISSTools Library, Spectra, foonathan/memory, fmt, nlohmann JSON. `EIGEN_PATH` and `HISS_PATH` point it at local copies instead.
- `-DFLUID_ARCH=-mnative` is for local-only builds. An "illegal instruction" crash means the default vector flag isn't supported by the CPU.
- Use the algorithm layer (`include/flucoma/algorithms/public/*.hpp`) directly, not the client layer the Max, Pd and SuperCollider wrappers use, which is built around their buffer objects.
- `examples/describe.cpp`, `examples/umap.cpp` and `examples/dataset.cpp` are close to this tool's whole offline pipeline and are the starting reference.

Algorithms this tool needs:

| Job | Algorithms |
|---|---|
| Slicing | `OnsetSegmentation`, `NoveltySegmentation`, `TransientSegmentation`, `EnvelopeSegmentation` |
| Descriptors | `MelBands` + `DCT` (MFCC), `SpectralShape`, `YINFFT` (pitch), `Loudness`, `MultiStats` (per-slice statistics) |
| Scaling | `Normalization`, `Standardization`, `RobustScaling` |
| Nearest slice | `KDTree` |
| 2D map | `UMAP`, `PCA`, `MDS`, and `Grid` to snap the map to a grid |

## Real-time model

1. **Loading and analysis (background thread).** Read the files through `juce::AudioFormatManager` and resample to one rate. Then slice → describe → stats → scale → KDTree → UMAP. The result is cached to disk, keyed by the files and the analysis parameters.
2. **Publishing.** When the analysis finishes, the whole corpus (audio, slice boundaries, map positions, lookup structure) is handed to the audio thread as one immutable snapshot, using the same pattern as SequenceTree's `AudioSnapshotPublisher`: a pointer is published, and the outgoing snapshot is freed only once no audio block can still be reading it. The audio thread never allocates or frees.
3. **Playback (audio thread).** Plays slices from the published snapshot, with a fixed voice pool, crossfades and windows.
4. **Display (message thread).** The play position and the active slices go from the audio thread to the UI through lock-free FIFOs, polled once per display frame, the way SequenceTree's `AudioUIBridge` and the editor's `VBlankAttachment` work.

Where the nearest-slice lookup runs depends on what drives browsing:

- **Mouse on the map:** the lookup runs on the message thread, and only the chosen slice ids are sent to the audio thread. This is safe by construction.
- **Automation or MIDI driving the position:** the lookup must answer on the audio thread. Either verify that a `KDTree` query doesn't allocate (the RealtimeSanitizer setup from SequenceTree, `SEQUENCETREE_REALTIME_SANITIZER`, answers this), or have the analysis precompute an N×N table that maps each map cell to its nearest slice. Then the audio thread does a plain array read.

## Components

| Component | Responsibility | Difficulty |
|---|---|---|
| Corpus loader | File or folder → decoded, resampled audio in memory | Easy |
| Analysis pipeline | FluCoMa slicing, descriptors, scaling, KDTree, UMAP; disk cache | Medium, mostly build plumbing |
| Corpus snapshot publisher | Hands the finished analysis to the audio thread | Easy, known pattern |
| Slice player | Voice pool, crossfades and windows, what plays next as the position moves | Medium |
| Parameter panel | Slicing threshold, descriptor choice and weights, grain length, overlap, voice count | Easy with the shared editors |
| Corpus map view | 2D scatter of slices, zoom and pan, browsing position, active-slice highlights | Medium |
| Audio → UI bridge | Play position and active slices to the map | Easy, known pattern |
| State save/restore | Sample paths, parameters, cached analysis reference | Easy |

Estimated size of a usable first version: 4–6k lines. For comparison, SequenceTree is about 30.7k lines, about 15k of it in `Source/UI`.

## UI library

### Three layers, not two

SequenceTree's code splits three ways. The library is only the first:

| Layer | Contents | Where it goes |
|---|---|---|
| **Generic UI** | `Theme`, `CustomLookAndFeel`'s icons, buttons, menus, scrollbars, popups, frosted glass and grid; `IconButton`, `ButtonPane`, `ValueSlider`, `ContextMenu`, `ItemSelector`, `PanelResizer`, `PopupWindow`, `ScaledView`, `LabelPanel`, `FileLabel`, `FilePage`, `Bar`, `DynamicPort`, `ColourSelector`, `SettingsMenu`; `ValueEditor` with `NumberFormat`, `TextFormat`, `DualIntFormat` | The library |
| **SequenceTree's domain** | the document model (`GraphState`, `NodeFactory`, `EncapsulationOps`, `ArrowBindingOps`, `TraversalState`, `OrnamentLibraries`), `Util/` (`NodeInfo`, `ArrowInfo`), the canvas (`NodeCanvas`, `Node`, `Arrow` and their managers), `Source/Input`, the traversal and node menus, `PitchFormat`, `TraversalFlagFormat`, `GreekLetterFormat` | Stays in SequenceTree |
| **SequenceTree's engine** | `Source/Audio`, `Source/Script`, `Source/Plugin`, `RTGraphBuilder`, the compiled-graph types in `RTData.h` | Stays in SequenceTree |

The domain layer is free of engine *code* but not of SequenceTree's *concepts*. It speaks notes, traversals, modulators, count limits and pitch-bound arrows:

- `NodeInfo.h:47` `NodeType { Node, Root, Modulator, TraversalFlag, Encapsulator }`
- `NodeInfo.h:10` `NodeNote { pitch, velocity, duration, midiChannel }`
- `ArrowInfo.h:11-12` `ArrowType { Node, Polyphonic, Traversal, CrossRootTree, StepIntoTree }`, `ArrowBinding { NoBind, PitchBind, DurationBind }`

Moving that layer into a library would not make it generic. It would be SequenceTree's front end in another repository, and the concatenator would carry a MIDI sequencer's model it never uses. So the canvas, `GraphState` and undo stay in SequenceTree. A tool that wants its own node graph builds its model with the library's widgets and drawing.

### What the domain layer takes from `RTData.h`

`RTData.h` holds three kinds of type in one file. The domain layer uses none of the compiled graph:

| Kind | Types | Used outside the engine by |
|---|---|---|
| Engine's compiled graph | `RTNote`, `RTConnection`, `RTNode`, `NodeMap`, `OrnamentStep`, `OrnamentNodeKind` | only `OrnamentLibraries::figureSteps` (`OrnamentLibraries.h:63`), which compiles a figure for `RTGraphBuilder.cpp:519` |
| Domain vocabulary | `TraversalKey` (`maxInstances` 26), `RTtraversal`'s tempo-multiplier limits | `TraversalState.cpp:37-101`, `AllowedTraversalsMenu`, `TraversalFlagFormat`; `TempoDisplay.cpp:9`, `TraversalMenu.cpp:39` |
| UI → engine message | `RTPreviewRequest` | `NodeCanvas.cpp:264`, `NodeController.cpp:68`, `BottomBar.cpp:268` |

`GraphState.h:7` and `NodeCanvas.h:5` include `RTData.h`. A name search finds no `RTData` type used in `GraphState.*`, and `NodeCanvas` uses only `RTPreviewRequest`. Splitting `RTData.h` along these three kinds would let the domain layer include only its vocabulary. This is SequenceTree housekeeping and not needed for the library.

### `CustomLookAndFeel` and `Theme`

All of `CustomLookAndFeel`'s drawing is wanted in more than one project, including nodes and arrows. The functions that take only `Graphics`, bounds and a `ButtonState` (every icon, the buttons, menus, scrollbars and popups) move to the library unchanged. Four take a SequenceTree class and move once they take plain data instead, the way `drawNode` already takes `NodeVisual` (`Node.h:15`):

| Function | Takes today | Reads | Takes in the library |
|---|---|---|---|
| `drawCanvas` (`CustomLookAndFeel.cpp:31`) | `const NodeCanvas&` | `gridSpacing`, `gridVisible`, `gridOrigin`, `modelTransform`, `viewTransform` | a grid struct with those values |
| `drawFrostedGlass` (`CustomLookAndFeel.cpp:77`) | `NodeCanvas&` | its bounds, `paint`, children | any `juce::Component&` |
| `drawArrow` (`CustomLookAndFeelArrows.cpp`) | `const Arrow&` | `getGeometry`, `buildShaftPath`, `animation`, `hovered`, `selected`, `isGhost`, `shaftStyle` | an arrow-visual struct, filled in by SequenceTree's `Arrow` |
| `drawNode`, `drawModulatorNode` | `NodeVisual` | already plain data | unchanged; `NodeVisual` moves with them |
| `drawOrnamentSymbol`, `drawNodeOrnament` | symbol name | `OrnamentLibraries::readSymbol` | the symbol's strokes, looked up by SequenceTree |

`Bar`'s frosted constructor (`Bar.cpp:10`) takes `NodeCanvas&` and becomes `juce::Component&` with `drawFrostedGlass`. `ScaledView.cpp:7` reads `ValueTreeIdentifiers::InterfaceZoom`, and `ColourSelector` and `SettingsMenu` read `ValueTreeIdentifiers` keys; each takes its key as a parameter. `ValueEditor.cpp:3-5` includes `NodeCanvas.h` and `NodeInfo.h`, which appear unused; confirm by building without them.

The constructor (`CustomLookAndFeelMenus.cpp:29-39`) loads the Geist fonts from `BinaryData` and the SVG icons from `IconData` (the `SequenceTreeFonts` and `SequenceTreeIcons` targets in `CMakeLists.txt`). Those files move into the library with their own binary-data target. SequenceTree subclasses the library's `CustomLookAndFeel` for any drawing that stays its own.

### SequenceTree's UI → engine crossings

Not needed for the library, but it is the map of where SequenceTree's own UI touches its engine. These are the only places `Source/UI` and `Source/Input` include anything from `Audio/`, `Plugin/` or `Script/`. Each could become a narrow channel instead:

| Site | Reaches | Purpose | Channel it becomes |
|---|---|---|---|
| `NodeCanvas` ctor (`NodeCanvas.h:31`, `NodeCanvas.cpp:13-22`) | the whole `SequenceTreeAudioProcessor&` | `eventManager.bridge` for the drainer, `ornamentLibraries` | the specific objects, not the processor |
| `NodeCanvas.cpp:264`, `NodeController.cpp:68`, `BottomBar.cpp:267` | `traversalSession.previewRequests` | UI → engine: start or stop a preview | a preview-request queue |
| `NodeCanvas::setProcessorPlayback` (`NodeCanvas.cpp:328-332`) | `processor.isPlaying` | UI → engine: transport | a transport atomic |
| `AudioCommandDrainer.cpp` (13 sites) | `AudioUIBridge` FIFOs | engine → UI: highlights, arrow trails, counts | the receiving end of the bridge; the command structs are the contract |
| `ArrowAnimation.h:4` | `AudioUIBridge.h` | only the `TrailSource` enum (`AudioUIBridge.h:10`) | the enum moves to the library side |
| `Titlebar.cpp` (7 sites) | `wrapperType`, `isPlaying`, `valueTreeState`, `patchFolder`, `resetRequested` | transport, reset, standalone-only controls | tool-specific bar, or the same atomics passed in |
| `PatchControls.cpp` (4 sites) | `get/setStateInformation`, `getParameters` | patch save and load | tool-specific |
| `TraversalRulesWindow.cpp:158,288` | `AudioSnapshotPublisher`, `ScriptCompiler` | the script editor publishing a compiled rule | SequenceTree-only window |
| `RTGraphBuilder` (`RTGraphBuilder.h:32`) | `processor.snapshots`, `processor.ornamentLibraries` | document → engine | engine side, as above |
| `TraversalRuleState.cpp:3` | `ScriptCompiler.h` | compiles rule source | engine side with the script language |

`createEditor` (`PluginProcessor.cpp:256`) constructing the editor is JUCE's standard hook, and each tool keeps its own. It is where a tool wires its UI to its own engine.

### How each tool gets the library

The library is a third repository. Each tool depends on it; neither tool depends on the other. Options:

| Option | How it works | Good for | Cost |
|---|---|---|---|
| Local path | Each tool's CMake points at one checkout on disk, e.g. `~/Documents/GitHub/<Library>` | Rapid iteration: an edit shows up in both tools on the next build | A change made for one tool can break the other; another machine needs the same path |
| Submodule | Each tool's repo records a pinned commit of the library, the way SequenceTree pins `JUCE` and `Catch2` in `.gitmodules` | Each tool builds the exact library version it was tested with; a fresh clone works anywhere | Each library change means a commit there and a pointer update in each tool |
| Copy | The files are copied into the new tool | Nothing to set up | The two copies drift apart |

Plan: a local path while the library is still changing, then a submodule once it settles.

The library is `ArsNovaUI`, a plain CMake `INTERFACE` target with a gallery app. Its creation is planned step by step in `.claude/context/Proposals/Unimplemented/GenericUILibrary.md`.

## Patterns to copy, not share

These are tied to each tool's own data types, so the new tool gets its own version:

- `AudioSnapshotPublisher`: epoch-based publication and deferred freeing
- `AudioUIBridge` with editor-side `VBlankAttachment` polling
- Catch2 setup and the RealtimeSanitizer test target

## Build order

1. **Prove the build.** An empty JUCE plugin that links flucoma-core and runs `OnsetSegmentation` + MFCC on one file. If this fails, the plan changes.
2. **Offline pipeline.** Load a folder, slice, describe, run UMAP, draw a rough scatter plot. No library yet.
3. **Playback.** Click a point to play its slice. Then add the corpus snapshot publisher and the voice pool.
4. **Extract the UI library** from SequenceTree as described above, and move the new tool's panels onto it.
5. **Browsing and parameters.** Movement-driven playback, the parameter panel, save/restore.

The library is extracted after step 3 so it is shaped by a real second user rather than guesses.

## Open questions

- **What the graph shows.** The corpus map (slices by sound, from UMAP), a waveform or timeline of what was played, or both. Corpus browsing makes the map the main view.
- **What drives browsing.** Mouse only, or also automation and MIDI. This decides where the nearest-slice lookup runs (see *Real-time model*).
- **During re-analysis.** Whether the previous corpus keeps playing while a new one is analysed, or playback stops until it finishes.
- **Canvas mechanics in the library.** Zoom and pan (`DynamicPort`) and grid drawing are generic. Hit testing, selection and dragging are written against SequenceTree's `Node` and `Arrow` today (`CanvasHitTester`, `NodeController`). Decide whether a generic version of those belongs in the library once the corpus map shows what it needs.
- **Name.** The concatenator's name. The library is `ArsNovaUI`.

## Sources

- https://github.com/flucoma/flucoma-core
- https://github.com/flucoma/flucoma-core/tree/main/include/flucoma/algorithms/public
