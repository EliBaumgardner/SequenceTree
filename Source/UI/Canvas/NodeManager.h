#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../Util/NodeInfo.h"

/*
    The owner of the canvas's Node components: one Node for each node in the graph, kept in a map
    by node id and added to the NodeCanvas as a child. Which class a node becomes (Node, RootNode,
    Modulator, TraversalFlagNode or Encapsulator, and whether it is an alternative) follows the
    type of its tree.

    instantiateFromTree builds the component for a tree. add does that for a node just added to
    the graph, connects its arrows to the nodes it already links to and updates the groups it
    belongs to; remove undoes that. setPosition moves a component to where the graph says its node
    is, drawing a node in a collapsed group at the group's position. moveDescendants writes a
    new position for every node below a node, and for the group of any member it reaches, by the
    same offset as a drag. setDisplayMode, clearHighlights, clearOutlines and setInterceptsClicks
    apply one change to every node, and equipRootTraversals asks each root to equip its
    traversals.

    NodeCanvas::handleAsyncUpdate calls it for NodeAdded, NodeRemoved and NodeMoved updates.
    ArrowManager is its partner for arrows; each calls into the other.
*/

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

    void setDisplayMode (NodeInfo::DisplayMode mode);
    void clearHighlights();
    void clearOutlines  ();

    void equipRootTraversals();
    void setInterceptsClicks(bool shouldIntercept, bool shouldChildrenIntercept);

    NodeInfo::DisplayMode displayMode = NodeInfo::DisplayMode::Pitch;

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
