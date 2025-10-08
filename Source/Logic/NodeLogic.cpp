/*
  ==============================================================================

    NodeLogic.cpp
    Created: 13 Jun 2025 10:29:36pm
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "../PluginProcessor.h"
#include "../Node/Node.h"
#include "NodeLogic.h"
#include "../Node/NodeData.h"
#include "../Util/RTData.h"

bool NodeLogic::start = false;
bool NodeLogic::hardStart = false;

NodeLogic::NodeLogic(){

}

NodeLogic::~NodeLogic(){
    
}

void NodeLogic::incCount(int inc){
    count += inc;
    int maxLimit = getChildCount(); // this returns max child countLimit
    if (count > maxLimit) {
        count = 1;
    }
    //std::cout << "Node count is now: " << count << std::endl;
}

bool NodeLogic::isDivisible(int countNum){
    if(countNum%countLimit == 0){
        //std::cout<<"is divisible"<<std::endl;
        return true;
    }
    //std::cout<<"is not divisible"<<std::endl;
    return false;
}

void NodeLogic::traverse(){
    if(!start)
        return;
    
    std::cout<<"traversing"<<std::endl;
    if(target == nullptr){
        root = node;
        target = root;
    }
    
    referenceNode = target;
    if(target->nodeData.children.isEmpty()){
        //std::cout<<"no children"<<std::endl;
        target->setSelectVisual(false);
        target = root;
        target->setSelectVisual(true);
        start = false;
        //pushNote();
        return;
    }
    
    NodeLogic* targetLogic = &target->nodeLogic;
    NodeData* targetData = &target->nodeData;
    targetLogic->incCount(1);
    
    int countNum = targetLogic->count;
    int childCount = 0;
    
    for(Node* child : targetData->children){
        if(countNum % child->nodeLogic.countLimit == 0){
            if(child->nodeLogic.countLimit > childCount){
                referenceNode = child;
                childCount = child->nodeLogic.countLimit;
            }
                
        }
    }
    
    target->setSelectVisual(false);
    
    if(target != referenceNode){
        target = referenceNode;
    }
    else {
        target = root;
    }
    
        start = false;
        target->setSelectVisual(true);
        //pushNote();
    
}

void NodeLogic::setNode(Node* node){
    this->node = node;
    data = &this->node->nodeData;
    if(data != nullptr){

        count =  static_cast<int>(data->nodeData.getProperty("count"));
        countLimit = static_cast<int>(data->nodeData.getProperty("countLimit"));
    }
}

void NodeLogic::setCount(int count){
    this->count = count;
}

void NodeLogic::setCountLimit(int countLimit){
    this->countLimit = countLimit;
}

int NodeLogic::getChildCount(){
    auto children = node->nodeData.children;

    if (children.isEmpty()) {
        return 1;
    }

    int maxLimit = 0;
    
    for(Node* child : children){
        
        if(child){
            if(child->nodeLogic.countLimit > maxLimit)
                maxLimit = child->nodeLogic.countLimit;
        }
    }
    
    
    return maxLimit;
}

