#pragma once

#include <juce_data_structures/juce_data_structures.h>

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
