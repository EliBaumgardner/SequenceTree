#pragma once

#include "../Util/PluginModules.h"
#include "TraversalPool.h"
#include "../Graph/RTData.h"

class EventManager;

class TraversalSession
{
public:

    explicit TraversalSession(EventManager& eventManager);

    void prepare();

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

    static constexpr int scratchCapacity           = 256;
    static constexpr int maxConcurrentTraversals   = 128;

    std::vector<int> activeRootIdScratch;

    int traversalInstanceCounter = 0;
};
