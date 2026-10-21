#pragma once

#include "Traversal/TraversalPool.h"
#include "../Graph/RTData.h"

#include <juce_audio_basics/juce_audio_basics.h>

/*
    The inputs to one block of playback, bundled so the playback classes hand them to each other as
    one argument: the graph (nodes), the pool of traversals being played (traversalMap), the
    MidiBuffer the notes are written to, and the sample rate, tempo multiplier, transpose and
    velocity scale that decide how they sound.

    It describes a single block and holds references, so a new one is made every block.
    SequenceTreeAudioProcessor::driveWalk makes one for playback and one for note preview, and
    TraversalSession makes one writing into replayMidi while it replays to a new host position.
*/

struct DispatchContext
{
    const NodeMap&    nodes;
    TraversalPool&    traversalMap;
    juce::MidiBuffer& midiMessages;

    double sampleRate      = 44100.0;
    double tempoMultiplier = 1.0;
    int    transpose       = 0;
    double velocityScale   = 1.0;
};
