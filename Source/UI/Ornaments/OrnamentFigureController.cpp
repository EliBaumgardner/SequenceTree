#include "OrnamentFigureController.h"
#include "OrnamentCanvas.h"
#include <ArsNovaUI/Canvas/DynamicPort.h>
#include "../../Util/ArrowInfo.h"

OrnamentFigureController::OrnamentFigureController(OrnamentCanvas& canvas, ArsNovaUI::DynamicPort& port, OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : canvas(canvas),
      port(port),
      ornamentLibraries(ornamentLibraries),
      undoManager(undoManager)
{
}

void OrnamentFigureController::mouseMove(const juce::MouseEvent& event)
{
    const juce::Point<float> cursor       = event.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform);
    OrnamentNode* const      hoveredNode  = canvas.findNodeAt(cursor.roundToInt());
    Arrow* const             hoveredArrow = canvas.findArrowNear(cursor, arrowHoverRadius, OrnamentCanvas::ArrowPart::Shaft);

    for (const auto& [nodeId, node] : canvas.nodes) {
        if (node->isHovered != (node.get() == hoveredNode)) {
            node->setHoverVisual(node.get() == hoveredNode);
        }
    }

    for (Arrow* const arrow : canvas.arrows) {
        if (arrow->hovered != (arrow == hoveredArrow)) {
            arrow->hovered = arrow == hoveredArrow;

            arrow->repaint();
        }
    }
}

void OrnamentFigureController::mouseExit(const juce::MouseEvent& event)
{
    const juce::Point<int> cursor      = event.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt();
    OrnamentNode* const    hoveredNode = canvas.findNodeAt(cursor);

    for (const auto& [nodeId, node] : canvas.nodes) {
        if (node->isHovered != (node.get() == hoveredNode)) {
            node->setHoverVisual(node.get() == hoveredNode);
        }
    }
}

void OrnamentFigureController::mouseDown(const juce::MouseEvent& event)
{
    const int              grid    = static_cast<int>(ArrowInfo::pixelsPerGridSpace);
    const juce::Point<int> point   = event.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt();
    const juce::Point<int> nearest { juce::roundToInt(point.x / ArrowInfo::pixelsPerGridSpace) * grid,
                                     juce::roundToInt(point.y / ArrowInfo::pixelsPerGridSpace) * grid };
    juce::Point<int>       snapped = point;
    OrnamentNode* const    node    = canvas.findNodeAt(point);

    if (std::abs(point.x - nearest.x) < OrnamentCanvas::snapThreshold) {
        snapped.x = nearest.x;
    }

    if (std::abs(point.y - nearest.y) < OrnamentCanvas::snapThreshold) {
        snapped.y = nearest.y;
    }

    dragState     = DragState::Idle;
    draggedNodeId = -1;

    if (! ornament.isValid()) {
        return;
    }

    if (clickMode == ClickMode::PickHost) {
        if (node != nullptr && ! event.mods.isRightButtonDown()) {
            undoManager.beginNewTransaction();

            ornamentLibraries.toggleHost(ornament, node->nodeId, &undoManager);
        }

        switchClickMode(ClickMode::Build);

        return;
    }

    if (event.mods.isRightButtonDown() && event.mods.isShiftDown()) {
        const juce::Point<float> cursor      = point.toFloat();
        Arrow* const             danglingTip = canvas.findArrowNear(cursor, OrnamentCanvas::tipGrabRadius, OrnamentCanvas::ArrowPart::DanglingTip);
        Arrow* const             shaft       = canvas.findArrowNear(cursor, arrowHoverRadius, OrnamentCanvas::ArrowPart::Shaft);

        if (danglingTip != nullptr || (shaft != nullptr && shaft->isDangling())) {
            undoManager.beginNewTransaction();

            ornamentLibraries.removeDanglingArrow(ornament, &undoManager);

            return;
        }

        if (node != nullptr) {
            undoManager.beginNewTransaction();

            ornamentLibraries.removeFigureNode(ornament, node->nodeId, &undoManager);

            return;
        }
    }

    if (event.mods.isRightButtonDown()) {
        return;
    }

    if (node != nullptr) {
        handleNodeMouseDown(event, *node);

        return;
    }

    handleCanvasMouseDown(event, point, snapped);
}

void OrnamentFigureController::mouseDrag(const juce::MouseEvent& event)
{
    const int              grid                = static_cast<int>(ArrowInfo::pixelsPerGridSpace);
    const juce::Point<int> point               = event.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt();
    const juce::Point<int> nearest             { juce::roundToInt(point.x / ArrowInfo::pixelsPerGridSpace) * grid,
                                                 juce::roundToInt(point.y / ArrowInfo::pixelsPerGridSpace) * grid };
    juce::Point<int>       snapped             = point;
    const bool             pastThreshold       = event.getDistanceFromDragStart() >= dragThreshold;
    const int              scrubSteps          = -event.getOffsetFromDragStart().y / pixelsPerScrubStep;
    const auto             draggedNode         = canvas.nodes.find(draggedNodeId);
    const juce::ValueTree  draggedNodeTree     = canvas.figure.getChildWithProperty(ValueTreeIdentifiers::Id, draggedNodeId);
    const juce::Point<int> draggedNodePosition { draggedNodeTree.getProperty(ValueTreeIdentifiers::XPosition), draggedNodeTree.getProperty(ValueTreeIdentifiers::YPosition) };

    if (std::abs(point.x - nearest.x) < OrnamentCanvas::snapThreshold) {
        snapped.x = nearest.x;
    }

    if (std::abs(point.y - nearest.y) < OrnamentCanvas::snapThreshold) {
        snapped.y = nearest.y;
    }

    switch (dragState) {
        case DragState::MovingNode:
        case DragState::MovingArrowHead:
            if (pastThreshold) {
                ornamentLibraries.setFigureNodePosition(ornament, draggedNodeId, snapped, &undoManager);
            }
            break;

        case DragState::GrowingChain:
            if (pastThreshold) {
                draggedNodeId = ornamentLibraries.addFigureNode(ornament, draggedNodeId, snapped, &undoManager).getProperty(ValueTreeIdentifiers::Id, -1);
                dragState     = DragState::MovingArrowHead;
            }
            break;

        case DragState::CreatingDanglingArrow:
            if (pastThreshold) {
                juce::Point<int> tipOffset = snapped - draggedNodePosition;

                if (tipOffset.x == 0 && tipOffset.y == 0) {
                    tipOffset.x = static_cast<int>(ArrowInfo::pixelsPerGridSpace);
                }

                ornamentLibraries.addDanglingArrow(ornament, draggedNodeId, tipOffset, &undoManager);

                dragState = DragState::MovingDanglingTip;
            }
            break;

        case DragState::MovingDanglingTip:
            ornamentLibraries.setLastNodeLength(ornament, snapped - draggedNodePosition, &undoManager);
            break;

        case DragState::ScrubbingValue:
            if (draggedNode != canvas.nodes.end()) {
                draggedNode->second->nodeValueEditor.setNumericValue(dragStartValue + scrubSteps);
                draggedNode->second->refreshValueDisplay();
            }
            break;

        case DragState::Panning:
            port.mouseDrag(event.getEventRelativeTo(&port));
            break;

        case DragState::Idle:
            break;
    }
}

void OrnamentFigureController::mouseUp(const juce::MouseEvent& event)
{
    const auto node         = canvas.nodes.find(draggedNodeId);
    const bool isPlainClick = dragState == DragState::MovingNode && node != canvas.nodes.end() && event.getDistanceFromDragStart() < dragThreshold
                              && ! event.mods.isAnyModifierKeyDown();

    if (dragState == DragState::MovingDanglingTip) {
        for (Arrow* const arrow : canvas.arrows) {
            if (arrow->isDangling() && arrow->startNode != nullptr && arrow->startNode->nodeId == draggedNodeId) {
                arrow->triggerSnapAnimation();
            }
        }
    }
    else if (dragState == DragState::MovingArrowHead) {
        for (Arrow* const arrow : canvas.arrows) {
            if (arrow->endNode != nullptr && arrow->endNode->nodeId == draggedNodeId) {
                arrow->triggerSnapAnimation();
            }
        }
    }

    if (isPlainClick) {
        node->second->respondToClick(event.getEventRelativeTo(node->second.get()).getPosition());
    }

    dragState = DragState::Idle;
}

void OrnamentFigureController::switchClickMode(ClickMode mode)
{
    juce::Image    cursorImage(juce::Image::ARGB, hostCursorSize, hostCursorSize, true);
    juce::Graphics cursorGraphics(cursorImage);

    clickMode = mode;

    if (mode == ClickMode::PickHost) {
        arrowMode = false;

        cursorGraphics.setColour(juce::Colours::black);
        cursorGraphics.drawEllipse(cursorImage.getBounds().toFloat().reduced(1.0f), 1.5f);

        canvas.setMouseCursor(juce::MouseCursor(cursorImage, hostCursorSize / 2, hostCursorSize / 2));
    }
    else {
        canvas.setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    if (onClickModeChanged != nullptr) {
        onClickModeChanged();
    }

    if (onArrowModeChanged != nullptr) {
        onArrowModeChanged();
    }
}

void OrnamentFigureController::setArrowMode(bool enabled)
{
    if (arrowMode == enabled) {
        return;
    }

    arrowMode = enabled;

    if (enabled && clickMode == ClickMode::PickHost) {
        switchClickMode(ClickMode::Build);

        return;
    }

    if (onArrowModeChanged != nullptr) {
        onArrowModeChanged();
    }
}

void OrnamentFigureController::handleNodeMouseDown(const juce::MouseEvent& event, OrnamentNode& node)
{
    const bool isLastNode = node.nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getNumChildren() == 0;

    undoManager.beginNewTransaction();

    draggedNodeId = node.nodeId;

    if (node.nodeValueEditor.isVisible() && event.mods.isCtrlDown() && event.mods.isShiftDown()) {
        dragState      = DragState::ScrubbingValue;
        dragStartValue = static_cast<double>(node.nodeValueEditor.boundValue.getValue());

        return;
    }

    dragState = DragState::MovingNode;

    if (event.mods.isShiftDown() && isLastNode) {
        if (arrowMode) {
            const juce::ValueTree dangling = node.nodeValueTree.getChildWithName(ValueTreeIdentifiers::DanglingArrows).getChild(0);

            if (dangling.isValid()) {
                dragState = DragState::MovingDanglingTip;
            }
            else {
                dragState = DragState::CreatingDanglingArrow;
            }
        }
        else {
            dragState = DragState::GrowingChain;
        }
    }

    for (Arrow* const arrow : canvas.arrows) {
        if (arrow->selected) {
            arrow->selected = false;

            arrow->repaint();
        }
    }

    for (const auto& [nodeId, other] : canvas.nodes) {
        if (other.get() != &node && other->isSelected) {
            other->setSelectVisual(false);
        }
    }

    node.setHoverVisual(true);
    node.setSelectVisual();
}

void OrnamentFigureController::handleCanvasMouseDown(const juce::MouseEvent& event, juce::Point<int> point, juce::Point<int> snapped)
{
    const juce::Point<float> cursor      = point.toFloat();
    Arrow* const             danglingTip = canvas.findArrowNear(cursor, OrnamentCanvas::tipGrabRadius, OrnamentCanvas::ArrowPart::DanglingTip);
    Arrow* const             label       = canvas.findArrowNear(cursor, arrowLabelGrabRadius, OrnamentCanvas::ArrowPart::Label);
    Arrow* const             head        = canvas.findArrowNear(cursor, arrowHeadGrabRadius, OrnamentCanvas::ArrowPart::Head);
    Arrow* const             shaft       = canvas.findArrowNear(cursor, arrowHoverRadius, OrnamentCanvas::ArrowPart::Shaft);

    if (event.mods.isShiftDown()) {
        undoManager.beginNewTransaction();

        ornamentLibraries.addFigureNode(ornament, -1, snapped, &undoManager);

        return;
    }

    if (danglingTip != nullptr) {
        undoManager.beginNewTransaction();

        draggedNodeId = danglingTip->startNode->nodeId;
        dragState     = DragState::MovingDanglingTip;

        return;
    }

    if (label != nullptr) {
        label->beginDurationEdit();

        return;
    }

    for (Arrow* const arrow : canvas.arrows) {
        arrow->selected = arrow == head || (head == nullptr && arrow == shaft);

        arrow->repaint();
    }

    if (head != nullptr) {
        undoManager.beginNewTransaction();

        draggedNodeId = head->endNode->nodeId;
        dragState     = DragState::MovingArrowHead;

        return;
    }

    if (shaft != nullptr) {
        return;
    }

    for (const auto& [nodeId, node] : canvas.nodes) {
        if (node->isSelected) {
            node->setSelectVisual(false);
        }
    }

    dragState = DragState::Panning;

    port.mouseDown(event.getEventRelativeTo(&port));
}
