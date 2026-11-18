#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <span>
#include <vector>

/*
    The functions that create, change and remove encapsulation groups. A group is an
    EncapsulatorData node in nodeMap whose EncapsulatedIds child lists its member node ids in
    order, and each member stores the group's id in its EncapsulatorId property. Every function
    here updates both sides together, so a group's list and its members' EncapsulatorId always
    agree.

    create makes a group from the given node ids, in that order. dissolve deletes the group node
    and clears its members' EncapsulatorId, leaving the members in the graph; removeGroup deletes
    the members as well. insertNodeAfter adds a node to its sibling's group directly after the
    sibling. removeMember takes a node out of its group and dissolves the group once it has no
    members. memberIds returns the member ids in order, and unusedLabel returns the smallest
    EncapsulatorLabel no group uses.

    GraphState owns one as encapsulation. GraphState::removeNode calls removeGroup when the node is
    a group and removeMember when it is a member, and removeGroup calls removeNode for each member.
*/

class GraphState;

class EncapsulationOps {

public:

    explicit EncapsulationOps(GraphState& graphState) : graphState(graphState) {}

    juce::ValueTree create(std::span<const int> memberNodeIds, juce::UndoManager* undoManager);

    void dissolve       (int encapsulatorId, juce::UndoManager* undoManager);
    void removeGroup    (int encapsulatorId, juce::UndoManager* undoManager);
    void insertNodeAfter(int nodeId, int siblingNodeId, juce::UndoManager* undoManager);
    void removeMember   (int nodeId, int encapsulatorId, juce::UndoManager* undoManager);

    std::vector<int> memberIds(int encapsulatorId) const;

    int unusedLabel() const;

private:

    GraphState& graphState;
};
