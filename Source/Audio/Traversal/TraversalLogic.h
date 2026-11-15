#pragma once

#include "../../Graph/RTData.h"
#include "ChildSelector.h"
#include "ModulatorWalk.h"
#include "ScriptRun.h"
#include <unordered_map>
#include <vector>

/*
    A single traversal: the state of one walk through the graph and the step that moves it from
    node to node. The state is the root it started from, the node it is on and its counts; the step
    decides which node comes next each time the current note ends. It only decides the path.
    TraversalDispatcher turns that into MIDI and canvas updates, and since it uses no JUCE, the
    tests can run it on its own.

    reset and begin start the walk at a root. Each handleNodeEvent moves it forward and returns a
    StepResult: the node it left, the node it entered, and whether it went back to the root, jumped
    to another tree or ended. The note walk (primary) and the modulator walk under it (mod) follow
    the same rules and keep separate counts in childSelector's nodeState.

    The next node comes from the script when one is set, and from childSelector otherwise. peekNextTarget
    asks where the walk would go next without moving it. advanceCrossTreeCounts picks the other
    trees' roots this step jumps to and advances each one's CrossTree and CrossTreeSwitch counts, the
    only place they advance. Mode::Preview is the note preview, started by beginPreview.

    TraversalPool holds one in each slot, and TraversalSession starts and stops them. It runs on the
    audio thread and never allocates. advanceCrossTreeCounts fills a vector TraversalDispatcher reserved.
*/

class TraversalLogic {
public:

    struct StepResult;

    constexpr TraversalLogic() = default;

    void reset       (int root, const RTtraversal& newTraversal);
    void begin       (const NodeMap& nodes, int startNodeId, int graphLoopLimit);
    void beginPreview(const NodeMap& nodes, int startNodeId, int alternativeId);

    void advance(const NodeMap& nodes, int steps = 1);

    void advanceCrossTreeCounts(const NodeMap& nodes, std::vector<int>& traverserIds);

    StepResult handleNodeEvent(const NodeMap& nodes, int steps = 1);

    ScriptRunContext makeScriptContext(const NodeMap& nodes, Walk walk, ScriptWrites writes, ScriptHost* host);

    const RTNode* peekNextTarget     (const NodeMap& nodes);
    const RTNode* decideNextModulator(const NodeMap& nodes);

    bool shouldTraverse() const;

    struct LoopState
    {
        bool active = false;

        int count    = 0;
        int limit    = 0;
        int returnId = -1;
    };

    enum class TraversalState { Active, End, Reset, Jump };

    struct StepResult
    {
        enum class Kind { None, Advanced, LoopedToRoot, JumpedToTree, Ended };

        Kind kind = Kind::None;

        int leftId         = -1;
        int enteredId      = -1;
        int referenceOffId = -1;

        bool clearTrail = false;

        int jumpedFromRootId          = -1;
        int leftAlternativeId         = -1;
        int enteredAlternativeId      = -1;
        int referenceOffAlternativeId = -1;

        bool pushCounts = false;

        int countSourceNodeId = -1;
        int countSourceCount  = 0;
    };

    RTtraversal   traversal;
    ChildSelector childSelector;

    Walker primary;

    LoopState loop;

    ModulatorWalk mod;

    int rootId = 0;

    TraversalState state = TraversalState::End;

    TraversalMode mode = TraversalMode::Live;

    int previewVisitCount = 1;

    static constexpr int maxAdvanceSteps     = 64;
    static constexpr int maxCrossTreeTargets = NodeStateTable::maxNodeIds;
    static constexpr int xorshiftFirstShift  = 13;
    static constexpr int xorshiftSecondShift = 17;
    static constexpr int xorshiftThirdShift  = 5;

    static constexpr unsigned int seedRootMultiplier     = 2654435761u;
    static constexpr unsigned int seedTypeMultiplier     = 40503u;
    static constexpr unsigned int seedInstanceMultiplier = 2246822519u;
    static constexpr unsigned int seedOffset             = 2463534242u;

    const RTScript* script        = nullptr;
    const RTScript* membersScript = nullptr;

    ScriptMembers scriptMembers {};

    ScriptGlobals* sharedGlobals = nullptr;

private:

    StepResult stepActive(const NodeMap& nodes, int steps);

    void handleLoopReset(const NodeMap& nodes, StepResult& result);
    void handleTreeJump (const NodeMap& nodes, StepResult& result);
    void fillEndedResult(StepResult& result) const;

    void advanceByScript(const NodeMap& nodes, const RTNode& leavingNode, int steps, TraversalState deadEndState);
    void skipAhead      (const NodeMap& nodes, int steps);

    int referenceTargetId   = -1;
    int pendingJumpTargetId = -1;
};
