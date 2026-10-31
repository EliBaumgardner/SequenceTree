#include "NodeGestures.h"
#include "../UI/Canvas/EditCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "CanvasDocument.h"

NodeGestures::NodeGestures(EditCanvas& canvas, CanvasDocument& document, juce::UndoManager& undoManager)
    : canvas(canvas),
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
        canvas.arrowManager.triggerSnapForNode(static_cast<int>(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id)));
    }

    draggedNodeTree  = juce::ValueTree();
    isDragStart      = true;
    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    canvas.hideGrid();

    canvas.arrowManager.endNodeDrag();
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

    canvas.arrowManager.endNodeDrag();

    if (headNode == nullptr) {
        return;
    }

    canvas.arrowManager.triggerSnapForNode(headNode->nodeId);
}

void NodeGestures::handleNodeDrag(int nodeId, juce::Point<int> cursor)
{
    const juce::ValueTree nodeTree = document.findNode(nodeId);

    if (isDragStart) {
        isDragStart                           = false;
        canvas.arrowManager.nodesDragging = true;

        undoManager.beginNewTransaction();

        canvas.showGrid();
    }

    document.moveNode(nodeTree, snapToGrid(cursor, nodeTree));
}

void NodeGestures::handleNodeDragStart(int nodeId, juce::Point<int> cursor, const juce::ModifierKeys& mods)
{
    const int   parentNodeId = document.growFrom(nodeId);
    Node* const parentNode   = canvas.nodeManager.find(parentNodeId);

    if (parentNode == nullptr) {
        return;
    }

    undoManager.beginNewTransaction();

    canvas.showGrid();

    snapSourceNodeId                      = parentNodeId;
    canvas.arrowManager.nodesDragging = true;

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

    Node* const sourceNode  = canvas.nodeManager.find(snapSourceNodeId);
    Node*       nearestRoot = canvas.hitTester.rootNear(canvasPoint.toFloat(), rootSnapThreshold, snapSourceNodeId);

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

            canvas.arrowManager.showSnapGhost(sourceNode, nearestRoot);
        }
    }
    else if (snapTargetRoot != nullptr) {
        snapTargetRoot = nullptr;

        setDraggedNodeVisible(true);

        canvas.arrowManager.hideSnapGhost();
    }
}

void NodeGestures::connectDraggedNodeToRoot()
{
    const int rootNodeId    = snapTargetRoot->nodeId;
    const int parentNodeId  = snapSourceNodeId;
    const int draggedNodeId = static_cast<int>(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id));

    canvas.arrowManager.hideSnapGhost();

    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    undoManager.undo();

    canvas.cancelPendingUpdatesFor(draggedNodeId);

    undoManager.beginNewTransaction();

    document.connect(parentNodeId, rootNodeId, CanvasDocument::ConnectionGesture::NodeOntoRoot);

    canvas.arrowManager.connectWithSnapAnimation(parentNodeId, rootNodeId);
}

void NodeGestures::setDraggedNodeVisible(bool shouldBeVisible)
{
    const int draggedId   = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);
    Node*     draggedNode = canvas.nodeManager.find(draggedId);

    if (draggedNode == nullptr) {
        return;
    }

    draggedNode->setVisible(shouldBeVisible);

    Node* sourceNode = canvas.nodeManager.find(snapSourceNodeId);

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
    const juce::Point<int> storedPosition { nodeTree.getProperty(ValueTreeIdentifiers::XPosition), nodeTree.getProperty(ValueTreeIdentifiers::YPosition) };

    return canvas.snapPointToGrid(position - (canvas.drawnPosition(nodeTree) - storedPosition));
}
