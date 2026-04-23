#pragma once

#include "TraversalLogic.h"
#include "NoteScheduler.h"
#include <memory>
#include <atomic>

class AudioUIBridge;
class SequenceTreeAudioProcessor;

class TraversalDispatcher
{
public:

    TraversalDispatcher(SequenceTreeAudioProcessor& processor,
                        NoteScheduler& scheduler,
                        AudioUIBridge& bridge);

    void pushNote(const RTNode& node, int traversalId,
                  juce::MidiBuffer& midiMessages, int sample,
                  NodeMap& nodes, TraversalMap& traversalMap);

    void handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                           int priorityNoteDuration,
                           juce::MidiBuffer& midiMessages,
                           NodeMap& nodes, TraversalMap& traversalMap);

    void resetTraversal(int graphId, int newTargetId,
                        NodeMap& nodes, TraversalMap& traversalMap);

private:

    void pushConnectorNotes(int traverserId, juce::MidiBuffer& midiMessages,
                            int sample, NodeMap& nodes, TraversalMap& traversalMap);

    void pushModulatorNotes(int modulatorRootId, juce::MidiBuffer& midiMessages,
                            int sample, NodeMap& nodes, TraversalMap& traversalMap);

    void pushRootNodeConnection(int rootNodeId, juce::MidiBuffer& midiMessages,
                                int sample, NodeMap& nodes, TraversalMap& traversalMap);

    int resolveDuration(const RTNode& node, const RTNode* nextTarget,
                        int lastTargetId, const NodeMap& nodes);

    SequenceTreeAudioProcessor& processor;
    NoteScheduler&              scheduler;
    AudioUIBridge&              bridge;
};
