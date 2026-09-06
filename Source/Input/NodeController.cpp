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
#include "../UI/Node/NodeFactory.h"
#include "../UI/Canvas/DynamicPort.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Graph/RTGraphBuilder.h"
#include "../Graph/GraphState.h"
#include "../UI/Menus/AllowedTraversalsMenu.h"
#include "../UI/Theme/CustomLookAndFeel.h"





NodeController::NodeController(ApplicationContext& context, NodeCanvas& canvasRef)
    : applicationContext(context), canvas(canvasRef)
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

    updateArrowHover(e.getEventRelativeTo(&canvas).position);
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

        const float dist = CanvasHitTester::distanceToSegment(cursor,
                                       arrow->startNode->getNodeCentre().toFloat(),
                                       arrow->endNode->getNodeCentre().toFloat());

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

void NodeController::showArrowContextMenu(Arrow* arrow)
{
    if (arrow == nullptr) {
        return;
    }

    juce::PopupMenu menu;
    menu.setLookAndFeel(applicationContext.lookAndFeel);
    menu.addItem(ArrowMenuItem::editAllowedTraversals, "edit allowed traversals");
    menu.addItem(ArrowMenuItem::traversalArrow, "traversal arrow",
                 connectionOps.canBeTraversalArrow(arrow), arrow->isTraversalArrow());

    juce::Component::SafePointer<Arrow> safeArrow(arrow);

    menu.showMenuAsync(juce::PopupMenu::Options(), [this, safeArrow] (int result)
    {
        if (safeArrow == nullptr) {
            return;
        }

        switch (result)
        {
            case ArrowMenuItem::editAllowedTraversals:
            {
                juce::ValueTree connection = connectionOps.connectionTreeFor(safeArrow);
                if (!connection.isValid()) {
                    break;
                }

                allowedTraversalsLauncher.show([this, connection]() {
                    auto content = std::make_unique<AllowedTraversalsMenu>(applicationContext, connection);
                    content->setSize(AllowedTraversalsMenu::defaultWidth, content->getIdealHeight());

                    return content;
                });
                break;
            }
            case ArrowMenuItem::traversalArrow:
            {
                connectionOps.setArrowType(safeArrow, safeArrow->isTraversalArrow() ? ArrowType::Node
                                                                                    : ArrowType::Traversal);
                break;
            }
            default: break;
        }
    });
}

void NodeController::showSelectionMenu(juce::Point<int> canvasPoint)
{
    const bool hasSelection = selectionOps.hasSelection();

    juce::PopupMenu menu;
    menu.setLookAndFeel(applicationContext.lookAndFeel);

    menu.addItem(SelectionMenuItem::copy,   "copy",   hasSelection);
    menu.addItem(SelectionMenuItem::paste,  "paste",  selectionOps.hasClipboard());
    menu.addItem(SelectionMenuItem::remove, "delete", hasSelection);

    menu.showMenuAsync(juce::PopupMenu::Options(), [this, canvasPoint] (int result)
    {
        switch (result)
        {
            case SelectionMenuItem::copy:   selectionOps.copySelection();   break;
            case SelectionMenuItem::paste:  selectionOps.pasteAt(canvasPoint); break;
            case SelectionMenuItem::remove: selectionOps.deleteSelection(); break;
            default: break;
        }
    });
}

void NodeController::endDrag()
{
    draggedNodeTree  = juce::ValueTree();
    isDragStart      = true;
    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;
    danglingSnapRoot = nullptr;

    canvas.hideGrid();
}

void NodeController::finishArrowHeadDrag()
{
    const int nodeId = draggingArrowHeadNode->getComponentID().getIntValue();

    draggingArrowHeadNode = nullptr;
    dragState             = DragState::Idle;
    isDragStart           = true;

    canvas.arrowManager.triggerSnapForNode(nodeId);
    canvas.hideGrid();
}

void NodeController::finishDanglingTipDrag()
{
    Arrow* const arrow = draggingDanglingArrow;

    draggingDanglingArrow = nullptr;
    dragState             = DragState::Idle;

    if (danglingSnapRoot != nullptr) {
        connectDanglingToRoot(arrow->startNode);
        NodeFactory::destroyDanglingArrow(arrow->arrowTree, applicationContext.undoManager);
    }
    else {
        applicationContext.undoManager->beginNewTransaction();
        NodeFactory::setDanglingArrowTip(arrow->arrowTree, arrow->tipOffset, applicationContext.undoManager);
    }

    canvas.hideGrid();
}

void NodeController::finishFlagConnection()
{
    dragState = DragState::Idle;
    canvas.arrowManager.cancelPreview();

    if (flagConnectionTarget != nullptr) {
        commitFlagConnection(flagConnectionSourceId, flagConnectionTarget);
    }

    flagConnectionTarget   = nullptr;
    flagConnectionSourceId = -1;

    canvas.hideGrid();
}

void NodeController::finishDanglingArrowCreation()
{
    if (danglingSnapRoot != nullptr) {
        Node* const startNode = canvas.arrowManager.previewStartNode();

        canvas.arrowManager.cancelPreview();
        connectDanglingToRoot(startNode);
    }
    else {
        applicationContext.undoManager->beginNewTransaction();
        canvas.arrowManager.commitPreview();
    }

    dragState   = DragState::Idle;
    isDragStart = true;

    canvas.hideGrid();
}

Node* NodeController::findDanglingSnapRoot(const Node* startNode, juce::Point<int> tip) const
{
    if (startNode == nullptr) {
        return nullptr;
    }

    const int startNodeId = startNode->getComponentID().getIntValue();

    Node* const nearestRoot = canvas.hitTester.rootNear(tip.toFloat(), rootSnapThreshold, startNodeId);
    if (nearestRoot == nullptr) {
        return nullptr;
    }

    if (startNode->nodeArrows.count(nearestRoot->getComponentID().getIntValue()) > 0) {
        return nullptr;
    }

    return nearestRoot;
}

juce::Point<int> NodeController::danglingTipFor(const Node* startNode, juce::Point<int> cursor)
{
    danglingSnapRoot = findDanglingSnapRoot(startNode, cursor);

    if (danglingSnapRoot != nullptr) {
        return danglingSnapRoot->getNodeCentre();
    }

    return canvas.snapPointToGrid(cursor);
}

void NodeController::connectDanglingToRoot(const Node* startNode)
{
    Node* const targetRoot = danglingSnapRoot;
    danglingSnapRoot = nullptr;

    if (startNode == nullptr || targetRoot == nullptr) {
        return;
    }

    connectWithSnapAnimation(startNode->getComponentID().getIntValue(),
                             targetRoot->getComponentID().getIntValue());
}

void NodeController::connectDraggedNodeToRoot()
{
    canvas.arrowManager.hideSnapGhost();

    const int rootNodeId   = snapTargetRoot->getComponentID().getIntValue();
    const int parentNodeId = snapSourceNodeId;

    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    const int draggedNodeId = (int) draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

    juce::UndoManager* undoManager = applicationContext.undoManager;
    undoManager->undo();

    canvas.cancelPendingUpdatesFor(draggedNodeId);

    connectWithSnapAnimation(parentNodeId, rootNodeId);
}

void NodeController::connectWithSnapAnimation(int parentNodeId, int childNodeId)
{
    Node* parentNode = canvas.nodeManager.find(parentNodeId);
    Node* childNode  = canvas.nodeManager.find(childNodeId);

    if (parentNode == nullptr || childNode == nullptr) {
        return;
    }

    connectionOps.connect(parentNodeId, childNodeId);

    canvas.arrowManager.connect(parentNode, childNode);
    canvas.arrowManager.refreshFor(parentNode);

    auto arrowIterator = parentNode->nodeArrows.find(childNodeId);
    if (arrowIterator != parentNode->nodeArrows.end()) {
        arrowIterator->second->triggerSnapAnimation();
    }
}

void NodeController::mouseUp(const juce::MouseEvent& e)
{
    if (dragState == DragState::MovingArrowHead && draggingArrowHeadNode != nullptr) {
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

    if (draggingDanglingArrow != nullptr) {
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

    if (isArrowMode() && canvas.arrowManager.hasPreview()) {
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
        canvas.arrowManager.triggerSnapForNode((int) draggedNodeTree.getProperty(ValueTreeIdentifiers::Id));
    }

    endDrag();
}

void NodeController::handleCanvasMouseDown(const juce::MouseEvent& e)
{
    juce::UndoManager* undoManager = applicationContext.undoManager;
    const juce::Point<float> clickPoint { (float) e.x, (float) e.y };

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

        showSelectionMenu(e.getEventRelativeTo(&canvas).getPosition());
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
            beginBoxSelection(e.getEventRelativeTo(&canvas).getPosition());
            return;
        }

        NodePosition nodePosition;
        nodePosition.xPosition = e.x;
        nodePosition.yPosition = e.y;
        nodePosition.radius    = 20;

        undoManager->beginNewTransaction();
        NodeFactory::createRootNode(*applicationContext.graphState, nodePosition, undoManager);
    }
}

bool NodeController::isNodeCreationModeActive() const
{
    return !isArrowMode();
}

void NodeController::setArrowMode(bool enabled)
{
    arrowMode = enabled;

    if (!enabled) {
        canvas.arrowManager.cancelPreview();
    }
}

void NodeController::beginBoxSelection(const juce::Point<int>& clickPoint)
{
    dragState       = DragState::BoxSelecting;
    selectionAnchor = clickPoint;

    canvas.selectionBounds = {};

    selectionOps.clearAll();
}

void NodeController::updateBoxSelection(const juce::MouseEvent& e)
{
    const juce::Point<int> cursor = e.getEventRelativeTo(&canvas).getPosition();

    canvas.selectionBounds = juce::Rectangle<int>(selectionAnchor, cursor);
    canvas.repaint();
}

void NodeController::finishBoxSelection()
{
    const juce::Rectangle<int> selection = canvas.selectionBounds;

    for (auto& [nodeId, node] : canvas.nodeManager.all()) {
        if (selection.contains(node->getNodeCentre())) {
            node->setSelectVisual(true);
        }
    }

    canvas.selectionBounds = {};
    dragState              = DragState::Idle;

    canvas.repaint();
}

void NodeController::handleNodeMouseDown(const juce::MouseEvent& e, Node& node)
{
    juce::UndoManager* undoManager = applicationContext.undoManager;

    dragParentCenter = node.getNodeCentre().toFloat();

    if (node.nodeValueEditor.isVisible()) {
        auto localPosition = e.getEventRelativeTo(&node.nodeValueEditor).getPosition();

        if (node.nodeValueEditor.getLocalBounds().contains(localPosition)) {
            dragState         = DragState::EditingValue;
            dragStartValue    = node.nodeValueEditor.boundValue.toString().getDoubleValue();
            draggingValueNode = &node;
            return;
        }
    }

    const bool isShiftLeftDrag = e.mods.isLeftButtonDown() && e.mods.isShiftDown();

    if (isShiftLeftDrag && isArrowMode()) {
        dragState = DragState::CreatingDanglingArrow;
    }
    else if (isShiftLeftDrag && node.nodeType == NodeType::TraversalFlag) {
        dragState = DragState::ConnectingFlag;
    }

    const int nodeId = node.getComponentID().getIntValue();

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

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    dragState             = DragState::Idle;
    draggingArrowHeadNode = nullptr;
    danglingSnapRoot      = nullptr;

    if (canvas.paintMode) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(&canvas);
            canvas.valueField.paintStroke(canvasEvent.position, true, e.mods.isRightButtonDown());
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

void NodeController::snapToGrid(juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree)
{
    juce::Point<int> snapped = canvas.snapPointToGrid({ newPosition.xPosition, newPosition.yPosition });
    newPosition.xPosition = snapped.x;
    newPosition.yPosition = snapped.y;

    GraphState::setNodePosition(draggedNodeTree, newPosition, undoManager);
}

void NodeController::dragValue(const juce::MouseEvent& e)
{
    const int    yOffset  = e.getOffsetFromDragStart().y;
    const int    delta    = -yOffset / 3;
    const double newValue = draggingValueNode->nodeValueEditor.clampToRange(dragStartValue + delta);

    draggingValueNode->nodeValueEditor.boundValue.setValue(newValue);
    draggingValueNode->refreshValueDisplay();
}

void NodeController::dragDanglingTip(const juce::MouseEvent& e)
{
    Node* startNode = draggingDanglingArrow->startNode;

    if (startNode == nullptr) {
        return;
    }

    const juce::Point<int> tip    = danglingTipFor(startNode, e.getEventRelativeTo(&canvas).getPosition());
    const juce::Point<int> centre = startNode->getNodeCentre();

    draggingDanglingArrow->setTipOffset({ tip.x - centre.x, tip.y - centre.y });
}

void NodeController::dragFlagConnection(const juce::MouseEvent& e, Node& node, const NodePosition& newPosition)
{
    canvas.showGrid();

    const juce::Point<int> cursor { newPosition.xPosition, newPosition.yPosition };
    flagConnectionTarget = canvas.hitTester.nodeNear(cursor.toFloat(), rootSnapThreshold, flagConnectionSourceId);

    const juce::Point<int> centre = node.getNodeCentre();
    juce::Point<int> tip = canvas.snapPointToGrid(cursor);

    if (flagConnectionTarget != nullptr) {
        tip = flagConnectionTarget->getNodeCentre();
    }

    canvas.arrowManager.updatePreview(&node, { tip.x - centre.x, tip.y - centre.y }, true);
}

void NodeController::handleCanvasMouseDrag(const juce::MouseEvent& e)
{
    if (dragState == DragState::MovingArrowHead && draggingArrowHeadNode != nullptr) {
        if (e.getDistanceFromDragStart() < dragThreshold) {
            return;
        }

        const int nodeId = draggingArrowHeadNode->getComponentID().getIntValue();
        const auto position = e.getEventRelativeTo(&canvas).getPosition();

        NodePosition newPosition;
        newPosition.xPosition = position.x;
        newPosition.yPosition = position.y;
        newPosition.radius    = defaultNodeRadius;

        handleNodeDrag(applicationContext.undoManager, nodeId, newPosition);
        return;
    }

    if (dragState == DragState::BoxSelecting) {
        updateBoxSelection(e);
        return;
    }

    if (dragState == DragState::ArrowSelected) {
        return;
    }

    if (auto* dynamicPort = dynamic_cast<DynamicPort*>(canvas.getParentComponent())) {
        dynamicPort->mouseDrag(e.getEventRelativeTo(dynamicPort));
    }
}

void NodeController::handleNodeMouseDrag(const juce::MouseEvent& e, Node& node)
{
    juce::UndoManager* undoManager = applicationContext.undoManager;

    const int  nodeId   = node.getComponentID().getIntValue();
    const auto position = e.getEventRelativeTo(node.getParentComponent());

    NodePosition newPosition;
    newPosition.xPosition = position.x;
    newPosition.yPosition = position.y;
    newPosition.radius    = defaultNodeRadius;

    if (e.getDistanceFromDragStart() < dragThreshold || !e.mods.isLeftButtonDown()) {
        return;
    }

    if (dragState == DragState::CreatingDanglingArrow && isArrowMode()) {
        updateConnectionPreview(&node, newPosition, false);
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
        snapToGrid(undoManager, newPosition, draggedNodeTree);
        checkRootNodeSnap(newPosition);
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

    if (dragState == DragState::EditingValue && draggingValueNode != nullptr) {
        dragValue(e);
        return;
    }

    if (draggingDanglingArrow != nullptr) {
        dragDanglingTip(e);
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

void NodeController::handleNodeDragStart(juce::UndoManager *undoManager, Node *node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods)
{
    juce::Identifier nodeType = node->nodeValueTree.getType();

    undoManager->beginNewTransaction();

    canvas.showGrid();

    snapSourceNodeId = nodeId;

    draggedNodeTree = NodeCreationDispatcher::create(nodeControllerMode,*applicationContext.graphState,
                                                     nodeId,nodeType,mods.isCtrlDown(),newPosition,undoManager);

    if (draggedNodeTree.isValid()) {
        connectionOps.applySelectedArrowInfo(nodeId, draggedNodeTree.getProperty(ValueTreeIdentifiers::Id));
    }
}

void NodeController::updateConnectionPreview(Node *node, const NodePosition& newPosition, bool dashed)
{
    canvas.showGrid();

    juce::Point<int> tip    = danglingTipFor(node, { newPosition.xPosition, newPosition.yPosition });
    juce::Point<int> centre = node->getNodeCentre();
    canvas.arrowManager.updatePreview(node, { tip.x - centre.x, tip.y - centre.y }, dashed);
}

void NodeController::commitFlagConnection(int sourceNodeId, Node* target)
{
    Node* sourceNode = canvas.nodeManager.find(sourceNodeId);
    if (sourceNode == nullptr || target == nullptr) {
        return;
    }

    int targetNodeId = target->getComponentID().getIntValue();

    if (sourceNode->nodeArrows.count(targetNodeId) > 0) {
        return;
    }

    connectWithSnapAnimation(sourceNodeId, targetNodeId);
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

void NodeController::checkRootNodeSnap(const NodePosition& pos)
{
    if (snapSourceNodeId < 0 || !draggedNodeTree.isValid()) {
        return;
    }

    juce::Point<int> dragPoint(pos.xPosition, pos.yPosition);

    Node* nearestRoot = canvas.hitTester.rootNear(dragPoint.toFloat(), rootSnapThreshold, snapSourceNodeId);

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
