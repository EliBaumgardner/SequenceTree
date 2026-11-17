#include "GraphDocument.h"
#include "Processor/Document/TypedProperties.h"
#include "Editor/Nodes/Node.h"
#include "Processor/Document/GraphState.h"
#include "Processor/Document/ArrowBindingOps.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include <ArsNovaUI/Theme/Theme.h>

GraphDocument::GraphDocument(GraphState& graphState, juce::UndoManager& undoManager, juce::ValueTree canvasSettings)
    : ArsNovaUI::CanvasDocument(graphState.nodeMap, undoManager),
      graphState    (graphState),
      canvasSettings(canvasSettings),
      nodeFactory   (graphState, &undoManager)
{
}

juce::ValueTree GraphDocument::findNode(int nodeId) const
{
    return graphState.getNode(nodeId);
}

juce::ValueTree GraphDocument::addChildNode(int parentNodeId, juce::Point<int> position, const juce::ModifierKeys& mods)
{
    const bool makeAlternative = mods.isCtrlDown() && mods.isShiftDown();

    const NodeInfo::Position nodePosition { position.x, position.y, ArsNovaUI::Theme::nodeRadius };

    const juce::ValueTree child = nodeFactory.create(creationMode, parentNodeId, makeAlternative, nodePosition);

    if (child.isValid()) {
        const ArrowInfo::Settings arrowInfo = ArrowBindingOps::getArrowInfo(canvasSettings);

        nodeFactory.applyArrowInfo(parentNodeId, child.getProperty(ArsNovaUI::CanvasIdentifiers::Id), arrowInfo, ArrowInfo::Type::StepIntoTree);
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

    nodeFactory.createRootNode(nodePosition);
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
    const int nodeId = nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

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
    const ArrowInfo::Settings arrowInfo = ArrowBindingOps::getArrowInfo(canvasSettings);

    nodeFactory.createDanglingArrow(nodeTree, tipOffset, arrowInfo);
}

void GraphDocument::setDanglingArrowTip(juce::ValueTree arrowTree, juce::Point<int> tipOffset)
{
    nodeFactory.setDanglingArrowTip(arrowTree, tipOffset);
}

void GraphDocument::removeDanglingArrow(juce::ValueTree arrowTree)
{
    nodeFactory.destroyDanglingArrow(arrowTree);
}

void GraphDocument::connect(int parentNodeId, int childNodeId, ConnectionGesture gesture)
{
    const ArrowInfo::Settings arrowInfo = ArrowBindingOps::getArrowInfo(canvasSettings);

    ArrowInfo::Type rootConnectionType = ArrowInfo::Type::StepIntoTree;

    if (gesture == ConnectionGesture::NodeOntoRoot) {
        rootConnectionType = ArrowInfo::Type::CrossRootTree;
    }

    graphState.connectNodes(parentNodeId, childNodeId, &undoManager);

    nodeFactory.applyArrowInfo(parentNodeId, childNodeId, arrowInfo, rootConnectionType);
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

bool GraphDocument::canConnect(const ArsNovaUI::CanvasNode& startNode, const ArsNovaUI::CanvasNode& endNode) const
{
    const bool startsOnModulator = NodeInfo::kindTraits[static_cast<int>(GraphState::kindOf(startNode.nodeValueTree))].isModulator;

    if (startNode.nodeArrows.count(endNode.nodeId) > 0) {
        return false;
    }

    return ! (startsOnModulator && endNode.nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData);
}

GraphDocument::ArrowOwnership GraphDocument::ownershipOf(int startNodeId, int endNodeId) const
{
    const juce::ValueTree startChildren = graphState.getNode(startNodeId).getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

    if (startChildren.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, endNodeId).isValid()) {
        return { startNodeId, endNodeId };
    }

    return { endNodeId, startNodeId };
}

void GraphDocument::moveDescendants(juce::ValueTree nodeTree, int deltaX, int deltaY, std::unordered_set<int>& visited, int draggedNodeId)
{
    juce::Identifier childIdListType = ArsNovaUI::CanvasIdentifiers::NodeChildrenIds;

    if (nodeTree.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        childIdListType = ValueTreeIdentifiers::EncapsulatedIds;
    }

    const juce::ValueTree childIds = nodeTree.getChildWithName(childIdListType);

    for (int childIndex = 0; childIndex < childIds.getNumChildren(); ++childIndex) {
        const int childId = childIds.getChild(childIndex).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

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
    const NodeProperties properties(graphState.getNode(nodeId));

    const int encapsulatorId = properties.encapsulatorId;

    const juce::ValueTree encapsulator = graphState.getNode(encapsulatorId);

    const std::vector<int> memberNodeIds = graphState.encapsulation.memberIds(encapsulatorId);

    if (memberNodeIds.empty() || encapsulatorId == draggedNodeId || memberNodeIds.front() != nodeId) {
        return;
    }

    NodeInfo::Position encapsulatorPosition = graphState.getNodePosition(encapsulatorId);

    encapsulatorPosition.xPosition += deltaX;
    encapsulatorPosition.yPosition += deltaY;

    graphState.setNodePosition(encapsulator, encapsulatorPosition, &undoManager);
}
