//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_NODEMANAGER_H
#define SEQUENCETREE_NODEMANAGER_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <unordered_map>
#include <unordered_set>

#include "../../Util/NodeInfo.h"

class NodeCanvas;
class Node;
struct ApplicationContext;

class NodeManager {

public:

    NodeManager(NodeCanvas& canvas, ApplicationContext& context);
    ~NodeManager();

    Node* find(int nodeId) const;
    const std::unordered_map<int, Node*>& all() const { return nodes; }

    Node* instantiateFromTree(const juce::ValueTree& nodeValueTree);

    void add(int nodeId);
    void remove(int nodeId);
    void clear();

    void setPosition(int nodeId);
    void moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY);

    void setDisplayMode(NodeDisplayMode mode) const;
    void equipRootTraversals() const;
    void setInterceptsClicks(bool shouldIntercept) const;

private:

    void moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY, std::unordered_set<int>& visited);

    NodeCanvas& canvas;
    ApplicationContext& applicationContext;

    std::unordered_map<int, Node*> nodes;
};

#endif //SEQUENCETREE_NODEMANAGER_H
