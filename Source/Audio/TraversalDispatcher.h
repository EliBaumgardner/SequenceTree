#pragma once

#include "TraversalLogic.h"
#include "NoteScheduler.h"
#include <memory>
#include <atomic>
#include <unordered_set>

class AudioUIBridge;
class SequenceTreeAudioProcessor;

class TraversalDispatcher
{
public:

    TraversalDispatcher(SequenceTreeAudioProcessor& processor,
                        NoteScheduler& scheduler,
                        AudioUIBridge& bridge);

    void pushNote(const RTNode& node, int instanceId,
                  juce::MidiBuffer& midiMessages, int sample,
                  const NodeMap& nodes, TraversalMap& traversalMap,
                  bool isPrimaryRepeat = false);

    void handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                           int priorityNoteDuration,
                           juce::MidiBuffer& midiMessages,
                           const NodeMap& nodes, TraversalMap& traversalMap);

    void resetTraversal(int graphId, int newTargetId,
                        const NodeMap& nodes, TraversalMap& traversalMap);

    void applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes, int traversalId);

private:

    void pushModulatorNotes(int modulatorRootId, juce::MidiBuffer& midiMessages,
                            int sample, const NodeMap& nodes, TraversalMap& traversalMap);

    void pushRootNodeConnection(int rootNodeId, juce::MidiBuffer& midiMessages,
                                int sample, const NodeMap& nodes, TraversalMap& traversalMap);

    int resolveDuration(const RTNode& node, const RTNode* nextTarget,
                        int lastTargetId, const NodeMap& nodes, int traversalId);

    void dispatchModulator(const RTNode &node, const NodeMap& nodes, TraversalLogic &traversalLogic, const RTNode*& modulatorNode, TraversalMap &traverserMap, bool isPrimaryRepeat);

    void pushChordNotes(const RTNode& node, int sample, int duration,
                        juce::MidiBuffer& midiMessages,
                        double sampleRate, double tempoMultiplier,
                        const NodeMap& nodes, int parentCount,
                        TraversalLogic& traversalLogic);

    void dispatchPrimaryArrow(const RTNode& node, const RTNode* nextTarget,
                              int rootId, int wallClockMs, int colourTraversalId);

    void dispatchModulatorArrow(const RTNode* modulatorNode,
                                const RTNode* nextModulatorTarget,
                                int activeModulatorRootId,
                                int rootId, int wallClockMs, int colourTraversalId);

    void dispatchCrossTree(const RTNode& node, int sourceInstanceId, int sample, int rootId,
                           juce::MidiBuffer& midiMessages,
                           double sampleRate, double tempoMultiplier,
                           const NodeMap& nodes, TraversalLogic& traversal, TraversalMap& traversalMap);

    bool hasActiveTraversalOnTree(int treeRootId, const TraversalMap& traversalMap) const;

    void startCrossTreeTraversal(const RTNode& targetRootNode, const RTtraversal& traversal,
                                 int sample, juce::MidiBuffer& midiMessages,
                                 const NodeMap& nodes, TraversalMap& traversalMap);

    void dispatchFlag(const RTNode& node, int hostInstanceId, int hostTypeId, int parentCount, int sample, juce::MidiBuffer& midiMessages,
                      const NodeMap& nodes, TraversalMap& traversalMap);

    void startFlagTraversal(const RTNode& flagNode, int hostTypeId, int sample, juce::MidiBuffer& midiMessages,
                            const NodeMap& nodes, TraversalMap& traversalMap);

    void queueFlagRemoval(const RTNode& flagNode, int hostInstanceId, int hostTypeId, TraversalMap& traversalMap);

    SequenceTreeAudioProcessor& processor;
    NoteScheduler&              scheduler;
    AudioUIBridge&              bridge;
};
