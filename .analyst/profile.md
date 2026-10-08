# SequenceTree — Analyst Profile

The project profile the analysis agent gets beside the research suite's generic brief. The brief (role, evidence and judgment standards, deliverables) and the analysis procedure come from the research-suite plugin, and the build and test targets from `.claude/research.toml`; this file holds only what is specific to SequenceTree.

## The Project

SequenceTree is a JUCE MIDI-generating plugin that walks a user-drawn directed graph. It is a solo-developer project: size recommendations for one person, and don't recommend enterprise patterns without a concrete problem.

### Feature Gates

`NodeController` and `NodeCanvas` are this project's feature gates, in the sense of the Key Design Rule on feature gates. Every canvas gesture enters through `NodeController`'s `mouseDown` / `mouseDrag` / `mouseUp`, and every canvas feature is mounted on `NodeCanvas`. New features keep arriving along both axes, so judge both by what the next feature costs them, not by whether they work today.

- `NodeCanvas` is the shape to aim for. Its features are parts it owns (`NodeManager`, `ArrowManager`, `AudioCommandDrainer`, `CanvasHitTester`, `EncapsulationView`, `ValueField`), and the canvas routes to them. Its remaining dispatch, `AsyncUpdateType` and `handleAsyncUpdate`, is where to watch it grow.
- `NodeController` is the counter-example. Hover, value dragging, dangling arrows, arrow-head moves, flag connections, child creation, root snapping, box selection, span selection, encapsulation expansion and the context menus each live in it as their own `DragState` values, branches and handlers, so every new gesture-borne feature grows the one class. An earlier unreviewed report (`node_controller_decomposition_analysis.md`) concluded it should not be decomposed because it is cohesive and its state is shared. Under the feature-gate rule, those are not reasons, and its growth test was never run.

## Frameworks, Versions and Tooling

- **JUCE 8.0.10** (submodule `JUCE/`) and **C++20**. Advice for JUCE 6/7 or C++23-only features must be marked as such. Confirm any JUCE facility by reading its header in `JUCE/modules/`.
- CMake with Ninja, Catch2 v3 (submodule `Catch2/`), clang/clangd tooling, and RealtimeSanitizer. Check whether each is used to its full extent.

| Purpose | Command |
|---|---|
| Build (fastest full link) | `cmake --build cmake-build-debug --target SequenceTree_Standalone` |
| Tests | `cmake --build cmake-build-debug --target SequenceTree_Tests SequenceTree_GraphTests && (cd cmake-build-debug && ctest --output-on-failure)` |

- Build only into `cmake-build-debug` / `cmake-build-release`.
- A RealtimeSanitizer build lives outside the repo; see CLAUDE.md's Tests section if a real-time claim needs confirming.

## Model Invariants to Check

The most valuable flaws are violations of the invariants in CLAUDE.md's Architecture Overview:

- **Every walk is the same traversal.** The primary walk, `TraversalLogic::ModulatorWalk`, alternatives and stepped-into trees must unfold by one mechanic. A behaviour present in one walk and not another is a bug. For any traversal finding, check all walks.
- **Geometry is data.** Duration is derived from arrow vectors; pitch is owned and only shifted through `ArrowPitchOffset`.
- **Host transport and replay.** Host-transport following and replay must land on the same state as playing through, at any tempo.
- **Audio-thread safety.** Nothing reachable from `processBlock` allocates, frees, locks, throws or posts a message. Name the call path from `processBlock` for any claimed violation.

Data enters through `processBlock`, `ValueTree::Listener` callbacks, mouse handlers and `handleAsyncUpdate`. Trace data flow across class boundaries: a finding about `TraversalSession` usually requires understanding `EventManager`, `TraversalPool` and `AudioSnapshotPublisher` too.

## What Counts as a Concrete Input

A claimed traversal bug comes with a graph shape; a host-transport bug with a host action; an editing bug with an edit sequence.

## Outward Research

Compare SequenceTree against other graph/node sequencers and patchers (Pure Data, Max, SuperCollider, VCV Rack, Bespoke, Bitwig Grid), JUCE's own examples and modules, and established real-time audio literature (Ross Bencina, Timur Doumler, the JUCE/ADC talks, RealtimeSanitizer docs). A research question names a class here, for example: "How do established sequencers keep scheduled note-offs sample-accurate across tempo changes, and does `EventManager::followTempo` match?"

## Existing Reports

Several items in the earlier reports have since been fixed (for example, the UI no longer drives `makeRTGraph` — `RTGraphBuilder` now owns its own `ValueTree::Listener` at processor level). Mark stale items as resolved rather than repeating them.

- `Unreviewed/ProgramAudits/`: `sequence_tree_deep_dive_audit.md`, `systemic_architecture_cpp20_and_juce8_critique.md`
- `Unreviewed/ResearchReports/`: `data_oriented_design_and_modern_cpp_juce_research.md`, `foundational_architecture_cross_engine_comparative_treatise.md`, `advanced_cross_engine_systems_treatise_and_architectural_scrutiny.md`
