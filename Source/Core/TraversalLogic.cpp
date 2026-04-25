#include "TraversalLogic.h"
#include "PluginProcessor.h"

TraversalLogic::TraversalLogic(int root, SequenceTreeAudioProcessor* processor)
    : rootId(root), audioProcessor(processor) {}

void TraversalLogic::advance(NodeMap& nodes)
{
    traversalModifiers.clear();

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
                    traversalModifiers.push_back(childIndex);
                break;
            }
            default: break;
        }
    }

    if (targetId == lastTargetId)
        state = isLooping ? TraversalState::Reset : TraversalState::End;
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

        if ((childNode.nodeType == RTNode::NodeType::Node || childNode.nodeType == RTNode::NodeType::Modulator) && count % limit == 0 && limit > maxLimit) {
            peekTargetId = childIndex;
            maxLimit     = limit;
        }
    }

    if (peekTargetId == -1 || peekTargetId == targetId)
        return nullptr;

    auto itPeek = nodes.find(peekTargetId);
    return (itPeek != nodes.end()) ? &itPeek->second : nullptr;
}

std::vector<int> TraversalLogic::peekTraversers(NodeMap& nodes)
{
    auto targetIterator = nodes.find(targetId);
    jassert(targetIterator != nodes.end());

    auto countIterator = counts.find(targetId);
    int targetCount = (countIterator != counts.end() ? countIterator->second : 0) + 1;

    std::vector<int> traverserIds;

    for (int childId : targetIterator->second.children) {
        auto childIterator = nodes.find(childId);
        jassert(childIterator != nodes.end());

        const RTNode& childNode = childIterator->second;
        bool isCrossTreeRoot    = childNode.nodeType == RTNode::NodeType::RootNode
                               && childNode.nodeID   != rootId;

        bool isTraverser = childNode.nodeType == RTNode::NodeType::Connector
                        || childNode.nodeType == RTNode::NodeType::ModulatorRoot
                        || isCrossTreeRoot;

        if (isTraverser && targetCount % childNode.countLimit == 0) {
            traverserIds.push_back(childId);
        }
    }

    return traverserIds;
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

            // Push count ring updates: reset the node that was just played, fill its children
            {
                auto& bridge = audioProcessor->eventManager.bridge;

                // Reset lastTargetId's own ring (it has now played; cycle starts fresh)
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

void TraversalLogic::handleConnectorNodeEvent(int relayNodeId, const NodeMap& nodes) const
{
    auto it = nodes.find(relayNodeId);
    if (it != nodes.end())
        audioProcessor->eventManager.bridge.highlightNode(it->second, false);
}