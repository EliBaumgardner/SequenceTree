/*
  ==============================================================================

    NodeLogic.h
    Created: 13 Jun 2025 10:29:36pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once


class Node;
class NodeData;

struct RTNode;
struct RTGraph;

static Node* target = nullptr;

class NodeLogic {
    
    public:
    
    NodeLogic();
    ~NodeLogic();
    
    void incCount(int inc);
    bool isDivisible(int count);
    
    void traverse();
    
    void setNode(Node* node);
    
    static bool start;
    static bool hardStart;
  
    void setCount(int countNum);
    void setCountLimit(int countLimit);
    
    int getChildCount();
    
    RTGraph makeRTGraph(Node* root);
    void updateProcessorGraph(Node* root);

    
    Node* node = nullptr;
    Node* referenceNode = nullptr;
    Node* target = nullptr;
    Node* root = nullptr;
    NodeData* data = nullptr;
    
    int count = 0;
    int countLimit = 1;
    int childCount = 0;
    
    
    private:
    Node* lastReference = nullptr;
    
};
