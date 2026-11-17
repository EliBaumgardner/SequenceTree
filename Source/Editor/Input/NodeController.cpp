#include "NodeController.h"
#include "Editor/Canvas/NodeCanvas.h"
#include "Editor/Nodes/Node.h"
#include "Editor/Nodes/Arrow.h"
#include "Processor/Document/GraphState.h"

#include "Editor/Canvas/GraphDocument.h"


NodeController::NodeController(NodeCanvas& nodeCanvas, GraphDocument& graphDocument, NotePreviewGestures& notePreviewGestures, OrnamentLibraries& ornamentLibraries)
    : ArsNovaUI::CanvasController(nodeCanvas, graphDocument),
      selectionGestures          (graphDocument.graphState, graphDocument.undoManager, nodeCanvas),
      encapsulationGestures      (nodeCanvas, graphDocument.graphState, graphDocument.undoManager),
      ornamentApplier            (nodeCanvas, ornamentLibraries, graphDocument.undoManager),
      arrowMenu                  (nodeCanvas, graphDocument),
      notePreviewGestures        (notePreviewGestures),
      nodeCanvas                 (nodeCanvas)
{
    nodeGestures.onChildCreated = [this](int parentNodeId, const juce::ValueTree& child) {
        encapsulationGestures.adoptCreatedChild(parentNodeId, child);
    };

    nodeCanvas.onToolLeft = [this](NodeCanvas::CanvasTool previousTool) {
        if (previousTool == NodeCanvas::CanvasTool::Span) {
            encapsulationGestures.spanAnchorNodeId = -1;
        }

        if (previousTool == NodeCanvas::CanvasTool::NotePreview) {
            this->notePreviewGestures.stopPreview();
        }
    };
}

NodeController::~NodeController() = default;

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    const juce::Point<float> clickPoint = e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform);

    gesture           = Gesture::Idle;
    graphGesture      = GraphGesture::None;
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
        notePreviewGestures.respondToClick(e.eventComponent);

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
        if (!e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
            if (ArsNovaUI::CanvasArrow* clickedArrow = nodeCanvas.hitTester.arrowNear(clickPoint, ArsNovaUI::ArrowGestures::arrowHoverRadius)) {
                Arrow* const graphArrow = static_cast<Arrow*>(clickedArrow);

                arrowMenu.showArrowContextMenu(graphArrow);
                return;
            }

            selectionGestures.showSelectionMenu(clickPoint.roundToInt());
            return;
        }

        if (e.mods.isShiftDown() && e.mods.isLeftButtonDown() && arrowGestures.arrowMode) {
            graphGesture = GraphGesture::BoxSelecting;

            boxSelectionGestures.begin(clickPoint.roundToInt());

            return;
        }
    }

    if (Node* clickedNode = dynamic_cast<Node*>(e.eventComponent)) {
        const bool isValueScrub = clickedNode->nodeValueEditor.isVisible() && e.mods.isCtrlDown() && ! e.mods.isShiftDown();

        if (! isValueScrub && e.mods.isRightButtonDown() && ! e.mods.isShiftDown() && encapsulationGestures.toggleEncapsulationExpansion(*clickedNode)) {
            return;
        }
    }

    ArsNovaUI::CanvasController::mouseDown(e);
}

void NodeController::mouseUp(const juce::MouseEvent& e)
{
    if (graphGesture == GraphGesture::BoxSelecting) {
        graphGesture = GraphGesture::None;

        boxSelectionGestures.finish();

        return;
    }

    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        nodeCanvas.valueField.endStroke();
        return;
    }

    if (nodeCanvas.activeTool != NodeCanvas::CanvasTool::None && gesture == Gesture::Idle) {
        draggingValueNode = nullptr;

        nodeGestures.finishDrag();
        return;
    }

    ArsNovaUI::CanvasController::mouseUp(e);
}

void NodeController::mouseEnter(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        return;
    }

    ArsNovaUI::CanvasController::mouseEnter(e);
}

void NodeController::mouseExit(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        return;
    }

    ArsNovaUI::CanvasController::mouseExit(e);
}

void NodeController::mouseMove(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        return;
    }

    nodeCanvas.fadeFlagArrows(e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform));

    ArsNovaUI::CanvasController::mouseMove(e);
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

    if (graphGesture == GraphGesture::BoxSelecting) {
        boxSelectionGestures.update(e);

        return;
    }

    ArsNovaUI::CanvasController::mouseDrag(e);
}