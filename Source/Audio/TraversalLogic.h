#pragma once

#include "../Graph/RTData.h"
#include <unordered_map>
#include <vector>


class TraversalLogic {

public:

    struct Walker
    {
        std::unordered_map<int,int> counts;
        std::unordered_map<int,int>  switchCounts;
        std::unordered_map<int,int>  subRootCounts;

        int target = 0;
        int last   = 0;

        int subRootNode = -1;

        int alternativeTarget = -1;
        int alternativeLast   = -1;
    };

    struct Counters
    {
        std::unordered_map<int,int> chord;
        std::unordered_map<int,int> crossTree;
        std::unordered_map<int,int> crossTreeSwitch;
        std::unordered_map<int,int> trigger;
    };

    struct LoopState
    {
        bool active = false;
        int  count  = 0;
        int  limit  = 0;
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

        bool isActive() const { return gate.activeRootId != -1; }

        void activate(int rootId, int hostId)
        {
            gate.activeRootId = rootId;
            gate.hostId       = hostId;
            gate.repeatCount  = 0;
            walker.target     = rootId;
            walker.last       = -1;
        }

        void deactivate()
        {
            gate          = {};
            walker.target = -1;
            walker.last   = -1;
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

        bool          advance(const NodeMap& nodes, TraversalLogic& owner);
        const RTNode* peek   (const NodeMap& nodes, TraversalLogic& owner) const;
    };

    enum class TraversalState { Start, Active, End, Reset };

    struct StepResult
    {
        enum class Kind { None, EnteredRoot, Advanced, LoopedToRoot, Ended };

        Kind kind = Kind::None;

        int  leftId         = -1;
        int  enteredId      = -1;
        int  referenceOffId = -1;
        int  rootForReset   = -1;

        int leftAlternativeId    = -1;
        int enteredAlternativeId = -1;
        int referenceOffAlternativeId = -1;

        bool pushCounts        = false;
        int  countSourceNodeId = -1;
        int  countSourceCount  = 0;
    };

    using ChildPredicate = bool (*)(RTNode::NodeType);

    RTtraversal    traversal;
    NodeStateMap   nodeStates;

    Walker         primary;
    Counters       counters;
    LoopState      loop;
    ModulatorWalk  mod;

    int            instanceId = 0;
    int            rootId     = 0;

    TraversalState state = TraversalState::Start;

    TraversalLogic() = default;

    void reset(int root, const RTtraversal& newTraversal);

    StepResult handleNodeEvent(const NodeMap& nodes);

    void advanceAlternative(const NodeMap& nodes, int parentId);

    void advance(const NodeMap& nodes);

    int advanceModulator(const NodeMap& nodes);

    const RTNode* peekNextTarget(const NodeMap& nodes);

    void peekCrossTreeNode(const NodeMap& nodes, std::vector<int>& traverserIds);
    const RTNode* peekModulators(const NodeMap& nodes);

    const RTNode& getTargetNode(const NodeMap& nodes) const;
    const RTNode& getRootNode  (const NodeMap& nodes) const;

    int findActiveModulatorRoot(const NodeMap& nodes, int regularNodeId) const;

    static bool isDescendantOf(const NodeMap& nodes, int nodeId, int ancestorId);

    bool shouldTraverse() const;

private:

    int  selectNextChild(const NodeMap& nodes, int parentId, int parentCount, ChildPredicate isEligible);
    void selectSwitchNode(const NodeMap& nodes, int targetId, int& chosenNodeId);
    void registerTrigger(const NodeMap& nodes, int nodeId);

    const RTNode* getModulatorNode(const NodeMap& nodes, int nodeId) const;

    StepResult enterRoot(const NodeMap& nodes);
    StepResult stepActive(const NodeMap& nodes);
    void       handleLoopReset(const NodeMap& nodes, StepResult& result);
    void       advanceSubRoot(const NodeMap& nodes, StepResult& result);
    void       fillEndedResult(StepResult& result) const;

    int referenceTargetId = 0;
};
