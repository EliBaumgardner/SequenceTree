#include "TraversalLogic.h"
#include "PluginProcessor.h"

TraversalLogic::TraversalLogic(int root, SequenceTreeAudioProcessor* processor)
    : rootId(root), audioProcessor(processor) {}

void TraversalLogic::advance(NodeMap& nodes)
{
    traversers.clear();

    referenceTargetId = lastTargetId;
    lastTargetId      = targetId;
    int count         = ++counts[targetId];
    auto itTarget     = nodes.find(targetId);
    if (itTarget == nodes.end()) {
        state = isLooping ? TraversalState::Reset : TraversalState::End;
        return;
    }

    if (itTarget->second.children.empty() && itTarget->second.connectors.empty()) {
        state = isLooping ? TraversalState::Reset : TraversalState::End;
        return;
    }

    int maxLimit = 0;

    for (int childIndex : itTarget->second.children) {
        auto itChild = nodes.find(childIndex);
        if (itChild == nodes.end()) continue;
        auto& childNode = itChild->second;
        int   limit     = childNode.countLimit;

        switch (childNode.nodeType) {
            case RTNode::NodeType::Node:
            case RTNode::NodeType::Modulator: {
                if (count % limit == 0 && limit > maxLimit) {
                    targetId = childIndex;
                    maxLimit = limit;
                }
                break;
            }
            case RTNode::NodeType::Connector:
            case RTNode::NodeType::ModulatorRoot: {
                if (count % limit == 0)
                    traversers.push_back(childIndex);
                break;
            }
            default: break;
        }
    }

    for (int connectorIndex : itTarget->second.connectors) {
        auto itConnector = nodes.find(connectorIndex);
        if (itConnector == nodes.end()) { continue; }
        int limit = itConnector->second.countLimit;
        if (count % limit == 0)
            traversers.push_back(connectorIndex);
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
    auto targetIdIterator = nodes.find(targetId);

    jassert(targetIdIterator != nodes.end());

    auto countIterator = counts.find(targetId);
    int count = 1;

    if (countIterator != counts.end()) {
        count = countIterator->second;
    }

    std::vector<int> childrenIndices;

    for (int childIndex : targetIdIterator->second.children) {
        auto itChild = nodes.find(childIndex);
        jassert(itChild != nodes.end());

        const auto& childNode = itChild->second;
        if ((childNode.nodeType == RTNode::NodeType::Connector || childNode.nodeType == RTNode::NodeType::ModulatorRoot)
            && count % childNode.countLimit == 0)
            childrenIndices.push_back(childIndex);
    }

    for (int connectorIndex : targetIdIterator->second.connectors) {
        auto itConnector = nodes.find(connectorIndex);
        jassert(itConnector != nodes.end());

        if (count % itConnector->second.countLimit == 0)
            childrenIndices.push_back(connectorIndex);
    }

    return childrenIndices;
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

void TraversalLogic::handleNodeEvent(NodeMap& nodes)
{
    auto safeHighlight = [&](int nodeId, bool on) {
        auto it = nodes.find(nodeId);
        if (it != nodes.end()) audioProcessor->eventManager.highlightNode(it->second, on);
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

            switch (state) {
                case TraversalState::Active: {
                    safeHighlight(lastTargetId, false);
                    safeHighlight(targetId,     true);
                    break;
                }
                case TraversalState::Reset: {
                    safeHighlight(targetId, false);
                    safeHighlight(rootId,   true);
                    targetId = rootId;
                    state    = TraversalState::Active;
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
        audioProcessor->eventManager.highlightNode(it->second, false);
}