/*
  ==============================================================================

    ObjectController.h.cpp
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include "NodeController.h"
#include "../UI/Node/Modulator.h"
#include "../UI/Node/Encapsulator.h"
#include "../Graph/NodeFactory.h"
#include "../UI/Canvas/DynamicPort.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Graph/GraphState.h"
#include "../UI/Menus/AllowedTraversalsMenu.h"
#include "../UI/Menus/ContextMenu.h"
#include "../UI/Theme/CustomLookAndFeel.h"
#include "../Plugin/PluginProcessor.h"

NodeController::NodeController(const ApplicationContext& context, NodeCanvas& canvasRef)
    : selectionOps(context), applicationContext(context), canvas(canvasRef)
{
}

NodeController::~NodeController() = default;

void NodeController::mouseEnter(const juce::MouseEvent& e)
{
    if (canvas.paintMode) {
        return;
    }

    juce::Component* component = e.eventComponent;
    if (Node* node = dynamic_cast<Node*>(component)) {
        node->setHoverVisual(true);
    }
}
void NodeController::mouseExit(const juce::MouseEvent& e)
{
    if (canvas.paintMode) {
        return;
    }

    juce::Component* component = e.eventComponent;
    if (Node* node = dynamic_cast<Node*>(component)) {
        node->setHoverVisual(false);
    }
}
void NodeController::mouseMove(const juce::MouseEvent& e)
{
    if (canvas.paintMode) {
        return;
    }

    updateArrowHover(e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform));
}

void NodeController::updateArrowHover(juce::Point<float> cursor)
{
    for (Arrow* arrow : canvas.arrowManager.all()) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr) {
            continue;
        }
        if (arrow->startNode->nodeType != NodeType::TraversalFlag) {
            continue;
        }

        const float dist = CanvasHitTester::distanceToSegment(cursor, arrow->startNode->getNodeCentre().toFloat(), arrow->endNode->getNodeCentre().toFloat());

        const bool nearby = dist < flagProximityRadius;
        if (nearby != arrow->proximityHovered) {
            arrow->proximityHovered = nearby;
            arrow->setHoverFade(arrow->sourceHovered || arrow->proximityHovered);
        }
    }

    Arrow* const hoveredArrow = canvas.hitTester.arrowNear(cursor, arrowHoverRadius);

    for (Arrow* arrow : canvas.arrowManager.all()) {
        const bool shouldBold = (arrow == hoveredArrow);
        if (shouldBold != arrow->hovered) {
            arrow->hovered = shouldBold;
            arrow->repaint();
        }
    }
}

void NodeController::mouseDrag(const juce::MouseEvent& e)
{
    if (canvas.paintMode) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(&canvas);
            canvas.valueField.paintStroke(canvasEvent.position, false);
        }
        return;
    }

    if (canvas.spanMode || canvas.quaverMode != NodeCanvas::QuaverMode::Off) {
        return;
    }

    if (dragState == DragState::EditingValue && draggingValueNode != nullptr) {
        dragValue(e);
        return;
    }

    if (dragState == DragState::MovingDanglingTip) {
        if (draggingDanglingArrow != nullptr) {
            dragDanglingTip(e);
        }
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

void NodeController::dragDanglingTip(const juce::MouseEvent& e)
{
    Node* startNode = draggingDanglingArrow->startNode;

    if (startNode == nullptr) {
        return;
    }

    const juce::Point<int> tip    = danglingTipFor(startNode, e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt());
    const juce::Point<int> centre = startNode->getNodeCentre();

    draggingDanglingArrow->setTipOffset({ tip.x - centre.x, tip.y - centre.y });
}

juce::Point<int> NodeController::danglingTipFor(const Node* startNode, juce::Point<int> cursor)
{
    danglingSnapTarget = findDanglingSnapTarget(startNode, cursor);

    if (danglingSnapTarget != nullptr) {
        return danglingSnapTarget->getNodeCentre();
    }

    return canvas.snapPointToGrid(cursor);
}

Node* NodeController::findDanglingSnapTarget(const Node* startNode, juce::Point<int> tip) const
{
    if (startNode == nullptr) {
        return nullptr;
    }

    const int startNodeId = startNode->nodeId;

    Node* const snapTarget = canvas.hitTester.nodeContaining(tip.toFloat(), startNodeId);

    if (snapTarget == nullptr) {
        return nullptr;
    }

    if (startNode->nodeArrows.count(snapTarget->nodeId) > 0) {
        return nullptr;
    }

    const juce::Identifier startType = startNode->nodeValueTree.getType();

    const bool startsOnModulator = startType == ValueTreeIdentifiers::ModulatorData
                                || startType == ValueTreeIdentifiers::ModulatorRootData
                                || startType == ValueTreeIdentifiers::AlternativeModulatorData;

    if (startsOnModulator && snapTarget->nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        return nullptr;
    }

    return snapTarget;
}

void NodeController::handleCanvasMouseDrag(const juce::MouseEvent& e)
{
    if (dragState == DragState::MovingArrowHead) {
        if (draggingArrowHeadNode == nullptr || e.getDistanceFromDragStart() < dragThreshold) {
            return;
        }

        const int nodeId = draggingArrowHeadNode->nodeId;
        const auto position = e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt();

        NodePosition newPosition;
        newPosition.xPosition = position.x;
        newPosition.yPosition = position.y;
        newPosition.radius    = Theme::nodeRadius;

        handleNodeDrag(applicationContext.undoManager, nodeId, newPosition);
        return;
    }

    if (dragState == DragState::BoxSelecting) {
        updateBoxSelection(e);
        return;
    }

    if (dragState == DragState::ArrowSelected || dragState == DragState::EditingValue) {
        return;
    }

    if (auto* dynamicPort = dynamic_cast<DynamicPort*>(canvas.getParentComponent())) {
        dynamicPort->mouseDrag(e.getEventRelativeTo(dynamicPort));
    }
}

void NodeController::handleNodeDrag(juce::UndoManager *undoManager, int nodeId, NodePosition newPosition)
{
    if (isDragStart) {
        isDragStart = false;
        undoManager->beginNewTransaction();
        canvas.showGrid();
    }

    juce::ValueTree nodeValueTree = applicationContext.graphState->getNode(nodeId);
    NodePosition oldPosition = applicationContext.graphState->getNodePosition(nodeId);

    snapToGrid(undoManager, newPosition, nodeValueTree);

    int deltaX = newPosition.xPosition - oldPosition.xPosition;
    int deltaY = newPosition.yPosition - oldPosition.yPosition;

    canvas.nodeManager.moveDescendants(nodeValueTree, deltaX, deltaY);
}

void NodeController::snapToGrid(juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree)
{
    const int draggedNodeId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);
    const juce::Point<int> collapseShift = canvas.encapsulationView.collapsedSpanShift(draggedNodeId);

    juce::Point<int> snapped = canvas.snapPointToGrid({ newPosition.xPosition - collapseShift.x,
                                                        newPosition.yPosition - collapseShift.y });
    newPosition.xPosition = snapped.x;
    newPosition.yPosition = snapped.y;

    applicationContext.graphState->setNodePosition(draggedNodeTree, newPosition, undoManager);
}

void NodeController::updateBoxSelection(const juce::MouseEvent& e)
{
    const juce::Point<int> cursor = e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt();

    canvas.selectionBounds = juce::Rectangle<int>(selectionAnchor, cursor);
    canvas.repaint();
}

void NodeController::handleNodeMouseDrag(const juce::MouseEvent& e, Node& node)
{
    juce::UndoManager* undoManager = applicationContext.undoManager;

    const int  nodeId   = node.nodeId;
    const auto position = e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform).roundToInt();

    NodePosition newPosition;
    newPosition.xPosition = position.x;
    newPosition.yPosition = position.y;
    newPosition.radius    = Theme::nodeRadius;

    if (e.getDistanceFromDragStart() < dragThreshold || !e.mods.isLeftButtonDown()) {
        return;
    }

    if (dragState == DragState::CreatingDanglingArrow && isArrowMode()) {
        if (danglingSourceNode != nullptr) {
            updateConnectionPreview(danglingSourceNode, newPosition, false);
        }
        return;
    }

    if (dragState == DragState::ConnectingFlag) {
        dragFlagConnection(e, node, newPosition);
        return;
    }

    if (!e.mods.isShiftDown() && !e.mods.isCtrlDown() && !draggedNodeTree.isValid()) {
        handleNodeDrag(undoManager, nodeId, newPosition);
        return;
    }

    if (isDragStart) {
        isDragStart = false;
        handleNodeDragStart(undoManager, &node, nodeId, newPosition, e.mods);
        return;
    }

    if (draggedNodeTree.isValid()) {
        const juce::Point<int> cursor { newPosition.xPosition, newPosition.yPosition };

        snapToGrid(undoManager, newPosition, draggedNodeTree);
        checkRootNodeSnap(cursor);
    }
}

void NodeController::updateConnectionPreview(Node *node, const NodePosition& newPosition, bool dashed)
{
    canvas.showGrid();

    juce::Point<int> tip    = danglingTipFor(node, { newPosition.xPosition, newPosition.yPosition });
    juce::Point<int> centre = node->getNodeCentre();
    canvas.arrowManager.updatePreview(node, { tip.x - centre.x, tip.y - centre.y }, dashed);
}

void NodeController::dragFlagConnection(const juce::MouseEvent& e, Node& node, const NodePosition& newPosition)
{
    const juce::Point<int> cursor { newPosition.xPosition, newPosition.yPosition };
    flagConnectionTarget = canvas.hitTester.nodeNear(cursor.toFloat(), rootSnapThreshold, flagConnectionSourceId);

    const juce::Point<int> centre = node.getNodeCentre();
    juce::Point<int> tip = canvas.snapPointToGrid(cursor);

    if (flagConnectionTarget != nullptr) {
        tip = flagConnectionTarget->getNodeCentre();
    }

    canvas.arrowManager.updatePreview(&node, { tip.x - centre.x, tip.y - centre.y }, true);
}

void NodeController::handleNodeDragStart(juce::UndoManager *undoManager, Node *node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods)
{
    GraphState& graphState = *applicationContext.graphState;

    int parentNodeId = nodeId;

    auto* const collapsedEncapsulator = dynamic_cast<Encapsulator*>(node);

    if (collapsedEncapsulator != nullptr && ! collapsedEncapsulator->memberNodeIds.empty()) {
        parentNodeId = collapsedEncapsulator->memberNodeIds.back();
    }

    Node* const parentNode = canvas.nodeManager.find(parentNodeId);

    if (parentNode == nullptr) {
        return;
    }

    const juce::Identifier nodeType = parentNode->nodeValueTree.getType();

    undoManager->beginNewTransaction();

    canvas.showGrid();

    snapSourceNodeId = parentNodeId;

    const NodePosition     parentPosition = graphState.getNodePosition(parentNodeId);
    const juce::Point<int> parentCentre   = parentNode->getNodeCentre();

    newPosition.xPosition += parentPosition.xPosition - parentCentre.x;
    newPosition.yPosition += parentPosition.yPosition - parentCentre.y;

    draggedNodeTree = NodeCreationDispatcher::create(nodeControllerMode, graphState, parentNodeId, nodeType, mods.isCtrlDown(), newPosition, undoManager);

    if (! draggedNodeTree.isValid()) {
        return;
    }

    connectionOps.applySelectedArrowInfo(parentNodeId, draggedNodeTree.getProperty(ValueTreeIdentifiers::Id), ArrowType::StepIntoTree);

    const int owningEncapsulatorId = parentNode->nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(canvas.nodeManager.find(owningEncapsulatorId));

    if (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded
        || owningEncapsulator->memberNodeIds.empty()) {
        return;
    }

    if (parentNodeId == owningEncapsulator->memberNodeIds.back()) {
        return;
    }

    graphState.encapsulation.insertNodeAfter(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id), parentNodeId, undoManager);
}

void NodeController::checkRootNodeSnap(juce::Point<int> canvasPoint)
{
    if (snapSourceNodeId < 0 || !draggedNodeTree.isValid()) {
        return;
    }

    const juce::Identifier sourceType = applicationContext.graphState->getNode(snapSourceNodeId).getType();

    if (sourceType == ValueTreeIdentifiers::ModulatorData
        || sourceType == ValueTreeIdentifiers::ModulatorRootData
        || sourceType == ValueTreeIdentifiers::AlternativeModulatorData) {
        return;
    }

    Node* nearestRoot = canvas.hitTester.rootNear(canvasPoint.toFloat(), rootSnapThreshold, snapSourceNodeId);

    if (nearestRoot != nullptr) {
        if (snapTargetRoot != nearestRoot) {
            snapTargetRoot = nearestRoot;

            setDraggedNodeVisible(false);

            Node* sourceNode = canvas.nodeManager.find(snapSourceNodeId);
            if (sourceNode != nullptr) {
                canvas.arrowManager.showSnapGhost(sourceNode, nearestRoot);
            }
        }
    }
    else if (snapTargetRoot != nullptr) {
        snapTargetRoot = nullptr;

        setDraggedNodeVisible(true);

        canvas.arrowManager.hideSnapGhost();
    }
}

void NodeController::setDraggedNodeVisible(bool shouldBeVisible)
{
    const int draggedId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

    Node* draggedNode = canvas.nodeManager.find(draggedId);
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

void NodeController::mouseUp(const juce::MouseEvent& e)
{
    Node* const clickedNode  = dynamic_cast<Node*>(e.eventComponent);
    const bool  isPlainClick = clickedNode != nullptr && dragState == DragState::Idle && e.mods.isLeftButtonDown() && ! e.mods.isAnyModifierKeyDown()
                            && e.getDistanceFromDragStart() < dragThreshold && ! canvas.spanMode && canvas.quaverMode == NodeCanvas::QuaverMode::Off;

    if (dragState == DragState::MovingArrowHead) {
        finishArrowHeadDrag();
        return;
    }

    if (dragState == DragState::BoxSelecting) {
        finishBoxSelection();
        return;
    }

    if (dragState == DragState::ArrowSelected) {
        dragState = DragState::Idle;
    }

    if (dragState == DragState::MovingDanglingTip) {
        finishDanglingTipDrag();
        return;
    }

    if (dragState == DragState::ConnectingFlag) {
        finishFlagConnection();
        return;
    }

    if (canvas.paintMode) {
        canvas.valueField.endStroke();
        return;
    }

    if (isArrowMode() && canvas.arrowManager.preview != nullptr) {
        finishDanglingArrowCreation();
        return;
    }

    if (dragState == DragState::EditingValue) {
        dragState         = DragState::Idle;
        draggingValueNode = nullptr;
    }
    else if (snapTargetRoot != nullptr) {
        connectDraggedNodeToRoot();
    }
    else if (draggedNodeTree.isValid()) {
        canvas.arrowManager.triggerSnapForNode(static_cast<int>(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id)));
    }

    endDrag();

    if (isPlainClick) {
        clickedNode->respondToClick(e.getEventRelativeTo(clickedNode).getPosition());
    }
}

void NodeController::finishArrowHeadDrag()
{
    const Node* const headNode = draggingArrowHeadNode;

    draggingArrowHeadNode = nullptr;
    dragState             = DragState::Idle;
    isDragStart           = true;

    if (headNode == nullptr) {
        return;
    }

    canvas.arrowManager.triggerSnapForNode(headNode->nodeId);
}

void NodeController::finishBoxSelection()
{
    const juce::Rectangle<int> selection = canvas.selectionBounds;

    for (auto& [nodeId, node] : canvas.nodeManager.all()) {
        if (node->isVisible() && selection.contains(node->getNodeCentre())) {
            node->setSelectVisual(true);
        }
    }

    canvas.selectionBounds = {};
    dragState              = DragState::Idle;

    canvas.repaint();
}

void NodeController::finishDanglingTipDrag()
{
    Arrow* const arrow = draggingDanglingArrow;

    draggingDanglingArrow = nullptr;
    dragState             = DragState::Idle;

    if (arrow == nullptr) {
        danglingSnapTarget = nullptr;
        canvas.hideGrid();
        return;
    }

    if (danglingSnapTarget != nullptr) {
        connectDanglingToTarget(arrow->startNode);
        NodeFactory::destroyDanglingArrow(arrow->arrowTree, applicationContext.undoManager);
    }
    else {
        applicationContext.undoManager->beginNewTransaction();
        NodeFactory::setDanglingArrowTip(*applicationContext.graphState, arrow->arrowTree, arrow->tipOffset, applicationContext.undoManager);
    }

    canvas.hideGrid();
}

void NodeController::connectDanglingToTarget(const Node* startNode)
{
    Node* const targetNode = danglingSnapTarget;
    danglingSnapTarget = nullptr;

    if (startNode == nullptr || targetNode == nullptr) {
        return;
    }

    connectWithSnapAnimation(startNode->nodeId, targetNode->nodeId, ArrowType::StepIntoTree);
}

void NodeController::connectWithSnapAnimation(int parentNodeId, int childNodeId,
                                              ArrowType rootConnectionType)
{
    Node* parentNode = canvas.nodeManager.find(parentNodeId);
    Node* childNode  = canvas.nodeManager.find(childNodeId);

    if (parentNode == nullptr || childNode == nullptr) {
        return;
    }

    connectionOps.connect(parentNodeId, childNodeId, rootConnectionType);

    canvas.arrowManager.connect(parentNode, childNode);
    canvas.arrowManager.refreshFor(parentNode);

    auto arrowIterator = parentNode->nodeArrows.find(childNodeId);
    if (arrowIterator != parentNode->nodeArrows.end()) {
        arrowIterator->second->triggerSnapAnimation();
    }
}

void NodeController::finishFlagConnection()
{
    dragState = DragState::Idle;
    canvas.arrowManager.preview.reset();

    if (flagConnectionTarget != nullptr) {
        commitFlagConnection(flagConnectionSourceId, flagConnectionTarget);
    }

    flagConnectionTarget   = nullptr;
    flagConnectionSourceId = -1;
}

void NodeController::commitFlagConnection(int sourceNodeId, Node* target)
{
    Node* sourceNode = canvas.nodeManager.find(sourceNodeId);
    if (sourceNode == nullptr || target == nullptr) {
        return;
    }

    int targetNodeId = target->nodeId;

    if (sourceNode->nodeArrows.count(targetNodeId) > 0) {
        return;
    }

    connectWithSnapAnimation(sourceNodeId, targetNodeId, ArrowType::StepIntoTree);
}

void NodeController::finishDanglingArrowCreation()
{
    if (danglingSnapTarget != nullptr) {
        Node* startNode = nullptr;

        if (canvas.arrowManager.preview != nullptr) {
            startNode = canvas.arrowManager.preview->startNode;
        }

        canvas.arrowManager.preview.reset();

        connectDanglingToTarget(startNode);
    }
    else {
        applicationContext.undoManager->beginNewTransaction();
        canvas.arrowManager.commitPreview();
    }

    dragState          = DragState::Idle;
    isDragStart        = true;
    danglingSourceNode = nullptr;

    canvas.hideGrid();
}

void NodeController::connectDraggedNodeToRoot()
{
    canvas.arrowManager.hideSnapGhost();

    const int rootNodeId   = snapTargetRoot->nodeId;
    const int parentNodeId = snapSourceNodeId;

    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    const int draggedNodeId = static_cast<int>(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id));

    juce::UndoManager* undoManager = applicationContext.undoManager;
    undoManager->undo();

    canvas.cancelPendingUpdatesFor(draggedNodeId);

    connectWithSnapAnimation(parentNodeId, rootNodeId, ArrowType::CrossRootTree);
}

void NodeController::endDrag()
{
    draggedNodeTree  = juce::ValueTree();
    isDragStart      = true;
    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;
    danglingSnapTarget = nullptr;

    canvas.hideGrid();
}

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    dragState             = DragState::Idle;
    draggingArrowHeadNode = nullptr;
    draggingDanglingArrow = nullptr;
    draggingValueNode     = nullptr;
    danglingSnapTarget    = nullptr;
    danglingSourceNode    = nullptr;
    snapTargetRoot        = nullptr;
    flagConnectionTarget  = nullptr;

    if (canvas.paintMode) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(&canvas);
            canvas.valueField.paintStroke(canvasEvent.position, true, e.mods.isRightButtonDown());
        }
        return;
    }

    if (canvas.spanMode) {
        if (Node* spanNode = dynamic_cast<Node*>(e.eventComponent)) {
            selectSpanNode(*spanNode);
        }
        return;
    }

    if (canvas.quaverMode == NodeCanvas::QuaverMode::Preview) {
        if (Node* previewNode = dynamic_cast<Node*>(e.eventComponent)) {
            applicationContext.processor->traversalSession.previewRequests.push({
                RTPreviewRequest::Kind::Start,
                previewNode->nodeId,
                static_cast<int>(canvas.quaverCount.getValue()),
                applicationContext.rtGraphBuilder->buildRTtraversal({ canvas.quaverTraversalId, 0 }),
                canvas.quaverRepeat
            });
        }
        return;
    }

    if (dynamic_cast<NodeCanvas*>(e.eventComponent) != nullptr) {
        handleCanvasMouseDown(e);
    }
    else if (Node* clickedNode = dynamic_cast<Node*>(e.eventComponent)) {
        handleNodeMouseDown(e, *clickedNode);
    }
}

void NodeController::selectSpanNode(Node& node)
{
    GraphState& graphState = *applicationContext.graphState;
    juce::UndoManager* const undoManager = applicationContext.undoManager;

    const int nodeId = node.nodeId;

    if (node.nodeType == NodeType::Encapsulator) {
        canvas.nodeManager.clearOutlines();
        canvas.spanAnchorNodeId = -1;

        undoManager->beginNewTransaction();
        graphState.encapsulation.dissolve(nodeId, undoManager);
        return;
    }

    if (node.nodeType != NodeType::Node && node.nodeType != NodeType::Root) {
        return;
    }

    const int nodeRootId = node.nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    const int anchorRootId = graphState.getNode(canvas.spanAnchorNodeId)
                                       .getProperty(ValueTreeIdentifiers::RootNodeId);

    if (canvas.spanAnchorNodeId < 0 || anchorRootId != nodeRootId) {
        canvas.nodeManager.clearOutlines();

        canvas.spanAnchorNodeId = nodeId;

        node.isOutlined = true;
        node.repaint();
        return;
    }

    const std::vector<int> spanNodeIds = graphState.nodeIdsBetween(canvas.spanAnchorNodeId, nodeId);

    canvas.spanAnchorNodeId = -1;

    if (spanNodeIds.empty()) {
        return;
    }

    canvas.nodeManager.clearOutlines();

    undoManager->beginNewTransaction();
    NodeFactory::createEncapsulator(graphState, spanNodeIds, undoManager);
}

void NodeController::handleCanvasMouseDown(const juce::MouseEvent& e)
{
    juce::UndoManager*       undoManager = applicationContext.undoManager;
    const juce::Point<float> clickPoint  = e.getEventRelativeTo(&canvas).position.transformedBy(canvas.modelTransform);

    if (e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (Arrow* arrow = canvas.hitTester.arrowNear(clickPoint, danglingArrowGrabRadius)) {
            if (arrow->isDangling()) {
                undoManager->beginNewTransaction();
                NodeFactory::destroyDanglingArrow(arrow->arrowTree, undoManager);
            }
            else {
                connectionOps.disconnect(arrow);
            }
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (Arrow* clickedArrow = canvas.hitTester.arrowNear(clickPoint, arrowHoverRadius)) {
            showArrowContextMenu(clickedArrow);
            return;
        }

        showSelectionMenu(clickPoint.roundToInt());
        return;
    }

    if (!e.mods.isShiftDown()) {
        if (Arrow* arrow = canvas.hitTester.danglingHeadNear(clickPoint, danglingArrowGrabRadius)) {
            draggingDanglingArrow = arrow;
            dragState             = DragState::MovingDanglingTip;
            canvas.showGrid();
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        if (Arrow* labelArrow = canvas.hitTester.arrowLabelNear(clickPoint, arrowLabelGrabRadius)) {
            labelArrow->beginDurationEdit();
            dragState = DragState::EditingValue;
            return;
        }

        if (Arrow* headArrow = canvas.hitTester.arrowHeadNear(clickPoint, arrowHeadGrabRadius)) {
            canvas.arrowManager.setSelected(headArrow);
            draggingArrowHeadNode = headArrow->endNode;
            dragState             = DragState::MovingArrowHead;
            isDragStart           = true;
            return;
        }

        if (Arrow* clickedArrow = canvas.hitTester.arrowNear(clickPoint, arrowHoverRadius)) {
            canvas.arrowManager.setSelected(clickedArrow);
            dragState = DragState::ArrowSelected;
            return;
        }

        canvas.arrowManager.clearSelection();
        selectionOps.clearAll();
    }

    if (auto* dynamicPort = dynamic_cast<DynamicPort*>(canvas.getParentComponent())) {
        dynamicPort->mouseDown(e.getEventRelativeTo(dynamicPort));
    }

    if (e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        if (!isNodeCreationModeActive()) {
            beginBoxSelection(clickPoint.roundToInt());
            return;
        }

        NodePosition nodePosition;
        nodePosition.xPosition = juce::roundToInt(clickPoint.x);
        nodePosition.yPosition = juce::roundToInt(clickPoint.y);
        nodePosition.radius    = Theme::nodeRadius;

        undoManager->beginNewTransaction();
        NodeFactory::createRootNode(*applicationContext.graphState, nodePosition, undoManager);
    }
}

void NodeController::showArrowContextMenu(Arrow* arrow)
{
    if (arrow == nullptr) {
        return;
    }

    ContextMenu menu(applicationContext);

    menu.addItem("edit allowed traversals", ContextMenu::ItemKind::Action, [this, arrow]() {
        juce::ValueTree connection = connectionOps.connectionTreeFor(arrow);
        if (!connection.isValid()) {
            return;
        }

        allowedTraversalsLauncher.show([this, connection]() {
            auto content = std::make_unique<AllowedTraversalsMenu>(applicationContext, connection);
            content->setSize(AllowedTraversalsMenu::defaultWidth, content->getIdealHeight());

            return content;
        });
    });

    menu.addItem("traversal arrow", ContextMenu::ItemKind::Toggle, [this, arrow]() {
        if (arrow->isTraversalArrow()) {
            connectionOps.setArrowType(arrow, ArrowType::Node);
            return;
        }

        connectionOps.setArrowType(arrow, ArrowType::Traversal);
    }, connectionOps.canBeTraversalArrow(arrow), arrow->isTraversalArrow());

    if (connectionOps.connectsToModulatorRoot(arrow)) {
        menu.addItem("sync", ContextMenu::ItemKind::Toggle, [this, arrow]() {
            connectionOps.setArrowSync(arrow, ! arrow->isSyncArrow());
        }, true, arrow->isSyncArrow());
    }

    menu.show(*arrow);
}

void NodeController::showSelectionMenu(juce::Point<int> canvasPoint)
{
    const bool hasSelection = selectionOps.hasSelection();
    ContextMenu menu(applicationContext);

    menu.addItem("copy",   ContextMenu::ItemKind::Action, [this]() { selectionOps.copySelection(); }, hasSelection);
    menu.addItem("paste",  ContextMenu::ItemKind::Action, [this, canvasPoint]() { selectionOps.pasteAt(canvasPoint); }, selectionOps.hasClipboard());
    menu.addItem("delete", ContextMenu::ItemKind::Action, [this]() { selectionOps.deleteSelection(); }, hasSelection);

    menu.show(canvas);
}

bool NodeController::isNodeCreationModeActive() const
{
    return !isArrowMode();
}

void NodeController::beginBoxSelection(const juce::Point<int>& clickPoint)
{
    dragState       = DragState::BoxSelecting;
    selectionAnchor = clickPoint;

    canvas.selectionBounds = {};

    selectionOps.clearAll();
}

void NodeController::handleNodeMouseDown(const juce::MouseEvent& e, Node& node)
{
    juce::UndoManager* undoManager = applicationContext.undoManager;

    dragParentCenter = node.getNodeCentre().toFloat();

    if (node.nodeValueEditor.isVisible() && e.mods.isCtrlDown() && e.mods.isShiftDown()) {
        undoManager->beginNewTransaction();

        dragState         = DragState::EditingValue;
        dragStartValue    = static_cast<double>(node.nodeValueEditor.boundValue.getValue());
        draggingValueNode = &node;
        return;
    }

    if (e.mods.isRightButtonDown() && ! e.mods.isShiftDown() && toggleEncapsulationExpansion(node)) {
        return;
    }

    const bool isShiftLeftDrag = e.mods.isLeftButtonDown() && e.mods.isShiftDown();

    if (isShiftLeftDrag && isArrowMode()) {
        dragState          = DragState::CreatingDanglingArrow;
        danglingSourceNode = &node;

        auto* const collapsedEncapsulator = dynamic_cast<Encapsulator*>(&node);

        if (collapsedEncapsulator != nullptr && ! collapsedEncapsulator->memberNodeIds.empty()) {
            Node* const exitMember = canvas.nodeManager.find(collapsedEncapsulator->memberNodeIds.back());

            if (exitMember != nullptr) {
                danglingSourceNode = exitMember;
            }
        }
    }
    else if (isShiftLeftDrag && node.nodeType == NodeType::TraversalFlag) {
        dragState = DragState::ConnectingFlag;
    }

    const int nodeId = node.nodeId;

    flagConnectionSourceId = -1;

    if (dragState == DragState::ConnectingFlag) {
        flagConnectionSourceId = nodeId;
    }

    flagConnectionTarget   = nullptr;

    node.setHoverVisual(true);
    canvas.arrowManager.clearSelection();

    selectionOps.deselectAllExcept(node);

    if (e.mods.isRightButtonDown() && e.mods.isShiftDown()) {
        undoManager->beginNewTransaction();
        applicationContext.graphState->removeNode(nodeId, undoManager);
    }
    else {
        node.setSelectVisual();
    }
}

bool NodeController::toggleEncapsulationExpansion(Node& node)
{
    const int nodeId = node.nodeId;

    if (node.nodeType == NodeType::Encapsulator) {
        canvas.encapsulationView.expand(nodeId);
        return true;
    }

    const int owningEncapsulatorId = node.nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(canvas.nodeManager.find(owningEncapsulatorId));

    if (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded || owningEncapsulator->memberNodeIds.empty()) {
        return false;
    }

    const bool isSpanBoundary = nodeId == owningEncapsulator->memberNodeIds.front()
                             || nodeId == owningEncapsulator->memberNodeIds.back();

    if (! isSpanBoundary) {
        return false;
    }

    canvas.encapsulationView.collapse(owningEncapsulatorId);
    return true;
}

void NodeController::setArrowMode(bool enabled)
{
    arrowMode = enabled;

    if (!enabled) {
        canvas.arrowManager.preview.reset();
    }
}
