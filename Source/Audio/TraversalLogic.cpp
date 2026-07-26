#include "TraversalLogic.h"

namespace {

bool isModulatorChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Modulator || t == RTNode::NodeType::ModulatorRoot;
}

bool isAlternativeChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Alternative;
}

bool isAdvanceableChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Node || t == RTNode::NodeType::Modulator;
}

bool isAudibleChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Node;
}

bool isChildEligible(const RTNode& parent, const RTNode& child, int childId,
                     TraversalLogic::ChildPredicate isEligible,
                     const std::unordered_map<int,int>& triggerCounts,
                     int traversalId)
{
    if (!isEligible(child.nodeType)) {
        return false;
    }

    if (child.countLimit <= 0) {
        return false;
    }

    if (child.triggerLimit > 0) {
        const auto triggerIt = triggerCounts.find(childId);
        const int  triggered = (triggerIt != triggerCounts.end()) ? triggerIt->second : 0;

        if (triggered >= child.triggerLimit) {
            return false;
        }
    }

    const auto durIt = parent.durationMap.find(childId);
    if (durIt != parent.durationMap.end() && durIt->second == 0) {
        return false;
    }

    const auto disabledIt = parent.disabledTraversalsByChild.find(childId);
    if (disabledIt != parent.disabledTraversalsByChild.end()
        && disabledIt->second.count(traversalId) > 0) {
        return false;
    }

    return true;
}

int selectEligibleChild(const NodeMap& nodes, const RTNode& parent, int parentCount,
                        TraversalLogic::ChildPredicate isEligible,
                        const std::unordered_map<int,int>& triggerCounts,
                        int traversalId)
{
    int chosen   = -1;
    int maxLimit = 0;

    for (const int childId : parent.children) {
        const auto childIt = nodes.find(childId);
        if (childIt == nodes.end()) {
            continue;
        }

        const RTNode& child = childIt->second;

        if (!isChildEligible(parent, child, childId, isEligible, triggerCounts, traversalId)) {
            continue;
        }

        if (parentCount % child.countLimit == 0 && child.countLimit > maxLimit) {
            chosen   = childId;
            maxLimit = child.countLimit;
        }
    }

    return chosen;
}

}

static void resetWalker(TraversalLogic::Walker& walker)
{
    walker.counts.clear();
    walker.switchCounts.clear();
    walker.subRootCounts.clear();

    walker.target = 0;
    walker.last   = 0;

    walker.subRootNode = -1;

    walker.alternativeTarget = -1;
    walker.alternativeLast   = -1;
}

void TraversalLogic::reset(int root, const RTtraversal& newTraversal)
{
    resetWalker(primary);

    nodeStates.clear();

    counters.chord.clear();
    counters.crossTree.clear();
    counters.crossTreeSwitch.clear();
    counters.trigger.clear();

    traversal = newTraversal;

    mod  = {};
    loop = {};

    instanceId        = 0;
    rootId            = root;
    referenceTargetId = 0;

    state = TraversalState::Start;
}

int TraversalLogic::selectNextChild(const NodeMap& nodes, int parentId, int parentCount,
                                    ChildPredicate isEligible)
{
    const auto parentIt = nodes.find(parentId);
    if (parentIt == nodes.end()) {
        return -1;
    }

    const int chosen = selectEligibleChild(nodes, parentIt->second, parentCount, isEligible,
                                     counters.trigger, traversal.traversalId);

    nodeStates[parentId].lastNodeId = chosen;
    return chosen;
}

void TraversalLogic::registerTrigger(const NodeMap& nodes, int nodeId)
{
    const auto nodeIterator = nodes.find(nodeId);

    if (nodeIterator == nodes.end()) {
        return;
    }

    if (nodeIterator->second.triggerLimit <= 0) {
        return;
    }

    counters.trigger[nodeId] = counters.trigger[nodeId] + 1;
}

bool TraversalLogic::ModulatorWalk::advance(const NodeMap& nodes, TraversalLogic& owner)
{
    if (gate.activeRootId == -1 || walker.target == -1) {
        return false;
    }

    const auto targetIt = nodes.find(walker.target);
    if (targetIt == nodes.end()) {
        return false;
    }

    walker.last = walker.target;
    const int count   = ++walker.counts[walker.target];

    const int chosen = owner.selectNextChild(nodes, walker.target, count, &isModulatorChild);

    if (chosen == -1) {
        bool hasModulatorChild = false;
        for (const int childId : targetIt->second.children) {
            const auto childIt = nodes.find(childId);
            if (childIt != nodes.end()
                && isModulatorChild(childIt->second.nodeType)
                && childIt->second.countLimit > 0) {
                hasModulatorChild = true;
                break;
            }
        }

        if (hasModulatorChild) {
            walker.target = walker.last;
            return false;
        }

        walker.target = gate.activeRootId;
        return true;
    }

    walker.target = chosen;
    return false;
}

int TraversalLogic::advanceModulator(const NodeMap& nodes)
{
    const int activeRootId = mod.gate.activeRootId;

    return mod.advance(nodes, *this) ? activeRootId : -1;
}

void TraversalLogic::advanceAlternative(const NodeMap& nodes,int parentId) {
    const auto parentIt = nodes.find(parentId);
    if (parentIt == nodes.end()) {
        primary.alternativeTarget = -1;
        return;
    }

    const RTNode& parent = parentIt->second;

    if (parent.alternativeRootId == -1) {
        nodeStates[parentId].activeAlternativeId = -1;
        primary.alternativeTarget  = -1;
        return;
    }

    const int currentAltId = nodeStates[parentId].activeAlternativeId;

    if (currentAltId == -1) {
        nodeStates[parentId].activeAlternativeId = parentId;
        primary.alternativeTarget  = -1;
        return;
    }

    if (currentAltId != parentId) {
        const auto currentAltIt = nodes.find(currentAltId);

        if (currentAltIt != nodes.end()) {
            const int switchCountLimit = currentAltIt->second.switchCountLimit;
            const int switchCount      = ++primary.switchCounts[currentAltId];

            if (switchCount < switchCountLimit && switchCountLimit > 1) {
                primary.alternativeTarget = currentAltId;
                return;
            }
            else {
                primary.switchCounts[currentAltId] = 0;
            }
        }
    }

    int count;
    if (currentAltId == parentId) {
        count = primary.counts[parentId];
    } else {
        count = ++primary.counts[currentAltId];
    }

    const int chosen = selectNextChild(nodes,currentAltId, count, &isAlternativeChild);

    if (chosen == -1) {
        nodeStates[parentId].activeAlternativeId = parentId;
        primary.alternativeTarget  = -1;
    } else {
        nodeStates[parentId].activeAlternativeId = chosen;
        primary.alternativeTarget  = chosen;
    }
}

void TraversalLogic::selectSwitchNode(const NodeMap& nodes,int targetId, int& chosenNodeId) {
    if (nodeStates[targetId].lastNodeId != -1) {

        const int switchCount = ++primary.switchCounts[targetId];

        const auto switchNodeIterator = nodes.find(nodeStates[targetId].lastNodeId);

        if (switchNodeIterator != nodes.end()) {
            const RTNode& switchNode = switchNodeIterator->second;
            const int switchCountLimit = switchNode.switchCountLimit;

            if (switchCount < switchCountLimit && switchCountLimit > 1) {
                chosenNodeId = switchNode.nodeID;
            }
            else {
                primary.switchCounts[targetId] = 0;
            }
        }
    }
}

void TraversalLogic::advance(const NodeMap& nodes)
{
    const int targetId            = primary.target;
    int chosenNodeId        = -1;

    referenceTargetId       = primary.last;
    primary.last            = targetId;
    primary.alternativeLast = primary.alternativeTarget;

    const auto targetIterator = nodes.find(targetId);

    if (targetIterator == nodes.end() || targetIterator->second.children.empty()) {
        state = loop.active ? TraversalState::Reset : TraversalState::End;
        return;
    }

    selectSwitchNode(nodes, targetId, chosenNodeId);

    if (chosenNodeId == -1) {
        const int count = ++primary.counts[targetId];
        chosenNodeId = selectNextChild(nodes,targetId, count, &isAdvanceableChild);

        if (chosenNodeId != -1) {
            registerTrigger(nodes, chosenNodeId);
        }
    }

    if (chosenNodeId  != -1) {
        const auto nextTargetIt = nodes.find(chosenNodeId);

        nodeStates[chosenNodeId].lastNodeId = chosenNodeId;

        if (nextTargetIt == nodes.end()) {
            primary.alternativeTarget = -1;
        }
        else {
            primary.target = chosenNodeId;
            advanceAlternative(nodes,chosenNodeId);

            const int nextSubLoopCountLimit = nextTargetIt->second.subLoopCountLimit;
            const bool subLoopsForever      = (nextSubLoopCountLimit == 0);
            const bool subLoopsFinitely     = (nextSubLoopCountLimit > 1);

            if ((subLoopsForever || subLoopsFinitely) && primary.subRootNode == -1) {
                primary.subRootNode = nextTargetIt->second.nodeID;
                primary.subRootCounts[primary.subRootNode] = 0;
            }
        }
    }

    if (primary.target == primary.last) {
        state = loop.active ? TraversalState::Reset : TraversalState::End;
    }
}

const RTNode* TraversalLogic::peekNextTarget(const NodeMap& nodes)
{
    const auto cIt = primary.counts.find(primary.target);

    const int count = (cIt != primary.counts.end()) ? cIt->second + 1 : 1;

    const int peekTargetId = selectNextChild(nodes,primary.target, count, &isAudibleChild);

    if (peekTargetId == -1 || peekTargetId == primary.target) {
        return nullptr;
    }

    const auto itPeek = nodes.find(peekTargetId);

    if (itPeek != nodes.end()) {
        return &itPeek->second;
    }

    return nullptr;
}

void TraversalLogic::peekCrossTreeNode(const NodeMap& nodes, std::vector<int>& traverserIds)
{
    traverserIds.clear();

    auto scanHost = [&](int hostId) {
        const auto hostIterator = nodes.find(hostId);
        if (hostIterator == nodes.end()) {
            return;
        }

        for (const int childId : hostIterator->second.children) {
            const auto childIterator = nodes.find(childId);
            if (childIterator == nodes.end()) {
                continue;
            }

            const RTNode& childNode = childIterator->second;

            if (childNode.nodeType != RTNode::NodeType::RootNode) {
                continue;
            }
            if (childNode.nodeID == rootId) {
                continue;
            }
            if (childNode.countLimit <= 0) {
                continue;
            }

            int& count       = counters.crossTree[childId];
            int& switchCount = counters.crossTreeSwitch[childId];

            if (switchCount > 0) {
                traverserIds.push_back(childId);
                switchCount++;
                if (switchCount >= childNode.switchCountLimit) {
                    switchCount = 0;
                    count       = 0;
                }
            }
            else {
                count++;
                if (count >= childNode.countLimit) {
                    traverserIds.push_back(childId);
                    if (childNode.switchCountLimit > 1) {
                        switchCount = 1;
                    }
                    else {
                        count = 0;
                    }
                }
            }
        }
    };

    scanHost(primary.target);
    if (primary.alternativeTarget != -1) {
        scanHost(primary.alternativeTarget);
    }
}

const RTNode* TraversalLogic::ModulatorWalk::peek(const NodeMap& nodes, TraversalLogic& owner) const
{
    if (walker.target == -1) {
        return nullptr;
    }

    const auto cIt   = walker.counts.find(walker.target);
    const int  count = (cIt != walker.counts.end()) ? cIt->second + 1 : 1;

    const int peekId = owner.selectNextChild(nodes, walker.target, count, &isModulatorChild);
    if (peekId == -1) {
        return nullptr;
    }

    const auto peekIt = nodes.find(peekId);
    return (peekIt != nodes.end()) ? &peekIt->second : nullptr;
}

const RTNode* TraversalLogic::peekModulators(const NodeMap& nodes)
{
    return mod.peek(nodes, *this);
}

const RTNode& TraversalLogic::getTargetNode(const NodeMap& nodes) const { return nodes.at(primary.target); }
const RTNode& TraversalLogic::getRootNode  (const NodeMap& nodes) const { return nodes.at(rootId);         }

bool TraversalLogic::shouldTraverse() const
{
    return state != TraversalState::End;
}

void TraversalLogic::fillEndedResult(StepResult& result) const
{
    result.kind              = StepResult::Kind::Ended;
    result.leftId            = primary.target;
    result.leftAlternativeId = primary.alternativeTarget;
    result.referenceOffId    = referenceTargetId;
}

TraversalLogic::StepResult TraversalLogic::enterRoot(const NodeMap& nodes)
{
    state          = TraversalState::Active;
    primary.target = rootId;
    advanceAlternative(nodes, rootId);

    StepResult result;
    result.kind                 = StepResult::Kind::EnteredRoot;
    result.enteredId            = primary.target;
    result.enteredAlternativeId = primary.alternativeTarget;
    return result;
}

void TraversalLogic::advanceSubRoot(const NodeMap& nodes, StepResult& result)
{
    const auto subRootIt   = nodes.find(primary.subRootNode);
    const int  subRootLimit = (subRootIt != nodes.end()) ? subRootIt->second.subLoopCountLimit : 0;

    const int  subRootCount        = ++primary.subRootCounts[primary.subRootNode];
    const bool subRootLoopsForever = (subRootLimit == 0);

    if (!subRootLoopsForever && subRootCount >= subRootLimit) {
        primary.subRootCounts[primary.subRootNode] = 0;
        primary.subRootNode = -1;
        result.rootForReset = rootId;
    }
    else {
        result.rootForReset = primary.subRootNode;
        primary.target      = primary.subRootNode;
        result.enteredId    = primary.subRootNode;
    }
}

void TraversalLogic::handleLoopReset(const NodeMap& nodes, StepResult& result)
{
    loop.count++;

    if (loop.limit > 0 && loop.count >= loop.limit) {
        state                    = TraversalState::End;
        result.kind              = StepResult::Kind::Ended;
        result.leftId            = primary.target;
        result.leftAlternativeId = primary.alternativeTarget;
        return;
    }

    result.kind              = StepResult::Kind::LoopedToRoot;
    result.leftId            = primary.target;
    result.leftAlternativeId = primary.alternativeTarget;

    primary.target = rootId;
    advanceAlternative(nodes, rootId);

    result.enteredId            = rootId;
    result.enteredAlternativeId = primary.alternativeTarget;

    if (primary.subRootNode != -1) {
        advanceSubRoot(nodes, result);
    }
    else {
        result.rootForReset = rootId;
    }

    state = TraversalState::Active;
}

TraversalLogic::StepResult TraversalLogic::stepActive(const NodeMap& nodes)
{
    advance(nodes);

    StepResult result;

    const int leftId            = primary.last;
    const int leftAlternativeId = primary.alternativeLast;

    result.pushCounts        = true;
    result.countSourceNodeId = leftId;
    result.countSourceCount  = primary.counts[leftId];

    switch (state) {
        case TraversalState::Active:
            result.kind                 = StepResult::Kind::Advanced;
            result.leftId               = leftId;
            result.leftAlternativeId    = leftAlternativeId;
            result.enteredId            = primary.target;
            result.enteredAlternativeId = primary.alternativeTarget;
            break;

        case TraversalState::Reset:
            handleLoopReset(nodes, result);
            break;

        case TraversalState::End:
            fillEndedResult(result);
            break;

        default:
            break;
    }

    return result;
}

TraversalLogic::StepResult TraversalLogic::handleNodeEvent(const NodeMap& nodes) {
    switch (state) {
        case TraversalState::Start:
            return enterRoot(nodes);

        case TraversalState::Active:
            return stepActive(nodes);

        case TraversalState::End: {
            StepResult result;
            fillEndedResult(result);
            return result;
        }

        default:
            return {};
    }
}

const RTNode* TraversalLogic::getModulatorNode(const NodeMap& nodes, int nodeId) const
{
    const auto nodeIterator = nodes.find(nodeId);
    if (nodeIterator == nodes.end()) {
        return nullptr;
    }

    for (const int childId : nodeIterator->second.children) {
        const auto childIt = nodes.find(childId);
        if (childIt == nodes.end()) {
            continue;
        }

        const RTNode* childNode = &childIt->second;

        if (childNode->nodeType == RTNode::NodeType::ModulatorRoot) {
            return childNode;
        }
    }

    return nullptr;
}

bool TraversalLogic::isDescendantOf(const NodeMap& nodes, int nodeId, int ancestorId)
{
    if (ancestorId == -1 || nodeId == ancestorId) {
        return false;
    }

    int current = nodeId;
    int guard   = 0;

    while (current != 0 && guard++ < 10000) {
        const auto it = nodes.find(current);
        if (it == nodes.end()) {
            return false;
        }

        const int parent = it->second.parentId;
        if (parent == ancestorId) {
            return true;
        }

        current = parent;
    }

    return false;
}

int TraversalLogic::findActiveModulatorRoot(const NodeMap& nodes, int regularNodeId) const
{
    if (nodes.find(regularNodeId) == nodes.end()) {
        return -1;
    }

    const RTNode* modRoot = getModulatorNode(nodes, regularNodeId);


    if (modRoot == nullptr || modRoot->countLimit <= 0) {
        return -1;
    }

    const auto countIt = primary.counts.find(regularNodeId);

    const int hostCount = (countIt != primary.counts.end()) ? countIt->second + 1 : 1;

    if (hostCount % modRoot->countLimit == 0) {
        return modRoot->nodeID;
    }

    return -1;
}
