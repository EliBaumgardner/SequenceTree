# GEMINI.md - SequenceTree Architecture & Engineering Guide

This file provides architectural guidance, real-time safety invariants, and engineering standards for Antigravity when working with code in this repository.

---

## 1. Architectural Documentation & Deep Dives

Detailed architectural audits, research, and design patterns are maintained in:
- **[`docs/architecture/sequence_tree_deep_dive_audit.md`](docs/architecture/sequence_tree_deep_dive_audit.md)**: Full 30-minute architectural audit, real-time safety scrutiny, discrete-event scheduling analysis, and modernization roadmap.
- **[`docs/architecture/data_oriented_design_and_modern_cpp_juce_research.md`](docs/architecture/data_oriented_design_and_modern_cpp_juce_research.md)**: Deep dive into Data-Oriented Design (DOD), flat graph arenas, interleaved cacheline-resident state tables, modern C++20 features (`std::span`, concepts, ranges), and JUCE 8 APVTS/graphics patterns.
- **[`docs/architecture/systemic_architecture_cpp20_and_juce8_critique.md`](docs/architecture/systemic_architecture_cpp20_and_juce8_critique.md)**: Systemic architecture, modern C++20, and JUCE 8 critique covering domain authority inversion, DOD cacheline false sharing, DAW musical grid synchronization, RCU allocation thrashing, and compiler VM semantics.
- **[`docs/architecture/foundational_architecture_cross_engine_comparative_treatise.md`](docs/architecture/foundational_architecture_cross_engine_comparative_treatise.md)**: Comprehensive cross-engine comparative treatise against SuperCollider, Pure Data, VCV Rack, Bespoke Synth, and modern C++ real-time systems, detailing theoretical time bases (Ross Bencina, Randy Brown), CSR flat graph arenas, lock-free triple buffers, monadic compilation, and single-component JUCE 8 retained-mode canvas.
- **[`docs/architecture/advanced_cross_engine_systems_treatise_and_architectural_scrutiny.md`](docs/architecture/advanced_cross_engine_systems_treatise_and_architectural_scrutiny.md)**: Advanced scrutiny and extension of the foundational treatise, delivering deep cross-engine analysis with Pure Data (ANSI C struct-header OO, tagged-union atoms, socket IPC, linear perform arrays, memory accounting, declarative `.pd` formats), SuperCollider, VCV Rack, Bespoke, and Faust, with refined solutions for dynamic graph mutations (Generational Slot-Maps), DAW tempo automation ramps (Dual-Domain PPQ queues), and state migration.
- **[`gemini complaints.md`](gemini%20complaints.md)**: Actionable defect and design item tracking list (P0 through P3).

---

## 2. Core Architectural Principles & Invariants

### A. Real-Time Audio Thread Safety (Hard Deadlines)
The audio thread (`processBlock` and all functions called within it) executes under hard sub-millisecond deadlines.
- **NEVER allocate or free memory on the audio thread**: Snapshots are published via `AudioSnapshotPublisher` (RCU pattern) with atomic pointer release/acquire and deferred message-thread retirement.
- **NEVER throw or catch C++ exceptions on the audio thread**: Never use `.at()` on maps or containers. Always use `.find() != .end()`.
- **NEVER perform linear scans over large collections in the per-event loop**: Use a preallocated binary min-heap for active note expiration.
- **Pure Lookahead**: Duration lookahead (`peekNextTarget`) must be strictly side-effect free. It must never mutate traversal state (`SwitchCandidate` or `LastNode`).
- **Container Iteration Invariants**: Never use swap-and-pop (`removeNote`) inside a decrementing backwards loop without handling the element moved from `back()`.

### B. Clean Architecture & Unidirectional Data Flow
- **Domain Model Authority**: The audio engine must never depend on visual UI components (`NodeCanvas`, `NodeManager`, `ArrowManager`) to trigger graph compilation.
- **Headless Execution**: Graph compilation (`RTGraphBuilder`) is orchestrated by a domain listener attached to `GraphState` at the `SequenceTreeAudioProcessor` level, functioning whether the plugin GUI window is open or closed.
- **Undo Lifetime**: `juce::UndoManager` belongs to `SequenceTreeAudioProcessor`, preserving undo/redo history across window open/close cycles.

### C. Data-Oriented Design (DOD) & Cache Locality
- **Contiguous Graph Arena**: Real-time graphs should favor flat, contiguous memory buffers (`FlatRTGraph`, `CompactNode`, `CompactConnection`) indexed by compact dense integers $[0 \dots N-1]$ rather than pointer-chased heap nodes (`unordered_map<int, shared_ptr<RTNode>>`).
- **Interleaved Node State**: `NodeStateTable` slots are laid out node-major (`nodeIndex * slotCount + slot`), packing all 11 slots for a node into 44 bytes (a single 64-byte L1 cache line), with memory dynamically sized to active nodes.

### D. Modern JUCE 8 Idiomatic Leverage
- **Host Automation & APVTS**: All musical parameters (tempo multiplier, master transpose, MIDI channel, probability) should be registered in `AudioProcessorValueTreeState::ParameterLayout` for sample-accurate DAW automation and MIDI controller binding.
- **Binary State Serialization**: Use `ValueTree::writeToStream` and `ValueTree::readFromStream` for fast, compact, bit-exact preset save/restore.
- **Centralized Frame Clock**: Arrow animations are coordinated by a single `juce::VBlankAttachment` on the canvas, advancing active trails in one pass and issuing a single coalesced repaint.

---

## 3. Build & Development Workflow

```bash
# Build (debug)
cmake --build cmake-build-debug

# Build (release)
cmake --build cmake-build-release
```

- Targets: `SequenceTree_Standalone` is the fastest link target for rapid testing.
- Formats built: AU, VST3, Standalone (`IS_SYNTH TRUE`).
- Compiler standard: C++20 (`cxx_std_20`).
