//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_NODEMANAGER_H
#define SEQUENCETREE_NODEMANAGER_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../Util/NodeInfo.h"

class NodeCanvas;
class Node;
struct ApplicationContext;

class NodeManager {

public:

    NodeManager(NodeCanvas& canvas, ApplicationContext& context);
    ~NodeManager();

    Node* find(int nodeId) const;
    const std::unordered_map<int, std::unique_ptr<Node>>& all() const { return nodes; }

    Node* instantiateFromTree(const juce::ValueTree& nodeValueTree);

    void add(int nodeId);
    void remove(int nodeId);
    void clear();

    void setPosition(int nodeId);
    void moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY);

    void setDisplayMode(NodeDisplayMode mode);
    void clearHighlights();
    void clearOutlines  ();
    void equipRootTraversals();
    void setInterceptsClicks(bool shouldIntercept, bool shouldChildrenIntercept);

    NodeDisplayMode displayMode = NodeDisplayMode::Pitch;

    std::vector<std::function<void(Node*, bool)>> nodeSelectedListeners;

private:

    void moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY,
                         std::unordered_set<int>& visited, int draggedNodeId);

    void moveEncapsulatorWithEntryMember(int nodeId, int draggedNodeId, int deltaX, int deltaY);

    void connectIncomingArrows(int nodeId, Node* node);
    void connectOutgoingArrows(const juce::ValueTree& nodeValueTree, Node* node);

    NodeCanvas& canvas;
    ApplicationContext& applicationContext;

    std::unordered_map<int, std::unique_ptr<Node>> nodes;
};

#endif //SEQUENCETREE_NODEMANAGER_H
