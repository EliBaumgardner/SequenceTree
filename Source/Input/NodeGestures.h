#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>

/*
    The node features of the canvas: creating a root, moving nodes, and creating a child by
    dragging. createRootNode makes a root where Shift+click landed. respondToDrag handles a drag
    that starts on a node: a plain drag moves the node and its descendants (handleNodeDrag),
    snapped to the grid, while a Shift drag creates a new child under it, of the kind
    GraphDocument::creationMode names (Ctrl+Shift makes an alternative), connects it with the
    toolbar's ArrowInfo::Settings, and then moves that child with the cursor. A child dragged onto another tree's
    root snaps to it, and finishDrag replaces the new child with a cross-root arrow to that root.
    A collapsed group creates its child from the group's last member.

    dragArrowHead and finishArrowHeadDrag do the same moving for the node an arrow points at,
    when the arrow's head is grabbed. NodeController routes to it. Every edit goes through
    GraphDocument in an undo transaction.
*/

class Node;
class NodeCanvas;
class GraphState;
class GraphDocument;

class NodeGestures
{
public:

    NodeGestures(NodeCanvas& nodeCanvas, GraphState& graphState, GraphDocument& document, juce::UndoManager& undoManager);

    void createRootNode(juce::Point<float> clickPoint);

    void respondToDrag(Node& node, juce::Point<int> cursor, const juce::ModifierKeys& mods);
    void finishDrag   ();

    void dragArrowHead      (juce::Point<int> cursor);
    void finishArrowHeadDrag();

    juce::Component::SafePointer<Node> arrowHeadNode;

private:

    void handleNodeDrag     (int nodeId, juce::Point<int> cursor);
    void handleNodeDragStart(Node* node, int nodeId, juce::Point<int> cursor, const juce::ModifierKeys& mods);

    void checkRootNodeSnap       (juce::Point<int> canvasPoint);
    void connectDraggedNodeToRoot();
    void setDraggedNodeVisible   (bool shouldBeVisible);

    juce::Point<int> snapToGrid(juce::Point<int> position, const juce::ValueTree& nodeTree) const;

    static constexpr float rootSnapThreshold = 20.0f;

    NodeCanvas&        nodeCanvas;
    GraphState&        graphState;
    GraphDocument&     document;
    juce::UndoManager& undoManager;

    juce::Component::SafePointer<Node> snapTargetRoot;

    juce::ValueTree draggedNodeTree;

    int snapSourceNodeId = -1;

    bool isDragStart = true;
};
