#pragma once

#include "FlagScheduler.h"
#include "NoteScheduler.h"
#include <cstdint>
#include <memory>
#include <atomic>

class AudioUIBridge;

class TraversalDispatcher
{
public:

    TraversalDispatcher(NoteScheduler& scheduler, AudioUIBridge& bridge);

    void pushNote(const RTNode& node, int instanceId, const DispatchContext& context,
                  double sample, bool isPrimaryRepeat = false);

    void handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                           double expiryTime,
                           const DispatchContext& context);

    TraversalPool::Instance* prepareTraversal(int instanceId, int rootId, int startNodeId,
                                              const RTtraversal& traversal, const DispatchContext& context);

    FlagScheduler flagScheduler;

private:

    void applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes, int traversalId);

    void applyTreeJump(const TraversalLogic::StepResult& step, TraversalLogic& traversal,
                       TraversalRuntime& runtime, const DispatchContext& context);

    void pushRootNodeConnection(int rootNodeId, const DispatchContext& context, double sample);

    int resolveDuration(const RTNode& node, const RTNode* nextTarget,
                        int lastTargetId, const NodeMap& nodes, int traversalId);

    void dispatchModulator(const RTNode& node, const DispatchContext& context,
                           TraversalLogic& traversalLogic, const RTNode*& modulatorNode,
                           bool isPrimaryRepeat);

    void pushChordNotes(const RTNode& node, double sample, int duration,
                        double tempoMultiplier, const DispatchContext& context, int parentCount,
                        TraversalLogic& traversalLogic, int transpose);

    void dispatchPrimaryArrow(const RTNode& node, const RTNode* nextTarget,
                              int rootId, int wallClockMs, int colourTraversalId);

    void dispatchModulatorArrow(const RTNode* modulatorNode,const RTNode* nextModulatorTarget,
                                int activeModulatorRootId, int rootId,
                                int wallClockMs, int colourTraversalId);

    void dispatchCrossTree(const RTNode& node, int sourceInstanceId, double sample, int rootId,
                           double tempoMultiplier, const DispatchContext& context,
                           TraversalLogic& traversal);

    void applyGraphLoopLimit(TraversalLogic& traversalLogic, int rootId, const DispatchContext& context);

    void startCrossTreeTraversal(const RTNode& targetRootNode, const RTtraversal& traversal,
                                 double sample, const DispatchContext& context);

    NoteScheduler&  scheduler;
    AudioUIBridge&  bridge;

    static constexpr int scratchCapacity = 256;

    bool markChordVisited(int nodeId);

    std::vector<std::uint32_t>         chordVisitStamps;
    std::uint32_t                      chordVisitToken = 0;
    std::vector<std::pair<int, int>>   chordFrontier;
    std::vector<int>                   crossTreeScratch;
};
