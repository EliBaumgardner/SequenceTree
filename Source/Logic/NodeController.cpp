/*
  ==============================================================================

    ObjectController.h.cpp
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "Node/NodeArrow.h"
#include "NodeController.h"
#include "Node/Modulator.h"
#include "Node/Connector.h"
#include "NodeFactory.h"
#include "DynamicPort.h"
#include "../Util/ValueTreeIdentifiers.h"





NodeController::NodeController() : nodeCanvas(ComponentContext::canvas) {

}

void NodeController::mouseEnter(const juce::MouseEvent& e)
{
    juce::Component* component = e.eventComponent;
    if (Node* node = dynamic_cast<Node*>(component)) {
        node->setHoverVisual(true);
    }
}
void NodeController::mouseExit(const juce::MouseEvent& e)
{
    juce::Component* component = e.eventComponent;
    if (Node* node = dynamic_cast<Node*>(component)) {
        node->setHoverVisual(false);
    }
}
void NodeController::mouseUp(const juce::MouseEvent& e)
{
    NodeCanvas* canvas = ComponentContext::canvas;

    if (isDraggingValue) {
        isDraggingValue = false;
        draggingValueNode = nullptr;
    }
    else if (snapTargetRoot != nullptr)
    {
        canvas->hideSnapGhostArrow();

        int rootNodeId = snapTargetRoot->getComponentID().getIntValue();
        DBG("rootNodeId:" + juce::String(rootNodeId));
        int parentNodeId   = snapSourceNodeId;

        snapTargetRoot   = nullptr;
        snapSourceNodeId = -1;

        int draggedNodeId = (int)draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

        juce::UndoManager* undoManager = ComponentContext::undoManager;
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

        if (parentIterator != canvas->nodeMap.end() && rootIterator != canvas->nodeMap.end())
        {
            canvas->addLinePoints(parentIterator->second, rootIterator->second);
            canvas->updateLinePoints(parentIterator->second);

            auto arrowIterator = parentIterator->second->nodeArrows.find(rootNodeId);

            if (arrowIterator != parentIterator->second->nodeArrows.end()) {
                arrowIterator->second->triggerSnapAnimation();
            }

            juce::ValueTree parentValueTree = ValueTreeState::getNode(parentNodeId);
            if (parentValueTree.isValid())
            {
                int parentRootId = parentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
                juce::ValueTree parentRootValueTree = ValueTreeState::getNode(parentRootId);
                if (parentRootValueTree.isValid()) {
                    canvas->makeRTGraph(parentRootValueTree);
                }
            }
        }
    }
    else
    {
        if (draggedNodeTree.isValid())
        {
            int nodeId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);
            canvas->triggerArrowSnapForNode(nodeId);
        }
    }

    draggedNodeTree  = juce::ValueTree();
    isDragStart      = true;
    snapTargetRoot   = nullptr;
    snapSourceNodeId = -1;

    if (canvas->showGrid)
    {
        canvas->showGrid = false;
        canvas->repaint();
    }
}

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    NodeCanvas* nodeCanvas = ComponentContext::canvas;
    juce::Component* component = e.eventComponent;
    juce::UndoManager* undoManager = ComponentContext::undoManager;

    jassert(nodeCanvas);

    if (NodeCanvas* canvas = dynamic_cast<NodeCanvas*>(component)) {

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

        if (node->nodeTextEditor != nullptr) {
            auto localPos = e.getEventRelativeTo(node->nodeTextEditor.get()).getPosition();
            if (node->nodeTextEditor->getLocalBounds().contains(localPos)) {
                isDraggingValue = true;
                dragStartValue = node->nodeTextEditor->bindValue.toString().getDoubleValue();
                draggingValueNode = node;
                return;
            }
        }

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

void NodeController::snapToGrid(juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree) {
    NodeCanvas* canvas = ComponentContext::canvas;

    if (!canvas->gridOriginSet) {
        ValueTreeState::setNodePosition(draggedNodeTree, newPosition, undoManager);
        return;
    }

    float spacing = canvas->gridSpacing;
    float ox = canvas->gridOrigin.x;
    float oy = canvas->gridOrigin.y;
    const float snapThreshold = 12.0f;

    // Snap against the absolute global grid, not relative to dragParentCenter.
    // newPosition is the canvas position that will be stored as the node's logical centre
    // (circle centre for root nodes, bounding-box centre for regular nodes).
    float snappedX = ox + std::round((float(newPosition.xPosition) - ox) / spacing) * spacing;
    float snappedY = oy + std::round((float(newPosition.yPosition) - oy) / spacing) * spacing;

    if (std::abs(float(newPosition.xPosition) - snappedX) < snapThreshold)
        newPosition.xPosition = int(snappedX);
    if (std::abs(float(newPosition.yPosition) - snappedY) < snapThreshold)
        newPosition.yPosition = int(snappedY);

    ValueTreeState::setNodePosition(draggedNodeTree, newPosition, undoManager);
}

void NodeController::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingValue && draggingValueNode != nullptr) {
        int yOffset = e.getOffsetFromDragStart().y;
        int delta = -yOffset / 3;
        double newValue = dragStartValue + delta;
        draggingValueNode->nodeTextEditor->bindValue.setValue(newValue);
        draggingValueNode->nodeTextEditor->formatDisplay(draggingValueNode->nodeTextEditor->mode);
        return;
    }

    juce::Component* component = e.eventComponent;
    juce::UndoManager* undoManager = ComponentContext::undoManager;

    if (NodeCanvas* nodeCanvas = dynamic_cast<NodeCanvas*>(component)) {

        auto parent = component->getParentComponent();

        if (auto* dynamicPort = dynamic_cast<DynamicPort*>(parent)){
            auto parentEvent = e.getEventRelativeTo(dynamicPort);
            dynamicPort->mouseDrag(parentEvent);
        }
    }
    else if (Node* node = dynamic_cast<Node*>(component)) {

        int nodeId = node->getComponentID().getIntValue();
        NodePosition newPosition;

        auto pos = e.getEventRelativeTo(node->getParentComponent());

        newPosition.xPosition = pos.x;
        newPosition.yPosition = pos.y;
        newPosition.radius    = 20;

        if (e.getDistanceFromDragStart() < 5 || !e.mods.isLeftButtonDown()) {
            return;
        }

        if (!e.mods.isShiftDown() && !draggedNodeTree.isValid()) {
            if (isDragStart) {
                isDragStart = false;
                undoManager->beginNewTransaction();
                if (ComponentContext::canvas->gridOriginSet)
                {
                    ComponentContext::canvas->showGrid = true;
                    ComponentContext::canvas->repaint();
                }
            }

            juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);
            NodePosition oldPosition = ValueTreeState::getNodePosition(nodeId);

            snapToGrid(undoManager, newPosition, nodeValueTree);

            int deltaX = newPosition.xPosition - oldPosition.xPosition;
            int deltaY = newPosition.yPosition - oldPosition.yPosition;

            ComponentContext::canvas->moveDescendants(nodeValueTree, deltaX, deltaY);
            return;
        }

        if (isDragStart) {
            isDragStart = false;
            undoManager->beginNewTransaction();
            if (ComponentContext::canvas->gridOriginSet)
            {
                ComponentContext::canvas->showGrid = true;
                ComponentContext::canvas->repaint();
            }

            if (nodeControllerMode == NodeControllerMode::Node) {
                if (auto parent = dynamic_cast<Connector*>(node)) {
                    draggedNodeTree = NodeFactory::createRootNode(nodeId, newPosition, undoManager);
                }
                else {
                    draggedNodeTree = NodeFactory::createNode(nodeId, newPosition, undoManager);
                }
            }
            else if (nodeControllerMode == NodeControllerMode::Connector) {
                draggedNodeTree = NodeFactory::createConnector(nodeId, newPosition, undoManager);
            }
            else if (nodeControllerMode == NodeControllerMode::Modulator) {
                if (node->nodeValueTree.getType() == ValueTreeIdentifiers::ModulatorRootData) {
                    draggedNodeTree = NodeFactory::createModulator(nodeId, newPosition, undoManager);
                }
                else {
                    draggedNodeTree = NodeFactory::createModulatorRoot(nodeId, newPosition, undoManager);
                }
            }
            snapSourceNodeId = nodeId;
            return;
        }

        if (draggedNodeTree.isValid()) {
            snapToGrid(undoManager, newPosition, draggedNodeTree);
            checkRootNodeSnap(newPosition);
        }
    }
}

void NodeController::checkRootNodeSnap(const NodePosition& pos)
{
    if (snapSourceNodeId < 0 || !draggedNodeTree.isValid()) return;

    NodeCanvas* canvas = ComponentContext::canvas;
    juce::Point<int> dragPoint(pos.xPosition, pos.yPosition);

    Node* nearestRoot = nullptr;
    float minDist = rootSnapThreshold;

    for (auto& [id, node] : canvas->nodeMap)
    {
        if (node->nodeValueTree.getType() != ValueTreeIdentifiers::RootNodeData) continue;
        if (id == snapSourceNodeId) continue;

        float dist = (float)dragPoint.getDistanceFrom(node->getNodeCentre());
        if (dist < minDist)
        {
            minDist = dist;
            nearestRoot = node;
        }
    }

    if (nearestRoot != nullptr)
    {
        if (snapTargetRoot != nearestRoot)
        {
            snapTargetRoot = nearestRoot;

            int draggedId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

            auto draggedIt = canvas->nodeMap.find(draggedId);
            if (draggedIt != canvas->nodeMap.end())
            {
                draggedIt->second->setVisible(false);

                auto sourceIt = canvas->nodeMap.find(snapSourceNodeId);
                if (sourceIt != canvas->nodeMap.end())
                {
                    auto arrowIt = sourceIt->second->nodeArrows.find(draggedId);
                    if (arrowIt != sourceIt->second->nodeArrows.end())
                        arrowIt->second->setVisible(false);
                }
            }

            auto sourceIt = canvas->nodeMap.find(snapSourceNodeId);
            if (sourceIt != canvas->nodeMap.end()) {
                canvas->showSnapGhostArrow(sourceIt->second, nearestRoot);
            }
        }
    }
    else if (snapTargetRoot != nullptr)
    {
        snapTargetRoot = nullptr;

        int draggedId = draggedNodeTree.getProperty(ValueTreeIdentifiers::Id);

        auto draggedIt = canvas->nodeMap.find(draggedId);

        if (draggedIt != canvas->nodeMap.end())
        {
            draggedIt->second->setVisible(true);

            auto sourceIt = canvas->nodeMap.find(snapSourceNodeId);
            if (sourceIt != canvas->nodeMap.end())
            {
                auto arrowIt = sourceIt->second->nodeArrows.find(draggedId);
                if (arrowIt != sourceIt->second->nodeArrows.end())
                    arrowIt->second->setVisible(true);
            }
        }

        canvas->hideSnapGhostArrow();
    }
}