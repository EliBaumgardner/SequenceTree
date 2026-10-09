<div align="center">

# SequenceTree

**Draw a graph. Hear a sequence.**

A MIDI sequencer plugin where you compose music by drawing graphs of notes that gets traversed.

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![JUCE 8](https://img.shields.io/badge/JUCE-8-8DC63F)
![Formats](https://img.shields.io/badge/formats-AU%20%7C%20VST3%20%7C%20Standalone-555)
![License: MIT](https://img.shields.io/badge/license-MIT-green)

<img src="documentation/media/demo.gif" alt="SequenceTree playing a graph: nodes light up as the sequence walks through them" width="800">

[Features](#features) · [What's under the hood](#whats-under-the-hood) · [Built with AI agents](#built-with-ai-agents) · [Building](#building) · [Scripting guide](documentation/ScriptingLanguage.md)

</div>

---

## How it works

You place **nodes** on a canvas and connect them with **arrows**. Each node holds a note. When you press play, SequenceTree walks the graph and plays every node it lands on.

- **An arrow's length is the note's length.** Stretch an arrow and that note lasts longer. The layout of the graph *is* the rhythm.
- **Nodes count their visits.** Each node has a count limit. Once a node has been played that many times, the walk moves on to a different child. Simple counts like this build up into long patterns that evolve over time without repeating.
- **It plays into anything.** SequenceTree only sends MIDI, so it can drive any synth or sampler in your DAW, or run standalone.

## Features

| | |
|---|---|
| **Visual composition** | Shift-click to place a node and drag to connect it. Moving a node retimes the music as you drag. |
| **Chords** | Stack nodes vertically to play them together. |
| **Pitch-linked arrows** | Bind an arrow to pitch, so moving a node up or down also transposes it. |
| **Modulators** | A small graph attached to a node that walks alongside it, shifting its pitch and stretching its length differently on each visit. |
| **Many walkers at once** | Up to 128 walks run through the same graph at the same time, and flags on the canvas can start new ones. |
| **Groups** | Gather nodes into a group to handle them as one, and dissolve it again when you need to. |
| **Scripting** | Write a short script to change how the walk picks its next note. Your script is checked as you type and errors are marked on the line. See the [scripting guide](documentation/ScriptingLanguage.md). |
| **DAW sync** | Follows your DAW's play, stop, tempo and playhead. Jump anywhere in the song and the graph lands where it would have been. |

## What's under the hood

### Audio that never stutters

Audio software has a strict deadline. Every few milliseconds the computer asks the plugin for the next slice of sound. If the plugin is even slightly late, the listener hears a click or a dropout.

The usual cause of lateness is waiting: on memory being handed out, or on another part of the program to finish. **SequenceTree's audio code never waits.**

- **It never asks for memory while playing.** Everything it needs is set aside before playback starts.
- **The editor never blocks the music.** Editing builds a complete new copy of the graph off to the side, then swaps it in all at once. The music always reads either the old graph or the new one, never a half-edited mix. Old copies are cleaned up only once the audio is certainly finished with them.
- **The audio side never waits for the screen.** It drops messages for the display ("light up this node") into a mailbox the screen picks up once per frame. If the window is closed, the music carries on as normal.
- **This is tested, not just intended.** A dedicated test build uses a checker that crashes the program the instant the audio code reaches for memory, waits on another part of the program, or makes any other call that could be slow. That test plays a graph while it is edited, retimed, rewound and stopped.

### A programming language built in

The scripting feature is a small language written from scratch. The script is read, checked and translated into compact instructions for a tiny virtual machine, and that machine runs the script during playback. The translating happens off the audio path, so only the finished instructions ever reach it.

User code can't be allowed to freeze the music, so every script runs with a **fixed step allowance**. A script stuck in an endless loop is cut off and playback continues. A script with errors never goes live, and the last version that worked keeps playing.

### One set of rules for every walk

The main walk, modulators and nested graphs all move through a graph by **the same code path**. A rule written once behaves the same everywhere. The test suite checks this by building the same shape both ways and confirming they play the identical sequence.

### Tested

About 90 automated tests ([Catch2](https://github.com/catchorg/Catch2)) cover how the walk moves through a graph, the script language, and how a graph's shape becomes timing and pitch. The core that walks the graph is independent of the plugin framework, so its tests build in seconds.

## Built with AI agents

This project is also where I develop and test my own tooling for working with AI coding agents. It's included on purpose, and it lives alongside the code:

- **`.claude/` is the agent's instructions and research trail.** `CLAUDE.md` is the project briefing the agent works from. `context/` holds the pipeline's paper trail: an analysis agent audits the code, a second agent reviews each claim against the real source like an advisor grading a paper, the claims that survive become a written plan, and only an approved plan gets built. Each stage is a folder of plain markdown you can read.
- **[research-suite](https://github.com/EliBaumgardner/research-suite)**, a plugin I wrote, provides that pipeline, plus a set of C++ refactoring commands and automatic checks. The checks hold every edit to this project's design rules before the agent is allowed to finish.
- **`treejev/`** holds decision trees from TreeJev, another tool of mine. On every request, a tree decides what kind of task it is (a quick question, a code change, research) and which checks the agent owes before it can stop. `TreeJevAgentProtocol.md` is generated from those trees.

Agents do the typing. I make the design decisions, set the rules they're held to, and decide what gets merged.

## Building

**Requirements:** CMake 3.22+, a C++20 compiler (Xcode on macOS), and Git.

```bash
git clone https://github.com/EliBaumgardner/SequenceTree.git
cd SequenceTree
git submodule update --init --recursive   # fetches JUCE and Catch2

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

A successful build installs the AU, VST3 and Standalone app to your system plugin folders automatically.

**Running the tests:**

```bash
cmake --build build --target SequenceTree_Tests SequenceTree_GraphTests
cd build && ctest --output-on-failure
```

## License

SequenceTree's own source is MIT licensed. See [LICENSE](LICENSE).

It is built on [JUCE](https://juce.com), which is separately licensed under AGPLv3 or a JUCE commercial license. Using and distributing the built plugin is subject to those terms.
