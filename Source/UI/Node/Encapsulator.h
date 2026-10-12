#pragma once

#include "Node.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class GraphState;
class NodeCanvas;

class Encapsulator : public Node
{
public:

    Encapsulator(juce::UndoManager& undoManager, GraphState& graphState, NodeCanvas& nodeCanvas);

    void bindToTree            () override;
    void bindValueEditorForMode() override;
    void lookAndFeelChanged    () override;

    void syncHighlightsFromMembers();

    std::vector<int> memberNodeIds;
    juce::ValueTree  firstMemberValueTree;
    bool             isExpanded = false;

private:

    GraphState& graphState;
    NodeCanvas& nodeCanvas;
};
