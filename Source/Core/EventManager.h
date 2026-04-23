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
    TraversalMap        traversals;

    explicit EventManager(SequenceTreeAudioProcessor* p);

    void processEvents(int numSamples, juce::MidiBuffer& midiMessages,
                       NodeMap& nodes, TraversalMap& traversalMap);

private:

    void handleOrphanNotes(juce::MidiBuffer& midiMessages,
                           NodeMap& nodes, TraversalMap& traversalMap);
};
