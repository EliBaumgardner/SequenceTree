/*
  ==============================================================================

    ObjectController.h.cpp
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

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





NodeController::NodeController(ApplicationContext& context) : applicationContext(context)
{
}

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

void NodeController::deleteArrow(NodeArrow* arrow)
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return;
    }

    NodeCanvas* canvas = applicationContext.canvas;
    juce::UndoManager* undoManager = applicationContext.undoManager;

    int parentNodeId = arrow->startNode->getComponentID().getIntValue();
    int childNodeId  = arrow->endNode->getComponentID().getIntValue();

    undoManager->beginNewTransaction();
    ValueTreeState::disconnectNodes(parentNodeId, childNodeId, undoManager);

    canvas->removeArrow(arrow);

    juce::ValueTree parentTree = ValueTreeState::getNode(parentNodeId);
    if (parentTree.isValid()) {
        int rootId = parentTree.getProperty(ValueTreeIdentifiers::RootNodeId);
        juce::ValueTree rootTree = ValueTreeState::getNode(rootId);
        if (rootTree.isValid()) {
            applicationContext.rtGraphBuilder->makeRTGraph(rootTree);
        }
    }
}

void NodeController::mouseUp(const juce::MouseEvent& e)
{
    NodeCanvas* canvas = applicationContext.canvas;

    if (draggingDanglingArrow != nullptr) {
        applicationContext.undoManager->beginNewTransaction();
        canvas->commitDanglingArrowTip(draggingDanglingArrow);
        draggingDanglingArrow = nullptr;

        if (canvas->showGrid) {
            canvas->showGrid = false;
            canvas->repaint();
        }
        return;
    }

    if (draggingFlagConnection) {
        draggingFlagConnection = false;
        canvas->cancelDanglingPreview();

        if (flagConnectionTarget != nullptr) {
            commitFlagConnection(flagConnectionSourceId, flagConnectionTarget);
        }

        flagConnectionTarget   = nullptr;
        flagConnectionSourceId = -1;

        if (canvas->showGrid) {
            canvas->showGrid = false;
            canvas->repaint();
        }
        return;
    }

    if (canvas->paintMode) {
        canvas->endStroke();
        return;
    }

    if (canvas->arrowMode && canvas->hasDanglingPreview()) {
        applicationContext.undoManager->beginNewTransaction();
        canvas->commitDanglingArrow();
        isDragStart = true;

        if (canvas->showGrid) {
            canvas->showGrid = false;
            canvas->repaint();
        }
        return;
    }

    if (isDraggingValue) {
        isDraggingValue = false;
        draggingValueNode = nullptr;
    }
    else if (snapTargetRoot != nullptr) {
        canvas->hideSnapGhostArrow();

        int rootNodeId = snapTargetRoot->getComponentID().getIntValue();
        DBG("rootNodeId:" + juce::String(rootNodeId));
        int parentNodeId   = snapSourceNodeId;

        snapTargetRoot   = nullptr;
        snapSourceNodeId = -1;

        int draggedNodeId = (int)draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

        juce::UndoManager* undoManager = applicationContext.undoManager;
        undoManager->undo();

        canvas->asyncUpdates.erase(
            std::remove_if(canvas->asyncUpdates.begin(), canvas->asyncUpdates.end(),
                [draggedNodeId](const NodeCanvas::AsyncUpdate& u) {
                    return u.nodeId == draggedNodeId
                        && u.type != NodeCanvas::AsyncUpdateType::NodeRemoved;
                }),
            canvas->asyncUpdates.end()
        );

        undoManager->beginNewTransaction();
        ValueTreeState::connectNodes(parentNodeId, rootNodeId, undoManager);

        auto parentIterator = canvas->nodeMap.find(parentNodeId);
        auto rootIterator   = canvas->nodeMap.find(rootNodeId);

        if (parentIterator != canvas->nodeMap.end() && rootIterator != canvas->nodeMap.end()) {
            canvas->addLinePoints(parentIterator->second, rootIterator->second);
            canvas->updateLinePoints(parentIterator->second);

            auto arrowIterator = parentIterator->second->nodeArrows.find(rootNodeId);

            if (arrowIterator != parentIterator->second->nodeArrows.end()) {
                arrowIterator->second->triggerSnapAnimation();
            }

            juce::ValueTree parentValueTree = ValueTreeState::getNode(parentNodeId);
            if (parentValueTree.isValid()) {
                int parentRootId = parentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
                juce::ValueTree parentRootValueTree = ValueTreeState::getNode(parentRootId);
                if (parentRootValueTree.isValid()) {
                    applicationContext.rtGraphBuilder->makeRTGraph(parentRootValueTree);
                }
            }
        }
    }
    else {
        if (draggedNodeTree.isValid()) {
            int nodeId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);
            canvas->triggerArrowSnapForNode(nodeId);
        }
    }

    draggedNodeTree  = juce::ValueTree();
    isDragStart      = true;
    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    if (canvas->showGrid) {
        canvas->showGrid = false;
        canvas->repaint();
    }
}

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    NodeCanvas* nodeCanvas = applicationContext.canvas;
    juce::Component* component = e.eventComponent;
    juce::UndoManager* undoManager = applicationContext.undoManager;

    jassert(nodeCanvas);

    creatingDanglingArrow = false;
    draggingFlagConnection = false;

    if (nodeCanvas->paintMode) {
        if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
            auto canvasEvent = e.getEventRelativeTo(nodeCanvas);
            nodeCanvas->paintStroke(canvasEvent.position, true, e.mods.isRightButtonDown());
        }
        return;
    }

    if (NodeCanvas* canvas = dynamic_cast<NodeCanvas*>(component)) {

        if (! canvas->paintMode && e.mods.isShiftDown() && e.mods.isRightButtonDown()) {
            if (DanglingArrow* arrow = canvas->hitTestDanglingArrowHead({ e.x, e.y }, danglingArrowGrabRadius)) {
                undoManager->beginNewTransaction();
                canvas->removeDanglingArrow(arrow);
                return;
            }
            if (NodeArrow* nodeArrow = findArrowNear({ (float) e.x, (float) e.y }, danglingArrowGrabRadius)) {
                deleteArrow(nodeArrow);
                return;
            }
        }

        if (! canvas->paintMode && ! e.mods.isShiftDown()) {
            if (DanglingArrow* arrow = canvas->hitTestDanglingArrowHead({ e.x, e.y }, danglingArrowGrabRadius)) {
                draggingDanglingArrow = arrow;
                if (canvas->gridOriginSet) {
                    canvas->showGrid = true;
                    canvas->repaint();
                }
                return;
            }
        }

        auto parent = component->getParentComponent();
        if (auto* dynamicPort = dynamic_cast<DynamicPort*>(parent)) {
            dynamicPort->mouseDown(e.getEventRelativeTo(dynamicPort));
        }

        if (e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
            NodePosition nodePosition;

            nodePosition.xPosition = e.x;
            nodePosition.yPosition = e.y;
            nodePosition.radius    = 20;

            undoManager->beginNewTransaction();
            NodeFactory::createRootNode(nodePosition,undoManager);
        }
    }
    else if (Node* node= dynamic_cast<Node*>(component) ) {

        dragParentCenter = node->getNodeCentre().toFloat();

        if (node->nodeTextEditor != nullptr && node->nodeTextEditor->isVisible()) {
            auto localPos = e.getEventRelativeTo(node->nodeTextEditor.get()).getPosition();
            if (node->nodeTextEditor->getLocalBounds().contains(localPos)) {
                isDraggingValue = true;
                dragStartValue = node->nodeTextEditor->bindValue.toString().getDoubleValue();
                draggingValueNode = node;
                return;
            }
        }

        creatingDanglingArrow = nodeCanvas->arrowMode
            && e.mods.isLeftButtonDown()
            && e.mods.isShiftDown();

        draggingFlagConnection = ! nodeCanvas->arrowMode
            && node->nodeType == NodeType::TraversalFlag
            && e.mods.isLeftButtonDown()
            && e.mods.isShiftDown();

        flagConnectionSourceId = draggingFlagConnection ? node->getComponentID().getIntValue() : -1;
        flagConnectionTarget   = nullptr;

        node->setHoverVisual(true);
        int nodeId = node->getComponentID().getIntValue();

        for (auto& [nodeId, canvasNode] : nodeCanvas->nodeMap) {
            if(canvasNode != node) {
                canvasNode->setSelectVisual(false);
            }
        }

       if(e.mods.isRightButtonDown() && e.mods.isShiftDown()) {

            undoManager->beginNewTransaction();
            NodeFactory::destroyNode(nodeId, undoManager);
        }
        else {
            node->setSelectVisual();
        }
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

    ValueTreeState::setNodePosition(draggedNodeTree, newPosition, undoManager);
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

    if (isDraggingValue && draggingValueNode != nullptr) {
        int yOffset = e.getOffsetFromDragStart().y;
        int delta = -yOffset / 3;
        double newValue = dragStartValue + delta;
        draggingValueNode->nodeTextEditor->bindValue.setValue(newValue);
        draggingValueNode->nodeTextEditor->formatDisplay(draggingValueNode->nodeTextEditor->mode);
        return;
    }

    if (draggingDanglingArrow != nullptr) {
        Node* startNode = draggingDanglingArrow->startNode;
        if (startNode != nullptr) {
            auto pos = e.getEventRelativeTo(canvas).getPosition();
            juce::Point<int> snapped = snapPointToGrid(pos);
            juce::Point<int> centre = startNode->getNodeCentre();
            canvas->setDanglingArrowTip(draggingDanglingArrow, { snapped.x - centre.x, snapped.y - centre.y });
        }
        return;
    }

    juce::Component* component = e.eventComponent;
    juce::UndoManager* undoManager = applicationContext.undoManager;

    if (NodeCanvas* nodeCanvas = dynamic_cast<NodeCanvas*>(component)) {

        auto parent = component->getParentComponent();

        if (auto* dynamicPort = dynamic_cast<DynamicPort*>(parent)) {
            auto parentEvent = e.getEventRelativeTo(dynamicPort);
            dynamicPort->mouseDrag(parentEvent);
        }
        return;
    }

    if (Node* node = dynamic_cast<Node*>(component)) {

        int nodeId = node->getComponentID().getIntValue();
        NodePosition newPosition;

        int defaultRadius = 20;

        auto pos = e.getEventRelativeTo(node->getParentComponent());
        newPosition.xPosition = pos.x;
        newPosition.yPosition = pos.y;
        newPosition.radius    = defaultRadius;

        if (e.getDistanceFromDragStart() < 5 || !e.mods.isLeftButtonDown()) {
            return;
        }

        if (canvas->arrowMode && creatingDanglingArrow) {
            updateConnectionPreview(node, newPosition, false);
            return;
        }

        if (draggingFlagConnection) {
            if (canvas->gridOriginSet) {
                canvas->showGrid = true;
                canvas->repaint();
            }

            juce::Point<int> cursor { newPosition.xPosition, newPosition.yPosition };
            flagConnectionTarget = findConnectionTarget(cursor, flagConnectionSourceId);

            juce::Point<int> centre = node->getNodeCentre();
            juce::Point<int> tip = flagConnectionTarget != nullptr
                ? flagConnectionTarget->getNodeCentre()
                : snapPointToGrid(cursor);

            canvas->updateDanglingPreview(node, { tip.x - centre.x, tip.y - centre.y }, true);
            return;
        }

        if (!e.mods.isShiftDown() && !e.mods.isCtrlDown() && !draggedNodeTree.isValid()) {
            handleNodeDrag(undoManager, nodeId, newPosition);
            return;
        }

        if (isDragStart) {
            isDragStart = false;
            handleNodeDragStart(undoManager, node, nodeId, newPosition, e.mods);
            return;
        }

        if (draggedNodeTree.isValid()) {
            snapToGrid(undoManager, newPosition, draggedNodeTree);
            checkRootNodeSnap(newPosition);
        }
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
            draggedNodeTree = NodeFactory::createAlternativeNode(nodeId, newPosition, undoManager);
        }
        else {
            draggedNodeTree = NodeFactory::createNode(nodeId, newPosition, undoManager);
        }
    }
    else if (nodeControllerMode == NodeControllerMode::Modulator) {
        if (nodeType == ValueTreeIdentifiers::ModulatorRootData || nodeType == ValueTreeIdentifiers::ModulatorData) {
            draggedNodeTree = NodeFactory::createModulator(nodeId, newPosition, undoManager);
        }
        else {
            draggedNodeTree = NodeFactory::createModulatorRoot(nodeId, newPosition, undoManager);
        }
    }
    else if (nodeControllerMode == NodeControllerMode::TraversalFlag) {
        draggedNodeTree = NodeFactory::createTraversalFlagNode(nodeId, newPosition, undoManager);
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
    canvas->updateDanglingPreview(node, { snapped.x - centre.x, snapped.y - centre.y }, dashed);
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
    ValueTreeState::connectNodes(sourceNodeId, targetNodeId, undoManager);

    canvas->addLinePoints(sourceIt->second, target);
    canvas->updateLinePoints(sourceIt->second);

    auto arrowIt = sourceIt->second->nodeArrows.find(targetNodeId);
    if (arrowIt != sourceIt->second->nodeArrows.end()) {
        arrowIt->second->triggerSnapAnimation();
    }

    juce::ValueTree sourceTree = ValueTreeState::getNode(sourceNodeId);
    if (sourceTree.isValid()) {
        int rootId = sourceTree.getProperty(ValueTreeIdentifiers::RootNodeId);
        juce::ValueTree rootTree = ValueTreeState::getNode(rootId);
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

    juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);
    NodePosition oldPosition = ValueTreeState::getNodePosition(nodeId);

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
