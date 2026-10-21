#pragma once

#include "../Util/ArrowInfo.h"

#include <juce_data_structures/juce_data_structures.h>

/*
    The functions that read and write what an arrow carries besides joining two nodes (its type,
    bindings, multipliers, sync and duration override), and that keep the pitch an arrow's binding
    gives its target note in step with where the nodes are. An arrow is an entry in its parent
    node's NodeChildrenIds or DanglingArrows list, and its settings are properties of that entry.

    getArrowInfo reads an arrow's type, X and Y bindings, X and Y multipliers, sync and duration
    override into an ArrowInfo, and setArrowInfo writes one back. A pitch-bound arrow shifts its
    target's pitch by an amount computed from the arrow's X or Y length, and records the shift it
    has applied in ArrowPitchOffset. When a node moves, syncPitchBindings recomputes that amount
    for every arrow into, out of and dangling from the node, and changes the target's pitch only by
    the difference from the recorded shift, kept inside the MIDI range. clearArrowDurations removes
    the duration override from every arrow into or out of a node, so their lengths set the
    duration again.

    GraphState::setNodePosition calls both after a move, and NodeFactory calls syncPitchBindings
    when a dangling arrow is created or its tip moves. Every write goes through the caller's
    UndoManager.
*/

class GraphState;

class ArrowBindingOps {

public:

    explicit ArrowBindingOps(GraphState& graphState) : graphState(graphState) {}

    void syncPitchBindings  (int nodeId, juce::UndoManager* undoManager);
    void clearArrowDurations(int nodeId, juce::UndoManager* undoManager);

    static void      setArrowInfo(juce::ValueTree arrowTree, const ArrowInfo& arrowInfo, juce::UndoManager* undoManager);
    static ArrowInfo getArrowInfo(const juce::ValueTree& arrowTree);

private:

    void applyArrowPitchOffset(juce::ValueTree arrowTree, int targetNodeId, int deltaX, int deltaY, juce::UndoManager* undoManager);

    GraphState& graphState;
};
