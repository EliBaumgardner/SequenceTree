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

NodeController::NodeController(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager, TraversalSession& traversalSession,
                               RTGraphBuilder& rtGraphBuilder)
    : selectionOps(graphState, undoManager, nodeCanvas),
      graphState(graphState),
      undoManager(undoManager),
      traversalSession(traversalSession),
      rtGraphBuilder(rtGraphBuilder),
      nodeCanvas(nodeCanvas),
      connectionOps(graphState, undoManager, nodeCanvas.arrowManager.currentArrowInfo)
{
}

NodeController::~NodeController() = default;

void NodeController::mouseEnter(const juce::MouseEvent& e)
{
    if (nodeCanvas.paintMode) {
        return;
    }

    juce::Component* component = e.eventComponent;
    if (Node* node = dynamic_cast<Node*>(component)) {
        node->setHoverVisual(true);
    }
}
void NodeController::mouseExit(const juce::MouseEvent& e)
{
    if (nodeCanvas.paintMode) {
        return;
    }

    juce::Component* component = e.eventComponent;
    if (Node* node = dynamic_cast<Node*>(component)) {
        node->setHoverVisual(false);
    }
}
void NodeController::mouseMove(const juce::MouseEvent& e)
{
    if (nodeCanvas.paintMode) {
        return;
    }

    updateArrowHover(e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform));
}

void NodeController::updateArrowHover(juce::Point<float> cursor)
{
    for (Arrow* arrow : nodeCanvas.arrowManager.all()) {
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

    Arrow* const hoveredArrow = nodeCanvas.hitTester.arrowNear(cursor, arrowHoverRadius);

    for (Arrow* arrow : nodeCanvas.arrowManager.all()) {
        const bool shouldBold = (arrow == hoveredArrow);
        if (shouldBold != arrow->hovered) {
            arrow->hovered = shouldBold;
            arrow->repaint();
        }
    }
}

void NodeController::mouseDrag(const juce::MouseEvent& e)
{
    if (nodeCanvas.paintMode) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(&nodeCanvas);
            nodeCanvas.valueField.paintStroke(canvasEvent.position, false);
        }
        return;
    }

    if (nodeCanvas.spanMode || nodeCanvas.quaverMode != NodeCanvas::QuaverMode::Off) {
        return;
    }

    if (dragState == DragState::EditingValue && draggingValueNode != nullptr) {
        dragValue(e);
        return;
    }

    if (dragState == DragState::MovingDanglingTip) {
        if (draggingDanglingArrow != nullptr) {
            dragArrowTip(e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform).roundToInt());
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

void NodeController::dragArrowTip(juce::Point<int> cursor)
{
    Node* sourceNode = newArrowSourceNode;

    if (dragState == DragState::MovingDanglingTip) {
        sourceNode = draggingDanglingArrow->startNode;
    }

    if (sourceNode == nullptr) {
        return;
    }

    tipSnapTarget = findTipSnapTarget(*sourceNode, cursor);

    juce::Point<int>       tip    = nodeCanvas.snapPointToGrid(cursor);
    const juce::Point<int> centre = sourceNode->getNodeCentre();

    if (tipSnapTarget != nullptr) {
        tip = tipSnapTarget->getNodeCentre();
    }

    if (dragState == DragState::MovingDanglingTip) {
        draggingDanglingArrow->setTipOffset({ tip.x - centre.x, tip.y - centre.y });
        return;
    }

    if (dragState == DragState::CreatingDanglingArrow) {
        nodeCanvas.showGrid();
    }

    nodeCanvas.arrowManager.updatePreview(sourceNode, { tip.x - centre.x, tip.y - centre.y }, dragState == DragState::ConnectingFlag);
}

Node* NodeController::findTipSnapTarget(const Node& startNode, juce::Point<int> tip) const
{
    const int startNodeId = startNode.nodeId;

    if (dragState == DragState::ConnectingFlag) {
        return nodeCanvas.hitTester.nodeNear(tip.toFloat(), rootSnapThreshold, startNodeId);
    }

    Node* const snapTarget = nodeCanvas.hitTester.nodeContaining(tip.toFloat(), startNodeId);

    if (snapTarget == nullptr) {
        return nullptr;
    }

    if (startNode.nodeArrows.count(snapTarget->nodeId) > 0) {
        return nullptr;
    }

    const juce::Identifier startType = startNode.nodeValueTree.getType();

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
        const auto position = e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform).roundToInt();

        NodePosition newPosition;
        newPosition.xPosition = position.x;
        newPosition.yPosition = position.y;
        newPosition.radius    = Theme::nodeRadius;

        handleNodeDrag(nodeId, newPosition);
        return;
    }

    if (dragState == DragState::BoxSelecting) {
        updateBoxSelection(e);
        return;
    }

    if (dragState == DragState::ArrowSelected || dragState == DragState::EditingValue) {
        return;
    }

    if (auto* dynamicPort = dynamic_cast<DynamicPort*>(nodeCanvas.getParentComponent())) {
        dynamicPort->mouseDrag(e.getEventRelativeTo(dynamicPort));
    }
}

void NodeController::handleNodeDrag(int nodeId, NodePosition newPosition)
{
    if (isDragStart) {
        isDragStart = false;
        undoManager.beginNewTransaction();
        nodeCanvas.showGrid();
    }

    juce::ValueTree nodeValueTree = graphState.getNode(nodeId);
    NodePosition oldPosition = graphState.getNodePosition(nodeId);

    snapToGrid(newPosition, nodeValueTree);

    int deltaX = newPosition.xPosition - oldPosition.xPosition;
    int deltaY = newPosition.yPosition - oldPosition.yPosition;

    nodeCanvas.nodeManager.moveDescendants(nodeValueTree, deltaX, deltaY);
}

void NodeController::snapToGrid(NodePosition &newPosition, juce::ValueTree draggedNodeTree)
{
    const int draggedNodeId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);
    const juce::Point<int> collapseShift = nodeCanvas.encapsulationView.collapsedSpanShift(draggedNodeId);

    juce::Point<int> snapped = nodeCanvas.snapPointToGrid({ newPosition.xPosition - collapseShift.x,
                                                            newPosition.yPosition - collapseShift.y });
    newPosition.xPosition = snapped.x;
    newPosition.yPosition = snapped.y;

    graphState.setNodePosition(draggedNodeTree, newPosition, &undoManager);
}

void NodeController::updateBoxSelection(const juce::MouseEvent& e)
{
    const juce::Point<int> cursor = e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform).roundToInt();

    nodeCanvas.selectionBounds = juce::Rectangle<int>(selectionAnchor, cursor);
    nodeCanvas.repaint();
}

void NodeController::handleNodeMouseDrag(const juce::MouseEvent& e, Node& node)
{
    const int  nodeId   = node.nodeId;
    const auto position = e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform).roundToInt();

    NodePosition newPosition;
    newPosition.xPosition = position.x;
    newPosition.yPosition = position.y;
    newPosition.radius    = Theme::nodeRadius;

    if (e.getDistanceFromDragStart() < dragThreshold || !e.mods.isLeftButtonDown()) {
        return;
    }

    if ((dragState == DragState::CreatingDanglingArrow && isArrowMode()) || dragState == DragState::ConnectingFlag) {
        dragArrowTip(position);
        return;
    }

    if (!e.mods.isShiftDown() && !e.mods.isCtrlDown() && !draggedNodeTree.isValid()) {
        handleNodeDrag(nodeId, newPosition);
        return;
    }

    if (isDragStart) {
        isDragStart = false;
        handleNodeDragStart(&node, nodeId, newPosition, e.mods);
        return;
    }

    if (draggedNodeTree.isValid()) {
        const juce::Point<int> cursor { newPosition.xPosition, newPosition.yPosition };

        snapToGrid(newPosition, draggedNodeTree);
        checkRootNodeSnap(cursor);
    }
}

void NodeController::handleNodeDragStart(Node *node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods)
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

    snapSourceNodeId = parentNodeId;

    const NodePosition     parentPosition = graphState.getNodePosition(parentNodeId);
    const juce::Point<int> parentCentre   = parentNode->getNodeCentre();

    newPosition.xPosition += parentPosition.xPosition - parentCentre.x;
    newPosition.yPosition += parentPosition.yPosition - parentCentre.y;

    draggedNodeTree = NodeCreationDispatcher::create(nodeControllerMode, graphState, parentNodeId, nodeType, mods.isCtrlDown(), newPosition, &undoManager);

    if (! draggedNodeTree.isValid()) {
        return;
    }

    connectionOps.applySelectedArrowInfo(parentNodeId, draggedNodeTree.getProperty(ValueTreeIdentifiers::Id), ArrowType::StepIntoTree);

    const int owningEncapsulatorId = parentNode->nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(nodeCanvas.nodeManager.find(owningEncapsulatorId));

    if (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded
        || owningEncapsulator->memberNodeIds.empty()) {
        return;
    }

    if (parentNodeId == owningEncapsulator->memberNodeIds.back()) {
        return;
    }

    graphState.encapsulation.insertNodeAfter(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id), parentNodeId, &undoManager);
}

void NodeController::checkRootNodeSnap(juce::Point<int> canvasPoint)
{
    if (snapSourceNodeId < 0 || !draggedNodeTree.isValid()) {
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

void NodeController::setDraggedNodeVisible(bool shouldBeVisible)
{
    const int draggedId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

    Node* draggedNode = nodeCanvas.nodeManager.find(draggedId);
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

void NodeController::mouseUp(const juce::MouseEvent& e)
{
    Node* const clickedNode  = dynamic_cast<Node*>(e.eventComponent);
    const bool  isPlainClick = clickedNode != nullptr && dragState == DragState::Idle && e.mods.isLeftButtonDown() && !e.mods.isAnyModifierKeyDown() && e.getDistanceFromDragStart() < dragThreshold && !nodeCanvas.spanMode && nodeCanvas.quaverMode == NodeCanvas::QuaverMode::Off;

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

    if (dragState == DragState::MovingDanglingTip || dragState == DragState::ConnectingFlag) {
        finishArrowTip();
        return;
    }

    if (nodeCanvas.paintMode) {
        nodeCanvas.valueField.endStroke();
        return;
    }

    if (isArrowMode() && nodeCanvas.arrowManager.preview != nullptr) {
        finishArrowTip();
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
        nodeCanvas.arrowManager.triggerSnapForNode(static_cast<int>(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id)));
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

    nodeCanvas.arrowManager.triggerSnapForNode(headNode->nodeId);
}

void NodeController::finishBoxSelection()
{
    const juce::Rectangle<int> selection = nodeCanvas.selectionBounds;

    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node->isVisible() && selection.contains(node->getNodeCentre())) {
            node->setSelectVisual(true);
        }
    }

    nodeCanvas.selectionBounds = {};
    dragState              = DragState::Idle;

    nodeCanvas.repaint();
}

void NodeController::finishArrowTip()
{
    const DragState tipGesture   = dragState;
    Node* const     targetNode   = tipSnapTarget;
    Arrow*          draggedArrow = nodeCanvas.arrowManager.preview.get();

    if (tipGesture == DragState::MovingDanglingTip) {
        draggedArrow = draggingDanglingArrow;
    }

    dragState             = DragState::Idle;
    draggingDanglingArrow = nullptr;
    newArrowSourceNode    = nullptr;
    tipSnapTarget         = nullptr;
    isDragStart           = true;

    nodeCanvas.hideGrid();

    if (draggedArrow == nullptr) {
        return;
    }

    Node* const sourceNode = draggedArrow->startNode;

    if (targetNode == nullptr && tipGesture == DragState::MovingDanglingTip) {
        undoManager.beginNewTransaction();
        NodeFactory::setDanglingArrowTip(graphState, draggedArrow->arrowTree, draggedArrow->tipOffset, &undoManager);
        return;
    }

    if (targetNode == nullptr && tipGesture != DragState::ConnectingFlag) {
        undoManager.beginNewTransaction();
        nodeCanvas.arrowManager.commitPreview();
        return;
    }

    nodeCanvas.arrowManager.preview.reset();

    if (targetNode != nullptr && sourceNode != nullptr && sourceNode->nodeArrows.count(targetNode->nodeId) == 0) {
        connectWithSnapAnimation(sourceNode->nodeId, targetNode->nodeId, ArrowType::StepIntoTree);
    }

    if (targetNode != nullptr && tipGesture == DragState::MovingDanglingTip) {
        NodeFactory::destroyDanglingArrow(draggedArrow->arrowTree, &undoManager);
    }
}

void NodeController::connectWithSnapAnimation(int parentNodeId, int childNodeId,
                                              ArrowType rootConnectionType)
{
    Node* parentNode = nodeCanvas.nodeManager.find(parentNodeId);
    Node* childNode  = nodeCanvas.nodeManager.find(childNodeId);

    if (parentNode == nullptr || childNode == nullptr) {
        return;
    }

    connectionOps.connect(parentNodeId, childNodeId, rootConnectionType);

    nodeCanvas.arrowManager.connect(parentNode, childNode);
    nodeCanvas.arrowManager.refreshFor(parentNode);

    auto arrowIterator = parentNode->nodeArrows.find(childNodeId);
    if (arrowIterator != parentNode->nodeArrows.end()) {
        arrowIterator->second->triggerSnapAnimation();
    }
}

void NodeController::connectDraggedNodeToRoot()
{
    nodeCanvas.arrowManager.hideSnapGhost();

    const int rootNodeId   = snapTargetRoot->nodeId;
    const int parentNodeId = snapSourceNodeId;

    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    const int draggedNodeId = static_cast<int>(draggedNodeTree.getProperty(ValueTreeIdentifiers::Id));

    undoManager.undo();

    nodeCanvas.cancelPendingUpdatesFor(draggedNodeId);

    connectWithSnapAnimation(parentNodeId, rootNodeId, ArrowType::CrossRootTree);
}

void NodeController::endDrag()
{
    draggedNodeTree  = juce::ValueTree();
    isDragStart      = true;
    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;
    tipSnapTarget    = nullptr;

    nodeCanvas.hideGrid();
}

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    dragState             = DragState::Idle;
    draggingArrowHeadNode = nullptr;
    draggingDanglingArrow = nullptr;
    draggingValueNode     = nullptr;
    tipSnapTarget         = nullptr;
    newArrowSourceNode    = nullptr;
    snapTargetRoot        = nullptr;

    if (nodeCanvas.paintMode) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(&nodeCanvas);
            nodeCanvas.valueField.paintStroke(canvasEvent.position, true, e.mods.isRightButtonDown());
        }
        return;
    }

    if (nodeCanvas.spanMode) {
        if (Node* spanNode = dynamic_cast<Node*>(e.eventComponent)) {
            selectSpanNode(*spanNode);
        }
        return;
    }

    if (nodeCanvas.quaverMode == NodeCanvas::QuaverMode::Preview) {
        if (Node* previewNode = dynamic_cast<Node*>(e.eventComponent)) {
            traversalSession.previewRequests.push({ RTPreviewRequest::Kind::Start,
                                                    previewNode->nodeId,
                                                    static_cast<int>(nodeCanvas.quaverCount.getValue()),
                                                    rtGraphBuilder.buildRTtraversal({ nodeCanvas.quaverTraversalId, 0 }),
                                                    nodeCanvas.quaverRepeat });
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
    const int nodeId = node.nodeId;

    if (node.nodeType == NodeType::Encapsulator) {
        nodeCanvas.nodeManager.clearOutlines();
        nodeCanvas.spanAnchorNodeId = -1;

        undoManager.beginNewTransaction();
        graphState.encapsulation.dissolve(nodeId, &undoManager);
        return;
    }

    if (node.nodeType != NodeType::Node && node.nodeType != NodeType::Root) {
        return;
    }

    const int nodeRootId = node.nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    const int anchorRootId = graphState.getNode(nodeCanvas.spanAnchorNodeId)
                                 .getProperty(ValueTreeIdentifiers::RootNodeId);

    if (nodeCanvas.spanAnchorNodeId < 0 || anchorRootId != nodeRootId) {
        nodeCanvas.nodeManager.clearOutlines();

        nodeCanvas.spanAnchorNodeId = nodeId;

        node.isOutlined = true;
        node.repaint();
        return;
    }

    const std::vector<int> spanNodeIds = graphState.nodeIdsBetween(nodeCanvas.spanAnchorNodeId, nodeId);

    nodeCanvas.spanAnchorNodeId = -1;

    if (spanNodeIds.empty()) {
        return;
    }

    nodeCanvas.nodeManager.clearOutlines();

    undoManager.beginNewTransaction();
    NodeFactory::createEncapsulator(graphState, spanNodeIds, &undoManager);
}

void NodeController::handleCanvasMouseDown(const juce::MouseEvent& e)
{
    const juce::Point<float> clickPoint = e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform);

    if (e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (Arrow* arrow = nodeCanvas.hitTester.arrowNear(clickPoint, danglingArrowGrabRadius)) {
            if (arrow->isDangling()) {
                undoManager.beginNewTransaction();
                NodeFactory::destroyDanglingArrow(arrow->arrowTree, &undoManager);
            }
            else {
                connectionOps.disconnect(arrow);
            }
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (Arrow* clickedArrow = nodeCanvas.hitTester.arrowNear(clickPoint, arrowHoverRadius)) {
            showArrowContextMenu(clickedArrow);
            return;
        }

        showSelectionMenu(clickPoint.roundToInt());
        return;
    }

    if (!e.mods.isShiftDown()) {
        if (Arrow* arrow = nodeCanvas.hitTester.danglingHeadNear(clickPoint, danglingArrowGrabRadius)) {
            draggingDanglingArrow = arrow;
            dragState             = DragState::MovingDanglingTip;
            nodeCanvas.showGrid();
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        if (Arrow* labelArrow = nodeCanvas.hitTester.arrowLabelNear(clickPoint, arrowLabelGrabRadius)) {
            labelArrow->beginDurationEdit();
            dragState = DragState::EditingValue;
            return;
        }

        if (Arrow* headArrow = nodeCanvas.hitTester.arrowHeadNear(clickPoint, arrowHeadGrabRadius)) {
            nodeCanvas.arrowManager.setSelected(headArrow);
            draggingArrowHeadNode = headArrow->endNode;
            dragState             = DragState::MovingArrowHead;
            isDragStart           = true;
            return;
        }

        if (Arrow* clickedArrow = nodeCanvas.hitTester.arrowNear(clickPoint, arrowHoverRadius)) {
            nodeCanvas.arrowManager.setSelected(clickedArrow);
            dragState = DragState::ArrowSelected;
            return;
        }

        nodeCanvas.arrowManager.clearSelection();
        selectionOps.clearAll();
    }

    if (auto* dynamicPort = dynamic_cast<DynamicPort*>(nodeCanvas.getParentComponent())) {
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

        undoManager.beginNewTransaction();
        NodeFactory::createRootNode(graphState, nodePosition, &undoManager);
    }
}

void NodeController::showArrowContextMenu(Arrow* arrow)
{
    if (arrow == nullptr) {
        return;
    }

    ContextMenu menu;

    menu.addItem("edit allowed traversals", ContextMenu::ItemKind::Action, [this, arrow]() {
        juce::ValueTree connection = connectionOps.connectionTreeFor(arrow);
        if (!connection.isValid()) {
            return;
        }

        allowedTraversalsLauncher.show([this, connection]() {
            auto content = std::make_unique<AllowedTraversalsMenu>(CustomLookAndFeel::get(nodeCanvas), graphState, undoManager, connection);
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
    ContextMenu menu;

    menu.addItem("copy",   ContextMenu::ItemKind::Action, [this]() { selectionOps.copySelection(); }, hasSelection);
    menu.addItem("paste",  ContextMenu::ItemKind::Action, [this, canvasPoint]() { selectionOps.pasteAt(canvasPoint); }, selectionOps.hasClipboard());
    menu.addItem("delete", ContextMenu::ItemKind::Action, [this]() { selectionOps.deleteSelection(); }, hasSelection);

    menu.show(nodeCanvas);
}

bool NodeController::isNodeCreationModeActive() const
{
    return !isArrowMode();
}

void NodeController::beginBoxSelection(const juce::Point<int>& clickPoint)
{
    dragState       = DragState::BoxSelecting;
    selectionAnchor = clickPoint;

    nodeCanvas.selectionBounds = {};

    selectionOps.clearAll();
}

void NodeController::handleNodeMouseDown(const juce::MouseEvent& e, Node& node)
{
    dragParentCenter = node.getNodeCentre().toFloat();

    if (node.nodeValueEditor.isVisible() && e.mods.isCtrlDown() && e.mods.isShiftDown()) {
        undoManager.beginNewTransaction();

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
        newArrowSourceNode = &node;

        auto* const collapsedEncapsulator = dynamic_cast<Encapsulator*>(&node);

        if (collapsedEncapsulator != nullptr && ! collapsedEncapsulator->memberNodeIds.empty()) {
            Node* const exitMember = nodeCanvas.nodeManager.find(collapsedEncapsulator->memberNodeIds.back());

            if (exitMember != nullptr) {
                newArrowSourceNode = exitMember;
            }
        }
    }
    else if (isShiftLeftDrag && node.nodeType == NodeType::TraversalFlag) {
        dragState          = DragState::ConnectingFlag;
        newArrowSourceNode = &node;
    }

    const int nodeId = node.nodeId;

    node.setHoverVisual(true);
    nodeCanvas.arrowManager.clearSelection();

    selectionOps.deselectAllExcept(node);

    if (e.mods.isRightButtonDown() && e.mods.isShiftDown()) {
        undoManager.beginNewTransaction();
        graphState.removeNode(nodeId, &undoManager);
    }
    else {
        node.setSelectVisual();
    }
}

bool NodeController::toggleEncapsulationExpansion(Node& node)
{
    const int nodeId = node.nodeId;

    if (node.nodeType == NodeType::Encapsulator) {
        nodeCanvas.encapsulationView.expand(nodeId);
        return true;
    }

    const int owningEncapsulatorId = node.nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(nodeCanvas.nodeManager.find(owningEncapsulatorId));

    if (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded || owningEncapsulator->memberNodeIds.empty()) {
        return false;
    }

    const bool isSpanBoundary = nodeId == owningEncapsulator->memberNodeIds.front()
                             || nodeId == owningEncapsulator->memberNodeIds.back();

    if (! isSpanBoundary) {
        return false;
    }

    nodeCanvas.encapsulationView.collapse(owningEncapsulatorId);
    return true;
}

void NodeController::setArrowMode(bool enabled)
{
    arrowMode = enabled;

    if (!enabled) {
        nodeCanvas.arrowManager.preview.reset();
    }
}
