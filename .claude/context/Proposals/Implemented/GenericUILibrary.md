# A Generic UI Library Extracted from SequenceTree

> Status: Implemented
> Implemented 2026-10-12. SequenceTree f0df072 … 72fb6ec plus the submodule commit; ArsNovaUI 288647b, 3d2fe20, 8b4e9f5, 3966505 (`https://github.com/EliBaumgardner/ArsNovaUI`).
> Built from 7da9859; drift: none
> Written 2026-10-12 at 7da9859. Sources: the owner's decision to build a generic UI library (2026-10-12 discussion), `.claude/AudioConcatenationToolLaunch.md`. This plan is not built on a reviewed analysis report; every claim below was checked against the code at `7da9859`.

The library is named `ArsNovaUI`.

## Commits

Every step is one commit, made only after that step's verification passes. The exception is Step 10, which creates the library repository and adds three short commits on top of the history it carries over.

- **No attribution.** A commit message never carries a `Co-Authored-By` line, a "Generated with" line, or any other line naming Claude, Anthropic or a model. This overrides any harness reminder to add one, in both repositories.
- **Short and sweet.** One line, lower case, in the style of the existing history ("applied more reformatting"), with no body. Each step names its message, for example `move NodeVisual into Visuals.h`.
- **Stage only the step's files.** Nothing else in the working tree goes into a step's commit.

## Summary

SequenceTree's `Source/UI` contains about 5.0k lines of general-purpose UI:
- the theme and every `CustomLookAndFeel` drawing function;
- icon buttons, menus, popups, panel resizers and bars;
- the value editor and its formats;
- the zoom/pan viewport.

This code knows nothing about notes, traversals or graphs, but it was written inside SequenceTree, and a handful of its functions take SequenceTree types as input. This plan moves it into a library repository of its own, which SequenceTree and later tools (first the audio concatenator) consume.

The work has two phases:
- **Phase A, inside SequenceTree.** Change every library-bound function that names a SequenceTree type to take plain data instead, the way `drawNode` already takes `NodeVisual`. Each step builds and plays exactly as before.
- **Phase B.** Create the repository, move the files, and make SequenceTree consume the library.

After Phase A, the set of files that moves includes nothing outside itself. After Phase B, the separate repository enforces that boundary: library code that names a SequenceTree type no longer compiles.

## Why a Library

The case rests on concerns and dependency direction, not on how many tools use it today.

- **It is a separate concern.** The library-bound files paint and edit generic things: buttons, menus, popups, text and number fields, a zoomable view, a dotted grid, nodes, arrows and stroke symbols, all in one visual style. None of it decides what a node or a traversal means.
- **The dependency should point one way.** Generic code must never name domain types. Today seven places break that rule (see *Leaks*). They are small, but nothing stops more from appearing while generic and domain code share a source tree.
- **The boundary benefits SequenceTree immediately.** Once the code is in its own repository, a generic component that reaches for `NodeCanvas` or `RTData` fails to compile. That is what keeps it reusable, and future tools are the second benefit.

## Boundaries

### What the library is

The theme and the drawing, plus every component whose job is generic UI. It depends only on JUCE (`juce_gui_basics`, `juce_gui_extra`) and on itself.

| Area | Files (today's paths under `Source/UI/`) |
|---|---|
| Theme and drawing | `Theme/Theme.h`, `Theme/CustomLookAndFeel.h`, `Theme/CustomLookAndFeel.cpp`, `CustomLookAndFeelButtons.cpp`, `CustomLookAndFeelMenus.cpp`, `CustomLookAndFeelNodes.cpp`, `CustomLookAndFeelArrows.cpp`, `CustomLookAndFeelOrnaments.cpp`, `Theme/CustomTextCaret.{h,cpp}` |
| Buttons | `Buttons/IconButton`, `Buttons/ButtonPane`, `Buttons/ValueSlider` |
| Menus and windows | `Menus/ContextMenu`, `Menus/ItemSelector`, `Menus/ColourSelector`, `Menus/SettingsMenu`, `PopupWindow`, `ScaledView`, `PanelResizer`, `LabelPanel` |
| Editors | `Editors/ValueEditor`, `Editors/LabeledEditor.h`, `Editors/FileLabel`, `Editors/FilePage`, `Editors/Formats/ValueFormat.h`, `NumberFormat.cpp`, `TextFormat.cpp`, `DualIntFormat.cpp`, `PitchFormat.cpp`, `GreekLetterFormat.cpp` |
| Layout | `Bars/Bar`, `Canvas/DynamicPort` |
| New, from Phase A | `Theme/Visuals.h` (plain drawing data), `Theme/InterfaceIdentifiers.{h,cpp}` (the keys the settings components save under) |
| Assets | `Resources/Fonts/*.ttf` (3 Geist fonts), `Resources/Icons/*.svg` (37 icons) |

### What the library is not

Everything that knows SequenceTree's model stays in SequenceTree:
- the document model (`Source/Graph`) and `Source/Util`;
- the canvas and its parts (`NodeCanvas`, `NodeManager`, `ArrowManager`, `CanvasHitTester`, `CanvasFrame`, `ValueField`, `OrnamentMarks`, `EncapsulationView`, `AudioCommandDrainer`, `NodeCanvasTreeListener`);
- `Node` and its subclasses, `Arrow`, `ArrowAnimation`;
- `Source/Input`;
- every bar and menu that lays out SequenceTree's own controls (`Titlebar`, `BottomBar`, `MenuBar`, `MenuArea`, `NodeMenu`, `TraversalMenu`, `TraversalRulesWindow`, `AllowedTraversalsMenu`, `PatchControls`);
- `RootRectangle`, `TempoDisplay`, `PaintToolSettings`, the ornament windows and `TraversalFlagFormat`;
- the whole engine (`Source/Audio`, `Source/Script`, `Source/Plugin`).

`ArrowAnimation` stays because its behaviour is SequenceTree's: live trails pause with the transport and preview trails don't (`ArrowAnimation.cpp:10`, `:32`, `:117`, keyed on `TrailSource` from `AudioUIBridge.h:10`).

### Rules the library keeps

- It includes nothing outside itself and JUCE.
- Its drawing functions take JUCE types or the plain structs in `Visuals.h`, never a component from a consumer.
- Saved-state keys it owns live in `InterfaceIdentifiers`, with the same strings SequenceTree saves today.
- The Key Design Rules and the Project Design Rules that concern UI code (header order, grouping, initialiser lists, no structs in `.cpp`) apply unchanged.

## Current Design

### Leaks: library-bound code naming SequenceTree types

| Site | Names | What it uses |
|---|---|---|
| `CustomLookAndFeel::drawCanvas` (`CustomLookAndFeel.cpp:31-64`), caller `NodeCanvas.cpp:35` | `const NodeCanvas&` | `gridSpacing`, `gridVisible`, `gridOrigin`, `gridOriginSet`, `modelTransform`, `viewTransform`, `getLocalBounds` |
| `CustomLookAndFeel::drawFrostedGlass` (`CustomLookAndFeel.cpp:77`), callers `Bar.cpp:20`, `CanvasFrame.cpp:29`, `MenuArea.cpp:45-47` | `NodeCanvas&` | only `getLocalBounds`, `paint`, `getChildren`, all on `juce::Component` |
| `Bar`'s frosted constructor (`Bar.h:15`, `Bar.cpp:10`) | `NodeCanvas&`, member `NodeCanvas* nodeCanvas` | passes it to `drawFrostedGlass` |
| `CustomLookAndFeel::drawArrow` (`CustomLookAndFeelArrows.cpp:170-257`), caller `Arrow.cpp:54` | `const Arrow&` | `getGeometry`, `buildShaftPath`, `getLabel`, `getDurationLabel`, `shaftStyle`, `isTraversalArrow`, `editingDuration`, `hovered`, `selected`, `isGhost`, `getX/getY`, `animation.{snapProgress, rulerFade, trails}`, `Arrow::arrowHeadLength(Hover)`, `Arrow::labelVisibleThreshold`, `ArrowInfo::pixelsPerGridSpace` |
| `drawOrnamentSymbol`, `drawNodeOrnament` (`CustomLookAndFeelOrnaments.cpp`), callers `BottomBar.cpp:179`, `OrnamentTile.cpp:59`, `OrnamentSymbolEditor.cpp:69`, `OrnamentMarks.cpp:10` | `OrnamentLibraries::readSymbol`, `OrnamentMark` (`NodeInfo.h:49`) | parses the stored symbol into `SymbolStroke`s |
| `ValueFormat.h:7` | `#include "RTData.h"` | only for `TraversalFlagFormat` (`ValueFormat.h:97-111`) |
| `PitchFormat.cpp:2` | `NodeInfo.h` | `minimumMidiPitch` (0), `maximumMidiPitch` (127) |
| `ScaledView.cpp:7,48`, `ColourSelector.cpp:54-177`, `SettingsMenu.cpp:42,50` | `ValueTreeIdentifiers` | `InterfaceZoom`, `PresetColour`, `ColourPreset`, `ThemeColour` |
| `ValueEditor.cpp:3-5` | `NodeCanvas.h`, `ValueTreeIdentifiers.h`, `NodeInfo.h` | nothing found by name search; to be confirmed by building without them |

Types the drawing borrows from domain headers, which move to `Visuals.h`:
- `NodeVisual` (`Node.h:15`)
- `ArrowGeometry` and `ArrowLabel` (`Arrow.h:10`, `:26`)
- `Arrow::ShaftStyle` (`Arrow.h:82`)
- `SymbolStroke` (`OrnamentLibraries.h:18`)
- `OrnamentMark` (`NodeInfo.h:49`)

### Structure

`CustomLookAndFeel` is one class (`LookAndFeel_V4` + `Theme`) defined across six `.cpp` files, one per kind of drawing.
- The editor owns the instance: `PluginEditor.h:56`.
- Components reach it through `CustomLookAndFeel::get`, a `static_cast` of `getLookAndFeel()`.
- `IconButton::Icon` (`IconButton.h`) is a pointer to a `CustomLookAndFeel` member taking `(Graphics&, Rectangle<float>, const ButtonState&)`. `IconButton::painter` is the `std::function` alternative.
- The constructor (`CustomLookAndFeelMenus.cpp:29-39`) loads the fonts from `BinaryData` and the icons from `IconData`. These are the `SequenceTreeFonts` (default `BinaryData` namespace) and `SequenceTreeIcons` (`HEADER_NAME IconData.h NAMESPACE IconData`) targets in `CMakeLists.txt`. Nothing else in `Source/` reads either.
- Frosted glass caches one blurred backdrop on the look-and-feel (`frostedBackdrop`, `frostedBackdropStale`, `unfrostedArea`). The editor marks it stale (`PluginEditor.cpp:74`) and sets the unfrosted area (`PluginEditor.cpp:133`).

### Data flow

Everything here runs on the message thread inside `paint`, `resized` and user handlers. The audio thread never reaches any library-bound code. `TrailSource` is the only shared type, and it stays in `AudioUIBridge.h`.

The settings components write the interface settings file and the colour-presets tree:
- `ScaledView` and `SettingsMenu` write `InterfaceZoom` and `ThemeColour` to the `PropertiesFile`.
- `ColourSelector` writes `ColourPreset` children with a `PresetColour` property into `processor.colourPresets` (`PluginProcessor.h:110`), which the processor saves and restores (`PluginProcessor.cpp:402`).

## Caveats

1. **The library must compile inside each consumer, never as a prebuilt static library.**
   - JUCE modules are CMake `INTERFACE` targets whose sources compile into whatever links them. SequenceTree sets JUCE configuration macros on its own target (`JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0`, the plugin defines).
   - A static `ArsNovaUI` would compile JUCE a second time under different settings, which risks duplicate symbols and one-definition-rule violations.
   - Both packaging options below compile the library's sources inside the consumer.
2. **The JUCE module format is a unity build.**
   - `juce_add_module` compiles only files at the module root whose names begin with the module name (`JUCEModuleSupport.cmake:216-228`). Every other `.cpp` has to be `#include`d by one of those.
   - The compile database then lists only the wrapper files, which is what clangd and every `refactor.*` command read.
   - Unity builds also make one file's `static` helpers visible to the files after it. Today's file-scope helpers have distinct names (`paintNodeShadow`, `strokeArrowShaft`, `trimPathToFraction`, `drawArrowProgress`, `buildArrowHeadPath`, `drawArrowHead`, `drawArrowLabel`, `loadGlyph`, `drawGlyph`, `drawTitledButton`, and the `PitchFormat` / `GreekLetterFormat` tables), so nothing collides today, but every new one would need a unique name.
   - This is why the library is a CMake `INTERFACE` target.
3. **Saved state depends on identifier strings.**
   - `"ColourPresets"`, `"ColourPreset"`, `"PresetColour"`, `"ThemeColour"` and `"InterfaceZoom"` (`ValueTreeIdentifiers.cpp:84-86`, `:109-110`) are in every saved plugin state and in the standalone's interface settings file.
   - Moving them into `InterfaceIdentifiers` must keep these strings exactly, or saved colour presets, theme colour and zoom stop loading.
4. **Binary data needs its own namespace.** The library's font target must not use JUCE's default `BinaryData` namespace and header, or it collides with a consumer's own `BinaryData`. It gets `HEADER_NAME ArsNovaUIFonts.h NAMESPACE ArsNovaUIFonts`, and likewise `ArsNovaUIIcons`.
5. **Icons registered by member pointer are fixed to the library class.**
   - `IconButton::Icon` is `void (CustomLookAndFeel::*)(...)`, and a pointer to a member of a subclass doesn't convert to it.
   - A tool's own icons go through `IconButton::painter`, or into the library itself.
   - Every icon SequenceTree has today moves into the library, so nothing changes for SequenceTree.
6. **`CustomLookAndFeel::get` is a `static_cast`.** Every consumer's editor must set the library's `CustomLookAndFeel`, or a subclass of it, as its look-and-feel. The rule in *How components get what they need* (read the theme only once parented, or after setting the look-and-feel explicitly) carries over unchanged.
7. **Frosted glass has one cache per look-and-feel.** `frostedBackdrop` belongs to the `CustomLookAndFeel` instance. A tool that frosts over two different backdrops at once would share one cache and redraw it on every switch. SequenceTree frosts over one canvas, so nothing changes; a tool that needs two would need the cache moved onto `Bar`.
8. **SequenceTree's names would leak into the library's vocabulary.** Icons, node shapes, theme constants and glyph files named after SequenceTree concepts (`drawTraversalIcon`, `drawModulatorNode`, `isEncapsulationRinged`, `scriptErrorColour`, `Tree.svg`, …) would carry SequenceTree's model into a generic library. Step 9 renames each one to what it draws. SequenceTree's own components keep their names: `Node::isEncapsulationRinged` stays, and only the `NodeVisual` field it fills is renamed.
9. **The two repositories have to be committed in step.** A library change that SequenceTree depends on is two commits in two repositories. With the local-path setup both build from the working tree, so a change made for one tool can break the other until that tool is rebuilt.
10. **Tooling and gates (logistics, not design).**
    - SequenceTree's refactor gates, design-rule checks and TreeJev flow cover `Source/` and `Tests/` only (`.claude/refactor.toml`: `sources`, `edit_roots`).
    - The library repository needs its own `.claude` setup (research-suite enabled, `refactor.toml` from `refactor.init`). `refactor.init` reads the build tree of the library's gallery app (Step 10).
    - The two `core_purpose_api` entries for moved classes (`Source/UI/Menus/ItemSelector.h:addItem`, `Source/UI/Menus/ContextMenu.cpp:addItem`) move to the library's config.

## Approaches Considered

| Approach | Verdict |
|---|---|
| **A. Move the generic layer as it is, keeping the leaks** | Rejected. The library would need SequenceTree's headers to compile, so it would not be a library. |
| **B. Move the canvas, `GraphState`, `Node`, `Arrow` and input too** | Rejected. They speak SequenceTree's model (`NodeType`, `NodeNote`, `ArrowType`, `ArrowBinding`, traversals), so the library would be SequenceTree's front end in another repository. |
| **C. Decouple in place (Phase A), then move the self-contained set (Phase B)** | Chosen. Every Phase A step is behaviour-neutral, verifiable in the existing build and revertible on its own. Phase B is then a mechanical move. |

How SequenceTree consumes the library:

| Option | How | Verdict |
|---|---|---|
| JUCE module | `juce_add_module(ArsNovaUI)`, a module header with a `BEGIN_JUCE_MODULE_DECLARATION` block, root `ArsNovaUI*.cpp` files `#include`ing the sources | Rejected because of the unity-build caveat (Caveat 2) |
| CMake `INTERFACE` target | `add_library(ArsNovaUI INTERFACE)`, every `.cpp` listed in `target_sources(ArsNovaUI INTERFACE ...)`, include root and JUCE modules as `INTERFACE` usage requirements | Chosen. Compiles in the consumer like a JUCE module, keeps one translation unit and one compile command per file, and lists sources explicitly the way `CMakeLists.txt` already does |
| Static library | `add_library(ArsNovaUI STATIC ...)` | Rejected (Caveat 1) |

## Plan

Phase A steps 1–9 are each one commit in SequenceTree and leave it building, passing its tests and playing as before. Phase B steps 10–13 create the repository and switch SequenceTree over. Commits follow *Commits* above.

Every step is verified the same way. The step-specific checks are listed per step.

- **Build:** `cmake --build cmake-build-debug --target SequenceTree_Standalone`
- **Tests:** `cmake --build cmake-build-debug --target SequenceTree_Tests SequenceTree_GraphTests && (cd cmake-build-debug && ctest --output-on-failure)`
- **Manual:** the standalone, comparing against the previous build.

No step touches the audio thread, `RTData`, the snapshot publisher or the bridge, so none of the real-time invariants is in play. All the changed code runs on the message thread.

### Phase A: decouple inside SequenceTree

### Step 1: Drop dead includes from `ValueEditor.cpp`

- **Goal:** `ValueEditor` includes only its own header, `Theme` and `CustomLookAndFeel`.
- **Changes:** remove `ValueEditor.cpp:3-5` (`NodeCanvas.h`, `ValueTreeIdentifiers.h`, `NodeInfo.h`). If the build shows one of them is used, keep it and add it to this step's findings: it becomes a leak to fix before Step 10.
- **Mechanical edits:** `refactor.edit` with one `@@@ Source/UI/Editors/ValueEditor.cpp` block.
- **Behaviour delta:** none.
- **Verification:** build, tests.
- **Commit:** `drop dead ValueEditor includes`
- **Rollback:** revert the commit.

### Step 2: Take `TraversalFlagFormat` out of `ValueFormat.h`; give `PitchFormat` its own MIDI range

- **Goal:** `ValueFormat.h` no longer includes `RTData.h`, and `PitchFormat.cpp` no longer includes `NodeInfo.h`.
- **Changes:**
  - New `Source/UI/Editors/Formats/TraversalFlagFormat.h` declaring `TraversalFlagFormat` exactly as `ValueFormat.h:97-111` does, including `ValueFormat.h` and `RTData.h`. `TraversalFlagFormat.cpp` includes the new header.
  - Every file that uses `TraversalFlagFormat` includes the new header: `AllowedTraversalsMenu.cpp`, `RootNode.cpp`, `RootRectangle.cpp` and `TraversalFlagNode.cpp`.
  - `PitchFormat` gains `static constexpr int lowestPitch = 0;` and `highestPitch = 127;`, used where `minimumMidiPitch` / `maximumMidiPitch` are used today (`PitchFormat.cpp:4,39,56,84`).
  - Register the new header nowhere: headers are not listed in `CMakeLists.txt`.
- **Mechanical edits:** `refactor.edit` batch for the declaration move and the include additions; `refactor.edit` `<<< all` for the four constant uses in `PitchFormat.cpp`.
- **Behaviour delta:** none. The constants have the same values.
- **Verification:** build, tests. Manually, type a pitch name (`c4`, `f#3`) into a node and check it parses and displays as before.
- **Commit:** `split TraversalFlagFormat out of ValueFormat`
- **Rollback:** revert the commit.

### Step 3: `InterfaceIdentifiers` for the settings keys

- **Goal:** the settings components name only identifiers that move with them.
- **Changes:**
  - New `Source/UI/Theme/InterfaceIdentifiers.{h,cpp}`: a struct of `static const juce::Identifier`s, `ColourPresets`, `ColourPreset`, `PresetColour`, `ThemeColour`, `InterfaceZoom`, with the strings currently at `ValueTreeIdentifiers.cpp:84-86, 109-110`. Register the `.cpp` in `CMakeLists.txt`.
  - Switch every user: `ScaledView.cpp:7,48`, `ColourSelector.cpp:54,61,133,169,170,177`, `SettingsMenu.cpp:42,50`, `PluginProcessor.h:110`, `PluginProcessor.cpp:402`, `PluginEditor.cpp:24-25`.
  - Remove the five from `ValueTreeIdentifiers.{h,cpp}`.
- **Mechanical edits:** `refactor.edit` `<<< regex` replacing `ValueTreeIdentifiers::(ColourPresets|ColourPreset|PresetColour|ThemeColour|InterfaceZoom)\b` with `InterfaceIdentifiers::$1` across `Source`, plus include additions.
- **Behaviour delta:** none, because the strings are identical (Caveat 3).
- **Verification:** build, tests. Manually:
  1. Using a build from before this step, set a theme colour, a zoom level and two colour presets, then quit.
  2. Launch the new build and check all three came back.
  3. Load a saved patch and check its colour presets.
- **Commit:** `add InterfaceIdentifiers`
- **Rollback:** revert the commit.

### Step 4: Frosted glass over any component

- **Goal:** `drawFrostedGlass` and `Bar` take a `juce::Component&` backdrop.
- **Changes:**
  - `CustomLookAndFeel::drawFrostedGlass(juce::Graphics&, juce::Component& surface, juce::Component& backdrop, juce::Rectangle<int> area)`, with its body's `nodeCanvas` renamed `backdrop` (`CustomLookAndFeel.cpp:77-130`).
  - `Bar(juce::Component& backdrop, Orientation, float)`, with member `juce::Component* backdrop` replacing `NodeCanvas* nodeCanvas` (`Bar.h:15,35`, `Bar.cpp:10-20`).
  - `Bar.cpp` and `CustomLookAndFeel.cpp` drop `NodeCanvas.h`, and `Bar.h` drops its `class NodeCanvas;`.
  - Call sites `CanvasFrame.cpp:29` and `MenuArea.cpp:45-47`, and every `Bar` subclass passing a `NodeCanvas&`, compile unchanged, since `NodeCanvas` is a `juce::Component`.
- **Mechanical edits:** `refactor.replace` for the parameter and member renames; `refactor.edit` for the signature and include changes.
- **Behaviour delta:** none.
- **Verification:** build, tests. Manually, check the title bar, bottom bar, menu area and canvas frame stay frosted over nodes and arrows while panning, zooming and dragging a node under a bar.
- **Commit:** `frost over any component`
- **Rollback:** revert the commit.

### Step 5: `CanvasGrid` for `drawCanvas`

- **Goal:** `drawCanvas` draws from plain data.
- **Changes:**
  - New `Source/UI/Theme/Visuals.h` with a sketch like the one below. `drawCanvas(juce::Graphics&, const CanvasGrid&)` reads `grid.*` where it read `nodeCanvas.*` (`CustomLookAndFeel.cpp:33-52`).
  - `NodeCanvas::paint` (`NodeCanvas.cpp:35`) fills a `CanvasGrid` from `getLocalBounds().toFloat()`, `modelTransform`, `viewTransform`, `gridOrigin`, `gridSpacing`, `gridVisible` and `gridOriginSet`, and passes it.
  - `CustomLookAndFeel.h` drops `class NodeCanvas;`.

  ```cpp
  struct CanvasGrid
  {
      juce::Rectangle<float> bounds;
      juce::AffineTransform  modelTransform;
      juce::AffineTransform  viewTransform;
      juce::Point<float>     origin;
      float                  spacing   = 0.0f;
      bool                   visible   = false;
      bool                   originSet = false;
  };
  ```
- **Behaviour delta:** none.
- **Verification:** build, tests. Manually, check the dot grid at several zoom levels, with the snap origin set (drag a node) and unset, and hidden below 15 px spacing.
- **Commit:** `draw canvas grid from CanvasGrid`
- **Rollback:** revert the commit.

### Step 6: `NodeVisual` into `Visuals.h`

- **Goal:** node drawing names no SequenceTree header.
- **Changes:**
  - Move `struct NodeVisual` from `Node.h:15-27` into `Visuals.h` unchanged. `Node.h` includes `Visuals.h`.
  - `CustomLookAndFeel.h` replaces `struct NodeVisual;` with an include of `Visuals.h`.
  - `CustomLookAndFeelNodes.cpp` drops `Node.h`.
- **Mechanical edits:** `refactor.edit` batch (cut from `Node.h`, insert into `Visuals.h`, include changes).
- **Behaviour delta:** none.
- **Verification:** build, tests. Manually, check nodes, root nodes and modulators render with hover, selection, highlights and encapsulation rings.
- **Commit:** `move NodeVisual into Visuals.h`
- **Rollback:** revert the commit.

### Step 7: `ArrowVisual` for `drawArrow`

- **Goal:** arrow drawing takes plain data. `Arrow` keeps every decision about what an arrow is, and the library keeps every decision about how it looks.
- **Changes:**
  - Move `ArrowGeometry` and `ArrowLabel` (`Arrow.h:10-30`) into `Visuals.h` unchanged.
  - Add `ArrowVisual` and `ArrowTrail` to `Visuals.h` (sketch below). `ShaftStyle` moves from `Arrow.h:82` into `ArrowVisual`, and `Arrow::shaftStyle()` (`Arrow.cpp:510-538`) returns `ArrowVisual::ShaftStyle`.
  - Add `ArrowVisual Arrow::getArrowVisual() const`, the counterpart of `Node::getNodeVisual` (`Node.cpp:369`). It does what `drawArrow` does today with `Arrow`'s own members:
    - geometry from `getGeometry(animation.snapProgress)`;
    - `emphasised` from `hovered || selected`;
    - `headLength` from `arrowHeadLength` / `arrowHeadLengthHover`;
    - `shaft` from `buildShaftPath(geometry, headLength, origin)`;
    - `rulerOpacity` from `animation.rulerFade`, or 1 for a ruler;
    - `rulerSpacing` from `ArrowInfo::pixelsPerGridSpace`;
    - `head` set to `Outlined` when `isTraversalArrow()`;
    - `showsLabel` and `label` from `getDurationLabel()`, `editingDuration`, `labelVisibleThreshold` and `getLabel(geometry, headLength)`;
    - one `ArrowTrail` per entry of `animation.trails`, in map order, `Dashed` when `source == TrailSource::Preview`.
  - `Arrow.cpp:54` becomes `drawArrow(graphics, getArrowVisual())`.
  - `drawArrow`, `drawArrowProgress` and `drawArrowLabel` read the visual. Head width (4.25 / 5.25), ghost alpha (0.5), tick drawing, dash lengths and colours stay in the library.
  - `CustomLookAndFeelArrows.cpp` drops `Arrow.h`, and `CustomLookAndFeel.h` drops `class Arrow;`.
  - `CanvasHitTester.cpp:74` keeps calling `Arrow::getLabel` as today.

  ```cpp
  struct ArrowTrail
  {
      enum class Stroke { Solid, Dashed };

      juce::Colour colour;
      float        progress = 0.0f;
      Stroke       stroke   = Stroke::Solid;
  };

  struct ArrowVisual
  {
      enum class ShaftStyle { Solid, Dashed, Ruler };
      enum class HeadStyle  { Filled, Outlined };

      ArrowGeometry           geometry;
      ArrowLabel              label;
      juce::Path              shaft;
      juce::Point<float>      origin;
      juce::String            labelText;
      std::vector<ArrowTrail> trails;
      ShaftStyle              shaftStyle   = ShaftStyle::Solid;
      HeadStyle               head         = HeadStyle::Filled;
      float                   headLength   = 0.0f;
      float                   rulerOpacity = 0.0f;
      float                   rulerSpacing = 0.0f;
      bool                    emphasised   = false;
      bool                    ghost        = false;
      bool                    showsLabel   = false;
  };
  ```
- **Mechanical edits:** `refactor.edit` batch for the struct moves and the `Arrow::ShaftStyle` → `ArrowVisual::ShaftStyle` renames (`Arrow.h:36,80,82`, `Arrow.cpp:510-538`); hand-written `getArrowVisual` and the `drawArrow` body changes go through `refactor.edit` as well.
- **Behaviour delta:** none intended. `trails` is a vector filled once per arrow paint on the message thread, where today the map is read in place. That's a message-thread allocation per painted arrow with active trails.
- **Verification:** build, tests. Manually, compare old and new builds on one graph:
  - a solid arrow, a dashed one and a ruler (binding);
  - hover thickening and the larger head;
  - a ghost arrow while dragging;
  - traversal-arrow outlined heads;
  - a duration label appearing after the snap settles and hidden while editing;
  - live trails during playback and dashed preview trails, with several trails offset side by side.
- **Commit:** `draw arrows from ArrowVisual`
- **Rollback:** revert the commit.

### Step 8: Ornament symbols from strokes

- **Goal:** symbol drawing takes strokes, not a stored symbol to parse.
- **Changes:**
  - Move `SymbolStroke` (`OrnamentLibraries.h:18-22`) and `OrnamentMark` (`NodeInfo.h:49`) into `Visuals.h`. `OrnamentLibraries.h` and `NodeInfo.h` include it.
  - `drawOrnamentSymbol(juce::Graphics&, juce::Rectangle<float>, const std::vector<SymbolStroke>&)` and `drawNodeOrnament(juce::Graphics&, juce::Rectangle<float>, OrnamentMark, const std::vector<SymbolStroke>&)`.
  - Each caller passes `OrnamentLibraries::readSymbol(symbol)`: `BottomBar.cpp:179`, `OrnamentTile.cpp:59`, `OrnamentSymbolEditor.cpp:69`, `OrnamentMarks.cpp:10`.
  - `CustomLookAndFeelOrnaments.cpp` drops `OrnamentLibraries.h`, and `CustomLookAndFeel.h` drops `NodeInfo.h`.
- **Mechanical edits:** `refactor.edit` batch.
- **Behaviour delta:** none. `readSymbol` runs once per paint, as it does today, only in the caller.
- **Verification:** build, `SequenceTree_GraphTests` (`OrnamentTests.cpp`), tests. Manually, check symbols in the bottom bar's ornament button, library tiles, the symbol editor while drawing, and node ornament marks, including a missing ornament's `?`.
- **Commit:** `draw symbols from strokes`
- **Rollback:** revert the commit.

### Step 9: Rename SequenceTree vocabulary to what it draws

- **Goal:** no name in the library-bound files refers to a SequenceTree concept. Each name says what is drawn.
- **Rule for what is renamed:** a name is renamed when it names a SequenceTree concept or what the icon is for in SequenceTree. A name is kept when it already names the picture (`Pencil`, `Eraser`, `Eye`, `Quaver`, `Chevron`) or a universal UI action (`Add`, `Remove`, `Play`, `Undo`, `Redo`, `Settings`, `Import`). Names were chosen from renders of each SVG.
- **Changes, icon functions** (all in `CustomLookAndFeel.h` / `CustomLookAndFeelButtons.cpp`, plus every `&CustomLookAndFeel::draw…` assignment and direct call):

  | Today | Draws | Renamed to |
  |---|---|---|
  | `drawTreeIcon` | one dot forking into two | `drawForkIcon` |
  | `drawNodeIcon` | a ring with a dot inside | `drawRingDotIcon` |
  | `drawTraversalIcon` | an S-shaped path between two dots | `drawRouteIcon` |
  | `drawSyncIcon` | a chain link | `drawLinkIcon` |
  | `drawTempoIcon` | a metronome | `drawMetronomeIcon` |
  | `drawResetIcon` | a bar and a left-pointing triangle | `drawSkipBackIcon` |
  | `drawResetPatchIcon` | a circular arrow | `drawReloadIcon` |
  | `drawNodeModeIcon` | a circle outline | `drawCircleIcon` |
  | `drawModulatorIcon` | a rounded square | `drawRoundedSquareIcon` |
  | `drawTraversalFlagIcon` | a triangle | `drawTriangleIcon` |
  | `drawDisplayArrowIcon` | a down chevron | `drawChevronDownIcon` |
  | `drawCollapseIcon` | an up chevron | `drawChevronUpIcon` |
  | `drawPaintToolIcon` | a brush | `drawBrushIcon` |
  | `drawSpanToolIcon` | a dashed rounded box with two dots and a dash | `drawDashedBoxIcon` |
  | `drawQuaverToolIcon` | an eighth note | `drawQuaverIcon` |
  | `drawHostButton` | a titled button with a circled H | `drawCircledHButton` |
  | `drawDanglingArrowButton` | a titled button with a right arrow | `drawArrowButton` |
  | `drawPublishButton` | a titled button with an upload arrow | `drawUploadButton` |
  | `drawOrnamentButton` | a wavy line | `drawWaveButton` |
  | `drawRulesButton` | a titled button with `< >` brackets | `drawCodeButton` |
  | `drawAxisButton` | a tile with centred text | `drawTextButton` |

- **Changes, glyphs and SVG files.** `GlyphPaths` members, their `loadGlyph` lines (`CustomLookAndFeelMenus.cpp:33-69`), the SVG files under `Resources/Icons` and their entries in `juce_add_binary_data(SequenceTreeIcons ...)` follow the icons:
  - `tree` → `fork`, `nodeRing` → `ring`, `nodeDot` → `dot`, `traversal` → `route`, `sync` → `link`, `tempo` → `metronome`;
  - `resetBar` / `resetTriangle` → `skipBar` / `skipTriangle`, `resetPatch` → `reload`, `nodeMode` → `circle`, `modulator` → `roundedSquare`, `traversalFlag` → `triangle`;
  - `paintHandle` / `paintTip` → `brushHandle` / `brushTip`;
  - `spanOutline` / `spanEnds` / `spanBridge` → `dashedBox` / `boxDots` / `boxDash`;
  - `host` → `circledH`, `danglingArrow` → `arrow`, `publish` → `upload`, `ornament` → `wave`, `rules` → `code`.

  The files are renamed with `git mv`, so the history extraction in Step 10 follows them.
- **Changes, shapes, symbols and theme:**

  | Today | Renamed to |
  |---|---|
  | `drawNode` | `drawCircleNode` |
  | `drawModulatorNode` | `drawSquareNode` |
  | `drawRootRectangle` | `drawRaisedPanel` |
  | `drawOrnamentSymbol` | `drawStrokeSymbol` |
  | `drawNodeOrnament` | `drawSymbolMark` |
  | `OrnamentMark` | `SymbolMark` |
  | `NodeVisual::isEncapsulationRinged` | `NodeVisual::hasOuterRing` |
  | `NodeVisual::encapsulationRingColour` | `NodeVisual::outerRingColour` |
  | `Theme::modulatorCornerRatio` | `Theme::squareNodeCornerRatio` |
  | `Theme::encapsulatorRimInset` / `encapsulatorRimWidth` | `Theme::innerRimInset` / `innerRimWidth` |
  | `Theme::encapsulationRingWidth` | `Theme::outerRingWidth` |
  | `Theme::spanOutlineColour` / `spanOutlineGap` / `spanOutlineWidth` (the ring drawn when `isOutlined`) | `Theme::outlineRingColour` / `outlineRingGap` / `outlineRingWidth` |
  | `Theme::scriptErrorColour` / `scriptOkColour` | `Theme::errorColour` / `successColour` |

  `recolour()` in `Theme.h` follows the colour renames.
- **Mechanical edits:**
  - `refactor.replace` or `refactor.rewrite` for each C++ symbol, so every call site and member-pointer assignment is found by clang rather than by text, including `MenuBar.cpp:7-9` and every `IconButton::icon` assignment.
  - `git mv` for the SVG files.
  - `refactor.edit` for `CMakeLists.txt` and the `IconData::…_svg` names.
- **Behaviour delta:** none. Names change and nothing that is drawn changes.
- **Verification:** build, tests. Manually, check every button in the title bar, bottom bar, menu bar, node menu, traversal menu, rules window and ornament windows shows its icon, and nodes, modulators, encapsulated nodes, outlined nodes and root rectangles render as before.
- **Commit:** `rename drawing vocabulary by appearance`
- **Rollback:** revert the commit.

**Phase A exit check:** every file in the *What the library is* table includes only files in that table, `Visuals.h`, `InterfaceIdentifiers.h`, JUCE, `BinaryData.h` and `IconData.h`. The include map used to write this plan (`grep '#include "'` over those files) is the check, and it must come back clean before Step 10.

### Phase B: the repository

### Step 10: Create the `ArsNovaUI` repository, with history

- **Goal:** a repository holding the library and nothing else, buildable as part of a consumer.
- **Changes:**
  - Create `~/Documents/GitHub/ArsNovaUI` with this layout:

    ```
    ArsNovaUI/                 repository root, the include root
    ├── CMakeLists.txt
    ├── README.md
    ├── .gitignore
    ├── Resources/Fonts/       Geist-Regular, Geist-SemiBold, GeistMono-Regular
    ├── Resources/Icons/       the 37 SVGs from SequenceTree's Resources/Icons
    └── ArsNovaUI/             sources, keeping today's sub-folders
        ├── Theme/  Buttons/  Menus/  Editors/  Editors/Formats/  Bars/  Canvas/
        └── PanelResizer, PopupWindow, ScaledView, LabelPanel
    ```

  - **Carry each file's history** with `git filter-repo`, which is not installed today; installing it with `brew install git-filter-repo` needs the owner's approval. The extraction runs on a throwaway clone, never on the working repository:
    1. `git clone --no-local ~/Documents/GitHub/SequenceTree /private/tmp/ArsNovaUIExtract`.
    2. Write a paths file listing every library file (the *What the library is* table, as renamed in Step 9, plus `Resources/Fonts` and the library's `Resources/Icons`) **and every path each one had before**, collected with `git log --follow --name-only --format= -- <file>`. Several files were moved in the past: `CustomLookAndFeel.h` was `Source/CustomLookAndFeel.h` and `Source/UI/CustomLookAndFeel.h`, and `DynamicPort.cpp` was `Source/DynamicPort.cpp` and `Source/Logic/DynamicPort.cpp`. Without the old paths, history stops at the last move.
    3. `git filter-repo --paths-from-file <paths> --path-rename Source/UI/:ArsNovaUI/ --path-rename Source/:ArsNovaUI/`, plus a `--message-callback` that drops any `Co-Authored-By` or "Generated with" line from carried messages. One commit in SequenceTree's history has such a line. Commits that touch none of the paths drop out.
    4. Check that `git log --follow` on three moved files (`CustomLookAndFeel.h`, `DynamicPort.cpp`, `IconButton.cpp`) reaches their first commit. Then move the clone to `~/Documents/GitHub/ArsNovaUI`.
  - Files that were never at a path under `Source/UI/` or `Source/` land where the renames put them. Any that end up outside the layout above are moved into place in one follow-up commit, `arrange library layout`.
  - `CustomLookAndFeelMenus.cpp` switches `BinaryData::` / `IconData::` to `ArsNovaUIFonts::` / `ArsNovaUIIcons::`.
  - `CMakeLists.txt` defines the `INTERFACE` target:

    ```cmake
    cmake_minimum_required(VERSION 3.22)
    project(ArsNovaUI VERSION 0.1.0)

    if(NOT COMMAND juce_add_binary_data)
        message(FATAL_ERROR "ArsNovaUI is added after JUCE by the project that uses it")
    endif()

    juce_add_binary_data(ArsNovaUIFonts HEADER_NAME ArsNovaUIFonts.h NAMESPACE ArsNovaUIFonts SOURCES ...)
    juce_add_binary_data(ArsNovaUIIcons HEADER_NAME ArsNovaUIIcons.h NAMESPACE ArsNovaUIIcons SOURCES ...)

    add_library(ArsNovaUI INTERFACE)
    target_sources(ArsNovaUI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ArsNovaUI/Theme/CustomLookAndFeel.cpp ...)
    target_include_directories(ArsNovaUI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(ArsNovaUI INTERFACE cxx_std_20)
    target_link_libraries(ArsNovaUI INTERFACE juce::juce_gui_basics juce::juce_gui_extra ArsNovaUIFonts ArsNovaUIIcons)
    ```

    Every `.cpp` is listed by absolute path, with no glob.
  - **The gallery app.** JUCE becomes a submodule of the library (`git submodule add https://github.com/juce-framework/JUCE.git JUCE`, pinned to the same commit as SequenceTree's). The block below sits at the top of the library's `CMakeLists.txt`, ahead of the `juce_add_binary_data` guard:

    ```cmake
    if(PROJECT_IS_TOP_LEVEL)
        add_subdirectory(JUCE)
    endif()
    ```

    The block below sits at the bottom:

    ```cmake
    if(PROJECT_IS_TOP_LEVEL)
        juce_add_gui_app(ArsNovaUIGallery PRODUCT_NAME "ArsNovaUI Gallery")
        target_sources(ArsNovaUIGallery PRIVATE Gallery/GalleryApplication.cpp Gallery/GalleryWindow.cpp)
        target_link_libraries(ArsNovaUIGallery PRIVATE ArsNovaUI juce::juce_recommended_config_flags)
    endif()
    ```

    - `Gallery/` holds one window showing every icon in its normal, hovered, selected and disabled `ButtonState`, plus a value editor per format, a context menu, an item selector, a colour selector, a bar frosted over a `DynamicPort` with a dot grid, nodes, arrows in every shaft and head style with trails, and stroke symbols.
    - When SequenceTree adds the library, `PROJECT_IS_TOP_LEVEL` is false, so neither the second JUCE nor the gallery is configured.
    - The gallery's build tree is `cmake-build-debug` inside the library repository, configured the same way as SequenceTree's (Ninja, the bundled CLion `ninja`).
  - `.claude/settings.json` enabling the research-suite plugin, so the Key Design Rules are injected in library sessions. `refactor.init` writes `refactor.toml` from the gallery's build tree, and the two `core_purpose_api` entries (`ArsNovaUI/Menus/ItemSelector.h:addItem`, `ArsNovaUI/Menus/ContextMenu.cpp:addItem`) are added to it.
  - A `CLAUDE.md` for the library: what it is, its boundary rules (from *Rules the library keeps*), the build and gallery commands, and the commit rules from *Commits*.
- **Behaviour delta:** none. SequenceTree is not touched yet.
- **Verification:** configure and build `ArsNovaUIGallery`, run it, and check every component in the window against SequenceTree's standalone.
- **Commits** (in `ArsNovaUI`, after the carried history): `add library build`, `add gallery app`, `add claude config`.
- **Rollback:** delete the repository.

### Step 11: SequenceTree builds against the library

- **Goal:** SequenceTree compiles the library from the local checkout and no longer holds the moved files.
- **Changes:**
  - `CMakeLists.txt`, after `add_subdirectory(JUCE)`:

    ```cmake
    set(ARSNOVAUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../ArsNovaUI" CACHE PATH "Checkout of the ArsNovaUI library")
    add_subdirectory(${ARSNOVAUI_PATH} ${CMAKE_CURRENT_BINARY_DIR}/ArsNovaUI)
    ```

    The binary directory sits inside the existing `cmake-build-debug` / `cmake-build-release` tree, so no new build tree is created.
  - Link `ArsNovaUI` `PRIVATE` to `SequenceTree`. Remove `SequenceTreeFonts` and `SequenceTreeIcons`: their targets, their link entries, and `Resources/Fonts` and `Resources/Icons`, which now live in the library.
  - Remove each moved `.cpp` from `target_sources(SequenceTree ...)`.
  - `SequenceTree_GraphTests` and `SequenceTree_RealtimeTests` add `${ARSNOVAUI_PATH}` to their `target_include_directories`, as they already add `JUCE/modules`.
  - Rewrite includes of moved headers to `#include <ArsNovaUI/...>`. That's 127 include lines in 79 files, counting the moved files, which are deleted.
  - Delete the moved files from `Source/UI`.
- **Mechanical edits:**
  - `refactor.edit` `<<< regex` over `Source` and `Tests`, mapping `#include "(\.\./)*(UI/)?(Theme/Theme|Theme/CustomLookAndFeel|...|Canvas/DynamicPort)\.h"` to `#include <ArsNovaUI/$3.h>`, run after the moved files are gone.
  - The file deletions go through whatever the `edit-tools` gate allows. If it denies `git rm` in `Source/`, the owner runs it.
- **Behaviour delta:** none.
- **Verification:**
  - Reload CMake in CLion, then build and run the tests.
  - Check that `cmake-build-debug/compile_commands.json` has an entry for each library `.cpp`.
  - Check that the installed AU and VST3 come from this build (`COPY_PLUGIN_AFTER_BUILD`).
  - Repeat the manual checks from Steps 3–9 in the standalone and in Live (`scripts/run-in-live.sh`).
- **Commit:** `build against ArsNovaUI`
- **Rollback:** revert the commit. The library repository is unaffected.

### Step 12: Update SequenceTree's documentation and config

- **Goal:** the project's instructions describe what is where.
- **Changes:**
  - `.claude/CLAUDE.md`: the *Build Commands* section names the library checkout and `ARSNOVAUI_PATH`.
  - *Directory Responsibilities* (`Source/UI/` bullets for `Theme`, `CustomLookAndFeel`'s file list, `DynamicPort`, `Bar`; `Source/UI/Editors/`) points at the library.
  - The look-and-feel paragraph in *How components get what they need* names the library's `CustomLookAndFeel`.
  - `.claude/refactor.toml`: remove the two moved `core_purpose_api` entries.
  - `.claude/research-tools.md`: note that library files are outside `sources`.
  - `.claude/AudioConcatenationToolLaunch.md`: the *UI library* section points here.
- **Behaviour delta:** none.
- **Verification:** `refactor.order all --check` and the design gates still run clean over `Source`.
- **Commit:** `update docs for ArsNovaUI`
- **Rollback:** revert the commit.

### Step 13 (later): Pin the library as a submodule

- **Goal:** a fresh clone of SequenceTree builds anywhere, against the exact library version it was tested with.
- **When:** once the library has stopped changing with every SequenceTree change.
- **Changes:**
  - Publish `ArsNovaUI` (the owner creates the remote).
  - `git submodule add <url> ArsNovaUI` beside `JUCE` and `Catch2`.
  - `ARSNOVAUI_PATH` defaults to `${CMAKE_CURRENT_SOURCE_DIR}/ArsNovaUI`.
  - The *Build Commands* section adds the submodule to `git submodule update --init`.
- **Behaviour delta:** none.
- **Verification:** a fresh clone with `git submodule update --init` configures and builds.
- **Commit:** `add ArsNovaUI submodule`
- **Rollback:** remove the submodule and restore the default path.

## Risks

- **Step 7 is the only step with real drawing logic in motion.** Arrow painting has the most states (style, hover, ghost, ruler fade, label timing, trails, head style), and no test covers painting. Its manual comparison list is the safeguard: run old and new builds on one saved patch.
- **Identifier strings (Step 3).** A typo silently drops saved presets, zoom and theme colour. Verify by loading state saved before the change.
- **Include rewrite (Step 11).** A missed relative include fails the build loudly, which is the safe kind of failure.
- **Installed plugin.** Step 11 changes the target's sources and links. `COPY_PLUGIN_AFTER_BUILD` must still install from `cmake-build-debug` / `cmake-build-release` only.

## Out of Scope

- The concatenator itself. It is planned in `.claude/AudioConcatenationToolLaunch.md`.
- Generic canvas mechanics: hit testing, selection and dragging written against plain data. Those are written against `Node` and `Arrow` today, and belong in the library only once a second canvas shows what is shared.
- Splitting `RTData.h` into its compiled-graph, vocabulary and message parts.
- Narrowing `NodeCanvas`'s and `RTGraphBuilder`'s `SequenceTreeAudioProcessor&` dependencies.
- Any visual redesign.

## Decisions for the Owner

All answered by the owner on 2026-10-12:

1. **Name:** `ArsNovaUI`.
2. **Packaging:** a plain CMake `INTERFACE` target listing every `.cpp` (*Approaches Considered*, Step 10).
3. **Gallery app:** yes. JUCE is a submodule of the library, and the gallery is built only when the library is the top-level project (Step 10).
4. **SequenceTree-named vocabulary:** renamed to what it draws as it moves (Step 9).
5. **Git history:** each file's history is carried over with `git filter-repo`, including its earlier paths (Step 10).
6. **`PitchFormat` and `GreekLetterFormat`:** in the library.
7. **Commits:** one per step, one short line, with no attribution of any kind (*Commits*).

## Implementation Notes

Every step is built and tested: the standalone, AU and VST3 build against the library and all 122 tests pass. The manual host checks of Steps 3–9 and 11 (the standalone and Live against a build from before the move) are still the owner's.

- **Steps 1–9:** one commit each, as planned (f0df072, fcf7f0d, 0bddb61, a0e148e, e7fe092, fcb9474, 82cd5de, 39ce593, c6da041). One unplanned commit followed, 7e6576d `give dangling arrow editor its own count format`, and it was carried into the library's history with the rest.
- **Step 10:** `git filter-repo` carried 114 commits, back to SequenceTree's first, with the moved files followed through their old paths and no attribution lines. Differences from the plan:
  - The gallery is four classes (`GalleryApplication`, `GalleryWindow`, `GalleryPage`, `GalleryCanvas`) rather than two files, and takes `--snapshot <png>` to write the window to an image and quit, which is how it was checked.
  - `refactor.init` chose `ArsNovaUIFonts` as the build target and `.` (JUCE included) as the sources, so `refactor.toml` names `ArsNovaUIGallery`, `ArsNovaUI` and `["ArsNovaUI", "Gallery"]` by hand.
  - The JUCE submodule's git directory was absorbed into `.git/modules` (`git submodule absorbgitdirs`).
  - The library's `CLAUDE.md` is at `.claude/CLAUDE.md`, as in SequenceTree.
- **Step 11:** 109 include lines in 59 files were rewritten by one `refactor.edit` batch, and no object in the build depends on the old `Source/UI` copies. Auto mode denied `git rm` of the 52 moved files and `Resources/`, so the owner ran it. `.idea/SequenceTree.iml` was found deleted during this step and was left out of its commit.
- **Step 12:** the *Editors* section of `.claude/CLAUDE.md` still described `LineEditor` and `FileLine`, which no longer exist; it now describes the library's `FilePage` and `FileLabel`.
- **Step 13:** built the same day on the owner's request, rather than once the library settled. The owner created the repository as public, since SequenceTree is public and a private submodule would stop its clones building. `ARSNOVAUI_PATH` defaults to the submodule, and the old `../ArsNovaUI` value was cleared from the `cmake-build-debug` and `cmake-build-release` caches with `cmake -U ARSNOVAUI_PATH`. Both `CLAUDE.md` files describe moving the submodule to a new library commit, and pointing a build tree at the sibling checkout while working on both. SequenceTree itself was not pushed.
