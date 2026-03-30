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
    handleNodeRelease();
}

void NodeController::mouseDown(const juce::MouseEvent& e)
{


    juce::Component* component = e.eventComponent;
    juce::UndoManager* undoManager = ComponentContext::undoManager;


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

        for (Node* canvasNode : nodeCanvas->canvasNodes) {

            if(canvasNode != node) {
                canvasNode->setSelectVisual(false);
            }
        }

        if(e.mods.isRightButtonDown() && e.mods.isShiftDown()) {

            NodeFactory::destroyNode(*node, undoManager);
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

        DBG("canvas is being dragged");
        auto grandParent = component->getParentComponent()->getParentComponent();

        if (auto* dynamicPort = dynamic_cast<DynamicPort*>(grandParent)){

            auto parentEvent = e.getEventRelativeTo(dynamicPort);
            dynamicPort->mouseDrag(parentEvent);
        }

    }
    else if (Node* node = dynamic_cast<Node*>(component)) {

        DBG("Node is being dragged");

        NodePosition nodePosition;

        nodePosition.xPosition = e.x;
        nodePosition.yPosition = e.y;
        nodePosition.radius    = 20;

        if (e.getDistanceFromDragStart() < 5 || !e.mods.isLeftButtonDown()) {
            return;
        }

        const auto position = e.getEventRelativeTo(node->getParentComponent()).position;

        if (e.mods.isShiftDown()){
            dragNode(node,nodePosition);
            return;
        }
        if (isDragStart) {
            isDragStart = false;
            addNode(node,nodePosition,undoManager); return;
        }

        node->setSelectVisual(false);

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

void NodeController::dragNode(Node* node, NodePosition nodePosition)
{
    isDragStart = false;

    int radius    = node->getWidth()/2;

    juce::ValueTree nodeValueTree = ValueTreeState::getNode(node->nodeID);

    ValueTreeState::setNodePosition(nodeValueTree,nodePosition,nullptr);

    // selectedNode->nodeData.nodeData.setProperty("x", selectedNode->getX(), nullptr);
    // selectedNode->nodeData.nodeData.setProperty("y", selectedNode->getY(), nullptr);
    //
    // nodeCanvas->updateLinePoints(selectedNode);
    // nodeCanvas->repaint();
}

void NodeController::addNode(Node* parentNode,NodePosition nodePosition,juce::UndoManager* undoManager)
{
    // switch (nodeControllerMode)
    // {
    //     case NodeControllerMode::Node       : childNode = new Node();      break;
    //     case NodeControllerMode::Connector  : childNode = new Counter();   break;
    //     default: jassertfalse;
    // }

    // nodeCanvas->canvasNodes.add(childNode);
    // childNode->parent = node;
    //
    // nodeCanvas->addAndMakeVisible(childNode);

    int parentId = parentNode->getComponentID().getIntValue();

    juce::ValueTree parentNodeValueTree = ValueTreeState::getNode(parentId);
    int rootNodeId = parentNodeValueTree.getProperty(ValueTreeState::RootNodeId);

    if (Connector* parentConnector = dynamic_cast<Connector*>(parentNode)) {

        NodeFactory::createRootNode(*parentConnector,nodePosition,undoManager);

        // childNode->root = childNode;
        // selectedNode->nodeData.addConnector(childNode);
        // nodeCanvas->makeRTGraph(selectedNode->root);
    }
    else if (Node* nodeParent = dynamic_cast<Node*>(parentNode)){

        NodeFactory::createNode(*nodeParent,nodePosition, undoManager);
        // selectedNode->nodeData.addChild(childNode);
        // childNode->root = selectedNode->root;
    }
    //
    // if (selectedNode == nullptr) {DBG("NODE IS NULL");}
    // if (selectedNode->root == nullptr) {DBG("Node HAS NULL ROOT");}
    // if (childNode == nullptr) { DBG("CHILD NODE NULL");}
    // if (childNode->root == nullptr) { DBG("CHILD HAS NULL ROOT");}

    // nodeCanvas->makeRTGraph(childNode->root);
    // nodeCanvas->addLinePoints(selectedNode, childNode);
    //
    // childNode->toBack();
    // childNode->nodeData.nodeData.setProperty("radius",childNode->getWidth()/2,nullptr);
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

void NodeController::setObjects(juce::Component& component)
{

    // if (node == childNode) { return; }
    //
    // if ( node != this->selectedNode || node == nodeCanvas->canvasNodes.getFirst() && node != nullptr){
    //     this->selectedNode = node;
    //     this->selectedNode->addMouseListener(this,true);
    // }
}

void NodeController::handleNodeRelease()
{
    // if (hasConnection && connectorNode != nullptr) {
    //
    //     if (auto traverser = dynamic_cast<Connector*>(selectedNode)) { selectedNode->nodeData.removeConnector(childNode); selectedNode->nodeData.addConnector(connectorNode);}
    //     else                                                 { selectedNode->nodeData.removeChild(childNode);  selectedNode->nodeData.addChild(connectorNode); }
    //
    //     nodeCanvas->removeNodeFromCanvas(childNode);
    //
    //     nodeCanvas->addLinePoints(selectedNode,connectorNode);
    //     nodeCanvas->updateLinePoints(selectedNode);
    //
    //     nodeCanvas->makeRTGraph(selectedNode);
    // }

    isDragStart = true;
}