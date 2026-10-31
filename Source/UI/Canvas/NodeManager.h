#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../../Util/NodeInfo.h"

/*
    The owner of an EditCanvas's Node components: one Node for each node in the canvas's
    document, kept in a map by node id and added to the canvas as a child. Which class a node
    becomes is the canvas's choice (EditCanvas::makeNode), and EditCanvas::nodeCreated finishes
    its setup.

    instantiateFromTree builds the component for a tree. add does that for a node just added to
    the document, connects its arrows to the nodes it already links to and calls
    EditCanvas::nodeAdded; remove calls EditCanvas::nodeRemoving and undoes it. setPosition moves
    a component to EditCanvas::drawnPosition. setDisplayMode, clearHighlights, clearOutlines and
    setInterceptsClicks apply one change to every node, equipRootTraversals asks each root to
    equip its traversals, and selectAll, clearAll and deselectAllExcept change which nodes are
    selected.

    EditCanvas::handleAsyncUpdate calls it for NodeAdded, NodeRemoved and NodeMoved updates.
    ArrowManager is its partner for arrows; each calls into the other.
*/

class EditCanvas;
class Node;

class NodeManager
{
public:

    explicit NodeManager(EditCanvas& canvas);
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

    void setPosition(int nodeId);

    void setDisplayMode (NodeInfo::DisplayMode mode);
    void clearHighlights();
    void clearOutlines  ();

    void equipRootTraversals();
    void setInterceptsClicks(bool shouldIntercept, bool shouldChildrenIntercept);

    void selectAll () const;
    void clearAll () const;
    void deselectAllExcept(const Node& keptNode) const;

    NodeInfo::DisplayMode displayMode = NodeInfo::DisplayMode::Pitch;

    std::vector<std::function<void(Node*, bool)>> nodeSelectedListeners;

private:

    void connectIncomingArrows(int nodeId, Node* node);
    void connectOutgoingArrows(const juce::ValueTree& nodeValueTree, Node* node);

    EditCanvas& canvas;

    std::unordered_map<int, std::unique_ptr<Node>> nodes;
};
