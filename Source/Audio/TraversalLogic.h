#pragma once

#include "../Graph/RTData.h"
#include <unordered_map>
#include <vector>

class AudioUIBridge;

class TraversalLogic {
public:

    struct Walker
    {
        std::unordered_map<int, int> counts;
        int target = 0;
        int last   = 0;
    };

    Walker primary;
    Walker modulator;

    int activeModulatorRootId = -1;

    bool isFirstEvent = false;
    bool isLooping    = false;
    int  rootId            = 0;
    int  referenceTargetId = 0;
    int  loopCount         = 0;  // incremented each time the traversal resets to root
    int  loopLimit         = 0;  // 0 = loop infinitely; N > 0 = stop after N full loops

    enum class TraversalState { Start, Active, End, Reset };
    enum class EventType      { Node, Connector, Modulator };

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

        bool pushCounts        = false;
        int  countSourceNodeId = -1;
        int  countSourceCount  = 0;
    };

    TraversalLogic(int root, AudioUIBridge& bridge);

    StepResult handleNodeEvent(NodeMap& nodes);

    void advance(NodeMap& nodes);
    void advanceModulator(NodeMap& nodes);
    RTNode* peekNextTarget(NodeMap& nodes);

    using ChildPredicate = bool (*)(RTNode::NodeType);
    static int selectNextChild(const NodeMap& nodes, int parentId, int parentCount,
                               ChildPredicate isEligible);
    std::vector<int> peekCrossTreeNode(NodeMap& nodes);
    RTNode* peekModulators(NodeMap& nodes);

    const RTNode& getTargetNode   (const NodeMap& nodes) const;
    const RTNode& getLastNode     (const NodeMap& nodes) const;
    const RTNode& getReferenceNode(const NodeMap& nodes) const;
    const RTNode& getRootNode     (const NodeMap& nodes) const;
    static const RTNode& getRelayNode(int relayNodeId, const NodeMap& nodes);

    RTNode* getModulatorNode(NodeMap& nodes, int nodeId);
    int     findActiveModulatorRoot(NodeMap& nodes, int regularNodeId);

    bool shouldTraverse() const;

    void resetCounts() { primary.counts.clear(); }
};

using TraversalMap = std::unordered_map<int, TraversalLogic>;