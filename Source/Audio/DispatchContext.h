#pragma once

#include "Traversal/TraversalPool.h"
#include "../Graph/RTData.h"

#include <juce_audio_basics/juce_audio_basics.h>

/*
    The configuration for one block of playback: which graph and which traversals are played,
    where the resulting MIDI is written, and how notes sound (sample rate, tempo, transpose and
    velocity scale).

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
