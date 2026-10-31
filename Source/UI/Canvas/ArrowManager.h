#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

/*
    The owner of the canvas's Arrow components: one Arrow for each connection between two nodes in
    the graph and one for each dangling arrow (an arrow with a loose tip that is not connected to
    a node yet), added to the NodeCanvas as children. Each arrow is also entered in its start
    node's nodeArrows, by the end node's id (dangling arrows by a negative index), so the audio
    commands can find it.

    connect and connectParentToChild create the component for a connection, and an alternative
    child gets the arrow drawn from the child to its parent. refreshFor re-lays-out every arrow of
    a node that moved or changed, and rebuildDanglingForNode rebuilds a node's dangling arrows
    from its tree. updatePreview shows the arrow being drawn and commitPreview writes it into the
    document as a dangling arrow (GraphDocument::addDanglingArrow). The snap-ghost functions show
    and remove the arrow that previews a connection to a root. The progress functions start,
    pause, resume and reset the trails that show the walk moving along arrows. setSelected
    selects one arrow.

    NodeCanvas::handleAsyncUpdate calls it for arrow updates and AudioCommandDrainer for the audio
    thread's trail commands. NodeManager is its partner for nodes; each calls into the other.
*/

class NodeCanvas;
class Node;
class Arrow;
class GraphState;

class ArrowManager
{
public:

    ArrowManager(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager);
    ~ArrowManager();

    const juce::OwnedArray<Arrow>& all() const
    {
        return arrows;
    }

    Arrow* find                (int parentNodeId, int childNodeId) const;
    Arrow* connect             (Node* startNode, Node* endNode);
    Arrow* connectParentToChild(Node* parentNode, Node* childNode);

    void adopt(std::unique_ptr<Arrow> arrow);

    void refreshFor               (const Node* movedNode);
    void endNodeDrag              ();
    void rebuildDanglingForNode   (int nodeId);
    void refreshEncapsulatedArrows();

    void remove        (Arrow* arrow);
    void removeForNode (const Node* node);
    void removeMatching(const std::function<bool(Arrow*)>& predicate);
    void clear         ();

    void handleArrowAdded      (int parentNodeId, int childNodeId);
    void handleArrowInfoChanged(int parentNodeId, int childNodeId);

    void updatePreview(Node* node, juce::Point<int> tipOffset);
    void commitPreview();

    void showSnapGhost           (Node* from, Node* to);
    void hideSnapGhost           ();
    void triggerSnapForNode      (int nodeId);
    void connectWithSnapAnimation(int parentNodeId, int childNodeId);

    void setSelected   (Arrow* arrow);
    void clearSelection();

    void resetAllProgress ();
    void pauseAllProgress ();
    void resumeAllProgress();
    void resetTrail       (int trailId);

    std::unique_ptr<Arrow> preview;
    bool                   nodesDragging = false;

private:

    void detach(Arrow* arrow);

    juce::ValueTree connectionTreeFor(int startNodeId, int endNodeId) const;

    static int arrowKey(const Arrow& arrow);

    NodeCanvas&        nodeCanvas;
    GraphState&        graphState;
    juce::UndoManager& undoManager;

    juce::OwnedArray<Arrow> arrows;
    std::unique_ptr<Arrow>  snapGhostArrow;
};
