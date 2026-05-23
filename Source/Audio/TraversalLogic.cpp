#include "TraversalLogic.h"
#include "AudioUIBridge.h"

TraversalLogic::TraversalLogic(int root, AudioUIBridge& b)
    : rootId(root), bridge(&b) {

}

static bool isModulatorChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Modulator || t == RTNode::NodeType::ModulatorRoot;
}

static bool isAlternativeChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Alternative;
}

void TraversalLogic::advanceModulator(NodeMap& nodes) {

    if (activeModulatorRootId == -1 || modulator.target == -1) {
        return;
    }

    auto targetIt = nodes.find(modulator.target);
    if (targetIt == nodes.end()) {
        return;
    }

    modulator.last = modulator.target;
    int count      = ++modulator.counts[modulator.target];

    int chosen = selectNextChild(nodes, modulator.target, count, &isModulatorChild);

    if (chosen == -1) {
        bridge->pushArrowReset(activeModulatorRootId);
        modulator.target = activeModulatorRootId;
    }
    else {
        modulator.target = chosen;
    }
}

int TraversalLogic::advanceAlternativeNode(NodeMap &nodes) {
    int targetId = primary.target;
    auto targetIt = nodes.find(targetId);

    int activeAlternativeNodeId = targetIt->second.activeAlternativeId;

    if (targetIt == nodes.end()) {
        return 0;
    }

    int count = primary.counts[primary.target];
    count = count + 1;

    int chosen = 0;

    if (activeAlternativeNodeId == -1) {
        chosen = selectNextChild(nodes, targetId,count, &isAlternativeChild);
        targetIt->second.activeAlternativeId = chosen;
    }
    else {
        chosen = selectNextChild(nodes, activeAlternativeNodeId, count, &isAlternativeChild);
        targetIt->second.activeAlternativeId = chosen;
    }

    if (chosen == -1) {
        bridge->pushArrowReset(targetId);
    }

    return chosen;
}

int TraversalLogic::selectNextChild(NodeMap& nodes, int parentId, int parentCount,
                                    ChildPredicate isEligible)
{
    auto parentNodeIterator = nodes.find(parentId);

    if (parentNodeIterator == nodes.end()) {
        return -1;
    }

    int chosen   = -1;
    int maxLimit = 0;

    for (int childId : parentNodeIterator->second.children) {
        auto childIt = nodes.find(childId);
        if (childIt == nodes.end()) {
            continue;
        }

        const RTNode& child = childIt->second;

        if (!isEligible(child.nodeType)) {
            continue;
        }

        if (child.countLimit <= 0) {
            continue;
        }

        auto durIt = parentNodeIterator->second.durationMap.find(childId);
        if (durIt != parentNodeIterator->second.durationMap.end() && durIt->second == 0) {
            continue;
        }

        if (parentCount % child.countLimit == 0 && child.countLimit > maxLimit) {
            chosen   = childId;
            maxLimit = child.countLimit;
        }
    }

    parentNodeIterator->second.lastNodeId = chosen;
    return chosen;
}

static bool isAdvanceableChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Node || t == RTNode::NodeType::Modulator;
}

static bool isAudibleChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Node;
}

void TraversalLogic::advanceAlternative(NodeMap& nodes, int parentId) {
    auto parentIt = nodes.find(parentId);
    if (parentIt == nodes.end()) {
        primary.alternativeTarget = -1;
        return;
    }

    RTNode& parent = parentIt->second;

    if (parent.alternativeRootId == -1) {
        parent.activeAlternativeId = -1;
        primary.alternativeTarget  = -1;
        return;
    }

    int currentAltId = parent.activeAlternativeId;

    if (currentAltId == -1) {
        parent.activeAlternativeId = parentId;
        primary.alternativeTarget  = -1;
        return;
    }

    int count;
    if (currentAltId == parentId) {
        count = primary.counts[parentId];
    } else {
        count = ++primary.counts[currentAltId];
    }

    int chosen = selectNextChild(nodes, currentAltId, count, &isAlternativeChild);

    if (chosen == -1) {
        parent.activeAlternativeId = parentId;
        primary.alternativeTarget  = -1;
    } else {
        parent.activeAlternativeId = chosen;
        primary.alternativeTarget  = chosen;
    }
}

void TraversalLogic::advance(NodeMap& nodes)
{
    int targetId            = primary.target;
    int chosenNodeId        = -1;

    referenceTargetId       = primary.last;
    primary.last            = targetId;
    primary.alternativeLast = primary.alternativeTarget;

    auto targetIt = nodes.find(targetId);

    if (targetIt == nodes.end() || targetIt->second.children.empty()) {
        state = isLooping ? TraversalState::Reset : TraversalState::End;
        return;
    }


    if (targetIt->second.lastNodeId != -1) {


        int switchCount = ++primary.switchCounts[targetId];

        auto switchNodeIterator = nodes.find(targetIt->second.lastNodeId);

        if (switchNodeIterator != nodes.end()) {
            const RTNode& switchNode = switchNodeIterator->second;
            int switchCountLimit = switchNode.switchCountLimit;

            if (switchCount < switchCountLimit && switchCountLimit > 1) {
                chosenNodeId = switchNode.nodeID;
            }
            else {
                primary.switchCounts[targetId] = 0;
            }
        }
    }

    if (chosenNodeId == -1) {
        int count = ++primary.counts[targetId];
        chosenNodeId = selectNextChild(nodes, targetId, count, &isAdvanceableChild);
    }

    if (chosenNodeId  != -1) {
        auto nextTargetIt = nodes.find(chosenNodeId);

        if (nextTargetIt == nodes.end()) {
            primary.alternativeTarget = -1;
        }
        else {
            primary.target = chosenNodeId;
            advanceAlternative(nodes, chosenNodeId);
        }

        nextTargetIt->second.lastNodeId = chosenNodeId;

        if (nextTargetIt->second.subLoopCountLimit > 1 && primary.subRootNode == -1) {
            primary.subRootNode = nextTargetIt->second.nodeID;
            primary.subRootCounts[primary.subRootNode] = 0;
            DBG("node subRoot found: "<< primary.subRootNode <<"");
        }
    }

    if (primary.target == primary.last) {

        state = isLooping ? TraversalState::Reset : TraversalState::End;
    }
}

RTNode* TraversalLogic::peekNextTarget(NodeMap& nodes)
{
    auto cIt = primary.counts.find(primary.target);
    int count = (cIt != primary.counts.end() ? cIt->second : 0) + 1;

    int peekTargetId = selectNextChild(nodes, primary.target, count, &isAudibleChild);

    if (peekTargetId == -1 || peekTargetId == primary.target) {
        return nullptr;
    }

    auto itPeek = nodes.find(peekTargetId);
    return (itPeek != nodes.end()) ? &itPeek->second : nullptr;
}

std::vector<int> TraversalLogic::peekCrossTreeNode(NodeMap& nodes)
{
    std::vector<int> traverserIds;

    auto countIterator = primary.counts.find(primary.target);
    int targetCount = (countIterator != primary.counts.end() ? countIterator->second : 0) + 1;

    auto scanHost = [&](int hostId) {
        auto hostIterator = nodes.find(hostId);
        if (hostIterator == nodes.end()) {
            return;
        }

        for (int childId : hostIterator->second.children) {
            auto childIterator = nodes.find(childId);
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

            if (targetCount % childNode.countLimit == 0) {
                traverserIds.push_back(childId);
            }
        }
    };

    scanHost(primary.target);
    if (primary.alternativeTarget != -1) {
        scanHost(primary.alternativeTarget);
    }

    return traverserIds;
}

RTNode* TraversalLogic::peekModulators(NodeMap& nodes) {

    if (modulator.target == -1) {
        return nullptr;
    }

    auto cIt  = modulator.counts.find(modulator.target);
    int count = (cIt != modulator.counts.end() ? cIt->second : 0) + 1;

    int peekId = selectNextChild(nodes, modulator.target, count, &isModulatorChild);
    if (peekId == -1) {
        return nullptr;
    }

    auto peekIt = nodes.find(peekId);
    return (peekIt != nodes.end()) ? &peekIt->second : nullptr;
}

const RTNode& TraversalLogic::getTargetNode   (const NodeMap& nodes) const { return nodes.at(primary.target);    }
const RTNode& TraversalLogic::getLastNode     (const NodeMap& nodes) const { return nodes.at(primary.last);      }
const RTNode& TraversalLogic::getReferenceNode(const NodeMap& nodes) const { return nodes.at(referenceTargetId); }
const RTNode& TraversalLogic::getRootNode     (const NodeMap& nodes) const { return nodes.at(rootId);            }

const RTNode& TraversalLogic::getRelayNode(int relayNodeId, const NodeMap& nodes) { return nodes.at(relayNodeId); }

bool TraversalLogic::shouldTraverse() const
{
    return state != TraversalState::End;
}

TraversalLogic::StepResult TraversalLogic::handleNodeEvent(NodeMap& nodes) {
    StepResult result;

    switch (state) {
        case TraversalState::Start: {
            state          = TraversalState::Active;
            primary.target = rootId;
            advanceAlternative(nodes, rootId);

            result.kind                 = StepResult::Kind::EnteredRoot;
            result.enteredId            = primary.target;
            result.enteredAlternativeId = primary.alternativeTarget;
            return result;
        }
        case TraversalState::Active: {
            advance(nodes);

            const int leftAlternativeId = primary.alternativeLast;
            const int leftId            = primary.last;
            const int leftParentCount = primary.counts[leftId];

            result.pushCounts        = true;
            result.countSourceNodeId = leftId;
            result.countSourceCount  = leftParentCount;

            switch (state) {
                case TraversalState::Active: {
                    result.kind                 = StepResult::Kind::Advanced;

                    result.leftId               = leftId;
                    result.leftAlternativeId    = leftAlternativeId;

                    result.enteredId            = primary.target;
                    result.enteredAlternativeId = primary.alternativeTarget;
                    break;
                }
                case TraversalState::Reset: {
                    loopCount++;
                    if (loopLimit > 0 && loopCount >= loopLimit) {
                        state          = TraversalState::End;
                        result.kind    = StepResult::Kind::Ended;
                        result.leftId  = primary.target;
                        result.leftAlternativeId = primary.alternativeTarget;
                    } else {
                        result.kind              = StepResult::Kind::LoopedToRoot;
                        result.leftId            = primary.target;
                        result.leftAlternativeId = primary.alternativeTarget;

                        primary.target           = rootId;
                        advanceAlternative(nodes, rootId);

                        result.enteredId            = rootId;
                        result.enteredAlternativeId = primary.alternativeTarget;

                        if (primary.subRootNode != -1) {
                            auto subRootIt = nodes.find(primary.subRootNode);
                            int subRootLimit = (subRootIt != nodes.end())
                                                 ? subRootIt->second.subLoopCountLimit
                                                 : 0;

                            int subRootCount = ++primary.subRootCounts[primary.subRootNode];

                            if (subRootCount >= subRootLimit) {
                                DBG("subRoot reached limit: " << primary.subRootNode << " count " << subRootCount);
                                primary.subRootCounts[primary.subRootNode] = 0;
                                primary.subRootNode = -1;
                                result.rootForReset = rootId;
                            }
                            else {
                                DBG("looping back to subRoot: " << primary.subRootNode << " count " << subRootCount);
                                result.rootForReset = primary.subRootNode;
                                primary.target = primary.subRootNode;
                                result.enteredId = primary.subRootNode;
                            }
                        }
                        else {
                            result.rootForReset = rootId;
                        }

                        state                       = TraversalState::Active;
                    }
                    break;
                }
                case TraversalState::End: {
                    result.kind              = StepResult::Kind::Ended;
                    result.leftId            = primary.target;
                    result.leftAlternativeId = primary.alternativeTarget;
                    result.referenceOffId    = referenceTargetId;
                    break;
                }
                default: break;
            }
            return result;
        }
        case TraversalState::End: {
            result.kind              = StepResult::Kind::Ended;
            result.leftId            = primary.target;
            result.leftAlternativeId = primary.alternativeTarget;
            result.referenceOffId = referenceTargetId;
            return result;
        }
        default: break;
    }

    return result;
}

RTNode* TraversalLogic::getModulatorNode(NodeMap& nodes, int nodeId)
{
    auto nodeIterator = nodes.find(nodeId);
    if (nodeIterator == nodes.end()) {
        return nullptr;
    }

    for (int childId : nodeIterator->second.children) {
        auto childIt = nodes.find(childId);
        if (childIt == nodes.end()) {
            continue;
        }

        RTNode* childNode = &childIt->second;

        if (childNode->nodeType == RTNode::NodeType::ModulatorRoot) {
            return childNode;
        }
    }

    return nullptr;
}

int TraversalLogic::findActiveModulatorRoot(NodeMap& nodes, int regularNodeId)
{
    if (nodes.find(regularNodeId) == nodes.end()) {
        return -1;
    }

    RTNode* modRoot = getModulatorNode(nodes, regularNodeId);


    if (modRoot == nullptr || modRoot->countLimit <= 0) {
        return -1;
    }

    auto countIt = primary.counts.find(regularNodeId);
    int hostCount = (countIt != primary.counts.end() ? countIt->second : 0) + 1;

    if (hostCount % modRoot->countLimit == 0) {
        return modRoot->nodeID;
    }

    return -1;
}
