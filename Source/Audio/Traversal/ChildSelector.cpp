#include "ChildSelector.h"

void ChildSelector::advanceAlternative(const NodeMap& nodes, Walker& walker, Walk walk, int parentId)
{
    const RTNode* const parentNode = nodes.find(parentId);

    const int currentAltId = nodeState.get(NodeStateSlot::ActiveAlternative, parentId);

    ChildPredicate isAlternative = &isAlternativeChild;

    NodeStateSlot countSlot = NodeStateSlot::Count;

    int count               = 0;
    int selectingId         = currentAltId;
    int activeAlternativeId = NodeStateTable::hostPlays;

    if (walk == Walk::Modulator) {
        isAlternative = &isAlternativeModulatorChild;
        countSlot     = NodeStateSlot::ModulatorCount;
    }

    if (parentNode == nullptr) {
        walker.alternativeTarget = -1;

        return;
    }

    if (parentNode->alternativeRootId == -1) {
        nodeState.set(NodeStateSlot::ActiveAlternative, parentId, -1);

        walker.alternativeTarget = -1;

        return;
    }

    if (currentAltId == -1) {
        nodeState.set(NodeStateSlot::ActiveAlternative, parentId, NodeStateTable::hostPlays);

        walker.alternativeTarget = -1;

        return;
    }

    if (currentAltId != NodeStateTable::hostPlays) {
        const RTNode* const currentAltNode = nodes.find(currentAltId);

        if (currentAltNode != nullptr) {
            const int switchCountLimit = currentAltNode->switchCountLimit;
            const int switchCount      = nodeState.increment(NodeStateSlot::SwitchCount, currentAltId);

            if (switchCount < switchCountLimit && switchCountLimit > 1) {
                walker.alternativeTarget = currentAltId;

                return;
            }

            nodeState.set(NodeStateSlot::SwitchCount, currentAltId, 0);
        }
    }

    if (currentAltId == NodeStateTable::hostPlays) {
        count       = nodeState.get(countSlot, parentId);
        selectingId = parentId;
    }
    else {
        count = nodeState.increment(countSlot, currentAltId);
    }

    const int chosen = selectNextChild(nodes, selectingId, count, isAlternative);

    if (currentAltId != NodeStateTable::hostPlays) {
        nodeState.set(NodeStateSlot::LastNode, currentAltId, chosen);
    }

    if (chosen != -1) {
        activeAlternativeId = chosen;
    }

    nodeState.set(NodeStateSlot::ActiveAlternative, parentId, activeAlternativeId);

    walker.alternativeTarget = chosen;
}

void ChildSelector::armSubLoop(Walker& walker, const RTNode& enteredNode, TraversalMode mode)
{
    const bool nodeSubLoops = enteredNode.subLoopCountLimit != 1 && mode == TraversalMode::Live;

    if (walker.subRootNode != -1) {
        return;
    }

    if (! nodeSubLoops) {
        return;
    }

    walker.subRootNode = enteredNode.nodeID;

    nodeState.set(NodeStateSlot::SubRootCount, walker.subRootNode, 0);
}

int ChildSelector::chooseChild(const NodeMap& nodes, Walker& walker, NodeStateSlot countSlot, ChildPredicate isEligible)
{
    const int targetId = walker.target;

    int chosenNodeId = -1;

    selectSwitchNode(nodes, targetId, chosenNodeId);

    if (chosenNodeId != -1) {
        return chosenNodeId;
    }

    const int count = nodeState.increment(countSlot, targetId);

    chosenNodeId = selectNextChild(nodes, targetId, count, isEligible);

    nodeState.set(NodeStateSlot::SwitchCandidate, targetId, chosenNodeId);
    nodeState.set(NodeStateSlot::LastNode, targetId, chosenNodeId);

    if (chosenNodeId == -1) {
        return -1;
    }

    const int encapsulationEntryId = encapsulationLoopTarget(nodes, walker, targetId, chosenNodeId);

    if (encapsulationEntryId != -1) {
        chosenNodeId = encapsulationEntryId;
    }

    registerTrigger(nodes, chosenNodeId);

    return chosenNodeId;
}

int ChildSelector::advanceSubRoot(const NodeMap& nodes, Walker& walker)
{
    const RTNode* const subRoot = nodes.find(walker.subRootNode);

    int subRootLimit = 0;

    const int subRootCount = nodeState.increment(NodeStateSlot::SubRootCount, walker.subRootNode);

    if (subRoot != nullptr) {
        subRootLimit = subRoot->subLoopCountLimit;
    }

    const bool subRootLoopsForever = (subRootLimit == 0);

    if (!subRootLoopsForever && subRootCount >= subRootLimit) {
        nodeState.set(NodeStateSlot::SubRootCount, walker.subRootNode, 0);

        walker.subRootNode = -1;

        return -1;
    }

    return walker.subRootNode;
}

int ChildSelector::selectNextChild(const NodeMap& nodes, int parentId, int parentCount, ChildPredicate isEligible) const
{
    const RTNode* const parentNode = nodes.find(parentId);

    if (parentNode == nullptr) {
        return -1;
    }

    const RuleContext context { nodes, *parentNode, parentCount, traversalKey, isEligible, nodeState, static_cast<int>(selectionRandom >> 1) };

    return rule->selectChild(context);
}

int ChildSelector::selectTreeJumpChild(const NodeMap& nodes, const Walker& walker, const RTNode& host) const
{
    const RTNode*       parent            = &host;
    const RTNode* const voicedAlternative = nodes.find(walker.alternativeTarget);

    if (voicedAlternative != nullptr) {
        parent = voicedAlternative;
    }

    const int parentCount = nodeState.get(NodeStateSlot::Count, parent->nodeID) + 1;

    int chosen   = -1;
    int maxLimit = 0;

    const RuleContext context { nodes, *parent, parentCount, traversalKey, &isTreeJumpChild, nodeState, static_cast<int>(selectionRandom >> 1), true };

    for (const RTConnection& connection : parent->connections) {
        const int childId = connection.childId;

        if (!connection.isTreeJump) {
            continue;
        }

        const RTNode* child = context.eligibleChild(childId);

        if (child == nullptr) {
            continue;
        }

        if (parentCount % child->countLimit == 0 && child->countLimit > maxLimit) {
            chosen   = childId;
            maxLimit = child->countLimit;
        }
    }

    return chosen;
}

bool ChildSelector::isModulatorChild(NodeInfo::Kind kind)
{
    return kind == NodeInfo::Kind::Modulator || kind == NodeInfo::Kind::ModulatorRoot;
}

bool ChildSelector::isAdvanceableChild(NodeInfo::Kind kind)
{
    return kind == NodeInfo::Kind::Node || kind == NodeInfo::Kind::Modulator || kind == NodeInfo::Kind::Root;
}

bool ChildSelector::isAudibleChild(NodeInfo::Kind kind)
{
    return kind == NodeInfo::Kind::Node || kind == NodeInfo::Kind::Root;
}

void ChildSelector::selectSwitchNode(const NodeMap& nodes, int targetId, int& chosenNodeId)
{
    if (nodeState.get(NodeStateSlot::SwitchCandidate, targetId) != -1) {
        const int switchCount = nodeState.increment(NodeStateSlot::SwitchCount, targetId);

        const RTNode* const switchEntry = nodes.find(nodeState.get(NodeStateSlot::SwitchCandidate, targetId));

        if (switchEntry != nullptr) {
            const RTNode& switchNode = *switchEntry;

            const int switchCountLimit = switchNode.switchCountLimit;

            if (switchCount < switchCountLimit && switchCountLimit > 1) {
                chosenNodeId = switchNode.nodeID;
            }
            else {
                nodeState.set(NodeStateSlot::SwitchCount, targetId, 0);
            }
        }
    }
}

void ChildSelector::registerTrigger(const NodeMap& nodes, int nodeId)
{
    const RTNode* const node = nodes.find(nodeId);

    if (node == nullptr) {
        return;
    }

    if (node->triggerLimit <= 0) {
        return;
    }

    nodeState.increment(NodeStateSlot::Trigger, nodeId);
}

int ChildSelector::encapsulationLoopTarget(const NodeMap& nodes, Walker& walker, int leavingNodeId, int chosenNodeId)
{
    const RTNode* const leavingNode = nodes.find(leavingNodeId);

    if (leavingNode == nullptr) {
        return -1;
    }

    const int entryId = leavingNode->encapsulationEntryId;

    if (entryId == -1 || walker.subRootNode != entryId) {
        return -1;
    }

    const RTNode* const chosenNode = nodes.find(chosenNodeId);

    if (chosenNode != nullptr && chosenNode->encapsulationEntryId == entryId) {
        return -1;
    }

    const RTNode* const entryNode = nodes.find(entryId);

    if (entryNode == nullptr) {
        return -1;
    }

    const int subLoopLimit = entryNode->subLoopCountLimit;
    const int subLoopCount = nodeState.increment(NodeStateSlot::SubRootCount, entryId);

    if (subLoopLimit > 0 && subLoopCount >= subLoopLimit) {
        nodeState.set(NodeStateSlot::SubRootCount, entryId, 0);

        walker.subRootNode = -1;

        return -1;
    }

    return entryId;
}

bool ChildSelector::isAlternativeChild(NodeInfo::Kind kind)
{
    return kind == NodeInfo::Kind::Alternative;
}

bool ChildSelector::isAlternativeModulatorChild(NodeInfo::Kind kind)
{
    return kind == NodeInfo::Kind::AlternativeModulator;
}

bool ChildSelector::isTreeJumpChild(NodeInfo::Kind kind)
{
    return kind == NodeInfo::Kind::Root;
}
