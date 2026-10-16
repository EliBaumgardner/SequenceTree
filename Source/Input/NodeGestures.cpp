#include "NodeGestures.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include "../UI/Node/Encapsulator.h"
#include "../Graph/NodeFactory.h"
#include "../Graph/GraphState.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Theme/Theme.h>

NodeGestures::NodeGestures(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager)
    : nodeCanvas(nodeCanvas),
      graphState(graphState),
      undoManager(undoManager)
{
}

void NodeGestures::createRootNode(juce::Point<float> clickPoint)
{
    NodePosition nodePosition;

    nodePosition.xPosition = juce::roundToInt(clickPoint.x);
    nodePosition.yPosition = juce::roundToInt(clickPoint.y);
    nodePosition.radius    = Theme::nodeRadius;

    undoManager.beginNewTransaction();

    NodeFactory::createRootNode(graphState, nodePosition, &undoManager);
}

void NodeGestures::respondToDrag(Node& node, juce::Point<int> cursor, const juce::ModifierKeys& mods)
{
    const int nodeId = node.nodeId;

    NodePosition newPosition;

    newPosition.xPosition = cursor.x;
    newPosition.yPosition = cursor.y;
    newPosition.radius    = Theme::nodeRadius;

    if (! mods.isShiftDown() && ! draggedNodeTree.isValid()) {
        handleNodeDrag(nodeId, newPosition);
        return;
    }

    if (isDragStart) {
        isDragStart = false;

        handleNodeDragStart(&node, nodeId, newPosition, mods);
        return;
    }

    if (draggedNodeTree.isValid()) {
        snapToGrid(newPosition, draggedNodeTree);
        checkRootNodeSnap(cursor);
    }
}

void NodeGestures::finishDrag()
{
    if (snapTargetRoot != nullptr) {
        connectDraggedNodeToRoot();
    }
    else if (draggedNodeTree.isValid()) {
        nodeCanvas.arrowManager.triggerSnapForNode(static_cast<int>(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id)));
    }

    draggedNodeTree  = juce::ValueTree();
    isDragStart      = true;
    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    nodeCanvas.hideGrid();

    nodeCanvas.arrowManager.endNodeDrag();
}

void NodeGestures::dragArrowHead(juce::Point<int> cursor)
{
    NodePosition newPosition;

    if (arrowHeadNode == nullptr) {
        return;
    }

    newPosition.xPosition = cursor.x;
    newPosition.yPosition = cursor.y;
    newPosition.radius    = Theme::nodeRadius;

    handleNodeDrag(arrowHeadNode->nodeId, newPosition);
}

void NodeGestures::finishArrowHeadDrag()
{
    const Node* const headNode = arrowHeadNode;

    arrowHeadNode = nullptr;
    isDragStart   = true;

    nodeCanvas.arrowManager.endNodeDrag();

    if (headNode == nullptr) {
        return;
    }

    nodeCanvas.arrowManager.triggerSnapForNode(headNode->nodeId);
}

void NodeGestures::handleNodeDrag(int nodeId, NodePosition newPosition)
{
    if (isDragStart) {
        isDragStart                           = false;
        nodeCanvas.arrowManager.nodesDragging = true;

        undoManager.beginNewTransaction();

        nodeCanvas.showGrid();
    }

    juce::ValueTree nodeValueTree = graphState.getNode(nodeId);
    NodePosition    oldPosition   = graphState.getNodePosition(nodeId);

    snapToGrid(newPosition, nodeValueTree);

    int deltaX = newPosition.xPosition - oldPosition.xPosition;
    int deltaY = newPosition.yPosition - oldPosition.yPosition;

    nodeCanvas.nodeManager.moveDescendants(nodeValueTree, deltaX, deltaY);
}

void NodeGestures::handleNodeDragStart(Node* node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods)
{
    int parentNodeId = nodeId;

    auto* const collapsedEncapsulator = dynamic_cast<Encapsulator*>(node);

    if (collapsedEncapsulator != nullptr && ! collapsedEncapsulator->memberNodeIds.empty()) {
        parentNodeId = collapsedEncapsulator->memberNodeIds.back();
    }

    Node* const parentNode = nodeCanvas.nodeManager.find(parentNodeId);

    if (parentNode == nullptr) {
        return;
    }

    const juce::Identifier nodeType = parentNode->nodeValueTree.getType();

    undoManager.beginNewTransaction();

    nodeCanvas.showGrid();

    snapSourceNodeId                      = parentNodeId;
    nodeCanvas.arrowManager.nodesDragging = true;

    const NodePosition     parentPosition = graphState.getNodePosition(parentNodeId);
    const juce::Point<int> parentCentre   = parentNode->getNodeCentre();

    newPosition.xPosition += parentPosition.xPosition - parentCentre.x;
    newPosition.yPosition += parentPosition.yPosition - parentCentre.y;

    draggedNodeTree = NodeFactory::create(creationMode, graphState, parentNodeId, nodeType, mods.isCtrlDown() && mods.isShiftDown(), newPosition, &undoManager);

    if (! draggedNodeTree.isValid()) {
        return;
    }

    NodeFactory::applyArrowInfo(graphState, parentNodeId, draggedNodeTree.getProperty(ValueTreeIdentifiers::Id), nodeCanvas.arrowManager.currentArrowInfo,
                                ArrowType::StepIntoTree, &undoManager);

    const int owningEncapsulatorId = parentNode->nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(nodeCanvas.nodeManager.find(owningEncapsulatorId));

    if (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded || owningEncapsulator->memberNodeIds.empty()) {
        return;
    }

    if (parentNodeId == owningEncapsulator->memberNodeIds.back()) {
        return;
    }

    graphState.encapsulation.insertNodeAfter(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id), parentNodeId, &undoManager);
}

void NodeGestures::snapToGrid(NodePosition& newPosition, juce::ValueTree draggedNodeTree)
{
    const int              draggedNodeId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);
    const juce::Point<int> collapseShift = nodeCanvas.encapsulationView.collapsedSpanShift(draggedNodeId);
    const juce::Point<int> snapped       = nodeCanvas.snapPointToGrid({ newPosition.xPosition - collapseShift.x, newPosition.yPosition - collapseShift.y });

    newPosition.xPosition = snapped.x;
    newPosition.yPosition = snapped.y;

    graphState.setNodePosition(draggedNodeTree, newPosition, &undoManager);
}

void NodeGestures::checkRootNodeSnap(juce::Point<int> canvasPoint)
{
    if (snapSourceNodeId < 0 || ! draggedNodeTree.isValid()) {
        return;
    }

    const juce::Identifier sourceType = graphState.getNode(snapSourceNodeId).getType();

    if (sourceType == ValueTreeIdentifiers::ModulatorData
        || sourceType == ValueTreeIdentifiers::ModulatorRootData
        || sourceType == ValueTreeIdentifiers::AlternativeModulatorData) {
        return;
    }

    Node* nearestRoot = nodeCanvas.hitTester.rootNear(canvasPoint.toFloat(), rootSnapThreshold, snapSourceNodeId);

    if (nearestRoot != nullptr) {
        if (snapTargetRoot != nearestRoot) {
            snapTargetRoot = nearestRoot;

            setDraggedNodeVisible(false);

            Node* sourceNode = nodeCanvas.nodeManager.find(snapSourceNodeId);

            if (sourceNode != nullptr) {
                nodeCanvas.arrowManager.showSnapGhost(sourceNode, nearestRoot);
            }
        }
    }
    else if (snapTargetRoot != nullptr) {
        snapTargetRoot = nullptr;

        setDraggedNodeVisible(true);

        nodeCanvas.arrowManager.hideSnapGhost();
    }
}

void NodeGestures::connectDraggedNodeToRoot()
{
    const int rootNodeId    = snapTargetRoot->nodeId;
    const int parentNodeId  = snapSourceNodeId;
    const int draggedNodeId = static_cast<int>(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id));

    nodeCanvas.arrowManager.hideSnapGhost();

    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    undoManager.undo();

    nodeCanvas.cancelPendingUpdatesFor(draggedNodeId);

    undoManager.beginNewTransaction();

    graphState.connectNodes(parentNodeId, rootNodeId, &undoManager);

    NodeFactory::applyArrowInfo(graphState, parentNodeId, rootNodeId, nodeCanvas.arrowManager.currentArrowInfo, ArrowType::CrossRootTree, &undoManager);

    nodeCanvas.arrowManager.connectWithSnapAnimation(parentNodeId, rootNodeId);
}

void NodeGestures::setDraggedNodeVisible(bool shouldBeVisible)
{
    const int draggedId   = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);
    Node*     draggedNode = nodeCanvas.nodeManager.find(draggedId);

    if (draggedNode == nullptr) {
        return;
    }

    draggedNode->setVisible(shouldBeVisible);

    Node* sourceNode = nodeCanvas.nodeManager.find(snapSourceNodeId);

    if (sourceNode == nullptr) {
        return;
    }

    auto arrowIt = sourceNode->nodeArrows.find(draggedId);

    if (arrowIt != sourceNode->nodeArrows.end()) {
        arrowIt->second->setVisible(shouldBeVisible);
    }
}
