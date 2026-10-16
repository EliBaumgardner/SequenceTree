#include "NodeController.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include "../Graph/GraphState.h"
#include "../Plugin/PluginProcessor.h"
#include <ArsNovaUI/Canvas/DynamicPort.h>

NodeController::NodeController(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager, TraversalSession& traversalSession,
                               RTGraphBuilder& rtGraphBuilder, OrnamentLibraries& ornamentLibraries)
    : selectionGestures(graphState, undoManager, nodeCanvas),
      ornamentApplier(nodeCanvas, ornamentLibraries, undoManager),
      arrowGestures(nodeCanvas, graphState, undoManager),
      nodeGestures(nodeCanvas, graphState, undoManager),
      encapsulationGestures(nodeCanvas, graphState, undoManager),
      graphState(graphState),
      undoManager(undoManager),
      traversalSession(traversalSession),
      rtGraphBuilder(rtGraphBuilder),
      nodeCanvas(nodeCanvas)
{
}

NodeController::~NodeController() = default;

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    gesture           = Gesture::Idle;
    draggingValueNode = nullptr;

    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(&nodeCanvas);
            nodeCanvas.valueField.paintStroke(canvasEvent.position, true, e.mods.isRightButtonDown());
        }
        return;
    }

    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Span) {
        if (Node* spanNode = dynamic_cast<Node*>(e.eventComponent)) {
            encapsulationGestures.selectSpanNode(*spanNode);
        }
        return;
    }

    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::NotePreview) {
        if (Node* previewNode = dynamic_cast<Node*>(e.eventComponent)) {
            traversalSession.previewRequests.push({ RTPreviewRequest::Kind::Start,
                                                    previewNode->nodeId,
                                                    static_cast<int>(nodeCanvas.quaverCount.getValue()),
                                                    rtGraphBuilder.buildRTtraversal({ nodeCanvas.quaverTraversalId, 0 }),
                                                    nodeCanvas.quaverRepeat });
        }
        return;
    }

    if (auto* const markView = dynamic_cast<OrnamentMarkView*>(e.eventComponent)) {
        ornamentApplier.respondToMarkClick(e, *markView);
        return;
    }

    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Ornament && e.mods.isLeftButtonDown() && ! e.mods.isAnyModifierKeyDown()) {
        ornamentApplier.respondToClick(e.eventComponent);
        return;
    }

    if (dynamic_cast<NodeCanvas*>(e.eventComponent) != nullptr) {
        handleCanvasMouseDown(e);
    }
    else if (Node* clickedNode = dynamic_cast<Node*>(e.eventComponent)) {
        handleNodeMouseDown(e, *clickedNode);
    }
}

void NodeController::mouseUp(const juce::MouseEvent& e)
{
    Node* const clickedNode  = dynamic_cast<Node*>(e.eventComponent);
    const bool  isPlainClick = clickedNode != nullptr && gesture == Gesture::Idle && e.mods.isLeftButtonDown() && !e.mods.isAnyModifierKeyDown()
                               && e.getDistanceFromDragStart() < dragThreshold && nodeCanvas.activeTool == NodeCanvas::CanvasTool::None;

    if (gesture == Gesture::MovingArrowHead) {
        gesture = Gesture::Idle;

        nodeGestures.finishArrowHeadDrag();
        return;
    }

    if (gesture == Gesture::BoxSelecting) {
        gesture = Gesture::Idle;

        selectionGestures.finishBoxSelection();
        return;
    }

    if (gesture == Gesture::MovingDanglingTip || gesture == Gesture::DrawingArrow) {
        gesture = Gesture::Idle;

        arrowGestures.finishTip();
        return;
    }

    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        nodeCanvas.valueField.endStroke();
        return;
    }

    gesture           = Gesture::Idle;
    draggingValueNode = nullptr;

    nodeGestures.finishDrag();

    if (isPlainClick) {
        clickedNode->respondToClick(e.getEventRelativeTo(clickedNode).getPosition());
    }
}

void NodeController::mouseEnter(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        return;
    }

    if (Node* node = dynamic_cast<Node*>(e.eventComponent)) {
        node->setHoverVisual(true);
    }
}

void NodeController::mouseExit(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        return;
    }

    if (Node* node = dynamic_cast<Node*>(e.eventComponent)) {
        node->setHoverVisual(false);
    }
}

void NodeController::mouseMove(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        return;
    }

    arrowGestures.updateHover(e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform));
}

void NodeController::mouseDrag(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(&nodeCanvas);
            nodeCanvas.valueField.paintStroke(canvasEvent.position, false);
        }
        return;
    }

    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Span || nodeCanvas.activeTool == NodeCanvas::CanvasTool::NotePreview) {
        return;
    }

    if (gesture == Gesture::EditingValue && draggingValueNode != nullptr) {
        dragValue(e);
        return;
    }

    if (gesture == Gesture::MovingDanglingTip) {
        arrowGestures.dragTip(e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform).roundToInt());
        return;
    }

    if (dynamic_cast<NodeCanvas*>(e.eventComponent) != nullptr) {
        handleCanvasMouseDrag(e);
        return;
    }

    if (Node* draggedNode = dynamic_cast<Node*>(e.eventComponent)) {
        handleNodeMouseDrag(e, *draggedNode);
    }
}

void NodeController::dragValue(const juce::MouseEvent& e)
{
    const int yOffset = e.getOffsetFromDragStart().y;
    const int delta   = -yOffset / 3;

    draggingValueNode->nodeValueEditor.setNumericValue(dragStartValue + delta);
    draggingValueNode->refreshValueDisplay();
}

void NodeController::handleCanvasMouseDown(const juce::MouseEvent& e)
{
    const juce::Point<float> clickPoint = e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform);

    if (e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (Arrow* arrow = nodeCanvas.hitTester.arrowNear(clickPoint, ArrowGestures::danglingArrowGrabRadius)) {
            arrowGestures.removeArrow(*arrow);
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (Arrow* clickedArrow = nodeCanvas.hitTester.arrowNear(clickPoint, ArrowGestures::arrowHoverRadius)) {
            arrowGestures.showArrowContextMenu(clickedArrow);
            return;
        }

        selectionGestures.showSelectionMenu(clickPoint.roundToInt());
        return;
    }

    if (!e.mods.isShiftDown()) {
        if (Arrow* arrow = nodeCanvas.hitTester.danglingHeadNear(clickPoint, ArrowGestures::danglingArrowGrabRadius)) {
            gesture = Gesture::MovingDanglingTip;

            arrowGestures.beginMovingTip(*arrow);
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        if (Arrow* labelArrow = nodeCanvas.hitTester.arrowLabelNear(clickPoint, ArrowGestures::arrowLabelGrabRadius)) {
            gesture = Gesture::EditingValue;

            labelArrow->beginDurationEdit();
            return;
        }

        if (Arrow* headArrow = nodeCanvas.hitTester.arrowHeadNear(clickPoint, ArrowGestures::arrowHeadGrabRadius)) {
            gesture                    = Gesture::MovingArrowHead;
            nodeGestures.arrowHeadNode = headArrow->endNode;

            nodeCanvas.arrowManager.setSelected(headArrow);
            return;
        }

        if (Arrow* clickedArrow = nodeCanvas.hitTester.arrowNear(clickPoint, ArrowGestures::arrowHoverRadius)) {
            gesture = Gesture::ArrowSelected;

            nodeCanvas.arrowManager.setSelected(clickedArrow);
            return;
        }

        nodeCanvas.arrowManager.clearSelection();
        selectionGestures.clearAll();
    }

    if (auto* dynamicPort = dynamic_cast<DynamicPort*>(nodeCanvas.getParentComponent())) {
        dynamicPort->mouseDown(e.getEventRelativeTo(dynamicPort));
    }

    if (e.mods.isShiftDown() && e.mods.isLeftButtonDown() && arrowGestures.arrowMode) {
        gesture = Gesture::BoxSelecting;

        selectionGestures.beginBoxSelection(clickPoint.roundToInt());
        return;
    }

    if (e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        nodeGestures.createRootNode(clickPoint);
    }
}

void NodeController::handleCanvasMouseDrag(const juce::MouseEvent& e)
{
    const juce::Point<int> cursor = e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform).roundToInt();

    if (gesture == Gesture::MovingArrowHead) {
        if (e.getDistanceFromDragStart() >= dragThreshold) {
            nodeGestures.dragArrowHead(cursor);
        }
        return;
    }

    if (gesture == Gesture::BoxSelecting) {
        selectionGestures.updateBoxSelection(e);
        return;
    }

    if (gesture == Gesture::ArrowSelected || gesture == Gesture::EditingValue) {
        return;
    }

    if (auto* dynamicPort = dynamic_cast<DynamicPort*>(nodeCanvas.getParentComponent())) {
        dynamicPort->mouseDrag(e.getEventRelativeTo(dynamicPort));
    }
}

void NodeController::handleNodeMouseDown(const juce::MouseEvent& e, Node& node)
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

    if (e.mods.isRightButtonDown() && ! e.mods.isShiftDown() && encapsulationGestures.toggleEncapsulationExpansion(node)) {
        return;
    }

    if (isShiftLeftDrag && arrowGestures.beginTipFromNode(node)) {
        gesture = Gesture::DrawingArrow;
    }

    node.setHoverVisual(true);
    nodeCanvas.arrowManager.clearSelection();

    selectionGestures.deselectAllExcept(node);

    if (e.mods.isRightButtonDown() && e.mods.isShiftDown()) {
        undoManager.beginNewTransaction();

        graphState.removeNode(nodeId, &undoManager);
    }
    else {
        node.setSelectVisual();
    }
}

void NodeController::handleNodeMouseDrag(const juce::MouseEvent& e, Node& node)
{
    const juce::Point<int> cursor = e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform).roundToInt();

    if (e.getDistanceFromDragStart() < dragThreshold || !e.mods.isLeftButtonDown()) {
        return;
    }

    if (gesture == Gesture::DrawingArrow) {
        arrowGestures.dragTip(cursor);
        return;
    }

    nodeGestures.respondToDrag(node, cursor, e.mods);
}
