#pragma once

#include "../../Graph/RTData.h"
#include "ScriptRun.h"
#include "TraversalRule.h"
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
    the same rules and keep separate counts in nodeState.

    The next node comes from the script when one is set, and from rule otherwise. peekNextTarget and
    peekCrossTreeNode ask where the walk would go next without moving it. Mode::Preview is the note
    preview, started by beginPreview.

    TraversalPool holds one in each slot, and TraversalSession starts and stops them. It runs on the
    audio thread and never allocates. peekCrossTreeNode fills a vector TraversalDispatcher reserved.
*/

class TraversalLogic {

public:

    struct StepResult;
    enum class Walk;

    constexpr TraversalLogic() = default;

    void reset       (int root, const RTtraversal& newTraversal);
    void begin       (const NodeMap& nodes, int startNodeId, int graphLoopLimit);
    void beginPreview(const NodeMap& nodes, int startNodeId, int alternativeId);

    void advance           (const NodeMap& nodes, int steps = 1);
    void advanceAlternative(const NodeMap& nodes, int parentId);

    void peekCrossTreeNode(const NodeMap& nodes, std::vector<int>& traverserIds);

    StepResult handleNodeEvent(const NodeMap& nodes, int steps = 1);

    ScriptRunContext makeScriptContext(const NodeMap& nodes, Walk walk, ScriptWrites writes, ScriptHost* host);

    const RTNode* peekNextTarget     (const NodeMap& nodes);
    const RTNode* decideNextModulator(const NodeMap& nodes);

    int findActiveModulatorRoot(const NodeMap& nodes, int regularNodeId) const;

    bool shouldTraverse() const;

    static bool isDescendantOf(const NodeMap& nodes, int nodeId, int ancestorId);

    struct Walker
    {
        int target = 0;
        int last   = 0;

        int subRootNode = -1;

        int alternativeTarget = -1;
        int alternativeLast   = -1;
    };

    struct LoopState
    {
        bool active   = false;
        int  count    = 0;
        int  limit    = 0;
        int  returnId = -1;
    };

    struct ModulatorGate
    {
        int activeRootId = -1;
        int hostId       = -1;
        int repeatCount  = 0;
    };

    struct ModulatorWalk
    {
        Walker        walker;
        ModulatorGate gate;

        int  decidedTarget  = -1;
        bool decidedRestart = false;

        constexpr bool isActive() const
        {
            return gate.activeRootId != -1;
        }

        void activate(int rootId, int hostId)
        {
            gate.activeRootId        = rootId;
            gate.hostId              = hostId;
            gate.repeatCount         = 0;
            walker.target            = rootId;
            walker.last              = -1;
            walker.subRootNode       = -1;
            walker.alternativeTarget = -1;
            walker.alternativeLast   = -1;
            decidedTarget            = -1;
            decidedRestart           = false;
        }

        void deactivate()
        {
            gate                     = {};
            walker.target            = -1;
            walker.last              = -1;
            walker.subRootNode       = -1;
            walker.alternativeTarget = -1;
            walker.alternativeLast   = -1;
            decidedTarget            = -1;
            decidedRestart           = false;
        }

        bool tickRepeat(int repeatLimit)
        {
            gate.repeatCount++;
            if (gate.repeatCount >= repeatLimit) {
                gate.repeatCount = 0;
                return true;
            }
            return false;
        }

        void decide(const NodeMap& nodes, TraversalLogic& owner);
        bool step  ();
    };

    enum class TraversalState { Active, End, Reset, Jump };

    enum class Mode { Live, Preview };

    enum class Walk { Primary, Modulator };

    struct StepResult
    {
        enum class Kind { None, Advanced, LoopedToRoot, JumpedToTree, Ended };

        Kind kind = Kind::None;

        int  leftId         = -1;
        int  enteredId      = -1;
        int  referenceOffId = -1;
        bool clearTrail     = false;

        int  jumpedFromRootId = -1;

        int leftAlternativeId         = -1;
        int enteredAlternativeId      = -1;
        int referenceOffAlternativeId = -1;

        bool pushCounts        = false;
        int  countSourceNodeId = -1;
        int  countSourceCount  = 0;
    };

    using ChildPredicate = ::ChildPredicate;

    RTtraversal    traversal;
    NodeStateTable nodeState;

    Walker        primary;
    LoopState     loop;
    ModulatorWalk mod;

    int rootId = 0;

    TraversalState state = TraversalState::End;

    Mode mode              = Mode::Live;
    int  previewVisitCount = 1;

    unsigned int selectionRandom = 1;

    const TraversalRule* rule = &NativeTraversalRule::instance();

    static constexpr int maxAdvanceSteps     = 64;
    static constexpr int maxCrossTreeTargets = NodeStateTable::maxNodeIds;

    const RTScript* script        = nullptr;
    const RTScript* membersScript = nullptr;
    ScriptMembers   scriptMembers {};
    ScriptGlobals*  sharedGlobals = nullptr;

private:

    StepResult stepActive(const NodeMap& nodes, int steps);

    void handleLoopReset(const NodeMap& nodes, StepResult& result);
    void handleTreeJump (const NodeMap& nodes, StepResult& result);
    void fillEndedResult(StepResult& result) const;

    void advanceByScript(const NodeMap& nodes, const RTNode& leavingNode, int steps, TraversalState deadEndState);
    void skipAhead      (const NodeMap& nodes, int steps);

    void selectSwitchNode(const NodeMap& nodes, int targetId, int& chosenNodeId);
    void registerTrigger (const NodeMap& nodes, int nodeId);
    void armSubLoop      (Walker& walker, const RTNode& enteredNode);

    int selectNextChild    (const NodeMap& nodes, int parentId, int parentCount, ChildPredicate isEligible) const;
    int selectTreeJumpChild(const NodeMap& nodes, const RTNode& host) const;
    int chooseChild        (const NodeMap& nodes, Walker& walker, NodeStateSlot countSlot, ChildPredicate isEligible);

    int encapsulationLoopTarget(const NodeMap& nodes, Walker& walker, int leavingNodeId, int chosenNodeId);
    int advanceSubRoot         (const NodeMap& nodes, Walker& walker);

    const RTNode* eligibleModulatorRoot(const NodeMap& nodes, const RTConnection& connection, int hostCount) const;

    int referenceTargetId   = 0;
    int pendingJumpTargetId = -1;
};
