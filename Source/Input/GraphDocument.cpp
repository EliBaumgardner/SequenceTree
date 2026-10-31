#include "GraphDocument.h"
#include "../UI/Node/Node.h"
#include "../Graph/GraphState.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Theme/Theme.h>

GraphDocument::GraphDocument(GraphState& graphState, juce::UndoManager& undoManager)
    : CanvasDocument(graphState.nodeMap, undoManager), graphState(graphState)
{
}

juce::ValueTree GraphDocument::findNode(int nodeId) const
{
    return graphState.getNode(nodeId);
}

juce::ValueTree GraphDocument::addChildNode(int parentNodeId, juce::Point<int> position, const juce::ModifierKeys& mods)
{
    const juce::Identifier   parentType   = graphState.getNode(parentNodeId).getType();
    const NodeInfo::Position nodePosition { position.x, position.y, ArsNovaUI::Theme::nodeRadius };
    const juce::ValueTree    child        = NodeFactory::create(creationMode, graphState, parentNodeId, parentType, mods.isCtrlDown() && mods.isShiftDown(), nodePosition, &undoManager);

    if (child.isValid()) {
        NodeFactory::applyArrowInfo(graphState, parentNodeId, child.getProperty(ValueTreeIdentifiers::Id), currentArrowInfo, ArrowInfo::Type::StepIntoTree, &undoManager);
    }

    return child;
}

int GraphDocument::growFrom(int nodeId) const
{
    const juce::ValueTree  nodeTree  = graphState.getNode(nodeId);
    const std::vector<int> memberIds = graphState.encapsulation.memberIds(nodeId);

    if (nodeTree.getType() == ValueTreeIdentifiers::EncapsulatorData && ! memberIds.empty()) {
        return memberIds.back();
    }

    return nodeId;
}

void GraphDocument::addRootNode(juce::Point<int> position)
{
    const NodeInfo::Position nodePosition { position.x, position.y, ArsNovaUI::Theme::nodeRadius };

    NodeFactory::createRootNode(graphState, nodePosition, &undoManager);
}

static std::unordered_set<int> collectAncestorIds(const GraphState& graphState, int nodeId)
{
    std::unordered_set<int> ancestors;
    std::vector<int>        frontier { nodeId };

    while (! frontier.empty()) {
        const int currentId = frontier.back();

        frontier.pop_back();

        const auto parents = graphState.parentIdsOf.find(currentId);

        if (parents == graphState.parentIdsOf.end()) {
            continue;
        }

        for (const int parentId : parents->second) {
            if (parentId == nodeId) {
                continue;
            }

            if (ancestors.insert(parentId).second) {
                frontier.push_back(parentId);
            }
        }
    }

    return ancestors;
}

void GraphDocument::moveNode(juce::ValueTree nodeTree, juce::Point<int> position)
{
    const int nodeId = nodeTree.getProperty(ValueTreeIdentifiers::Id);

    const NodeInfo::Position oldPosition = graphState.getNodePosition(nodeId);
    const NodeInfo::Position newPosition { position.x, position.y, ArsNovaUI::Theme::nodeRadius };

    const int deltaX = newPosition.xPosition - oldPosition.xPosition;
    const int deltaY = newPosition.yPosition - oldPosition.yPosition;

    std::unordered_set<int> visited = collectAncestorIds(graphState, nodeId);

    graphState.setNodePosition(nodeTree, newPosition, &undoManager);

    visited.insert(nodeId);

    moveEncapsulatorWithEntryMember(nodeId, nodeId, deltaX, deltaY);

    moveDescendants(nodeTree, deltaX, deltaY, visited, nodeId);
}

void GraphDocument::removeNode(int nodeId)
{
    graphState.removeNode(nodeId, &undoManager);
}

void GraphDocument::addDanglingArrow(juce::ValueTree nodeTree, juce::Point<int> tipOffset)
{
    NodeFactory::createDanglingArrow(graphState, nodeTree, tipOffset, currentArrowInfo, &undoManager);
}

void GraphDocument::setDanglingArrowTip(juce::ValueTree arrowTree, juce::Point<int> tipOffset)
{
    NodeFactory::setDanglingArrowTip(graphState, arrowTree, tipOffset, &undoManager);
}

void GraphDocument::removeDanglingArrow(juce::ValueTree arrowTree)
{
    NodeFactory::destroyDanglingArrow(arrowTree, &undoManager);
}

void GraphDocument::connect(int parentNodeId, int childNodeId, ConnectionGesture gesture)
{
    ArrowInfo::Type rootConnectionType = ArrowInfo::Type::StepIntoTree;

    if (gesture == ConnectionGesture::NodeOntoRoot) {
        rootConnectionType = ArrowInfo::Type::CrossRootTree;
    }

    graphState.connectNodes(parentNodeId, childNodeId, &undoManager);

    NodeFactory::applyArrowInfo(graphState, parentNodeId, childNodeId, currentArrowInfo, rootConnectionType, &undoManager);
}

void GraphDocument::disconnect(int startNodeId, int endNodeId)
{
    const auto [ownerNodeId, childNodeId] = ownershipOf(startNodeId, endNodeId);

    if (graphState.getNode(childNodeId).getType() == ValueTreeIdentifiers::TraversalFlagData) {
        graphState.removeNode(childNodeId, &undoManager);
        return;
    }

    graphState.disconnectNodes(ownerNodeId, childNodeId, &undoManager);
}

bool GraphDocument::canConnect(const Node& startNode, const Node& endNode) const
{
    const juce::Identifier startType = startNode.nodeValueTree.getType();

    const bool startsOnModulator = startType == ValueTreeIdentifiers::ModulatorData
                                || startType == ValueTreeIdentifiers::ModulatorRootData
                                || startType == ValueTreeIdentifiers::AlternativeModulatorData;

    if (startNode.nodeArrows.count(endNode.nodeId) > 0) {
        return false;
    }

    return ! (startsOnModulator && endNode.nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData);
}

GraphDocument::ArrowOwnership GraphDocument::ownershipOf(int startNodeId, int endNodeId) const
{
    const juce::ValueTree startChildren = graphState.getNode(startNodeId).getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    if (startChildren.getChildWithProperty(ValueTreeIdentifiers::Id, endNodeId).isValid()) {
        return { startNodeId, endNodeId };
    }

    return { endNodeId, startNodeId };
}

void GraphDocument::moveDescendants(juce::ValueTree nodeTree, int deltaX, int deltaY, std::unordered_set<int>& visited, int draggedNodeId)
{
    juce::Identifier childIdListType = ValueTreeIdentifiers::NodeChildrenIds;

    if (nodeTree.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        childIdListType = ValueTreeIdentifiers::EncapsulatedIds;
    }

    const juce::ValueTree childIds = nodeTree.getChildWithName(childIdListType);

    for (int childIndex = 0; childIndex < childIds.getNumChildren(); ++childIndex) {
        const int childId = childIds.getChild(childIndex).getProperty(ValueTreeIdentifiers::Id);

        if (! visited.insert(childId).second) {
            continue;
        }

        const juce::ValueTree childNodeTree = graphState.getNode(childId);
        NodeInfo::Position    childPosition = graphState.getNodePosition(childId);

        childPosition.xPosition += deltaX;
        childPosition.yPosition += deltaY;

        graphState.setNodePosition(childNodeTree, childPosition, &undoManager);

        moveEncapsulatorWithEntryMember(childId, draggedNodeId, deltaX, deltaY);

        moveDescendants(childNodeTree, deltaX, deltaY, visited, draggedNodeId);
    }
}

void GraphDocument::moveEncapsulatorWithEntryMember(int nodeId, int draggedNodeId, int deltaX, int deltaY)
{
    const int              encapsulatorId = graphState.getNode(nodeId).getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);
    const juce::ValueTree  encapsulator   = graphState.getNode(encapsulatorId);
    const std::vector<int> memberNodeIds  = graphState.encapsulation.memberIds(encapsulatorId);

    if (memberNodeIds.empty() || encapsulatorId == draggedNodeId || memberNodeIds.front() != nodeId) {
        return;
    }

    NodeInfo::Position encapsulatorPosition = graphState.getNodePosition(encapsulatorId);

    encapsulatorPosition.xPosition += deltaX;
    encapsulatorPosition.yPosition += deltaY;

    graphState.setNodePosition(encapsulator, encapsulatorPosition, &undoManager);
}
