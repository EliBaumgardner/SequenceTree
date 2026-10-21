#pragma once

#include "ThreadMessaging/AudioUIBridge.h"
#include "FlagScheduler.h"
#include "NoteScheduler.h"
#include <cstdint>
#include <memory>
#include <atomic>

/*
    The translator from traversal steps to output. A traversal only decides which node comes next;
    this turns each node it reaches into the notes that sound and the highlights, arrow progress
    and counts the canvas shows.

    pushNote plays a node with its modulator, chord notes, traversal flags and any jump into another
    tree. handleExpiredNote runs when a traversal's note ends: it repeats the node, steps the
    traversal on and plays the next one, or removes a traversal a flag marked. As a ScriptHost it
    lets a traversal script step the walk and choose the note to play.

    Each EventManager owns one, and it writes into that NoteScheduler and the AudioUIBridge on the
    audio thread. maxDispatchDepth caps how far chords and jumps can set each other off.
*/

class TraversalDispatcher : public ScriptHost
{
public:

    TraversalDispatcher(NoteScheduler& scheduler, AudioUIBridge& bridge);

    int  advance     (int steps) override;
    int  noteDuration(int nodeId) override;
    void playNote    (const ScriptNote& note) override;

    void pushNote         (const RTNode& node, int runId, const DispatchContext& context, double sample, bool isPrimaryRepeat = false);
    void handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote, double expiryTime, const DispatchContext& context);

    TraversalPool::Instance* prepareTraversal(int runId, int rootId, int startNodeId, const RTtraversal& traversal, const DispatchContext& context);

    FlagScheduler flagScheduler;

private:

    struct NoteDispatch;

    void applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes, int runId, int typeId);
    void applyTreeJump  (const TraversalLogic::StepResult& step, TraversalLogic& traversal, TraversalRuntime& runtime, const DispatchContext& context);
    void stepTraversal  (TraversalPool::Instance& instance, int runId, const DispatchContext& context, double expiryTime);

    void dispatchModulator(const RTNode& node, int runId, const DispatchContext& context, TraversalLogic& traversalLogic, const RTNode*& modulatorNode,
                           bool isPrimaryRepeat);

    void pushChordNotes(const NoteDispatch& dispatch, const DispatchContext& context, int parentCount, int transpose);

    void dispatchPrimaryArrow  (const NoteDispatch& dispatch, const RTNode& arrowNode, const RTNode* voicedAlternative, const RTNode* nextTarget, int danglingIndex);
    void dispatchModulatorArrow(const NoteDispatch& dispatch, const RTNode* modulatorNode, const RTNode* nextModulatorTarget, int danglingIndex);
    void dispatchCrossTree     (const NoteDispatch& dispatch, const DispatchContext& context);

    void startCrossTreeTraversal(const RTNode& targetRootNode, const RTtraversal& traversal, double sample, const DispatchContext& context);
    void pushRootNodeConnection (int rootNodeId, const DispatchContext& context, double sample);

    int resolveDuration(const RTNode& node, const RTNode* nextTarget, int lastTargetId, const NodeMap& nodes, int danglingIndex);

    bool markChordVisited(int nodeId);

    struct NoteDispatch
    {
        const RTNode&   node;
        TraversalLogic& traversal;

        int runId       = -1;
        int duration    = 0;
        int wallClockMs = 0;
        int typeId      = -1;

        double sample          = 0.0;
        double tempoMultiplier = 1.0;

        BridgeCommands::TrailSource source = BridgeCommands::TrailSource::Live;
    };

    struct MainRun
    {
        TraversalPool::Instance* instance = nullptr;
        const DispatchContext*   context  = nullptr;
        int                      runId    = -1;
        ScriptNote               note;
    };

    static constexpr int maxDispatchDepth = 32;

    NoteScheduler& scheduler;
    AudioUIBridge& bridge;

    int dispatchDepth = 0;

    MainRun mainRun;

    NodeRowMap                       chordVisits;
    std::vector<std::pair<int, int>> chordFrontier;

    std::vector<int> crossTreeScratch;
};
