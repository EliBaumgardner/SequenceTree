/*
  ==============================================================================

    ObjectController.h.cpp
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "Node/NodeData.h"
#include "NodeController.h"
#include "Node/Counter.h"
#include "Node/RelayNode.h"
#include "../Util/ValueTreeState.h"




NodeController::NodeController() : nodeCanvas(ComponentContext::canvas) {

}

void NodeController::mouseEnter(const juce::MouseEvent& e) { selectedNode->setHoverVisual(true);  }
void NodeController::mouseExit(const juce::MouseEvent& e)  { selectedNode->setHoverVisual(false); }
void NodeController::mouseUp(const juce::MouseEvent& e)    { handleNodeRelease(); }

void NodeController::mouseDown(const juce::MouseEvent& e)
{
    int nodeId = selectedNode->nodeID;

    juce::ValueTree nodeMap = ComponentContext::valueTreeState->nodeMap;
    juce::ValueTree selectedNodeValueTree = nodeMap.getChildWithProperty(ValueTreeState::Id, nodeId);

    for (auto canvasNode : nodeCanvas->canvasNodes) {

        if(canvasNode != selectedNode) {
            canvasNode->setSelectVisual(false);
        }
    }

    if(e.mods.isRightButtonDown() && e.mods.isShiftDown()) {

        ComponentContext::valueTreeState->removeNode(nodeId, *ComponentContext::undoManager);
        selectedNode->parent->nodeData.removeChild(selectedNode);
        nodeCanvas->removeNode(selectedNode);
    }
    else { selectedNode->setSelectVisual(); }
}

void NodeController::mouseDrag(const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() < 5 || !e.mods.isLeftButtonDown()) { return; }

    int deltaX = e.x - lastX;
    int deltaY = e.y - lastY;
    lastX = e.x;
    lastY = e.y;

    const auto position = e.getEventRelativeTo(selectedNode->getParentComponent()).position;

    if (e.mods.isShiftDown()) { dragNode(position);             return; }
    if (isDragStart)          { isDragStart = false; addNode(); return; }

    selectedNode->setSelectVisual(false);

    if (childNode != nullptr) { connectNode(deltaX, deltaY, position); }
}

void NodeController::dragNode(const juce::Point<float> position)
{
    isDragStart = false;

    selectedNode->setCentrePosition(position.toInt());

    selectedNode->nodeData.nodeData.setProperty("x", selectedNode->getX(), nullptr);
    selectedNode->nodeData.nodeData.setProperty("y", selectedNode->getY(), nullptr);

    nodeCanvas->updateLinePoints(selectedNode);
    nodeCanvas->repaint();
}

void NodeController::addNode()
{
    switch (nodeControllerMode)
    {
        case NodeControllerMode::Node     : childNode = new Node();      break;
        case NodeControllerMode::Connector  : childNode = new Counter();   break;
        default: DBG("INVALID CONTROLLER MODE SELECTED"); return;
    }

    nodeCanvas->canvasNodes.add(childNode);
    childNode->parent = selectedNode;

    nodeCanvas->addAndMakeVisible(childNode);

    if (auto parent = dynamic_cast<RelayNode*>(selectedNode)) {
        childNode->root = childNode;
        selectedNode->nodeData.addConnector(childNode);
        nodeCanvas->makeRTGraph(selectedNode->root);
    }
    else {
        selectedNode->nodeData.addChild(childNode);
        childNode->root = selectedNode->root;
    }

    if (selectedNode == nullptr) {DBG("NODE IS NULL");}
    if (selectedNode->root == nullptr) {DBG("Node HAS NULL ROOT");}
    if (childNode == nullptr) { DBG("CHILD NODE NULL");}
    if (childNode->root == nullptr) { DBG("CHILD HAS NULL ROOT");}

    nodeCanvas->makeRTGraph(childNode->root);
    nodeCanvas->addLinePoints(selectedNode, childNode);

    childNode->toBack();
    childNode->nodeData.nodeData.setProperty("radius",childNode->getWidth()/2,nullptr);
}

void NodeController::connectNode(int deltaX, int deltaY, const juce::Point<float> position)
{
    childNode->setCentrePosition(position.toInt());
    childNode->setSize(40, 40);
    childNode->nodeData.nodeData.setProperty("x", childNode->getX(), nullptr);
    childNode->nodeData.nodeData.setProperty("y", childNode->getY(), nullptr);

    auto* target = dynamic_cast<Node*>(nodeCanvas->getComponentAt(position.x, position.y));

    hasConnection = target && target != childNode && target != selectedNode && target != selectedNode->parent;

    connectorNode = hasConnection ? target : nullptr;
    childNode->setVisible(!hasConnection);

    if (hasConnection && abs(deltaX) < 3 && abs(deltaY) < 3) {
        auto point = target->localPointToGlobal(target->getLocalBounds().getCentre().toFloat());
        juce::Desktop::getInstance().setMousePosition(point.toInt());
    }

    nodeCanvas->updateLinePoints(childNode);
    nodeCanvas->repaint();
}

void NodeController::setObjects(Node* node)
{
    if (node == childNode) { return; }

    if ( node != this->selectedNode || node == nodeCanvas->canvasNodes.getFirst() && node != nullptr){
        this->selectedNode = node;
        this->selectedNode->addMouseListener(this,true);
    }
}

void NodeController::handleNodeRelease()
{
    if (hasConnection && connectorNode != nullptr) {

        if (auto traverser = dynamic_cast<RelayNode*>(selectedNode)) { selectedNode->nodeData.removeConnector(childNode); selectedNode->nodeData.addConnector(connectorNode);}
        else                                                 { selectedNode->nodeData.removeChild(childNode);  selectedNode->nodeData.addChild(connectorNode); }

        nodeCanvas->removeNode(childNode);

        nodeCanvas->addLinePoints(selectedNode,connectorNode);
        nodeCanvas->updateLinePoints(selectedNode);

        nodeCanvas->makeRTGraph(selectedNode);
    }

    isDragStart = true; childNode = nullptr; connectorNode = nullptr;
}