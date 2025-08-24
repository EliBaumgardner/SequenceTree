/*
  ==============================================================================

    NodeController.cpp
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "NodeCanvas.h"
#include "Node.h"
#include "NodeMenu.h"
#include "NodeData.h"
#include "NodeController.h"
#include "PluginContext.h"

NodeController::NodeController(NodeCanvas* nodeCanvas,Node* node) : nodeCanvas(nodeCanvas),node(node){
    
}


NodeController::~NodeController(){
    
}


void NodeController::mouseEnter(const juce::MouseEvent& e){
    
    node->setHoverVisual(true);
}


void NodeController::mouseExit(const juce::MouseEvent& e){
    
    node->setHoverVisual(false);
}


void NodeController::mouseDrag(const juce::MouseEvent& e){
    
    if(e.getDistanceFromDragStart() < 5){
        return;
    }
    
    auto position = e.getEventRelativeTo(node->getParentComponent()).position;
    
    if(e.mods.isRightButtonDown()){
        node->setCentrePosition(position.toInt());
        
        node->getNodeData()->nodeData.setProperty("x",node->getX(),nullptr);
        node->getNodeData()->nodeData.setProperty("y",node->getY(),nullptr);
        //node->getNodeData()->nodeData.setProperty("radius", childNode->getWidth()/2,nullptr);
    }
    
    else if(e.mods.isLeftButtonDown()){
        
        if(isDragStart){
            isDragStart = false;
            nodeCanvas->getCanvasNodes().add(new Node(nodeCanvas));
            std::cout<<"adding node"<<std::endl;
            childNode = nodeCanvas->getCanvasNodes().getLast();
            childNode->parent = node;
            
            node->getNodeData()->addChild(childNode);
            
            nodeCanvas->addAndMakeVisible(childNode);
            nodeCanvas->updateProcessorGraph(nodeCanvas->root);
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
    nodeCanvas->repaint();
}

void NodeController::mouseDown(const juce::MouseEvent& e){
     
    for (auto canvasNode : nodeCanvas->getCanvasNodes()){
        if(canvasNode != node){
            canvasNode->setSelectVisual(false);
        }
        
    }
    
    if(e.mods.isLeftButtonDown()&& e.mods.isShiftDown()){
        
        nodeCanvas->removeNode(node);
        //nodeCanvas->makeRTGraph(nodeCanvas->root);
    }
    else {
        node->setSelectVisual();
        nodeCanvas->getNodeMenu()->setDisplayedNode(node);
    }
}

void NodeController::mouseUp(const juce::MouseEvent& e){
    isDragStart = true;
    
}
