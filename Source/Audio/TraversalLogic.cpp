#include "TraversalLogic.h"
#include "AudioUIBridge.h"

TraversalLogic::TraversalLogic(int root, AudioUIBridge& b)
    : rootId(root), bridge(&b) {

}

static bool isModulatorChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Modulator || t == RTNode::NodeType::ModulatorRoot;
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


int TraversalLogic::selectNextChild(const NodeMap& nodes, int parentId, int parentCount,
                                    ChildPredicate isEligible)
{
    auto it = nodes.find(parentId);
    if (it == nodes.end()) return -1;

    int chosen   = -1;
    int maxLimit = 0;

    for (int childId : it->second.children) {
        auto childIt = nodes.find(childId);
        if (childIt == nodes.end()) continue;

        const RTNode& child = childIt->second;
        if (!isEligible(child.nodeType)) continue;
        if (child.countLimit <= 0) continue;

        if (parentCount % child.countLimit == 0 && child.countLimit > maxLimit) {
            chosen   = childId;
            maxLimit = child.countLimit;
        }
    }

    return chosen;
}

static bool isAdvanceableChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Node || t == RTNode::NodeType::Modulator;
}

static bool isAudibleChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Node;
}

void TraversalLogic::advance(NodeMap& nodes)
{
    referenceTargetId = primary.last;
    primary.last      = primary.target;
    int count         = ++primary.counts[primary.target];

    auto targetIt = nodes.find(primary.target);

    if (targetIt == nodes.end() || targetIt->second.children.empty()) {
        state = isLooping ? TraversalState::Reset : TraversalState::End;
        return;
    }

    int chosen = selectNextChild(nodes, primary.target, count, &isAdvanceableChild);
    if (chosen != -1) {
        primary.target = chosen;
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

    if (peekTargetId == -1 || peekTargetId == primary.target)
        return nullptr;

    auto itPeek = nodes.find(peekTargetId);
    return (itPeek != nodes.end()) ? &itPeek->second : nullptr;
}

std::vector<int> TraversalLogic::peekCrossTreeNode(NodeMap& nodes)
{
    std::vector<int> traverserIds;

    auto targetIterator = nodes.find(primary.target);
    if (targetIterator == nodes.end()) {
        return traverserIds;
    }

    auto countIterator = primary.counts.find(primary.target);
    int targetCount = (countIterator != primary.counts.end() ? countIterator->second : 0) + 1;

    for (int childId : targetIterator->second.children) {
        auto childIterator = nodes.find(childId);
        if (childIterator == nodes.end()) {
            continue;
        }

        RTNode::NodeType childNodeType = childIterator->second.nodeType;
        const RTNode& childNode = childIterator->second;

        bool isCrossTreeNode = false;

        if (childNodeType == RTNode::NodeType::RootNode ) {
            if (childNode.nodeID != rootId) {
                isCrossTreeNode = true;
            }
        }

        if (isCrossTreeNode && targetCount % childNode.countLimit == 0) {
            traverserIds.push_back(childId);
        }
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
    if (peekId == -1) return nullptr;

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

            result.kind      = StepResult::Kind::EnteredRoot;
            result.enteredId = primary.target;
            return result;
        }
        case TraversalState::Active: {
            advance(nodes);

            const int leftId          = primary.last;
            const int leftParentCount = primary.counts[leftId];

            result.pushCounts        = true;
            result.countSourceNodeId = leftId;
            result.countSourceCount  = leftParentCount;

            switch (state) {
                case TraversalState::Active: {
                    result.kind      = StepResult::Kind::Advanced;
                    result.leftId    = leftId;
                    result.enteredId = primary.target;
                    break;
                }
                case TraversalState::Reset: {
                    loopCount++;
                    if (loopLimit > 0 && loopCount >= loopLimit) {
                        state          = TraversalState::End;
                        result.kind    = StepResult::Kind::Ended;
                        result.leftId  = primary.target;
                    } else {
                        result.kind         = StepResult::Kind::LoopedToRoot;
                        result.leftId       = primary.target;
                        result.enteredId    = rootId;
                        result.rootForReset = rootId;
                        primary.target      = rootId;
                        state               = TraversalState::Active;
                    }
                    break;
                }
                case TraversalState::End: {
                    result.kind           = StepResult::Kind::Ended;
                    result.leftId         = primary.target;
                    result.referenceOffId = referenceTargetId;
                    break;
                }
                default: break;
            }
            return result;
        }
        case TraversalState::End: {
            result.kind           = StepResult::Kind::Ended;
            result.leftId         = primary.target;
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
    int currentId = regularNodeId;
    bool isFiringNode = true;

    while (currentId != 0) {
        auto it = nodes.find(currentId);
        if (it == nodes.end()) {
            break;
        }

        const RTNode& current = it->second;

        RTNode* modRoot = getModulatorNode(nodes, currentId);
        if (modRoot != nullptr && modRoot->countLimit > 0) {
            auto countIt = primary.counts.find(currentId);
            int hostCount = (countIt != primary.counts.end() ? countIt->second : 0);
            if (isFiringNode) {
                hostCount += 1;
            }

            if (hostCount > 0 && hostCount % modRoot->countLimit == 0) {
                return modRoot->nodeID;
            }
        }

        if (current.parentId == currentId) {
            break;
        }
        currentId = current.parentId;
        isFiringNode = false;
    }

    return -1;
}