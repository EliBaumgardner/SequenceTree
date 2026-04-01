/*
  ==============================================================================

    ObjectController.h.cpp
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "NodeController.h"
#include "Node/Counter.h"
#include "Node/Connector.h"
#include "NodeFactory.h"





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
    draggedNodeTree = juce::ValueTree();
    isDragStart = true;
}

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    NodeCanvas* nodeCanvas = ComponentContext::canvas;
    juce::Component* component = e.eventComponent;
    juce::UndoManager* undoManager = ComponentContext::undoManager;

    jassert(nodeCanvas);

    if (NodeCanvas* canvas = dynamic_cast<NodeCanvas*>(component)) {

        if (e.mods.isShiftDown() && e.mods.isLeftButtonDown()) {
            NodePosition nodePosition;

            nodePosition.xPosition = e.x;
            nodePosition.yPosition = e.y;
            nodePosition.radius    = 20;

            NodeFactory::createRootNode(nodePosition,undoManager);
        }
    }
    else if (Node* node= dynamic_cast<Node*>(component) ) {

        node->setHoverVisual(true);
        int nodeId = node->getComponentID().getIntValue();

        for (auto& [nodeId, canvasNode] : nodeCanvas->nodeMap) {
            if(canvasNode != node) {
                canvasNode->setSelectVisual(false);
            }
        }

        if(e.mods.isRightButtonDown() && e.mods.isShiftDown()) {

            NodeFactory::destroyNode(nodeId, undoManager);
            // selectedNode->parent->nodeData.removeChild(selectedNode);
            // nodeCanvas->removeNodeFromCanvas(selectedNode);
        }
        else {
            node->setSelectVisual();
        }
    }
}

void NodeController::mouseDrag(const juce::MouseEvent& e)
{

    juce::Component* component = e.eventComponent;
    juce::UndoManager* undoManager = ComponentContext::undoManager;

    if (NodeCanvas* nodeCanvas = dynamic_cast<NodeCanvas*>(component)) {

        auto grandParent = component->getParentComponent()->getParentComponent();

        if (auto* dynamicPort = dynamic_cast<DynamicPort*>(grandParent)){

            auto parentEvent = e.getEventRelativeTo(dynamicPort);
            dynamicPort->mouseDrag(parentEvent);
        }

    }
    else if (Node* node = dynamic_cast<Node*>(component)) {

        int nodeId = node->getComponentID().getIntValue();
        NodePosition nodePosition;

        auto pos = e.getEventRelativeTo(node->getParentComponent());

        nodePosition.xPosition = pos.x;
        nodePosition.yPosition = pos.y;
        nodePosition.radius    = 20;

        if (e.getDistanceFromDragStart() < 5 || !e.mods.isLeftButtonDown()) {
            return;
        }

        if (e.mods.isShiftDown()) {
            juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);
            ValueTreeState::setNodePosition(nodeValueTree,nodePosition,undoManager);
            return;
        }

        if (isDragStart) {
            isDragStart = false;
            if (nodeControllerMode == NodeControllerMode::Node) {
                if (auto parent = dynamic_cast<Connector*>(node)) {
                    draggedNodeTree = ValueTreeState::addRootNode(nodeId, nodePosition, undoManager);
                }
                else {
                    draggedNodeTree = NodeFactory::createNode(nodeId,nodePosition, undoManager);
                }
            }
            else if (nodeControllerMode == NodeControllerMode::Connector) {
                draggedNodeTree = ValueTreeState::addConnector(nodeId, nodePosition, undoManager);
            }
            return;
        }

        if (draggedNodeTree.isValid()) {
            ValueTreeState::setNodePosition(draggedNodeTree, nodePosition, undoManager);
        }

        // if (childNode != nullptr) {
        //     int deltaX = e.x - lastX;
        //     int deltaY = e.y - lastY;
        //     lastX = e.x;
        //     lastY = e.y;
        //
        //     connectNode(deltaX, deltaY, position);
        // }
    }
}

void NodeController::connectNode(int deltaX, int deltaY, const juce::Point<float> position)
{
    // childNode->setCentrePosition(position.toInt());
    // childNode->setSize(40, 40);
    // childNode->nodeData.nodeData.setProperty("x", childNode->getX(), nullptr);
    // childNode->nodeData.nodeData.setProperty("y", childNode->getY(), nullptr);
    //
    // auto* target = dynamic_cast<Node*>(nodeCanvas->getComponentAt(position.x, position.y));
    //
    // hasConnection = target && target != childNode && target != selectedNode && target != selectedNode->parent;
    //
    // connectorNode = hasConnection ? target : nullptr;
    // childNode->setVisible(!hasConnection);
    //
    // if (hasConnection && abs(deltaX) < 3 && abs(deltaY) < 3) {
    //     auto point = target->localPointToGlobal(target->getLocalBounds().getCentre().toFloat());
    //     juce::Desktop::getInstance().setMousePosition(point.toInt());
    // }
    //
    // nodeCanvas->updateLinePoints(childNode);
    // nodeCanvas->repaint();
}


