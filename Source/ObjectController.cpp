/*
  ==============================================================================

    ObjectController.h.cpp
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "NodeCanvas.h"
#include "Node.h"
#include "NodeMenu.h"
#include "NodeData.h"
#include "ObjectController.h"
#include "Counter.h"
#include "Traverser.h"


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
        node->getNodeData()->nodeData.setProperty("x",node->getX(),nullptr);
        node->getNodeData()->nodeData.setProperty("y",node->getY(),nullptr);
    }
    
    else {
        
        if(e.mods.isLeftButtonDown()){
            
            if(isDragStart){
                isDragStart = false;

                if (ComponentContext::canvas->controllerMode == NodeCanvas::ControllerMode::Node) {
                    nodeCanvas->getCanvasNodes().add(new Node(nodeCanvas));
                }
                else if (ComponentContext::canvas->controllerMode == NodeCanvas::ControllerMode::Counter){
                    nodeCanvas->getCanvasNodes().add(new Counter(nodeCanvas));
                }
                else if (ComponentContext::canvas->controllerMode == NodeCanvas::ControllerMode::Traverser) {
                    nodeCanvas->getCanvasNodes().add(new Traverser(nodeCanvas));
                }

                childNode = nodeCanvas->getCanvasNodes().getLast();
                childNode->parent = node;
                childNode->root = childNode->parent->root;
                
                node->getNodeData()->addChild(childNode);
                
                nodeCanvas->addAndMakeVisible(childNode);
                nodeCanvas->makeRTGraph(childNode->root);
            }
            
            else {
                node->setSelectVisual(false);
                
                childNode->setCentrePosition(position.toInt());
                childNode->setSize(40,40);
                nodeCanvas->addLinePoints(node,childNode);
                
                childNode->getNodeData()->nodeData.setProperty("x",childNode->getX(),nullptr);
                childNode->getNodeData()->nodeData.setProperty("y",childNode->getY(),nullptr);
                //childNode->getNodeData()->nodeData.setProperty("radius", childNode->getWidth()/2,nullptr);
            }
            
        }
    }


    nodeCanvas->repaint();
}

void ObjectController::mouseDown(const juce::MouseEvent& e){
     
    for (auto canvasNode : nodeCanvas->getCanvasNodes()){
        if(canvasNode != node){
            canvasNode->setSelectVisual(false);
        }
        
    }
    
    if(e.mods.isRightButtonDown()){
        
        nodeCanvas->removeNode(node);
    }
    else {
        node->setSelectVisual();
        nodeCanvas->getNodeMenu()->setDisplayedNode(node);
    }
    
}

void ObjectController::mouseUp(const juce::MouseEvent& e){
    isDragStart = true;
    //std::cout<<"drag completed" <<std::endl;
}
