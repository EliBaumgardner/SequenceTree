#pragma once

#include "../Graph/RTData.h"
#include <unordered_map>
#include <vector>


class AudioUIBridge;

class TraversalLogic {

public:

    RTtraversal traversal;

    struct Walker
    {
        std::unordered_map<int, int> counts;
        std::unordered_map<int,int>  switchCounts;
        std::unordered_map<int,int>  subRootCounts;

        int target = 0;
        int last   = 0;

        int subRootNode = -1;

        int alternativeTarget = -1;
        int alternativeLast   = -1;
    };

    Walker primary;
    Walker modulator;

    NodeStateMap nodeStates;

    std::unordered_map<int,int> chordCounts;
    std::unordered_map<int,int> crossTreeCounts;
    std::unordered_map<int,int> crossTreeSwitchCounts;
    std::unordered_map<int,int> triggerCounts;

    int activeModulatorRootId = -1;
    int modulatorHostId       = -1;

    bool isFirstEvent  = false;
    bool isLooping     = false;
    bool isFlagSpawned = false;
    bool pendingRemoval = false;

    int  flagSourceNodeId = -1;

    int  rootId            = 0;
    int  referenceTargetId = 0;

    int  loopCount            = 0;
    int  loopLimit            = 0;
    int  repeatCount          = 0;
    int  modulatorRepeatCount = 0;

    enum class TraversalState { Start, Active, End, Reset };
    enum class EventType      { Node, Modulator,  };

    TraversalState state = TraversalState::Start;

    AudioUIBridge* bridge;

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

    TraversalLogic(int root, AudioUIBridge& bridge, RTtraversal traversal);

    StepResult handleNodeEvent(const NodeMap& nodes);

    void advanceAlternative(const NodeMap& nodes, int parentId);

    void selectSwitchNode(const NodeMap& nodes, int targetId, int& chosenNodeId);

    void advance(const NodeMap& nodes);
    void advanceModulator(const NodeMap& nodes);
    int advanceAlternativeNode(const NodeMap& nodes);

    const RTNode* peekNextTarget(const NodeMap& nodes);

    using ChildPredicate = bool (*)(RTNode::NodeType);
    int selectNextChild(const NodeMap& nodes, int parentId, int parentCount, ChildPredicate isEligible);

    void registerTrigger(const NodeMap& nodes, int nodeId);

    std::vector<int> peekCrossTreeNode(const NodeMap& nodes);
    const RTNode* peekModulators(const NodeMap& nodes);

    const RTNode& getTargetNode   (const NodeMap& nodes) const;
    const RTNode& getLastNode     (const NodeMap& nodes) const;
    const RTNode& getReferenceNode(const NodeMap& nodes) const;
    const RTNode& getRootNode     (const NodeMap& nodes) const;
    static const RTNode& getRelayNode(int relayNodeId, const NodeMap& nodes);

    const RTNode* getModulatorNode(const NodeMap& nodes, int nodeId);
    int           findActiveModulatorRoot(const NodeMap& nodes, int regularNodeId);

    static bool isDescendantOf(const NodeMap& nodes, int nodeId, int ancestorId);

    bool shouldTraverse() const;

    void resetCounts() {
        primary.counts.clear();
        chordCounts.clear();
        nodeStates.clear();
    }
};

using TraversalMap = std::unordered_map<int, TraversalLogic>;