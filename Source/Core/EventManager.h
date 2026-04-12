#pragma once

#include "TraversalLogic.h"
#include "../Util/PluginModules.h"
#include <memory>
#include <atomic>
#include <vector>
#include <array>
#include <unordered_map>

class SequenceTreeAudioProcessor;

using TraversalMap = std::unordered_map<int, TraversalLogic>;

class EventManager
{
public:

    struct MidiEvent
    {
        int pitch    = 0;
        int velocity = 0;
        int duration = 0;
    };

    struct ActiveNote
    {
        MidiEvent event;
        int       traversalId      = 0;
        int       remainingSamples = 0;
        RTNode    noteNode;
        bool      isConnectionTrigger = false;
    };

    struct HighlightCommand
    {
        int  nodeId         = 0;
        bool shouldHighlight = false;
    };



    TraversalMap                   traversals;   // audio-thread-private, never accessed from GUI
    std::vector<ActiveNote>        activeNotes;
    SequenceTreeAudioProcessor*    processor;

    static constexpr int kHighlightFifoSize = 512;
    juce::AbstractFifo                                    highlightFifo { kHighlightFifoSize };
    std::array<HighlightCommand, kHighlightFifoSize>      highlightBuffer {};


    explicit EventManager(SequenceTreeAudioProcessor* p);

    void processEvents       (int numSamples, juce::MidiBuffer& midiMessages, NodeMap& nodes, TraversalMap& traversalMap);
    void pushNote            (const RTNode& node, int traversalId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap);
    void pushConnectorNotes     (int traverserId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap);
    void pushModulatorNotes     (int modulatorRootId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap);
    void pushRootNodeConnection (int rootNodeId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap);
    void clearOldEvents      (int traversalId);
    void highlightNode       (const RTNode& node, bool shouldHighlight);

private:

    static bool isNodeAudible(RTNode::NodeType nodeType);

    void resetTraversal(int graphId, int newTargetId, NodeMap& nodes, TraversalMap& traversalMap);

    void handleOrphanNotes(juce::MidiBuffer &midiMessages, NodeMap &nodes, TraversalMap &traversalMap);

    void handleNoteCreation(juce::MidiBuffer &midiMessages, NodeMap &nodes, TraversalMap &traversalMap,
                            int priorityNoteDuration, ActiveNote expiredNote, int traversalId,
                            TraversalLogic &traversal,
                            RTNode::NodeType type);
};