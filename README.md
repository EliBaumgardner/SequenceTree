# SequenceTree

An audio plugin (AU / VST3 / VST / Standalone) that generates MIDI by traversing a user-designed directed graph.

You place nodes on a canvas, give each one MIDI note data and a count limit, and connect them with arrows. During playback the plugin walks the graph: each time a node is visited its counter advances, and when the counter reaches the node's limit, traversal moves on to its matching children. Arrow length encodes note duration, so the shape of the graph is the shape of the sequence.

## Building

The JUCE submodule must be initialized first:

```bash
git submodule update --init
```

Then configure and build:

```bash
cmake -B build
cmake --build build --config Release
```

`JUCE_COPY_PLUGIN_AFTER_BUILD` is on, so a successful build installs the plugin to the system AU/VST3/VST/Standalone locations automatically.

Note that `CMakeLists.txt` currently points `juce_set_vst2_sdk_path` at a local path; adjust or remove it if you aren't building the VST2 target.

## License

SequenceTree's own source is MIT licensed — see [LICENSE](LICENSE).

It links against [JUCE](https://juce.com), which is separately licensed under AGPLv3 or a JUCE commercial license. Your use and distribution of the built plugin is subject to those terms.
