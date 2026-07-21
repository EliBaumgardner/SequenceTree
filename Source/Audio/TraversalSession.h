#pragma once

#include "../Util/PluginModules.h"
#include "TraversalLogic.h"
#include "../Graph/RTData.h"

class EventManager;

class TraversalSession
{
public:

    explicit TraversalSession(EventManager& eventManager);

    void silenceAllNotes(juce::MidiBuffer& midiMessages);
    void clearTraversals();

    void restartActiveTraversals(const NodeMap& nodes, RTGraphs& rtGraphs,
                                 juce::MidiBuffer& midiMessages);

    void syncWithGraph(const NodeMap& nodes, RTGraphs& rtGraphs,
                       juce::MidiBuffer& midiMessages);

    bool startTraversalsFromFirstRoot(const NodeMap& nodes, RTGraphs& rtGraphs,
                                      juce::MidiBuffer& midiMessages);

    TraversalMap&       getTraversals()       { return traversals; }
    const TraversalMap& getTraversals() const { return traversals; }

    bool isIdle() const { return traversals.empty(); }

    int nextTraversalInstanceId() { return ++traversalInstanceCounter; }

private:

    void updateTraversalCounts  (const NodeMap& nodes);
    void syncActiveTraversals   (const NodeMap& nodes);
    void removeDeletedTraversals(const NodeMap& nodes, juce::MidiBuffer& midiMessages);

    void startMissingTraversals (const NodeMap& nodes, RTGraphs& rtGraphs,
                                 juce::MidiBuffer& midiMessages);

    void syncTraversalLoopLimits(const NodeMap& nodes, RTGraphs& rtGraphs,
                                 juce::MidiBuffer& midiMessages);

    void startTraversal(const RTNode& rootNode, const RTtraversal& traversal,
                        const NodeMap& nodes, RTGraphs& rtGraphs,
                        juce::MidiBuffer& midiMessages);

    void stopTraversalNotes(int instanceId, juce::MidiBuffer& midiMessages);

    int findFirstUnlinkedRootId(const NodeMap& nodes) const;

    EventManager& eventManager;

    TraversalMap traversals;

    int traversalInstanceCounter = 0;
};
