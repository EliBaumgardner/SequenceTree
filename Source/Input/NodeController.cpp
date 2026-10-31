#include "NodeController.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include "../Graph/GraphState.h"
#include "../Plugin/PluginProcessor.h"
#include "GraphDocument.h"


NodeController::NodeController(NodeCanvas& nodeCanvas, GraphState& graphState, GraphDocument& graphDocument, juce::UndoManager& undoManager,
                               TraversalSession& traversalSession, RTGraphBuilder& rtGraphBuilder, OrnamentLibraries& ornamentLibraries)
    : CanvasController(nodeCanvas, graphDocument, undoManager),
      selectionGestures(graphState, undoManager, nodeCanvas),
      ornamentApplier(nodeCanvas, ornamentLibraries, undoManager),
      encapsulationGestures(nodeCanvas, graphState, undoManager),
      arrowMenu(nodeCanvas, graphState, graphDocument, undoManager),
      traversalSession(traversalSession),
      rtGraphBuilder(rtGraphBuilder),
      nodeCanvas(nodeCanvas)
{
    nodeGestures.onChildCreated = [this](int parentNodeId, const juce::ValueTree& child) {
        encapsulationGestures.adoptCreatedChild(parentNodeId, child);
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
        if (!e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
            if (Arrow* clickedArrow = nodeCanvas.hitTester.arrowNear(clickPoint, ArrowGestures::arrowHoverRadius)) {
                arrowMenu.showArrowContextMenu(clickedArrow);
                return;
            }

            selectionGestures.showSelectionMenu(clickPoint.roundToInt());
            return;
        }

        if (e.mods.isShiftDown() && e.mods.isLeftButtonDown() && arrowGestures.arrowMode) {
            graphGesture = GraphGesture::BoxSelecting;

            selectionGestures.beginBoxSelection(clickPoint.roundToInt());
            return;
        }
    }

    if (Node* clickedNode = dynamic_cast<Node*>(e.eventComponent)) {
        const bool isValueScrub = clickedNode->nodeValueEditor.isVisible() && e.mods.isCtrlDown() && ! e.mods.isShiftDown();

        if (! isValueScrub && e.mods.isRightButtonDown() && ! e.mods.isShiftDown() && encapsulationGestures.toggleEncapsulationExpansion(*clickedNode)) {
            return;
        }
    }

    CanvasController::mouseDown(e);
}

void NodeController::mouseUp(const juce::MouseEvent& e)
{
    if (graphGesture == GraphGesture::BoxSelecting) {
        graphGesture = GraphGesture::None;

        selectionGestures.finishBoxSelection();
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

    CanvasController::mouseUp(e);
}

void NodeController::mouseEnter(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        return;
    }

    CanvasController::mouseEnter(e);
}

void NodeController::mouseExit(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        return;
    }

    CanvasController::mouseExit(e);
}

void NodeController::mouseMove(const juce::MouseEvent& e)
{
    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        return;
    }

    CanvasController::mouseMove(e);
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
        selectionGestures.updateBoxSelection(e);
        return;
    }

    CanvasController::mouseDrag(e);
}