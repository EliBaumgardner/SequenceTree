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

    if (itTarget->second.children.empty() && itTarget->second.connectors.empty()) {
        state = isLooping ? TraversalState::Reset : TraversalState::End;
        return;
    }

    int maxLimit = 0;

    for (int childIndex : itTarget->second.children) {
        auto childNode = nodes.find(childIndex)->second;
        int  limit     = childNode.countLimit;

        switch (childNode.nodeType) {
            case RTNode::NodeType::Node: {
                if (count % limit == 0 && limit > maxLimit) {
                    targetId = childIndex;
                    maxLimit = limit;
                }
                break;
            }
            case RTNode::NodeType::RelayNode: {
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

    int count        = counts[targetId] + 1;
    int peekTargetId = -1;
    int maxLimit     = 0;

    for (int childIndex : itTarget->second.children) {
        auto itChild = nodes.find(childIndex);
        if (itChild == nodes.end()) continue;
        const auto& childNode = itChild->second;
        int limit = childNode.countLimit;

        if (childNode.nodeType == RTNode::NodeType::Node && count % limit == 0 && limit > maxLimit) {
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
    auto itTarget = nodes.find(targetId);
    if (itTarget == nodes.end()) return {};

    int count = counts[targetId] + 1;
    std::vector<int> result;

    for (int childIndex : itTarget->second.children) {
        auto itChild = nodes.find(childIndex);
        if (itChild == nodes.end()) continue;
        const auto& childNode = itChild->second;
        if (childNode.nodeType == RTNode::NodeType::RelayNode && count % childNode.countLimit == 0)
            result.push_back(childIndex);
    }

    for (int connectorIndex : itTarget->second.connectors) {
        auto itConnector = nodes.find(connectorIndex);
        if (itConnector == nodes.end()) continue;
        if (count % itConnector->second.countLimit == 0)
            result.push_back(connectorIndex);
    }

    return result;
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
    switch (state) {
        case TraversalState::Start: {
            state    = TraversalState::Active;
            targetId = rootId;
            audioProcessor->eventManager.highlightNode(getTargetNode(nodes), true);
            break;
        }
        case TraversalState::Active: {
            advance(nodes);

            switch (state) {
                case TraversalState::Active: {
                    audioProcessor->eventManager.highlightNode(getLastNode(nodes),   false);
                    audioProcessor->eventManager.highlightNode(getTargetNode(nodes),  true);
                    break;
                }
                case TraversalState::Reset: {
                    audioProcessor->eventManager.highlightNode(getTargetNode(nodes), false);
                    audioProcessor->eventManager.highlightNode(getRootNode(nodes),    true);
                    targetId = rootId;
                    state    = TraversalState::Active;
                    break;
                }
                case TraversalState::End: {
                    audioProcessor->eventManager.highlightNode(getTargetNode(nodes),    false);
                    audioProcessor->eventManager.highlightNode(getReferenceNode(nodes), false);
                    break;
                }
                default: break;
            }
            break;
        }
        case TraversalState::End: {
            audioProcessor->eventManager.highlightNode(getReferenceNode(nodes), false);
            audioProcessor->eventManager.highlightNode(getTargetNode(nodes),    false);
            break;
        }
        default: break;
    }
}

void TraversalLogic::handleRelayNodeEvent(int relayNodeId, const NodeMap& nodes) const
{
    audioProcessor->eventManager.highlightNode(nodes.at(relayNodeId), false);
}