/*
  ==============================================================================

    ObjectController.h.cpp
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "../Node/NodeCanvas.h"
#include "../Node/Node.h"
#include "../Node/NodeData.h"
#include "ObjectController.h"
#include "../Node/Counter.h"
#include "../Node/Traverser.h"


ObjectController::ObjectController(Node* node) : node(node){
    
    nodeCanvas = ComponentContext::canvas;
}



void ObjectController::mouseEnter(const juce::MouseEvent& e){
    
    node->setHoverVisual(true);
}


void ObjectController::mouseExit(const juce::MouseEvent& e){
    
    node->setHoverVisual(false);
}


void ObjectController::mouseDrag(const juce::MouseEvent& e){
    //check if drag is substantial
    if(e.getDistanceFromDragStart() < 5){
        return;
    }
    
    auto position = e.getEventRelativeTo(node->getParentComponent()).position;
    
    if(ComponentContext::canvas->controllerMode == NodeCanvas::ControllerMode::Inspect){
        
        node->setCentrePosition(position.toInt());
        node->nodeData.nodeData.setProperty("x",node->getX(),nullptr);
        node->nodeData.nodeData.setProperty("y",node->getY(),nullptr);
    }
    
    else {
        
        if(e.mods.isLeftButtonDown()){
            
            if(isDragStart){
                isDragStart = false;

                if (ComponentContext::canvas->controllerMode == NodeCanvas::ControllerMode::Node) {
                    nodeCanvas->canvasNodes.add(new Node(nodeCanvas));
                }
                else if (ComponentContext::canvas->controllerMode == NodeCanvas::ControllerMode::Counter){
                    nodeCanvas->canvasNodes.add(new Counter(nodeCanvas));
                }
                else if (ComponentContext::canvas->controllerMode == NodeCanvas::ControllerMode::Traverser) {
                    nodeCanvas->canvasNodes.add(new Traverser(nodeCanvas));
                }

                childNode = nodeCanvas->canvasNodes.getLast();
                childNode->parent = node;
                childNode->root = childNode->parent->root;
                
                node->nodeData.addChild(childNode);
                
                nodeCanvas->addAndMakeVisible(childNode);
                nodeCanvas->makeRTGraph(childNode->root);

                nodeCanvas->addLinePoints(node,childNode);
            }
            
            else {
                node->setSelectVisual(false);
                
                childNode->setCentrePosition(position.toInt());
                childNode->setSize(40,40);
                
                childNode->nodeData.nodeData.setProperty("x",childNode->getX(),nullptr);
                childNode->nodeData.nodeData.setProperty("y",childNode->getY(),nullptr);
                nodeCanvas->updateLinePoints(childNode);
            }
            
        }
    }


    nodeCanvas->repaint();
}

void ObjectController::mouseDown(const juce::MouseEvent& e){
     
    for (auto canvasNode : nodeCanvas->canvasNodes){
        if(canvasNode != node){
            canvasNode->setSelectVisual(false);
        }
        
    }
    
    if(e.mods.isRightButtonDown()){
        
        nodeCanvas->removeNode(node);
    }
    else {
        node->setSelectVisual();
    }
    
}

void ObjectController::mouseUp(const juce::MouseEvent& e){
    isDragStart = true;
    //std::cout<<"drag completed" <<std::endl;
}

void ObjectController::setObjects(Node* node) {

    if (node == childNode)
        return;

    if (node != this->node) {
        this->node = node;
    }
}