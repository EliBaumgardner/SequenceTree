#pragma once

#include "../../Graph/RTData.h"
#include "NodeStateTable.h"
#include "TraversalRule.h"

struct Walker
{
    int target            = -1;
    int last              = -1;
    int subRootNode       = -1;
    int alternativeTarget = -1;
    int alternativeLast   = -1;
};

enum class TraversalMode { Live, Preview };

enum class Walk { Primary, Modulator };

class ChildSelector
{
public:

    void advanceAlternative(const NodeMap& nodes, Walker& walker, Walk walk, int parentId);
    void armSubLoop        (Walker& walker, const RTNode& enteredNode, TraversalMode mode);

    int chooseChild        (const NodeMap& nodes, Walker& walker, NodeStateSlot countSlot, ChildPredicate isEligible);
    int advanceSubRoot     (const NodeMap& nodes, Walker& walker);
    int selectNextChild    (const NodeMap& nodes, int parentId, int parentCount, ChildPredicate isEligible) const;
    int selectTreeJumpChild(const NodeMap& nodes, const Walker& walker, const RTNode& host) const;

    static const RTNode* subLoopEntryFor(const NodeMap& nodes, const Walker& walker, int leavingNodeId, int chosenNodeId);

    static bool isModulatorChild  (NodeInfo::Kind kind);
    static bool isAdvanceableChild(NodeInfo::Kind kind);
    static bool isAudibleChild    (NodeInfo::Kind kind);

    NodeStateTable nodeState;

    TraversalKey traversalKey;

    unsigned int selectionRandom = 1;

    const TraversalRule* rule = &NativeTraversalRule::instance();

private:

    void selectSwitchNode(const NodeMap& nodes, int targetId, int& chosenNodeId);
    void registerTrigger (const NodeMap& nodes, int nodeId);

    int encapsulationLoopTarget(const NodeMap& nodes, Walker& walker, int leavingNodeId, int chosenNodeId);

    static bool isAlternativeChild         (NodeInfo::Kind kind);
    static bool isAlternativeModulatorChild(NodeInfo::Kind kind);
    static bool isTreeJumpChild            (NodeInfo::Kind kind);
};
