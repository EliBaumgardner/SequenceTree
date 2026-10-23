#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../Util/NodeInfo.h"
#include "../Graph/NodeFactory.h"

/*
    The node features of the canvas: creating a root, moving nodes, and creating a child by
    dragging. createRootNode makes a root where Shift+click landed. respondToDrag handles a drag
    that starts on a node: a plain drag moves the node and its descendants (handleNodeDrag),
    snapped to the grid, while a Shift drag creates a new child under it, of the kind
    creationMode names (Ctrl+Shift makes an alternative), connects it with the toolbar's
    ArrowInfo::Settings, and then moves that child with the cursor. A child dragged onto another tree's
    root snaps to it, and finishDrag replaces the new child with a cross-root arrow to that root.
    A collapsed group creates its child from the group's last member.

    dragArrowHead and finishArrowHeadDrag do the same moving for the node an arrow points at,
    when the arrow's head is grabbed. NodeController routes to it and Titlebar sets creationMode.
    Every edit goes through GraphState and NodeFactory in an undo transaction.
*/

class Node;
class NodeCanvas;
class GraphState;

class NodeGestures
{
public:

    NodeGestures(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager);

    void createRootNode(juce::Point<float> clickPoint);

    void respondToDrag(Node& node, juce::Point<int> cursor, const juce::ModifierKeys& mods);
    void finishDrag   ();

    void dragArrowHead      (juce::Point<int> cursor);
    void finishArrowHeadDrag();

    NodeCreationMode creationMode = NodeCreationMode::Node;

    juce::Component::SafePointer<Node> arrowHeadNode;

private:

    void handleNodeDrag     (int nodeId, NodeInfo::Position newPosition);
    void handleNodeDragStart(Node* node, int nodeId, NodeInfo::Position newPosition, const juce::ModifierKeys& mods);
    void snapToGrid         (NodeInfo::Position& newPosition, juce::ValueTree draggedNodeTree);

    void checkRootNodeSnap       (juce::Point<int> canvasPoint);
    void connectDraggedNodeToRoot();
    void setDraggedNodeVisible   (bool shouldBeVisible);

    static constexpr float rootSnapThreshold = 20.0f;

    NodeCanvas&        nodeCanvas;
    GraphState&        graphState;
    juce::UndoManager& undoManager;

    juce::Component::SafePointer<Node> snapTargetRoot;

    juce::ValueTree draggedNodeTree;

    int snapSourceNodeId = -1;

    bool isDragStart = true;
};
