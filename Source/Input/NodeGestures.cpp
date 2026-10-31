#include "NodeGestures.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "GraphDocument.h"

NodeGestures::NodeGestures(NodeCanvas& nodeCanvas, GraphDocument& document, juce::UndoManager& undoManager)
    : nodeCanvas(nodeCanvas),
      document(document),
      undoManager(undoManager)
{
}

void NodeGestures::createRootNode(juce::Point<float> clickPoint)
{
    undoManager.beginNewTransaction();

    document.addRootNode(clickPoint.roundToInt());
}

void NodeGestures::respondToDrag(Node& node, juce::Point<int> cursor, const juce::ModifierKeys& mods)
{
    const int nodeId = node.nodeId;

    if ((! mods.isShiftDown() || document.growFrom(nodeId) < 0) && ! draggedNodeTree.isValid()) {
        handleNodeDrag(nodeId, cursor);
        return;
    }

    if (isDragStart) {
        isDragStart = false;

        handleNodeDragStart(nodeId, cursor, mods);
        return;
    }

    if (draggedNodeTree.isValid()) {
        document.moveNode(draggedNodeTree, snapToGrid(cursor, draggedNodeTree));

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
    if (arrowHeadNode == nullptr) {
        return;
    }

    handleNodeDrag(arrowHeadNode->nodeId, cursor);
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

void NodeGestures::handleNodeDrag(int nodeId, juce::Point<int> cursor)
{
    const juce::ValueTree nodeTree = document.findNode(nodeId);

    if (isDragStart) {
        isDragStart                           = false;
        nodeCanvas.arrowManager.nodesDragging = true;

        undoManager.beginNewTransaction();

        nodeCanvas.showGrid();
    }

    document.moveNode(nodeTree, snapToGrid(cursor, nodeTree));
}

void NodeGestures::handleNodeDragStart(int nodeId, juce::Point<int> cursor, const juce::ModifierKeys& mods)
{
    const int   parentNodeId = document.growFrom(nodeId);
    Node* const parentNode   = nodeCanvas.nodeManager.find(parentNodeId);

    if (parentNode == nullptr) {
        return;
    }

    undoManager.beginNewTransaction();

    nodeCanvas.showGrid();

    snapSourceNodeId                      = parentNodeId;
    nodeCanvas.arrowManager.nodesDragging = true;

    const juce::Point<int> parentPosition { parentNode->nodeValueTree.getProperty(ValueTreeIdentifiers::XPosition), parentNode->nodeValueTree.getProperty(ValueTreeIdentifiers::YPosition) };
    const juce::Point<int> parentCentre   = parentNode->getNodeCentre();

    draggedNodeTree = document.addChildNode(parentNodeId, cursor + parentPosition - parentCentre, mods);

    if (draggedNodeTree.isValid() && onChildCreated != nullptr) {
        onChildCreated(parentNodeId, draggedNodeTree);
    }
}


void NodeGestures::checkRootNodeSnap(juce::Point<int> canvasPoint)
{
    if (snapSourceNodeId < 0 || ! draggedNodeTree.isValid()) {
        return;
    }

    Node* const sourceNode  = nodeCanvas.nodeManager.find(snapSourceNodeId);
    Node*       nearestRoot = nodeCanvas.hitTester.rootNear(canvasPoint.toFloat(), rootSnapThreshold, snapSourceNodeId);

    if (sourceNode == nullptr) {
        return;
    }

    if (nearestRoot != nullptr && ! document.canConnect(*sourceNode, *nearestRoot)) {
        nearestRoot = nullptr;
    }

    if (nearestRoot != nullptr) {
        if (snapTargetRoot != nearestRoot) {
            snapTargetRoot = nearestRoot;

            setDraggedNodeVisible(false);

            nodeCanvas.arrowManager.showSnapGhost(sourceNode, nearestRoot);
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

    document.connect(parentNodeId, rootNodeId, CanvasDocument::ConnectionGesture::NodeOntoRoot);

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

juce::Point<int> NodeGestures::snapToGrid(juce::Point<int> position, const juce::ValueTree& nodeTree) const
{
    const juce::Point<int> collapseShift = nodeCanvas.encapsulationView.collapsedSpanShift(nodeTree.getProperty(ValueTreeIdentifiers::Id));

    return nodeCanvas.snapPointToGrid(position - collapseShift);
}
