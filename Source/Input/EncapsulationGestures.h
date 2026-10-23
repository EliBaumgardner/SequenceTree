#pragma once

#include <juce_data_structures/juce_data_structures.h>

/*
    The encapsulation features of the canvas: making and dissolving groups with the span tool, and
    expanding and collapsing them. selectSpanNode handles a click with the span tool. On a group
    it dissolves the group; on the first plain node or root it marks the node as the span's start;
    on a second node in the same tree it creates a group of the nodes between them
    (GraphState::nodeIdsBetween). toggleEncapsulationExpansion handles a right-click: on a
    collapsed group it expands it, on the first or last member of an expanded group it collapses
    it, and it returns whether it did either. NodeController routes to it. Group changes go
    through GraphState and NodeFactory in an undo transaction; expanding and collapsing are
    EncapsulationView's.
*/

class Node;
class NodeCanvas;
class GraphState;

class EncapsulationGestures
{
public:

    EncapsulationGestures(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager);

    void selectSpanNode(Node& node);

    bool toggleEncapsulationExpansion(Node& node);

private:

    NodeCanvas&        nodeCanvas;
    GraphState&        graphState;
    juce::UndoManager& undoManager;
};
