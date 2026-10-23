#pragma once

#include "Node.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

/*
    The Node that stands for an encapsulation group (see EncapsulationOps) while the group is
    collapsed. memberNodeIds is the group's member ids in order, read from GraphState, and the
    badge editors are bound to the first member, so the group shows that member's count limit,
    switch count limit and sub-loop limit. Its main editor shows the group's EncapsulatorLabel as
    a Greek letter. isExpanded says whether the group is currently drawn as its members, in which
    case this node is hidden.

    syncHighlightsFromMembers gives it the highlights its members have, so a walk inside a
    collapsed group lights the group. When its look-and-feel changes it recolours the rings of
    its members through EncapsulationView.

    NodeManager creates it for an EncapsulatorData tree. EncapsulationView switches isExpanded
    and calls bindToTree when the membership changes.
*/

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
