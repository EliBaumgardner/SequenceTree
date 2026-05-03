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

    void applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes);

private:

    void pushConnectorNotes(int traverserId, juce::MidiBuffer& midiMessages,
                            int sample, NodeMap& nodes, TraversalMap& traversalMap);

    void pushModulatorNotes(int modulatorRootId, juce::MidiBuffer& midiMessages,
                            int sample, NodeMap& nodes, TraversalMap& traversalMap);

    void pushRootNodeConnection(int rootNodeId, juce::MidiBuffer& midiMessages,
                                int sample, NodeMap& nodes, TraversalMap& traversalMap);

    int resolveDuration(const RTNode& node, const RTNode* nextTarget,
                        int lastTargetId, const NodeMap& nodes);

    void dispatchModulator(const RTNode &node, NodeMap &nodes, TraversalLogic &traversalLogic, RTNode *&modulatorNode, TraversalMap &traverserMap);

    void pushChordNotes(const RTNode& node, int sample, int duration,
                        juce::MidiBuffer& midiMessages,
                        double sampleRate, double tempoMultiplier,
                        const NodeMap& nodes);

    void dispatchPrimaryArrow(const RTNode& node, const RTNode* nextTarget,
                              int traversalId, int rootId, int wallClockMs);

    void dispatchModulatorArrow(const RTNode* modulatorNode,
                                const RTNode* nextModulatorTarget,
                                int activeModulatorRootId,
                                int rootId, int wallClockMs);

    void dispatchCrossTree(const RTNode& node, int traversalId, int sample, int rootId,
                           juce::MidiBuffer& midiMessages,
                           double sampleRate, double tempoMultiplier,
                           NodeMap& nodes, TraversalLogic& traversal);

    SequenceTreeAudioProcessor& processor;
    NoteScheduler&              scheduler;
    AudioUIBridge&              bridge;
};
