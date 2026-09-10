#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <vector>

class GraphState;

class EncapsulationOps {

public:

    explicit EncapsulationOps(GraphState& graphState) : graphState(graphState) {}

    juce::ValueTree create         (const std::vector<int>& memberNodeIds, juce::UndoManager* undoManager);
    void            dissolve       (int encapsulatorId, juce::UndoManager* undoManager);
    void            removeGroup    (int encapsulatorId, juce::UndoManager* undoManager);
    void            insertNodeAfter(int nodeId, int siblingNodeId, juce::UndoManager* undoManager);

    std::vector<int> memberIds  (int encapsulatorId) const;
    int              unusedLabel() const;

private:

    GraphState& graphState;
};
