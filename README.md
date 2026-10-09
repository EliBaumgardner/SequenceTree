<div align="center">

# SequenceTree

**Draw a graph. Hear a sequence.**

A polyphonic, generative MIDI sequencer. You draw trees of notes, and traversals walk them: counting their visits, branching, looping, spawning one another, and playing many voices at once.

![C++20](https://img.shields.io/badge/C%2B%2B20-1f2328?style=for-the-badge&logo=cplusplus&logoColor=white)
![JUCE 8](https://img.shields.io/badge/JUCE_8-1f2328?style=for-the-badge&logo=juce&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-1f2328?style=for-the-badge&logo=cmake&logoColor=white)
![Catch2](https://img.shields.io/badge/Catch2-1f2328?style=for-the-badge)
![AU · VST3 · Standalone](https://img.shields.io/badge/AU_·_VST3_·_Standalone-1f2328?style=for-the-badge)
![MIT License](https://img.shields.io/badge/MIT_License-1f2328?style=for-the-badge)

<img src="documentation/media/demo.gif" alt="SequenceTree playing a graph: nodes light up as the sequence walks through them" width="800">

[How a traversal unfolds](#how-a-traversal-unfolds) · [Node types](#node-types) · [Node values](#node-values) · [Polyphony](#polyphony) · [Editing and playback](#editing-and-playback) · [Under the hood](#whats-under-the-hood) · [Building](#building) · [Scripting guide](documentation/ScriptingLanguage.md)

</div>

---

## What it is

Most sequencers store a pattern and play it back. SequenceTree stores **rules** for a pattern. Every node is a note, every arrow is a path the music can take, and a small set of counters on each node decides which path is taken on which pass. A graph of a dozen nodes can play for minutes before it repeats, and several graphs, or several voices in one graph, play at the same time.

SequenceTree is an instrument plugin that only sends MIDI. It drives any synth or sampler in your DAW (AU and VST3), or runs as a standalone app.

## How a traversal unfolds

A **traversal** is one voice moving through a tree. It plays a node, waits for the note to end, chooses the next node, and plays that.

1. **It starts at a root** and follows arrows downward, playing each node it lands on.
2. **The arrow's length is the note's length.** The note a node plays lasts as long as the arrow leaving it toward the next node, so the shape of the graph is its rhythm. Drag a node and the music retimes as you drag.
3. **Each node counts how many times it has been left**, and its children's **count limits** decide where it goes. A child with count limit *N* is a candidate every *N*th time, and the candidate with the highest limit wins.
4. **When it runs out of places to go, it returns to the root** and starts the next pass. The counts are kept, so the next pass takes a different route.
5. **It keeps looping** until the root's loop limit is reached (0 means forever).

For example, a root R leads to A, and A has three children: B (count limit 1), C (2) and D (4). Each pass ends at a leaf and goes back to R:

```
pass 1   R A B        A left 1 time:  only B qualifies
pass 2   R A C        A left 2 times: B and C qualify, C has the higher limit
pass 3   R A B
pass 4   R A D        A left 4 times: B, C and D qualify, D wins
...      the cycle repeats every 4 passes
```

Nest that pattern a few levels deep, with different limits at each level, and the cycles multiply into long phrases that evolve rather than loop.

## Node types

There are three kinds of node, chosen from the bottom bar before you drag out a new one.

### Note nodes

The notes themselves. A **root** starts a tree and carries the traversal that plays it, its loop limit and its tempo multiplier. Every **child** holds a pitch, velocity and MIDI channel, and new children inherit their parent's notes and limits so a branch can be sketched quickly.

**Alternative nodes** are variations on a note. Ctrl-drag from a node to hang an alternative off it. Each time the walk lands on the node, its alternatives are chosen by count limit exactly as children are, and the chosen one replaces the node's pitch, velocity, length and repeat for that visit. When none qualifies, the node plays itself. Alternatives can have alternatives of their own, so a single position in the tree can cycle through a set of voicings.

### Modulators

A **modulator** is a small tree hung off a note node that changes how that note plays. It has a traversal of its own that walks the modulator tree with the same counting rules, one step for each note played under its host. Each modulator node shifts the host's pitch by its offset and scales its length by its arrow (an arrow of one second leaves the length unchanged). Because the modulator walk has its own counts, the same melody comes back transposed and stretched differently on each pass. Unsync a modulator's arrow and it steps once per visit to the host instead.

### Traversal flags

A **traversal flag** starts or stops a voice. Hang a flag off a note node, then Shift-drag from the flag onto the node where the new voice should start, and type which traversal it controls: `+2` starts traversal 2 there, `-2` stops it. The flag fires every *N*th time its host is played (its count limit), and the length of the arrow to the flag is how long it waits before firing. Flags are how one melody calls in a bass line every eighth bar, or how a voice ends itself after a set number of passes.

## Node values

Every value below is edited on the node itself or in the node menu.

| Value | What it does |
|---|---|
| **Count limit** | How often this node is chosen. With limit *N* it becomes a candidate every *N*th time its parent is left; the highest limit among the candidates wins. Modulators, flags and alternatives use it the same way. |
| **Trigger limit** | The second number in the count editor. The node can be chosen at most this many times, then it is skipped. 0 means no limit. |
| **Switch count** | Once chosen, the parent keeps choosing this node for this many visits in a row before it counts again. A switch count of 3 on C in the example above turns pass 2 into three passes of R A C. |
| **Sub-loop count** | Turns the node into a loop point. After the walk enters it, reaching the end of the branch returns the walk here instead of to the root, this many times, before it goes back to the root. 1 (the default) means no sub-loop; 0 loops the branch forever. |
| **Loop limit** (roots) | How many passes the tree plays before its traversal stops. 0 loops forever. |
| **Repeat** | Plays the note this many times in a row before moving on. |
| **Probability** | Weights the choice when several candidates share the same count limit. If the weights add up to less than 100%, the rest is the chance that the walk stops here and returns to the root. |
| **Pitch, velocity, channel** | The MIDI note the node sends. |

Arrows carry values too:

- **Length** is the note's duration.
- **Disabled traversals.** An arrow can be switched off for particular traversals, so two voices walking the same tree take different routes through it.
- **Dangling arrows**, drawn from a node into empty space, give a leaf node its length. They have count limits, so a leaf can be short on most passes and long on every fourth.

## Polyphony

SequenceTree plays many voices at once, in several ways that combine freely.

- **Chords.** Place a child directly below its parent and it sounds together with the parent instead of after it. Chord notes keep their count limits, so a voice can join the chord only every *N*th time.
- **Many trees.** Every root on the canvas runs its own traversal at the same time, each with its own rhythm and cycle lengths.
- **Traversals are voices with their own settings.** Each traversal has a colour on the canvas, a MIDI channel, a transpose, a velocity scale and a tempo multiplier. Send traversal 1 to a piano on channel 1 and traversal 2 to a bass on channel 2, an octave down and at half speed, through the same tree.
- **Flags add and remove voices** while the music plays (see [Traversal flags](#traversal-flags)).
- **Trees connect to each other** in three ways:
  - A **cross root tree connection** (drag a node onto another root) starts that tree's own traversal every *N*th time it is reached, while the current voice carries on.
  - A **regular connection** steps into the other tree, walks it, and returns home. The other root's loop limit sets how many times it loops there first.
  - A **traversal arrow** moves the voice onto the other tree for good.

## Editing and playback

- **Drawing.** Shift-click places a root, Shift-drag from a node creates a child, Shift+right-click deletes. Dragging a node moves its descendants with it and snaps to a grid.
- **Pitch-bound arrows.** Bind arrows to the X or Y axis and moving a node transposes it, keeping whatever pitch you last typed as the reference.
- **Paint mode.** Paint pitch or velocity across the canvas with a brush, and the nodes under it take on the painted values.
- **Preview.** The Note tool auditions the sequence from any node you click, optionally on repeat.
- **Groups.** Gather nodes into a group that collapses to one node and loops as a unit, and dissolve it when you need the parts again.
- **Scripting.** Replace the built-in choosing rules with your own: a short script decides which child comes next and what each step plays. Errors are marked on the line as you type, and the last working version keeps playing. See the [scripting guide](documentation/ScriptingLanguage.md).
- **DAW sync.** In a DAW the plugin follows play, stop, tempo and the playhead. Jump anywhere in the song and the graph lands in exactly the state it would have reached by playing there, counts and all.

## What's under the hood

### Audio that never stutters

Audio software has a strict deadline. Every few milliseconds the computer asks the plugin for the next slice of sound. If the plugin is even slightly late, the listener hears a click or a dropout.

The usual cause of lateness is waiting: on memory being handed out, or on another part of the program to finish. **SequenceTree's audio code never waits.**

- **It never asks for memory while playing.** Everything it needs, including room for every voice that can run at once, is set aside before playback starts.
- **The editor never blocks the music.** Editing builds a complete new copy of the graph off to the side, then swaps it in all at once. The music always reads either the old graph or the new one, never a half-edited mix. Old copies are cleaned up only once the audio is certainly finished with them.
- **The audio side never waits for the screen.** It drops messages for the display ("light up this node") into a mailbox the screen picks up once per frame. If the window is closed, the music carries on as normal.
- **This is tested, not just intended.** A dedicated test build uses a checker that crashes the program the instant the audio code reaches for memory, waits on another part of the program, or makes any other call that could be slow. That test plays a graph while it is edited, retimed, rewound and stopped.

### A programming language built in

The scripting feature is a small language written from scratch. The script is read, checked and translated into compact instructions for a tiny virtual machine, and that machine runs the script during playback. The translating happens off the audio path, so only the finished instructions ever reach it.

User code can't be allowed to freeze the music, so every script runs with a **fixed step allowance**. A script stuck in an endless loop is cut off and playback continues. A script with errors never goes live, and the last version that worked keeps playing.

### One set of rules for every traversal

Note traversals, modulators, alternatives and trees that are stepped into all move by **the same code path**: the same counting, switch counts, sub-loops and trigger limits. A rule written once behaves the same everywhere. The test suite checks this by building the same shape as a note tree and as a modulator tree and confirming both play the identical sequence.

### Tested

About 90 automated tests ([Catch2](https://github.com/catchorg/Catch2)) cover how traversals move through a graph, the script language, and how a graph's shape becomes timing and pitch. The core that walks the graph is independent of the plugin framework, so its tests build in seconds.

## AI Integration and Workflow

I designed SequenceTree and wrote its core by hand. AI agents help with audits and large refactors under tooling I built to hold them to my design rules. See [AI Integration and Workflow](documentation/AIWorkflow.md).

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
