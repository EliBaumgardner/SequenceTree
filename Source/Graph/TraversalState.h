#pragma once

#include "RTData.h"

#include <juce_data_structures/juce_data_structures.h>

#include <vector>

/*
    The store of traversal settings. A traversal is a walk through the graph that plays the notes
    it reaches; its traversal id is its type, and a TraversalKey adds an instance number so one
    type can be equipped several times, each instance walking on its own. map holds one
    TraversalData tree per traversal id with the settings all its instances play with: tempo
    multiplier, colour, MIDI channel, transpose and velocity multiplier.

    addTraversalData creates the settings for a traversal id with default values, or returns the
    existing ones. collectKeys returns every key the graph uses, once each: the keys nodes list in
    their TraversalChildrenIds and the keys traversal flag nodes name. unusedInstance returns the
    smallest instance number of a traversal id that none of those keys has, and findReference
    finds a key's entry in a node's TraversalChildrenIds.

    GraphState owns one as traversals and saves map with nodeMap. TraversalMenu and
    AllowedTraversalsMenu edit the settings, and RTGraphBuilder copies them into each RTtraversal.
*/

class GraphState;

class TraversalState {

public:

    explicit TraversalState(GraphState& graphState);

    juce::ValueTree addTraversalData(int traversalId, juce::UndoManager* undoManager);

    void collectKeys(std::vector<TraversalKey>& keys) const;

    int unusedInstance(int traversalTypeId) const;

    static juce::ValueTree findReference(const juce::ValueTree& references, const TraversalKey& key);

    juce::ValueTree map;

    static constexpr int    defaultTraversalId {1};
    static constexpr int    defaultTempoMult   {1};
    static constexpr int    defaultChannel     {1};
    static constexpr int    defaultTranspose   {0};
    static constexpr double defaultVelocity    {1.0};

private:

    GraphState& graphState;
};
