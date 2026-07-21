/*
  ==============================================================================

    ObjectController.h.cpp
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/NodeArrow.h"
#include "../UI/Node/DanglingArrow.h"
#include "NodeController.h"
#include "../UI/Node/Modulator.h"
#include "../UI/Node/NodeFactory.h"
#include "../UI/Canvas/DynamicPort.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Graph/RTGraphBuilder.h"
#include "../Graph/ValueTreeState.h"
#include "../UI/AllowedTraversalsMenu.h"





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
static float distanceToSegment(juce::Point<float> p, juce::Point<float> a, juce::Point<float> b)
{
    juce::Point<float> ab = b - a;
    float lengthSquared = ab.x * ab.x + ab.y * ab.y;

    if (lengthSquared < 1.0e-6f) {
        return p.getDistanceFrom(a);
    }

    float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSquared;
    t = juce::jlimit(0.0f, 1.0f, t);

    juce::Point<float> projection = a + ab * t;
    return p.getDistanceFrom(projection);
}

void NodeController::mouseMove(const juce::MouseEvent& e)
{
    NodeCanvas* canvas = applicationContext.canvas;

    if (canvas->paintMode) {
        return;
    }

    juce::Point<float> cursor = e.getEventRelativeTo(canvas).position;

    for (NodeArrow* arrow : canvas->nodeArrows) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr) {
            continue;
        }
        if (arrow->startNode->nodeType != NodeType::TraversalFlag) {
            continue;
        }

        float dist = distanceToSegment(cursor,
                                       arrow->startNode->getNodeCentre().toFloat(),
                                       arrow->endNode->getNodeCentre().toFloat());

        bool nearby = dist < flagArrowVicinity;
        if (nearby != arrow->proximityHovered) {
            arrow->proximityHovered = nearby;
            arrow->refreshHoverVisibility();
        }
    }

    NodeArrow* hoveredArrow = findArrowNear(cursor, arrowHoverRadius);

    for (NodeArrow* arrow : canvas->nodeArrows) {
        bool shouldBold = (arrow == hoveredArrow);
        if (shouldBold != arrow->hovered) {
            arrow->hovered = shouldBold;
            arrow->repaint();
        }
    }

    DanglingArrow* hoveredDangling = findDanglingArrowNear(cursor, arrowHoverRadius);

    for (DanglingArrow* arrow : canvas->danglingArrowLayer.arrows) {
        bool shouldBold = (arrow == hoveredDangling);
        if (shouldBold != arrow->hovered) {
            arrow->hovered = shouldBold;
            arrow->repaint();
        }
    }
}

NodeArrow* NodeController::findArrowNear(juce::Point<float> point, float radius) const
{
    NodeCanvas* canvas = applicationContext.canvas;

    NodeArrow* nearest = nullptr;
    float minDist = radius;

    for (NodeArrow* arrow : canvas->nodeArrows) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr || ! arrow->isVisible()) {
            continue;
        }

        float dist = distanceToSegment(point,
                                       arrow->startNode->getNodeCentre().toFloat(),
                                       arrow->endNode->getNodeCentre().toFloat());
        if (dist < minDist) {
            minDist = dist;
            nearest = arrow;
        }
    }

    return nearest;
}

NodeArrow* NodeController::findArrowHeadNear(juce::Point<float> point, float radius) const
{
    NodeCanvas* canvas = applicationContext.canvas;

    NodeArrow* nearest = nullptr;
    float minDist = radius;

    for (NodeArrow* arrow : canvas->nodeArrows) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr || ! arrow->isVisible()) {
            continue;
        }

        juce::Point<float> start = arrow->startNode->getNodeCentre().toFloat();
        juce::Point<float> end   = arrow->endNode->getNodeCentre().toFloat();

        juce::Point<float> direction = end - start;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length < 1.0f) {
            continue;
        }

        direction /= length;

        float childRadius = arrow->endNode->getHeight() * 0.5f;
        juce::Point<float> headAnchor = end - direction * (childRadius + 8.0f);

        float dist = point.getDistanceFrom(headAnchor);
        if (dist < minDist) {
            minDist = dist;
            nearest = arrow;
        }
    }

    return nearest;
}

DanglingArrow* NodeController::findDanglingArrowNear(juce::Point<float> point, float radius) const
{
    NodeCanvas* canvas = applicationContext.canvas;

    DanglingArrow* nearest = nullptr;
    float minDist = radius;

    for (DanglingArrow* arrow : canvas->danglingArrowLayer.arrows) {
        if (arrow->startNode == nullptr) {
            continue;
        }

        float dist = distanceToSegment(point,
                                       arrow->startNode->getNodeCentre().toFloat(),
                                       arrow->getTip().toFloat());
        if (dist < minDist) {
            minDist = dist;
            nearest = arrow;
        }
    }

    return nearest;
}

void NodeController::deleteArrow(NodeArrow* arrow)
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return;
    }

    int startId = arrow->startNode->getComponentID().getIntValue();
    int endId   = arrow->endNode->getComponentID().getIntValue();

    juce::ValueTree startTree     = applicationContext.valueTreeState->getNode(startId);
    juce::ValueTree startChildren = startTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
    bool startOwnsEnd = startChildren.getChildWithProperty(ValueTreeIdentifiers::Id, endId).isValid();

    int ownerNodeId = startOwnsEnd ? startId : endId;
    int childNodeId = startOwnsEnd ? endId : startId;

    juce::UndoManager* undoManager = applicationContext.undoManager;
    undoManager->beginNewTransaction();
    applicationContext.valueTreeState->disconnectNodes(ownerNodeId, childNodeId, undoManager);
}

juce::ValueTree NodeController::getArrowConnectionTree(NodeArrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return {};
    }

    int startId = arrow->startNode->getComponentID().getIntValue();
    int endId   = arrow->endNode->getComponentID().getIntValue();

    juce::ValueTree startTree     = applicationContext.valueTreeState->getNode(startId);
    juce::ValueTree startChildren = startTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
    bool startOwnsEnd = startChildren.getChildWithProperty(ValueTreeIdentifiers::Id, endId).isValid();

    int ownerNodeId = startOwnsEnd ? startId : endId;
    int childNodeId = startOwnsEnd ? endId : startId;

    juce::ValueTree ownerTree     = applicationContext.valueTreeState->getNode(ownerNodeId);
    juce::ValueTree ownerChildren = ownerTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    return ownerChildren.getChildWithProperty(ValueTreeIdentifiers::Id, childNodeId);
}

void NodeController::showArrowContextMenu(NodeArrow* arrow)
{
    if (arrow == nullptr) {
        return;
    }

    juce::PopupMenu menu;
    menu.setLookAndFeel(applicationContext.lookAndFeel);
    menu.addItem(1, "edit allowed traversals");

    juce::Component::SafePointer<NodeArrow> safeArrow(arrow);

    menu.showMenuAsync(juce::PopupMenu::Options(), [this, safeArrow] (int result)
    {
        switch (result)
        {
            case 1:
            {
                if (safeArrow == nullptr) {
                    break;
                }

                juce::ValueTree connection = getArrowConnectionTree(safeArrow);
                if (!connection.isValid()) {
                    break;
                }

                allowedTraversalsWindow = std::make_unique<AllowedTraversalsWindow>(applicationContext, connection);
                allowedTraversalsWindow->centreWithSize(allowedTraversalsWindow->getWidth(),
                                                        allowedTraversalsWindow->getHeight());
                allowedTraversalsWindow->setVisible(true);
                break;
            }
            default: break;
        }
    });
}

void NodeController::showDanglingArrowContextMenu(DanglingArrow* arrow)
{
    if (arrow == nullptr) {
        return;
    }

    juce::PopupMenu menu;
    menu.setLookAndFeel(applicationContext.lookAndFeel);
    menu.addItem(1, "edit allowed traversals");

    juce::Component::SafePointer<DanglingArrow> safeArrow(arrow);

    menu.showMenuAsync(juce::PopupMenu::Options(), [this, safeArrow] (int result)
    {
        switch (result)
        {
            case 1:
            {
                if (safeArrow == nullptr) {
                    break;
                }

                juce::ValueTree connection = safeArrow->arrowTree;
                if (!connection.isValid()) {
                    break;
                }

                allowedTraversalsWindow = std::make_unique<AllowedTraversalsWindow>(applicationContext, connection);
                allowedTraversalsWindow->centreWithSize(allowedTraversalsWindow->getWidth(),
                                                        allowedTraversalsWindow->getHeight());
                allowedTraversalsWindow->setVisible(true);
                break;
            }
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

    canvas.triggerArrowSnapForNode(nodeId);
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
    canvas.hideSnapGhostArrow();

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

    undoManager->beginNewTransaction();
    applicationContext.valueTreeState->connectNodes(parentNodeId, rootNodeId, undoManager);

    auto parentIterator = canvas.nodeMap.find(parentNodeId);
    auto rootIterator   = canvas.nodeMap.find(rootNodeId);

    if (parentIterator == canvas.nodeMap.end() || rootIterator == canvas.nodeMap.end()) {
        return;
    }

    canvas.addLinePoints(parentIterator->second, rootIterator->second);
    canvas.updateLinePoints(parentIterator->second);

    auto arrowIterator = parentIterator->second->nodeArrows.find(rootNodeId);
    if (arrowIterator != parentIterator->second->nodeArrows.end()) {
        arrowIterator->second->triggerSnapAnimation();
    }

    juce::ValueTree parentValueTree = applicationContext.valueTreeState->getNode(parentNodeId);
    if (!parentValueTree.isValid()) {
        return;
    }

    const int parentRootId = parentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    juce::ValueTree parentRootValueTree = applicationContext.valueTreeState->getNode(parentRootId);

    if (parentRootValueTree.isValid()) {
        applicationContext.rtGraphBuilder->makeRTGraph(parentRootValueTree);
    }
}

void NodeController::mouseUp(const juce::MouseEvent& e)
{
    NodeCanvas* canvas = applicationContext.canvas;

    if (dragState == DragState::MovingArrowHead && draggingArrowHeadNode != nullptr) {
        finishArrowHeadDrag(*canvas);
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
        canvas->endStroke();
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
        canvas->triggerArrowSnapForNode((int) draggedNodeTree.getProperty(ValueTreeIdentifiers::Id));
    }

    endDrag(*canvas);
}

void NodeController::handleCanvasMouseDown(const juce::MouseEvent& e, NodeCanvas& canvas)
{
    juce::UndoManager* undoManager = applicationContext.undoManager;
    const juce::Point<float> clickPoint { (float) e.x, (float) e.y };

    if (e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (DanglingArrow* arrow = findDanglingArrowNear(clickPoint, danglingArrowGrabRadius)) {
            undoManager->beginNewTransaction();
            canvas.danglingArrowLayer.remove(arrow);
            return;
        }
        if (NodeArrow* nodeArrow = findArrowNear(clickPoint, danglingArrowGrabRadius)) {
            deleteArrow(nodeArrow);
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
        if (NodeArrow* clickedArrow = findArrowNear(clickPoint, arrowHoverRadius)) {
            showArrowContextMenu(clickedArrow);
            return;
        }
        if (DanglingArrow* clickedDangling = findDanglingArrowNear(clickPoint, arrowHoverRadius)) {
            showDanglingArrowContextMenu(clickedDangling);
            return;
        }
    }

    if (!e.mods.isShiftDown()) {
        if (DanglingArrow* arrow = canvas.danglingArrowLayer.hitTestHead({ e.x, e.y }, danglingArrowGrabRadius)) {
            draggingDanglingArrow = arrow;
            dragState             = DragState::MovingDanglingTip;
            showGrid(canvas);
            return;
        }
    }

    if (!e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        if (NodeArrow* headArrow = findArrowHeadNear(clickPoint, arrowHeadGrabRadius)) {
            canvas.setSelectedArrow(headArrow);
            draggingArrowHeadNode = headArrow->endNode;
            dragState             = DragState::MovingArrowHead;
            isDragStart           = true;
            return;
        }

        if (NodeArrow* clickedArrow = findArrowNear(clickPoint, arrowHoverRadius)) {
            canvas.setSelectedArrow(clickedArrow);
            dragState = DragState::ArrowSelected;
            return;
        }

        if (DanglingArrow* clickedDangling = findDanglingArrowNear(clickPoint, arrowHoverRadius)) {
            canvas.setSelectedDanglingArrow(clickedDangling);
            dragState = DragState::ArrowSelected;
            return;
        }

        canvas.clearArrowSelection();
    }

    if (auto* dynamicPort = dynamic_cast<DynamicPort*>(canvas.getParentComponent())) {
        dynamicPort->mouseDown(e.getEventRelativeTo(dynamicPort));
    }

    if (e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
        NodePosition nodePosition;
        nodePosition.xPosition = e.x;
        nodePosition.yPosition = e.y;
        nodePosition.radius    = 20;

        undoManager->beginNewTransaction();
        NodeFactory::createRootNode(*applicationContext.valueTreeState, nodePosition, undoManager);
    }
}

void NodeController::handleNodeMouseDown(const juce::MouseEvent& e, Node& node)
{
    NodeCanvas* canvas = applicationContext.canvas;
    juce::UndoManager* undoManager = applicationContext.undoManager;

    dragParentCenter = node.getNodeCentre().toFloat();

    if (node.nodeTextEditor != nullptr && node.nodeTextEditor->isVisible()) {
        auto localPosition = e.getEventRelativeTo(node.nodeTextEditor.get()).getPosition();

        if (node.nodeTextEditor->getLocalBounds().contains(localPosition)) {
            dragState         = DragState::EditingValue;
            dragStartValue    = node.nodeTextEditor->bindValue.toString().getDoubleValue();
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

    flagConnectionSourceId = (dragState == DragState::ConnectingFlag) ? nodeId : -1;
    flagConnectionTarget   = nullptr;

    node.setHoverVisual(true);
    canvas->clearArrowSelection();

    for (auto& [canvasNodeId, canvasNode] : canvas->nodeMap) {
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
            canvas->paintStroke(canvasEvent.position, true, e.mods.isRightButtonDown());
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

    draggingValueNode->nodeTextEditor->bindValue.setValue(newValue);
    draggingValueNode->nodeTextEditor->formatDisplay(draggingValueNode->nodeTextEditor->mode);
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
    flagConnectionTarget = findConnectionTarget(cursor, flagConnectionSourceId);

    const juce::Point<int> centre = node.getNodeCentre();
    const juce::Point<int> tip    = flagConnectionTarget != nullptr
        ? flagConnectionTarget->getNodeCentre()
        : snapPointToGrid(cursor);

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
            canvas->paintStroke(canvasEvent.position, false);
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
    juce::Identifier nodeType =  node->nodeValueTree.getType();

    undoManager->beginNewTransaction();

    if (applicationContext.canvas->gridOriginSet) {
        applicationContext.canvas->showGrid = true;
        applicationContext.canvas->repaint();
    }

    snapSourceNodeId = nodeId;

    const bool makeAlt = mods.isCtrlDown();

    if (nodeControllerMode == NodeControllerMode::Node) {
        if (makeAlt) {
            draggedNodeTree = NodeFactory::createAlternativeNode(*applicationContext.valueTreeState, nodeId, newPosition, undoManager);
        }
        else {
            draggedNodeTree = NodeFactory::createNode(*applicationContext.valueTreeState, nodeId, newPosition, undoManager);
        }
    }
    else if (nodeControllerMode == NodeControllerMode::Modulator) {
        if (nodeType == ValueTreeIdentifiers::ModulatorRootData || nodeType == ValueTreeIdentifiers::ModulatorData) {
            draggedNodeTree = NodeFactory::createModulator(*applicationContext.valueTreeState, nodeId, newPosition, undoManager);
        }
        else {
            draggedNodeTree = NodeFactory::createModulatorRoot(*applicationContext.valueTreeState, nodeId, newPosition, undoManager);
        }
    }
    else if (nodeControllerMode == NodeControllerMode::TraversalFlag) {
        draggedNodeTree = NodeFactory::createTraversalFlagNode(*applicationContext.valueTreeState, nodeId, newPosition, undoManager);
    }
}

void NodeController::updateConnectionPreview(Node *node, const NodePosition& newPosition, bool dashed)
{
    NodeCanvas* canvas = applicationContext.canvas;

    if (canvas->gridOriginSet) {
        canvas->showGrid = true;
        canvas->repaint();
    }

    juce::Point<int> snapped = snapPointToGrid({ newPosition.xPosition, newPosition.yPosition });
    juce::Point<int> centre  = node->getNodeCentre();
    canvas->danglingArrowLayer.updatePreview(node, { snapped.x - centre.x, snapped.y - centre.y }, dashed);
}

Node* NodeController::findConnectionTarget(juce::Point<int> point, int excludeNodeId) const
{
    NodeCanvas* canvas = applicationContext.canvas;

    Node* nearest = nullptr;
    float minDist = rootSnapThreshold;

    for (auto& [id, node] : canvas->nodeMap)
    {
        if (id == excludeNodeId) {
            continue;
        }

        float dist = (float) point.getDistanceFrom(node->getNodeCentre());
        if (dist < minDist) {
            minDist = dist;
            nearest = node;
        }
    }

    return nearest;
}

void NodeController::commitFlagConnection(int sourceNodeId, Node* target)
{
    NodeCanvas* canvas = applicationContext.canvas;

    auto sourceIt = canvas->nodeMap.find(sourceNodeId);
    if (sourceIt == canvas->nodeMap.end() || target == nullptr) {
        return;
    }

    int targetNodeId = target->getComponentID().getIntValue();

    if (sourceIt->second->nodeArrows.count(targetNodeId) > 0) {
        return;
    }

    juce::UndoManager* undoManager = applicationContext.undoManager;
    undoManager->beginNewTransaction();
    applicationContext.valueTreeState->connectNodes(sourceNodeId, targetNodeId, undoManager);

    canvas->addLinePoints(sourceIt->second, target);
    canvas->updateLinePoints(sourceIt->second);

    auto arrowIt = sourceIt->second->nodeArrows.find(targetNodeId);
    if (arrowIt != sourceIt->second->nodeArrows.end()) {
        arrowIt->second->triggerSnapAnimation();
    }

    juce::ValueTree sourceTree = applicationContext.valueTreeState->getNode(sourceNodeId);
    if (sourceTree.isValid()) {
        int rootId = sourceTree.getProperty(ValueTreeIdentifiers::RootNodeId);
        juce::ValueTree rootTree = applicationContext.valueTreeState->getNode(rootId);
        if (rootTree.isValid()) {
            applicationContext.rtGraphBuilder->makeRTGraph(rootTree);
        }
    }
}

void NodeController::handleNodeDrag(juce::UndoManager *undoManager, int nodeId, NodePosition newPosition)
{
    if (isDragStart) {
        isDragStart = false;
        undoManager->beginNewTransaction();
        if (applicationContext.canvas->gridOriginSet) {
            applicationContext.canvas->showGrid = true;
            applicationContext.canvas->repaint();
        }
    }

    juce::ValueTree nodeValueTree = applicationContext.valueTreeState->getNode(nodeId);
    NodePosition oldPosition = applicationContext.valueTreeState->getNodePosition(nodeId);

    snapToGrid(undoManager, newPosition, nodeValueTree);

    int deltaX = newPosition.xPosition - oldPosition.xPosition;
    int deltaY = newPosition.yPosition - oldPosition.yPosition;

    applicationContext.canvas->moveDescendants(nodeValueTree, deltaX, deltaY);
}

void NodeController::checkRootNodeSnap(const NodePosition& pos)
{
    if (snapSourceNodeId < 0 || !draggedNodeTree.isValid()) {
        return;
    }

    NodeCanvas* canvas = applicationContext.canvas;
    juce::Point<int> dragPoint(pos.xPosition, pos.yPosition);

    Node* nearestRoot = nullptr;
    float minDist = rootSnapThreshold;

    for (auto& [id, node] : canvas->nodeMap)
    {
        if (node->nodeValueTree.getType() != ValueTreeIdentifiers::RootNodeData) {
            continue;
        }
        if (id == snapSourceNodeId) {
            continue;
        }

        float dist = (float)dragPoint.getDistanceFrom(node->getNodeCentre());
        if (dist < minDist) {
            minDist = dist;
            nearestRoot = node;
        }
    }

    if (nearestRoot != nullptr) {
        if (snapTargetRoot != nearestRoot) {
            snapTargetRoot = nearestRoot;

            int draggedId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

            auto draggedIt = canvas->nodeMap.find(draggedId);
            if (draggedIt != canvas->nodeMap.end()) {
                draggedIt->second->setVisible(false);

                auto sourceIt = canvas->nodeMap.find(snapSourceNodeId);
                if (sourceIt != canvas->nodeMap.end()) {
                    auto arrowIt = sourceIt->second->nodeArrows.find(draggedId);
                    if (arrowIt != sourceIt->second->nodeArrows.end()) {
                        arrowIt->second->setVisible(false);
                    }
                }
            }

            auto sourceIt = canvas->nodeMap.find(snapSourceNodeId);
            if (sourceIt != canvas->nodeMap.end()) {
                canvas->showSnapGhostArrow(sourceIt->second, nearestRoot);
            }
        }
    }
    else if (snapTargetRoot != nullptr) {
        snapTargetRoot = nullptr;

        int draggedId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

        auto draggedIt = canvas->nodeMap.find(draggedId);

        if (draggedIt != canvas->nodeMap.end()) {
            draggedIt->second->setVisible(true);

            auto sourceIt = canvas->nodeMap.find(snapSourceNodeId);
            if (sourceIt != canvas->nodeMap.end()) {
                auto arrowIt = sourceIt->second->nodeArrows.find(draggedId);
                if (arrowIt != sourceIt->second->nodeArrows.end()) {
                    arrowIt->second->setVisible(true);
                }
            }
        }

        canvas->hideSnapGhostArrow();
    }
}
