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
#include "ObjectController.h"
#include "Node/Counter.h"
#include "Node/RelayNode.h"
#include "../UI/DynamicEditor.h"


ObjectController::ObjectController(Node* node) : node(node), nodeCanvas(ComponentContext::canvas) {}

void ObjectController::mouseEnter(const juce::MouseEvent& e) { node->setHoverVisual(true);  }
void ObjectController::mouseExit(const juce::MouseEvent& e)  { node->setHoverVisual(false); }
void ObjectController::mouseUp(const juce::MouseEvent& e)    { handleNodeRelease(); }

void ObjectController::mouseDown(const juce::MouseEvent& e)
{
    for (auto canvasNode : nodeCanvas->canvasNodes){ if(canvasNode != node){ canvasNode->setSelectVisual(false); } }

    if(e.mods.isRightButtonDown() && e.mods.isShiftDown()){ node->parent->nodeData.removeChild(node); nodeCanvas->removeNode(node); }
    else { node->setSelectVisual(); }
}

void ObjectController::mouseDrag(const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() < 5 || !e.mods.isLeftButtonDown()) { return; }

    int deltaX = e.x - lastX;
    int deltaY = e.y - lastY;
    lastX = e.x;
    lastY = e.y;

    const auto position = e.getEventRelativeTo(node->getParentComponent()).position;

    if (e.mods.isShiftDown()) { dragNode(position);             return; }
    if (isDragStart)          { isDragStart = false; addNode(); return; }

    node->setSelectVisual(false);

    if (childNode != nullptr) { connectNode(deltaX, deltaY, position); }
}

void ObjectController::dragNode(const juce::Point<float> position)
{
    isDragStart = false;

    node->setCentrePosition(position.toInt());

    node->nodeData.nodeData.setProperty("x", node->getX(), nullptr);
    node->nodeData.nodeData.setProperty("y", node->getY(), nullptr);

    nodeCanvas->updateLinePoints(node);
    nodeCanvas->repaint();
}

void ObjectController::addNode()
{
    switch (nodeCanvas->controllerMode)
    {
        case NodeCanvas::ControllerMode::Node     : childNode = new Node();      break;
        case NodeCanvas::ControllerMode::Counter  : childNode = new Counter();   break;
        case NodeCanvas::ControllerMode::Traverser: childNode = new RelayNode(); break;
        default: DBG("INVALID CONTROLLER MODE SELECTED"); return;
    }

    nodeCanvas->canvasNodes.add(childNode);
    childNode->parent = node;

    nodeCanvas->addAndMakeVisible(childNode);

    if (auto parent = dynamic_cast<RelayNode*>(node)) {
        node->nodeData.addConnector(childNode);
        childNode->root = childNode;
        nodeCanvas->makeRTGraph(node->root);

    }
    else {
        childNode->root = node->root;
        node->nodeData.addChild(childNode);
    }

    nodeCanvas->makeRTGraph(childNode->root);
    nodeCanvas->addLinePoints(node, childNode);
    childNode->toBack();
    childNode->nodeData.nodeData.setProperty("radius",childNode->getWidth()/2,nullptr);
}

void ObjectController::connectNode(int deltaX, int deltaY, const juce::Point<float> position)
{
    childNode->setCentrePosition(position.toInt());
    childNode->setSize(40, 40);
    childNode->nodeData.nodeData.setProperty("x", childNode->getX(), nullptr);
    childNode->nodeData.nodeData.setProperty("y", childNode->getY(), nullptr);

    auto* target = dynamic_cast<Node*>(nodeCanvas->getComponentAt(position.x, position.y));

    hasConnection = target && target != childNode && target != node && target != node->parent;

    connectorNode = hasConnection ? target : nullptr;
    childNode->setVisible(!hasConnection);

    if (hasConnection && abs(deltaX) < 3 && abs(deltaY) < 3) {
        auto point = target->localPointToGlobal(target->getLocalBounds().getCentre().toFloat());
        juce::Desktop::getInstance().setMousePosition(point.toInt());
    }

    nodeCanvas->updateLinePoints(childNode);
    nodeCanvas->repaint();
}

void ObjectController::setObjects(Node* node)
{
    if (node == childNode) { return; }

    if ( node != this->node || node == nodeCanvas->canvasNodes.getFirst() && node != nullptr){
        this->node->removeMouseListener(this);
        this->node = node;
        this->node->addMouseListener(this,true);
    }
}

void ObjectController::handleNodeRelease()
{

    if (hasConnection && connectorNode != nullptr) {

        if (auto traverser = dynamic_cast<RelayNode*>(node)) { node->nodeData.removeConnector(childNode); node->nodeData.addConnector(connectorNode);}
        else                                                 { node->nodeData.removeChild(childNode);  node->nodeData.addChild(connectorNode); }

        nodeCanvas->removeNode(childNode);

        nodeCanvas->addLinePoints(node,connectorNode);
        nodeCanvas->updateLinePoints(node);
        nodeCanvas->makeRTGraph(node);
    }

    isDragStart = true; childNode = nullptr; connectorNode = nullptr;

}