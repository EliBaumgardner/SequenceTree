#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../Util/NodeInfo.h"

class NodeCanvas;
class Node;
class GraphState;

class NodeManager
{
public:

    NodeManager(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager);
    ~NodeManager();

    const std::unordered_map<int, std::unique_ptr<Node>>& all() const
    {
        return nodes;
    }

    Node* find               (int nodeId) const;
    Node* instantiateFromTree(const juce::ValueTree& nodeValueTree);

    void add   (int nodeId);
    void remove(int nodeId);
    void clear ();

    void setPosition    (int nodeId);
    void moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY);

    void setDisplayMode (NodeDisplayMode mode);
    void clearHighlights();
    void clearOutlines  ();

    void equipRootTraversals();
    void setInterceptsClicks(bool shouldIntercept, bool shouldChildrenIntercept);

    NodeDisplayMode displayMode = NodeDisplayMode::Pitch;

    std::vector<std::function<void(Node*, bool)>> nodeSelectedListeners;

private:

    void connectIncomingArrows(int nodeId, Node* node);
    void connectOutgoingArrows(const juce::ValueTree& nodeValueTree, Node* node);

    void moveDescendants                (juce::ValueTree nodeValueTree, int deltaX, int deltaY, std::unordered_set<int>& visited, int draggedNodeId);
    void moveEncapsulatorWithEntryMember(int nodeId, int draggedNodeId, int deltaX, int deltaY);

    NodeCanvas&        nodeCanvas;
    GraphState&        graphState;
    juce::UndoManager& undoManager;

    std::unordered_map<int, std::unique_ptr<Node>> nodes;
};
