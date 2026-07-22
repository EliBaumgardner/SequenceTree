#pragma once

#include "TraversalPool.h"
#include "NoteScheduler.h"
#include <memory>
#include <atomic>
#include <unordered_set>

class AudioUIBridge;
class SequenceTreeAudioProcessor;

struct DispatchContext
{
    const NodeMap&    nodes;
    TraversalMap&     traversalMap;
    juce::MidiBuffer& midiMessages;
};

class TraversalDispatcher
{
public:

    TraversalDispatcher(SequenceTreeAudioProcessor& processor,
                        NoteScheduler& scheduler,
                        AudioUIBridge& bridge);

    void pushNote(const RTNode& node, int instanceId, const DispatchContext& context,
                  int sample, bool isPrimaryRepeat = false);

    void handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                           int priorityNoteDuration,
                           const DispatchContext& context);

    void applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes, int traversalId);

private:

    void pushRootNodeConnection(int rootNodeId, const DispatchContext& context, int sample);

    int resolveDuration(const RTNode& node, const RTNode* nextTarget,
                        int lastTargetId, const NodeMap& nodes, int traversalId);

    void dispatchModulator(const RTNode& node, const DispatchContext& context,
                           TraversalLogic& traversalLogic, const RTNode*& modulatorNode,
                           bool isPrimaryRepeat);

    void pushChordNotes(const RTNode& node, int sample, int duration,
                        double sampleRate, double tempoMultiplier,
                        const DispatchContext& context, int parentCount,
                        TraversalLogic& traversalLogic);

    void dispatchPrimaryArrow(const RTNode& node, const RTNode* nextTarget,
                              int rootId, int wallClockMs, int colourTraversalId);

    void dispatchModulatorArrow(const RTNode* modulatorNode,const RTNode* nextModulatorTarget,
                                int activeModulatorRootId, int rootId,
                                int wallClockMs, int colourTraversalId);

    void dispatchCrossTree(const RTNode& node, int sourceInstanceId, int sample, int rootId,
                           double sampleRate, double tempoMultiplier,
                           const DispatchContext& context, TraversalLogic& traversal);

    bool hasActiveTraversalOnTree(int treeRootId, const TraversalMap& traversalMap) const;

    int findTraversalInstance(int rootId, int typeId, const TraversalMap& traversalMap) const;

    void applyGraphLoopLimit(TraversalLogic& traversalLogic, int rootId);

    TraversalLogic* prepareTraversal(int instanceId, int rootId, int startNodeId,
                                     const RTtraversal& traversal, const DispatchContext& context);

    void startCrossTreeTraversal(const RTNode& targetRootNode, const RTtraversal& traversal,
                                 int sample, const DispatchContext& context);

    void dispatchFlag(const RTNode& node, int hostInstanceId, int hostTypeId,
                      int parentCount, int sample, const DispatchContext& context);

    void startFlagTraversal(const RTNode& flagNode, int hostTypeId, int sample,
                            const DispatchContext& context);

    void queueFlagRemoval(const RTNode& flagNode, int hostInstanceId, int hostTypeId, TraversalMap& traversalMap);

    SequenceTreeAudioProcessor& processor;
    NoteScheduler&              scheduler;
    AudioUIBridge&              bridge;

    static constexpr int scratchCapacity = 256;

    std::unordered_set<int>            chordVisited;
    std::vector<std::pair<int, int>>   chordFrontier;
    std::vector<int>                   crossTreeScratch;
};
