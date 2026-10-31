#include "ArrowGestures.h"
#include "../UI/Canvas/EditCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include "CanvasDocument.h"

ArrowGestures::ArrowGestures(EditCanvas& canvas, CanvasDocument& document, juce::UndoManager& undoManager)
    : canvas(canvas),
      document(document),
      undoManager(undoManager)
{
}

void ArrowGestures::beginMovingTip(Arrow& danglingArrow)
{
    draggingDanglingArrow = &danglingArrow;
    tipGesture            = TipGesture::MovingDanglingTip;

    canvas.showGrid();
}

void ArrowGestures::dragTip(juce::Point<int> cursor)
{
    Node* sourceNode = newArrowSourceNode;

    if (tipGesture == TipGesture::MovingDanglingTip && draggingDanglingArrow == nullptr) {
        return;
    }

    if (tipGesture == TipGesture::MovingDanglingTip) {
        sourceNode = draggingDanglingArrow->startNode;
    }

    if (sourceNode == nullptr) {
        return;
    }

    tipSnapTarget = findTipSnapTarget(*sourceNode, cursor);

    juce::Point <int> tip = canvas.snapPointToGrid(cursor);
    const juce::Point<int> centre = sourceNode->getNodeCentre();

    if (tipSnapTarget != nullptr) {
        tip = tipSnapTarget->getNodeCentre();
    }

    if (tipGesture == TipGesture::MovingDanglingTip) {
        draggingDanglingArrow->dragging = true;

        draggingDanglingArrow->setTipOffset({ tip.x - centre.x, tip.y - centre.y });
        return;
    }

    if (tipGesture == TipGesture::CreatingDanglingArrow) {
        canvas.showGrid();
    }

    canvas.arrowManager.updatePreview(sourceNode, { tip.x - centre.x, tip.y - centre.y });
}

void ArrowGestures::finishTip()
{
    const TipGesture finishedGesture = tipGesture;
    Node* const      targetNode      = tipSnapTarget;
    Arrow*           draggedArrow    = canvas.arrowManager.preview.get();

    if (finishedGesture == TipGesture::MovingDanglingTip) {
        draggedArrow = draggingDanglingArrow;
    }

    tipGesture            = TipGesture::None;
    draggingDanglingArrow = nullptr;
    newArrowSourceNode    = nullptr;
    tipSnapTarget         = nullptr;

    canvas.hideGrid();

    if (draggedArrow == nullptr) {
        return;
    }

    Node* const sourceNode = draggedArrow->startNode;

    draggedArrow->endDrag();

    if (targetNode == nullptr && finishedGesture == TipGesture::MovingDanglingTip) {
        undoManager.beginNewTransaction();

        document.setDanglingArrowTip(draggedArrow->arrowTree, draggedArrow->tipOffset);
        return;
    }

    if (targetNode == nullptr && finishedGesture != TipGesture::ConnectingFlag) {
        undoManager.beginNewTransaction();

        canvas.arrowManager.commitPreview();
        return;
    }

    canvas.arrowManager.preview.reset();

    if (targetNode != nullptr && sourceNode != nullptr && sourceNode->nodeArrows.count(targetNode->nodeId) == 0) {
        undoManager.beginNewTransaction();

        document.connect(sourceNode->nodeId, targetNode->nodeId, CanvasDocument::ConnectionGesture::TipOntoNode);

        canvas.arrowManager.connectWithSnapAnimation(sourceNode->nodeId, targetNode->nodeId);
    }

    if (targetNode != nullptr && finishedGesture == TipGesture::MovingDanglingTip) {
        document.removeDanglingArrow(draggedArrow->arrowTree);
    }
}

void ArrowGestures::updateHover(juce::Point<float> cursor)
{
    Arrow* const hoveredArrow = canvas.hitTester.arrowNear(cursor, arrowHoverRadius);

    for (Arrow* arrow : canvas.arrowManager.all()) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr) {
            continue;
        }

        if (arrow->startNode->nodeType != NodeInfo::Type::TraversalFlag) {
            continue;
        }

        const float dist   = CanvasHitTester::distanceToSegment(cursor, arrow->startNode->getNodeCentre().toFloat(), arrow->endNode->getNodeCentre().toFloat());
        const bool  nearby = dist < flagProximityRadius;

        if (nearby != arrow->proximityHovered) {
            arrow->proximityHovered = nearby;

            arrow->setHoverFade(arrow->sourceHovered || arrow->proximityHovered);
        }
    }

    for (Arrow* arrow : canvas.arrowManager.all()) {
        const bool shouldBold = (arrow == hoveredArrow);

        if (shouldBold != arrow->hovered) {
            arrow->hovered = shouldBold;

            arrow->repaint();
        }
    }
}

void ArrowGestures::removeArrow(Arrow& arrow)
{
    if (arrow.isDangling()) {
        undoManager.beginNewTransaction();

        document.removeDanglingArrow(arrow.arrowTree);
        return;
    }

    if (arrow.startNode == nullptr || arrow.endNode == nullptr) {
        return;
    }

    undoManager.beginNewTransaction();

    document.disconnect(arrow.startNode->nodeId, arrow.endNode->nodeId);
}


void ArrowGestures::setArrowMode(bool enabled)
{
    arrowMode = enabled;

    if (! enabled) {
        canvas.arrowManager.preview.reset();
    }
}

bool ArrowGestures::beginTipFromNode(Node& node)
{
    const int growthNodeId = document.growFrom(node.nodeId);

    if (node.nodeType == NodeInfo::Type::TraversalFlag && ! arrowMode) {
        tipGesture         = TipGesture::ConnectingFlag;
        newArrowSourceNode = &node;
        return true;
    }

    if (! arrowMode || growthNodeId < 0) {
        return false;
    }

    tipGesture         = TipGesture::CreatingDanglingArrow;
    newArrowSourceNode = canvas.nodeManager.find(growthNodeId);

    return true;
}

Node* ArrowGestures::findTipSnapTarget(const Node& startNode, juce::Point<int> tip) const
{
    const int startNodeId = startNode.nodeId;

    if (tipGesture == TipGesture::ConnectingFlag) {
        return canvas.hitTester.nodeNear(tip.toFloat(), flagSnapThreshold, startNodeId);
    }

    Node* const snapTarget = canvas.hitTester.nodeContaining(tip.toFloat(), startNodeId);

    if (snapTarget == nullptr || ! document.canConnect(startNode, *snapTarget)) {
        return nullptr;
    }

    return snapTarget;
}