#pragma once

#include "AudioUIBridge.h"
#include "FlagScheduler.h"
#include "NoteScheduler.h"
#include <cstdint>
#include <memory>
#include <atomic>

class TraversalDispatcher : public ScriptHost
{
public:

    TraversalDispatcher(NoteScheduler& scheduler, AudioUIBridge& bridge);

    int  advance(int steps) override;
    void playNote(const ScriptNote& note) override;
    int  noteDuration(int nodeId) override;

    void pushNote(const RTNode& node, int runId, const DispatchContext& context,
                  double sample, bool isPrimaryRepeat = false);

    void handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                           double expiryTime,
                           const DispatchContext& context);

    TraversalPool::Instance* prepareTraversal(int runId, int rootId, int startNodeId,
                                              const RTtraversal& traversal, const DispatchContext& context);

    FlagScheduler flagScheduler;

private:

    struct MainRun
    {
        TraversalPool::Instance* instance = nullptr;
        const DispatchContext*   context  = nullptr;
        int                      runId    = -1;
        ScriptNote               note;
    };

    void applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes,
                         int runId, int typeId);

    void applyTreeJump(const TraversalLogic::StepResult& step, TraversalLogic& traversal,
                       TraversalRuntime& runtime, const DispatchContext& context);

    int resolveDuration(const RTNode& node, const RTNode* nextTarget,
                        int lastTargetId, const NodeMap& nodes, int danglingIndex);

    void dispatchModulator(const RTNode& node, int runId, const DispatchContext& context,
                           TraversalLogic& traversalLogic, const RTNode*& modulatorNode,
                           bool isPrimaryRepeat);

    void pushChordNotes(const RTNode& node, int runId, double sample, int duration,
                        double tempoMultiplier, const DispatchContext& context, int parentCount,
                        TraversalLogic& traversalLogic, int transpose);

    bool markChordVisited(int nodeId);

    void dispatchPrimaryArrow(const RTNode& node, const RTNode* voicedAlternative,
                              const RTNode* nextTarget, int danglingIndex,
                              int runId, int wallClockMs, int colourTypeId, TrailSource source);

    void dispatchModulatorArrow(const RTNode* modulatorNode, const RTNode* nextModulatorTarget,
                                int danglingIndex, int runId, int wallClockMs, int colourTypeId,
                                TrailSource source);

    void dispatchCrossTree(const RTNode& node, int sourceRunId, double sample,
                           double tempoMultiplier, const DispatchContext& context,
                           TraversalLogic& traversal, TrailSource source);

    void pushRootNodeConnection(int rootNodeId, const DispatchContext& context, double sample);

    void startCrossTreeTraversal(const RTNode& targetRootNode, const RTtraversal& traversal,
                                 double sample, const DispatchContext& context);

    void stepTraversal(TraversalPool::Instance& instance, int runId, const DispatchContext& context,
                       double expiryTime);

    static constexpr int maxDispatchDepth = 32;

    NoteScheduler&  scheduler;
    AudioUIBridge&  bridge;

    int                                dispatchDepth = 0;

    MainRun                            mainRun;

    NodeRowMap                         chordVisits;
    std::vector<std::pair<int, int>>   chordFrontier;
    std::vector<int>                   crossTreeScratch;
};
