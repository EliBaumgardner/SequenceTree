/*
  ==============================================================================

    NodeLogic.cpp
    Created: 13 Jun 2025 10:29:36pm
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "Node.h"
#include "NodeLogic.h"
#include "NodeData.h"
#include "RTData.h"

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
    if(target->getNodeData()->children.isEmpty()){
        //std::cout<<"no children"<<std::endl;
        target->setSelectVisual(false);
        target = root;
        target->setSelectVisual(true);
        start = false;
        pushNote();
        return;
    }
    
    NodeLogic* targetLogic = &target->nodeLogic;
    NodeData* targetData = target->getNodeData();
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
        pushNote();
    
}

void NodeLogic::setNode(Node* node){
    this->node = node;
    data = this->node->getNodeData();
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
    auto children = node->getNodeData()->children;

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


void NodeLogic::pushNote(){
//    
//    auto* notes = &target->nodeData.midiNotes;
//    auto* processor = &context->processor;
//    
//    
//    if(notes->isEmpty()){
//        processor->pushNote(63,63,1000);
//    }
//    
//    for(auto note : *notes){
//        
//        float pitch = static_cast<float>(note.getProperty("pitch"));
//        float velocity = static_cast<float>(note.getProperty("velocity"));
//        float duration = static_cast<float>(note.getProperty("duration"));
//        
//        processor->pushNote(pitch,velocity,duration);
//    }
}

//RTGraph NodeLogic::makeRTGraph(Node* root){
//    
//    RTGraph rtGraph;
//    
//    std::unordered_map<Node*,int> nodeTold;
//    std::vector<Node*> stack = {root};
//    
//    int nextID = 0;
//    
//    while(!stack.empty()){
//        
//        Node* current = stack.back();
//        stack.pop_back();
//        if(nodeTold.count(current) == 0){
//            int id = nextID++;
//            nodeTold[current] = id;
//            
//            RTNode rtNode;
//            rtNode.nodeID = id;
//            rtNode.count = current->nodeLogic.count;
//            rtNode.countLimit = current->nodeLogic.countLimit;
//            
//            rtGraph.nodes.push_back(std::move(rtNode));
//            
//            for(auto child : root->nodeData.children){
//                
//                stack.push_back(child);
//            }
//        }
//    }
//    
//    for(auto& [child, id] : nodeTold){
//        
//        for(auto childNode : root->nodeData.children){
//            rtGraph.nodes[id].children.push_back(nodeTold[child]);
//        }
//    }
//    
//    rtGraph.currentTarget = nodeTold[root];
//    rtGraph.traversalRequested = true;
//    
//    return rtGraph;
//}

//void NodeLogic::updateProcessorGraph(Node* root){
//    
//    context->processor.rtGraph = makeRTGraph(root);
//}
