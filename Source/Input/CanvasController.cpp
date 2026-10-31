#include "CanvasController.h"
#include "CanvasDocument.h"
#include "../UI/Canvas/EditCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include <ArsNovaUI/Canvas/DynamicPort.h>

CanvasController::CanvasController(EditCanvas& canvas, CanvasDocument& document, juce::UndoManager& undoManager)
    : arrowGestures(canvas, document, undoManager),
      nodeGestures(canvas, document, undoManager),
      canvas(canvas),
      document(document),
      undoManager(undoManager)
{
}

void CanvasController::mouseDown(const juce::MouseEvent& e)
{
    gesture           = Gesture::Idle;
    draggingValueNode = nullptr;

    if (dynamic_cast<EditCanvas*>(e.eventComponent) != nullptr) {
        handleCanvasMouseDown(e);
    }
    else if (Node* clickedNode = dynamic_cast<Node*>(e.eventComponent)) {
        handleNodeMouseDown(e, *clickedNode);
    }
}

void CanvasController::mouseUp(const juce::MouseEvent& e)
{
    Node* const clickedNode  = dynamic_cast<Node*>(e.eventComponent);
    const bool  isPlainClick = clickedNode != nullptr && gesture == Gesture::Idle && e.mods.isLeftButtonDown() && !e.mods.isAnyModifierKeyDown()
                               && e.getDistanceFromDragStart() < dragThreshold;

    if (gesture == Gesture::MovingArrowHead) {
        gesture = Gesture::Idle;

        nodeGestures.finishArrowHeadDrag();
        return;
    }

    if (gesture == Gesture::MovingDanglingTip || gesture == Gesture::DrawingArrow) {
        gesture = Gesture::Idle;

        arrowGestures.finishTip();
        return;
    }

    gesture           = Gesture::Idle;
    draggingValueNode = nullptr;

    nodeGestures.finishDrag();

    if (isPlainClick) {
        clickedNode->respondToClick(e.getEventRelativeTo(clickedNode).getPosition());
    }
}

void CanvasController::mouseEnter(const juce::MouseEvent& e)
{
    if (Node* node = dynamic_cast<Node*>(e.eventComponent)) {
        node->setHoverVisual(true);
    }
}

void CanvasController::mouseExit(const juce::MouseEvent& e)
{
    if (Node* node = dynamic_cast<Node*>(e.eventComponent)) {
        node->setHoverVisual(false);
    }
}

void CanvasController::mouseMove(const juce::MouseEvent& e)
{
    arrowGestures.updateHover(e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform));
}

void CanvasController::mouseDrag(const juce::MouseEvent& e)
{
    if (gesture == Gesture::EditingValue && draggingValueNode != nullptr) {
        dragValue(e);
        return;
    }

    if (gesture == Gesture::MovingDanglingTip) {
        arrowGestures.dragTip(e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt());
        return;
    }

    if (dynamic_cast<EditCanvas*>(e.eventComponent) != nullptr) {
        handleCanvasMouseDrag(e);
        return;
    }

    if (Node* draggedNode = dynamic_cast<Node*>(e.eventComponent)) {
        handleNodeMouseDrag(e, *draggedNode);
    }
}

void CanvasController::dragValue(const juce::MouseEvent& e)
{
    const int yOffset = e.getOffsetFromDragStart().y;
    const int delta   = -yOffset / 3;

    draggingValueNode->nodeValueEditor.setNumericValue(dragStartValue + delta);
    draggingValueNode->refreshValueDisplay();
}

void CanvasController::handleCanvasMouseDown(const juce::MouseEvent& e)
{
    const juce::Point<float> clickPoint = e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform);

    if (e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (Arrow* arrow = canvas.hitTester.arrowNear(clickPoint, ArrowGestures::danglingArrowGrabRadius)) {
            arrowGestures.removeArrow(*arrow);
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        return;
    }

    if (!e.mods.isShiftDown()) {
        if (Arrow* arrow = canvas.hitTester.danglingHeadNear(clickPoint, ArrowGestures::danglingArrowGrabRadius)) {
            gesture = Gesture::MovingDanglingTip;

            arrowGestures.beginMovingTip(*arrow);
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        if (Arrow* labelArrow = canvas.hitTester.arrowLabelNear(clickPoint, ArrowGestures::arrowLabelGrabRadius)) {
            gesture = Gesture::EditingValue;

            labelArrow->beginDurationEdit();
            return;
        }

        if (Arrow* headArrow = canvas.hitTester.arrowHeadNear(clickPoint, ArrowGestures::arrowHeadGrabRadius)) {
            gesture                    = Gesture::MovingArrowHead;
            nodeGestures.arrowHeadNode = headArrow->endNode;

            canvas.arrowManager.setSelected(headArrow);
            return;
        }

        if (Arrow* clickedArrow = canvas.hitTester.arrowNear(clickPoint, ArrowGestures::arrowHoverRadius)) {
            gesture = Gesture::ArrowSelected;

            canvas.arrowManager.setSelected(clickedArrow);
            return;
        }

        canvas.arrowManager.clearSelection();
        canvas.nodeManager.clearAll();
    }

    if (auto* dynamicPort = dynamic_cast<ArsNovaUI::DynamicPort*>(canvas.getParentComponent())) {
        dynamicPort->mouseDown(e.getEventRelativeTo(dynamicPort));
    }

    if (e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        nodeGestures.createRootNode(clickPoint);
    }
}

void CanvasController::handleCanvasMouseDrag(const juce::MouseEvent& e)
{
    const juce::Point<int> cursor = e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt();

    if (gesture == Gesture::MovingArrowHead) {
        if (e.getDistanceFromDragStart() >= dragThreshold) {
            nodeGestures.dragArrowHead(cursor);
        }
        return;
    }

    if (gesture == Gesture::ArrowSelected || gesture == Gesture::EditingValue) {
        return;
    }

    if (auto* dynamicPort = dynamic_cast<ArsNovaUI::DynamicPort*>(canvas.getParentComponent())) {
        dynamicPort->mouseDrag(e.getEventRelativeTo(dynamicPort));
    }
}

void CanvasController::handleNodeMouseDown(const juce::MouseEvent& e, Node& node)
{
    const int  nodeId          = node.nodeId;
    const bool isShiftLeftDrag = e.mods.isLeftButtonDown() && e.mods.isShiftDown();

    if (node.nodeValueEditor.isVisible() && e.mods.isCtrlDown() && ! e.mods.isShiftDown()) {
        undoManager.beginNewTransaction();

        gesture           = Gesture::EditingValue;
        dragStartValue    = static_cast<double>(node.nodeValueEditor.boundValue.getValue());
        draggingValueNode = &node;
        return;
    }

    if (isShiftLeftDrag && arrowGestures.beginTipFromNode(node)) {
        gesture = Gesture::DrawingArrow;
    }

    node.setHoverVisual(true);
    canvas.arrowManager.clearSelection();

    canvas.nodeManager.deselectAllExcept(node);

    if (e.mods.isRightButtonDown() && e.mods.isShiftDown()) {
        undoManager.beginNewTransaction();

        document.removeNode(nodeId);
    }
    else {
        node.setSelectVisual();
    }
}

void CanvasController::handleNodeMouseDrag(const juce::MouseEvent& e, Node& node)
{
    const juce::Point<int> cursor = e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt();

    if (e.getDistanceFromDragStart() < dragThreshold || !e.mods.isLeftButtonDown()) {
        return;
    }

    if (gesture == Gesture::DrawingArrow) {
        arrowGestures.dragTip(cursor);
        return;
    }

    nodeGestures.respondToDrag(node, cursor, e.mods);
}
