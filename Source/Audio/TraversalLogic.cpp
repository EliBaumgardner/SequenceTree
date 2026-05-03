#include "TraversalLogic.h"
#include "../Plugin/PluginProcessor.h"

TraversalLogic::TraversalLogic(int root, SequenceTreeAudioProcessor* processor)
    : rootId(root), audioProcessor(processor) {

}

void TraversalLogic::advanceModulator(NodeMap& nodes) {

    if (activeModulatorRootId == -1 || targetModulatorId == -1) {
        return;
    }

    auto targetIt = nodes.find(targetModulatorId);
    if (targetIt == nodes.end()) {
        return;
    }

    lastModulatorId = targetModulatorId;

    int count = ++modulatorCounts[targetModulatorId];
    int maxLimit = 0;
    int nextTargetId = targetModulatorId;

    for (int childId : targetIt->second.children) {
        auto childIt = nodes.find(childId);
        if (childIt == nodes.end()) {
            continue;
        }

        const RTNode& child = childIt->second;
        bool isModChild = child.nodeType == RTNode::NodeType::Modulator
                       || child.nodeType == RTNode::NodeType::ModulatorRoot;

        if (isModChild && child.countLimit > 0
            && count % child.countLimit == 0
            && child.countLimit > maxLimit)
        {
            maxLimit = child.countLimit;
            nextTargetId = child.nodeID;
        }
    }

    if (nextTargetId == targetModulatorId) {
        audioProcessor->eventManager.bridge.pushArrowReset(activeModulatorRootId);
        targetModulatorId = activeModulatorRootId;
    }
    else {
        targetModulatorId = nextTargetId;
    }
}


void TraversalLogic::advance(NodeMap& nodes)
{

    referenceTargetId = lastTargetId;
    lastTargetId      = targetId;
    int count         = ++counts[targetId];

    auto targetNodeIterator     = nodes.find(targetId);
    std::vector<int> targetNodeChildren = targetNodeIterator->second.children;

    if (targetNodeIterator == nodes.end() || targetNodeChildren.empty()) {
        if (isLooping) {
            state = TraversalState::Reset;
        }
        else {
            state = TraversalState::End;
        }
        return;
    }

    int maxLimit = 0;

    for (int childIndex : targetNodeChildren) {
        auto itChild = nodes.find(childIndex);

        if (itChild == nodes.end()) {
            continue;
        }

        auto& childNode = itChild->second;
        int   countLimit     = childNode.countLimit;

        switch (childNode.nodeType) {
            case RTNode::NodeType::Node:
            case RTNode::NodeType::Modulator: {
                if (count % countLimit == 0 && countLimit > maxLimit) {
                    targetId = childIndex;
                    maxLimit = countLimit;
                }
                break;
            }
            case RTNode::NodeType::RootNode:
            case RTNode::NodeType::Connector:
            case RTNode::NodeType::ModulatorRoot: {
                if (count % countLimit == 0)
                break;
            }
            default: break;
        }
    }

    if (targetId == lastTargetId) {
        if (isLooping) {
            state = TraversalState::Reset;
        }
        else {
            state == TraversalState::End;
        }
    }
}

RTNode* TraversalLogic::peekNextTarget(NodeMap& nodes)
{
    auto itTarget = nodes.find(targetId);
    if (itTarget == nodes.end()) return nullptr;

    auto cIt = counts.find(targetId);
    int count        = (cIt != counts.end() ? cIt->second : 0) + 1;
    int peekTargetId = -1;
    int maxLimit     = 0;

    for (int childIndex : itTarget->second.children) {
        auto itChild = nodes.find(childIndex);
        if (itChild == nodes.end()) continue;
        const auto& childNode = itChild->second;
        int limit = childNode.countLimit;

        if ((childNode.nodeType == RTNode::NodeType::Node) && count % limit == 0 && limit > maxLimit) {
            peekTargetId = childIndex;
            maxLimit     = limit;
        }
    }

    if (peekTargetId == -1 || peekTargetId == targetId)
        return nullptr;

    auto itPeek = nodes.find(peekTargetId);
    return (itPeek != nodes.end()) ? &itPeek->second : nullptr;
}

std::vector<int> TraversalLogic::peekCrossTreeNode(NodeMap& nodes)
{
    std::vector<int> traverserIds;

    auto targetIterator = nodes.find(targetId);
    if (targetIterator == nodes.end()) {
        return traverserIds;
    }

    auto countIterator = counts.find(targetId);
    int targetCount = (countIterator != counts.end() ? countIterator->second : 0) + 1;

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

    if (targetModulatorId == -1) {
        return nullptr;
    }

    auto targetIterator = nodes.find(targetModulatorId);
    if (targetIterator == nodes.end()) {
        return nullptr;
    }

    const RTNode& modulatorNode = targetIterator->second;

    auto countIterator = modulatorCounts.find(modulatorNode.nodeID);
    int count = (countIterator != modulatorCounts.end() ? countIterator->second : 0) + 1;

    int maxLimit = 0;
    RTNode* peekChild = nullptr;

    for (int childId : modulatorNode.children) {
        auto childIterator = nodes.find(childId);
        if (childIterator == nodes.end()) {
            continue;
        }

        RTNode& modulatorChild = childIterator->second;
        int childCountLimit = modulatorChild.countLimit;

        bool isModulatorChild = modulatorChild.nodeType == RTNode::NodeType::Modulator
                             || modulatorChild.nodeType == RTNode::NodeType::ModulatorRoot;

        if (isModulatorChild && childCountLimit > 0
            && count % childCountLimit == 0
            && childCountLimit > maxLimit)
        {
            maxLimit = childCountLimit;
            peekChild = &modulatorChild;
        }
    }

    return peekChild;
}

const RTNode& TraversalLogic::getTargetNode   (const NodeMap& nodes) const { return nodes.at(targetId);          }
const RTNode& TraversalLogic::getLastNode     (const NodeMap& nodes) const { return nodes.at(lastTargetId);      }
const RTNode& TraversalLogic::getReferenceNode(const NodeMap& nodes) const { return nodes.at(referenceTargetId); }
const RTNode& TraversalLogic::getRootNode     (const NodeMap& nodes) const { return nodes.at(rootId);            }

const RTNode& TraversalLogic::getRelayNode(int relayNodeId, const NodeMap& nodes) { return nodes.at(relayNodeId); }

bool TraversalLogic::shouldTraverse() const
{
    return state != TraversalState::End;
}

void TraversalLogic::handleNodeEvent(NodeMap& nodes) {
    auto safeHighlight = [&](int nodeId, bool on) {
        auto it = nodes.find(nodeId);
        if (it != nodes.end()) audioProcessor->eventManager.bridge.highlightNode(it->second, on);
    };

    switch (state) {
        case TraversalState::Start: {
            state    = TraversalState::Active;
            targetId = rootId;
            safeHighlight(targetId, true);
            break;
        }
        case TraversalState::Active: {
            advance(nodes);

            {
                auto& bridge = audioProcessor->eventManager.bridge;

                bridge.pushCount(lastTargetId, 0, 1);

                auto it = nodes.find(lastTargetId);
                if (it != nodes.end())
                {
                    int parentCount = counts[lastTargetId];
                    for (int childId : it->second.children)
                    {
                        auto childIt = nodes.find(childId);
                        if (childIt != nodes.end())
                        {
                            int limit = childIt->second.countLimit;
                            int fill  = (parentCount % limit == 0) ? limit : (parentCount % limit);
                            bridge.pushCount(childId, fill, limit);
                        }
                    }
                }
            }

            switch (state) {
                case TraversalState::Active: {
                    safeHighlight(lastTargetId, false);
                    safeHighlight(targetId,     true);
                    break;
                }
                case TraversalState::Reset: {
                    loopCount++;
                    if (loopLimit > 0 && loopCount >= loopLimit) {
                        safeHighlight(targetId, false);
                        state = TraversalState::End;
                    } else {
                        safeHighlight(targetId, false);
                        safeHighlight(rootId,   true);
                        audioProcessor->eventManager.bridge.pushArrowReset(rootId);
                        targetId = rootId;
                        state    = TraversalState::Active;
                    }
                    break;
                }
                case TraversalState::End: {
                    safeHighlight(targetId,          false);
                    safeHighlight(referenceTargetId, false);
                    break;
                }
                default: break;
            }
            break;
        }
        case TraversalState::End: {
            safeHighlight(referenceTargetId, false);
            safeHighlight(targetId,          false);
            break;
        }
        default: break;
    }
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
            auto countIt = counts.find(currentId);
            int hostCount = (countIt != counts.end() ? countIt->second : 0);
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