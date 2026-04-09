#pragma once

#include "TraversalLogic.h"
#include "../Util/PluginModules.h"
#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>

class SequenceTreeAudioProcessor;

class EventManager {
public:

    std::shared_ptr<std::unordered_map<int, TraversalLogic>> traversals;

    std::atomic<int> pendingAsyncCalls { 0 };

    struct MidiEvent  { int pitch; int velocity; int duration; };
    struct ActiveNote { MidiEvent event; int traversalId = 0; int remainingSamples = 0; RTNode noteNode; };

    std::vector<ActiveNote> activeNotes;

    SequenceTreeAudioProcessor* processor;

    explicit EventManager(SequenceTreeAudioProcessor* p);

    void handleEventStream (int sample, juce::MidiBuffer& midiMessages, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap);
    void pushNote          (RTNode node, int traversalId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap);
    void pushConnectorNotes  (int traverserId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap);
    void clearOldEvents      (int traversalId);
    void highlightNode       (const RTNode& node, bool shouldHighlight);
};