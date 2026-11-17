#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <span>

/*
    The functions that draw an encapsulation group on the canvas. A group is an Encapsulator
    component standing for its member nodes, which are listed in order in the group's tree (see
    EncapsulationOps). A collapsed group shows its Encapsulator and hides the members and the
    arrows between them; its last member is the exit whose outgoing arrows stay visible. An
    expanded group hides the Encapsulator, shows the members and draws a ring in the group's
    colour around each.

    collapse, expand and collapseAll switch groups between the two states, refreshMembership
    redraws an expanded group after its members changed, and showMembers makes members ordinary
    visible nodes again, as when a group is removed. collapsedSpanShift gives the offset that
    closes the gap behind each collapsed group upstream of a node, the distance from the group's
    exit member to the group's own position, which NodeManager::setPosition applies.
    syncHighlights makes each Encapsulator show the highlights of its members, and recolourGroup
    copies a group's colour to the rings of its members.

    NodeCanvas owns it. EncapsulationGestures calls collapse and expand, NodeManager calls it
    as nodes come and go, and AudioCommandDrainer calls syncHighlights. It changes the
    components only and never writes the graph.
*/

class NodeCanvas;
class Encapsulator;
class GraphState;

class EncapsulationView
{
public:

    EncapsulationView(NodeCanvas& nodeCanvas, GraphState& graphState);

    void collapse         (int encapsulatorId) const;
    void expand           (int encapsulatorId) const;
    void refreshMembership(int encapsulatorId) const;
    void collapseAll      () const;

    void showMembers   (std::span<const int> memberNodeIds) const;
    void syncHighlights() const;
    void recolourGroup (const Encapsulator& encapsulator) const;

    juce::Point<int> collapsedSpanShift(int nodeId) const;

private:

    bool applyCollapsedState(int encapsulatorId) const;
    void repositionAll() const;

    NodeCanvas& nodeCanvas;
    GraphState& graphState;
};
