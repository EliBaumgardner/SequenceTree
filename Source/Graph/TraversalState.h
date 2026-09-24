#pragma once

#include "RTData.h"

#include <juce_data_structures/juce_data_structures.h>

#include <vector>

class GraphState;

class TraversalState {

public:

    explicit TraversalState(GraphState& graphState);

    juce::ValueTree addTraversalData(int traversalId, juce::UndoManager* undoManager);

    void collectKeys(std::vector<TraversalKey>& keys) const;

    int unusedInstance(int traversalTypeId) const;

    static juce::ValueTree findReference(const juce::ValueTree& references, const TraversalKey& key);

    juce::ValueTree map;

    static constexpr int defaultTraversalId {1};
    static constexpr int    defaultTempoMult {1};
    static constexpr int    defaultChannel   {1};
    static constexpr int    defaultTranspose {0};
    static constexpr double defaultVelocity  {1.0};

private:

    GraphState& graphState;
};
