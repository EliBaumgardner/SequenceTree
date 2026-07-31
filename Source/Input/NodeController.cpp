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
#include "../Graph/ValueTreeState.h"
#include "../UI/Menus/AllowedTraversalsMenu.h"
#include "../UI/Theme/CustomLookAndFeel.h"





NodeController::NodeController(ApplicationContext& context) : applicationContext(context)
{
}

NodeController::~NodeController() = default;

void NodeController::mouseEnter(const juce::MouseEvent& e)
{
    if (applicationContext.canvas->paintMode) {
        return;
    }

    juce::Component* component = e.eventComponent;
    if (Node* node = dynamic_cast<Node*>(component)) {
        node->setHoverVisual(true);
    }
}
void NodeController::mouseExit(const juce::MouseEvent& e)
{
    if (applicationContext.canvas->paintMode) {
        return;
    }

    juce::Component* component = e.eventComponent;
    if (Node* node = dynamic_cast<Node*>(component)) {
        node->setHoverVisual(false);
    }
}
void NodeController::mouseMove(const juce::MouseEvent& e)
{
    NodeCanvas* canvas = applicationContext.canvas;

    if (canvas->paintMode) {
        return;
    }

    canvas->hoverController.update(e.getEventRelativeTo(canvas).position);
}

void NodeController::showArrowContextMenu(Arrow* arrow)
{
    if (arrow == nullptr) {
        return;
    }

    juce::PopupMenu menu;
    menu.setLookAndFeel(applicationContext.lookAndFeel);
    menu.addItem(1, "edit allowed traversals");

    juce::Component::SafePointer<Arrow> safeArrow(arrow);

    menu.showMenuAsync(juce::PopupMenu::Options(), [this, safeArrow] (int result)
    {
        switch (result)
        {
            case 1:
            {
                if (safeArrow == nullptr) {
                    break;
                }

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

void NodeController::hideGrid(NodeCanvas& canvas) const
{
    if (canvas.showGrid) {
        canvas.showGrid = false;
        canvas.repaint();
    }
}

void NodeController::showGrid(NodeCanvas& canvas) const
{
    if (canvas.gridOriginSet) {
        canvas.showGrid = true;
        canvas.repaint();
    }
}

void NodeController::endDrag(NodeCanvas& canvas)
{
    draggedNodeTree  = juce::ValueTree();
    isDragStart      = true;
    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    hideGrid(canvas);
}

void NodeController::finishArrowHeadDrag(NodeCanvas& canvas)
{
    const int nodeId = draggingArrowHeadNode->getComponentID().getIntValue();

    draggingArrowHeadNode = nullptr;
    dragState             = DragState::Idle;
    isDragStart           = true;

    canvas.arrowManager.triggerSnapForNode(nodeId);
    hideGrid(canvas);
}

void NodeController::finishDanglingTipDrag(NodeCanvas& canvas)
{
    applicationContext.undoManager->beginNewTransaction();
    canvas.danglingArrowLayer.commitTip(draggingDanglingArrow);

    draggingDanglingArrow = nullptr;
    dragState             = DragState::Idle;

    hideGrid(canvas);
}

void NodeController::finishFlagConnection(NodeCanvas& canvas)
{
    dragState = DragState::Idle;
    canvas.danglingArrowLayer.cancelPreview();

    if (flagConnectionTarget != nullptr) {
        commitFlagConnection(flagConnectionSourceId, flagConnectionTarget);
    }

    flagConnectionTarget   = nullptr;
    flagConnectionSourceId = -1;

    hideGrid(canvas);
}

void NodeController::finishDanglingArrowCreation(NodeCanvas& canvas)
{
    applicationContext.undoManager->beginNewTransaction();
    canvas.danglingArrowLayer.commitPreview();

    dragState   = DragState::Idle;
    isDragStart = true;

    hideGrid(canvas);
}

void NodeController::connectDraggedNodeToRoot(NodeCanvas& canvas)
{
    canvas.arrowManager.hideSnapGhost();

    const int rootNodeId   = snapTargetRoot->getComponentID().getIntValue();
    const int parentNodeId = snapSourceNodeId;

    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    const int draggedNodeId = (int) draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

    juce::UndoManager* undoManager = applicationContext.undoManager;
    undoManager->undo();

    canvas.asyncUpdates.erase(
        std::remove_if(canvas.asyncUpdates.begin(), canvas.asyncUpdates.end(),
            [draggedNodeId](const NodeCanvas::AsyncUpdate& update) {
                return update.nodeId == draggedNodeId
                    && update.type != NodeCanvas::AsyncUpdateType::NodeRemoved;
            }),
        canvas.asyncUpdates.end()
    );

    connectWithSnapAnimation(parentNodeId, rootNodeId);
}

void NodeController::connectWithSnapAnimation(int parentNodeId, int childNodeId)
{
    NodeCanvas* canvas = applicationContext.canvas;

    Node* parentNode = canvas->nodeManager.find(parentNodeId);
    Node* childNode  = canvas->nodeManager.find(childNodeId);

    if (parentNode == nullptr || childNode == nullptr) {
        return;
    }

    connectionOps.connect(parentNodeId, childNodeId);

    canvas->arrowManager.connect(parentNode, childNode);
    canvas->arrowManager.refreshFor(parentNode);

    auto arrowIterator = parentNode->nodeArrows.find(childNodeId);
    if (arrowIterator != parentNode->nodeArrows.end()) {
        arrowIterator->second->triggerSnapAnimation();
    }
}

void NodeController::mouseUp(const juce::MouseEvent& e)
{
    NodeCanvas* canvas = applicationContext.canvas;

    if (dragState == DragState::MovingArrowHead && draggingArrowHeadNode != nullptr) {
        finishArrowHeadDrag(*canvas);
        return;
    }

    if (dragState == DragState::BoxSelecting) {
        finishBoxSelection(*canvas);
        return;
    }

    if (dragState == DragState::ArrowSelected) {
        dragState = DragState::Idle;
    }

    if (draggingDanglingArrow != nullptr) {
        finishDanglingTipDrag(*canvas);
        return;
    }

    if (dragState == DragState::ConnectingFlag) {
        finishFlagConnection(*canvas);
        return;
    }

    if (canvas->paintMode) {
        canvas->valueField.endStroke();
        return;
    }

    if (canvas->danglingArrowLayer.isArrowMode() && canvas->danglingArrowLayer.hasPreview()) {
        finishDanglingArrowCreation(*canvas);
        return;
    }

    if (dragState == DragState::EditingValue) {
        dragState         = DragState::Idle;
        draggingValueNode = nullptr;
    }
    else if (snapTargetRoot != nullptr) {
        connectDraggedNodeToRoot(*canvas);
    }
    else if (draggedNodeTree.isValid()) {
        canvas->arrowManager.triggerSnapForNode((int) draggedNodeTree.getProperty(ValueTreeIdentifiers::Id));
    }

    endDrag(*canvas);
}

void NodeController::handleCanvasMouseDown(const juce::MouseEvent& e, NodeCanvas& canvas)
{
    juce::UndoManager* undoManager = applicationContext.undoManager;
    const juce::Point<float> clickPoint { (float) e.x, (float) e.y };

    if (e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (Arrow* arrow = canvas.hitTester.arrowNear(clickPoint, danglingArrowGrabRadius)) {
            if (arrow->isDangling()) {
                undoManager->beginNewTransaction();
                canvas.danglingArrowLayer.remove(arrow);
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
            showGrid(canvas);
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
        clearNodeSelection(canvas);
    }

    if (auto* dynamicPort = dynamic_cast<DynamicPort*>(canvas.getParentComponent())) {
        dynamicPort->mouseDown(e.getEventRelativeTo(dynamicPort));
    }

    if (e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        if (!isNodeCreationModeActive(canvas)) {
            beginBoxSelection(e.getEventRelativeTo(&canvas).getPosition(), canvas);
            return;
        }

        NodePosition nodePosition;
        nodePosition.xPosition = e.x;
        nodePosition.yPosition = e.y;
        nodePosition.radius    = 20;

        undoManager->beginNewTransaction();
        NodeFactory::createRootNode(*applicationContext.valueTreeState, nodePosition, undoManager);
    }
}

bool NodeController::isNodeCreationModeActive(const NodeCanvas& canvas) const
{
    return !canvas.danglingArrowLayer.isArrowMode();
}

void NodeController::clearNodeSelection(NodeCanvas& canvas)
{
    for (auto& [nodeId, node] : canvas.nodeManager.all()) {
        if (node->isSelected) {
            node->setSelectVisual(false);
        }
    }
}

void NodeController::beginBoxSelection(const juce::Point<int>& clickPoint, NodeCanvas& canvas)
{
    dragState       = DragState::BoxSelecting;
    selectionAnchor = clickPoint;

    canvas.selectionBounds = {};

    clearNodeSelection(canvas);
}

void NodeController::updateBoxSelection(const juce::MouseEvent& e, NodeCanvas& canvas)
{
    const juce::Point<int> cursor = e.getEventRelativeTo(&canvas).getPosition();

    canvas.selectionBounds = juce::Rectangle<int>(selectionAnchor, cursor);
    canvas.repaint();
}

void NodeController::finishBoxSelection(NodeCanvas& canvas)
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
    NodeCanvas* canvas = applicationContext.canvas;
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

    if (isShiftLeftDrag && canvas->danglingArrowLayer.isArrowMode()) {
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
    canvas->arrowManager.clearSelection();

    for (auto& [canvasNodeId, canvasNode] : canvas->nodeManager.all()) {
        if (canvasNode != &node) {
            canvasNode->setSelectVisual(false);
        }
    }

    if (e.mods.isRightButtonDown() && e.mods.isShiftDown()) {
        undoManager->beginNewTransaction();
        NodeFactory::destroyNode(*applicationContext.valueTreeState, nodeId, undoManager);
    }
    else {
        node.setSelectVisual();
    }
}

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    NodeCanvas* canvas = applicationContext.canvas;
    jassert(canvas);

    dragState             = DragState::Idle;
    draggingArrowHeadNode = nullptr;

    if (canvas->paintMode) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(canvas);
            canvas->valueField.paintStroke(canvasEvent.position, true, e.mods.isRightButtonDown());
        }
        return;
    }

    if (NodeCanvas* clickedCanvas = dynamic_cast<NodeCanvas*>(e.eventComponent)) {
        handleCanvasMouseDown(e, *clickedCanvas);
    }
    else if (Node* clickedNode = dynamic_cast<Node*>(e.eventComponent)) {
        handleNodeMouseDown(e, *clickedNode);
    }
}

juce::Point<int> NodeController::snapPointToGrid(juce::Point<int> point) const
{
    NodeCanvas* canvas = applicationContext.canvas;

    if (!canvas->gridOriginSet) {
        return point;
    }

    float spacing = canvas->gridSpacing;
    float ox = canvas->gridOrigin.x;
    float oy = canvas->gridOrigin.y;
    const float snapThreshold = 12.0f;

    float snappedX = ox + std::round((float(point.x) - ox) / spacing) * spacing;
    float snappedY = oy + std::round((float(point.y) - oy) / spacing) * spacing;

    juce::Point<int> result = point;

    if (std::abs(float(point.x) - snappedX) < snapThreshold) {
        result.x = int(snappedX);
    }
    if (std::abs(float(point.y) - snappedY) < snapThreshold) {
        result.y = int(snappedY);
    }

    return result;
}

void NodeController::snapToGrid(juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree)
{
    juce::Point<int> snapped = snapPointToGrid({ newPosition.xPosition, newPosition.yPosition });
    newPosition.xPosition = snapped.x;
    newPosition.yPosition = snapped.y;

    applicationContext.valueTreeState->setNodePosition(draggedNodeTree, newPosition, undoManager);
}

void NodeController::dragValue(const juce::MouseEvent& e)
{
    const int    yOffset  = e.getOffsetFromDragStart().y;
    const int    delta    = -yOffset / 3;
    const double newValue = dragStartValue + delta;

    draggingValueNode->nodeValueEditor.boundValue.setValue(newValue);
    draggingValueNode->refreshValueDisplay();
}

void NodeController::dragDanglingTip(const juce::MouseEvent& e, NodeCanvas& canvas)
{
    Node* startNode = draggingDanglingArrow->startNode;

    if (startNode == nullptr) {
        return;
    }

    const juce::Point<int> snapped = snapPointToGrid(e.getEventRelativeTo(&canvas).getPosition());
    const juce::Point<int> centre  = startNode->getNodeCentre();

    canvas.danglingArrowLayer.setTip(draggingDanglingArrow, { snapped.x - centre.x, snapped.y - centre.y });
}

void NodeController::dragFlagConnection(const juce::MouseEvent& e, Node& node, const NodePosition& newPosition)
{
    NodeCanvas* canvas = applicationContext.canvas;

    showGrid(*canvas);

    const juce::Point<int> cursor { newPosition.xPosition, newPosition.yPosition };
    flagConnectionTarget = canvas->hitTester.nodeNear(cursor.toFloat(), rootSnapThreshold, flagConnectionSourceId);

    const juce::Point<int> centre = node.getNodeCentre();
    juce::Point<int> tip = snapPointToGrid(cursor);

    if (flagConnectionTarget != nullptr) {
        tip = flagConnectionTarget->getNodeCentre();
    }

    canvas->danglingArrowLayer.updatePreview(&node, { tip.x - centre.x, tip.y - centre.y }, true);
}

void NodeController::handleCanvasMouseDrag(const juce::MouseEvent& e, NodeCanvas& canvas)
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
        updateBoxSelection(e, canvas);
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
    NodeCanvas* canvas = applicationContext.canvas;
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

    if (dragState == DragState::CreatingDanglingArrow && canvas->danglingArrowLayer.isArrowMode()) {
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
    NodeCanvas* canvas = applicationContext.canvas;

    if (canvas->paintMode) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(canvas);
            canvas->valueField.paintStroke(canvasEvent.position, false);
        }
        return;
    }

    if (dragState == DragState::EditingValue && draggingValueNode != nullptr) {
        dragValue(e);
        return;
    }

    if (draggingDanglingArrow != nullptr) {
        dragDanglingTip(e, *canvas);
        return;
    }

    if (NodeCanvas* draggedCanvas = dynamic_cast<NodeCanvas*>(e.eventComponent)) {
        handleCanvasMouseDrag(e, *draggedCanvas);
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

    showGrid(*applicationContext.canvas);

    snapSourceNodeId = nodeId;

    draggedNodeTree = NodeCreationDispatcher::create(nodeControllerMode,*applicationContext.valueTreeState,
                                                     nodeId,nodeType,mods.isCtrlDown(),newPosition,undoManager);
}

void NodeController::updateConnectionPreview(Node *node, const NodePosition& newPosition, bool dashed)
{
    NodeCanvas* canvas = applicationContext.canvas;

    showGrid(*canvas);

    juce::Point<int> snapped = snapPointToGrid({ newPosition.xPosition, newPosition.yPosition });
    juce::Point<int> centre  = node->getNodeCentre();
    canvas->danglingArrowLayer.updatePreview(node, { snapped.x - centre.x, snapped.y - centre.y }, dashed);
}

void NodeController::commitFlagConnection(int sourceNodeId, Node* target)
{
    NodeCanvas* canvas = applicationContext.canvas;

    Node* sourceNode = canvas->nodeManager.find(sourceNodeId);
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
        showGrid(*applicationContext.canvas);
    }

    juce::ValueTree nodeValueTree = applicationContext.valueTreeState->getNode(nodeId);
    NodePosition oldPosition = applicationContext.valueTreeState->getNodePosition(nodeId);

    snapToGrid(undoManager, newPosition, nodeValueTree);

    int deltaX = newPosition.xPosition - oldPosition.xPosition;
    int deltaY = newPosition.yPosition - oldPosition.yPosition;

    applicationContext.canvas->nodeManager.moveDescendants(nodeValueTree, deltaX, deltaY);
}

void NodeController::checkRootNodeSnap(const NodePosition& pos)
{
    if (snapSourceNodeId < 0 || !draggedNodeTree.isValid()) {
        return;
    }

    NodeCanvas* canvas = applicationContext.canvas;
    juce::Point<int> dragPoint(pos.xPosition, pos.yPosition);

    Node* nearestRoot = canvas->hitTester.rootNear(dragPoint.toFloat(), rootSnapThreshold, snapSourceNodeId);

    if (nearestRoot != nullptr) {
        if (snapTargetRoot != nearestRoot) {
            snapTargetRoot = nearestRoot;

            setDraggedNodeVisible(false);

            Node* sourceNode = canvas->nodeManager.find(snapSourceNodeId);
            if (sourceNode != nullptr) {
                canvas->arrowManager.showSnapGhost(sourceNode, nearestRoot);
            }
        }
    }
    else if (snapTargetRoot != nullptr) {
        snapTargetRoot = nullptr;

        setDraggedNodeVisible(true);

        canvas->arrowManager.hideSnapGhost();
    }
}

void NodeController::setDraggedNodeVisible(bool shouldBeVisible)
{
    NodeCanvas* canvas = applicationContext.canvas;

    const int draggedId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

    Node* draggedNode = canvas->nodeManager.find(draggedId);
    if (draggedNode == nullptr) {
        return;
    }

    draggedNode->setVisible(shouldBeVisible);

    Node* sourceNode = canvas->nodeManager.find(snapSourceNodeId);
    if (sourceNode == nullptr) {
        return;
    }

    auto arrowIt = sourceNode->nodeArrows.find(draggedId);
    if (arrowIt != sourceNode->nodeArrows.end()) {
        arrowIt->second->setVisible(shouldBeVisible);
    }
}
