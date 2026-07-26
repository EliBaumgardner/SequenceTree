#pragma once

#include "AudioUIBridge.h"
#include "NoteScheduler.h"
#include "TraversalDispatcher.h"

class SequenceTreeAudioProcessor;

class EventManager
{
public:

    AudioUIBridge       bridge;
    NoteScheduler       scheduler   { bridge };
    TraversalDispatcher dispatcher;

    explicit EventManager(SequenceTreeAudioProcessor* p);

    void processEvents(int numSamples, juce::MidiBuffer& midiMessages,
                       const NodeMap& nodes, TraversalPool& traversalMap);

private:

    void handleOrphanNotes(juce::MidiBuffer& midiMessages,
                           const NodeMap& nodes, TraversalPool& traversalMap);
};
