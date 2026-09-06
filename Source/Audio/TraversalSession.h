#pragma once

#include "../Util/PluginModules.h"
#include "TraversalPool.h"
#include "ScriptTraversalRule.h"
#include "TraversalDispatcher.h"
#include "../Graph/RTData.h"
#include <cstdint>

class EventManager;

class TraversalSession
{
public:

    explicit TraversalSession(EventManager& eventManager);

    void prepare();

    void silenceAllNotes(juce::MidiBuffer& midiMessages);
    void clearTraversals();

    void suspendActiveNotes(juce::MidiBuffer& midiMessages);

    void restartActiveTraversals(const DispatchContext& context);

    void syncWithGraph(const DispatchContext& context, std::uint64_t graphGeneration);

    bool startTraversalsFromFirstRoot(const DispatchContext& context);

    TraversalPool&       getTraversals()       { return traversals; }
    const TraversalPool& getTraversals() const { return traversals; }

    bool isIdle() const { return traversals.empty(); }

    void setSelectChildScript(const RTScript* script);

private:

    void syncActiveTraversals   (const NodeMap& nodes);
    void removeDeletedTraversals(const NodeMap& nodes, juce::MidiBuffer& midiMessages);

    void startMissingTraversals (const DispatchContext& context);

    void syncTraversalLoopLimits(const DispatchContext& context);

    void startTraversal(const RTNode& rootNode, const RTtraversal& traversal,
                        const DispatchContext& context);

    void stopTraversalNotes(int instanceId, juce::MidiBuffer& midiMessages);

    int findFirstUnlinkedRootId(const NodeMap& nodes);


    EventManager& eventManager;
    TraversalPool traversals;

    RTScript            nativeFallbackScript;
    ScriptTraversalRule scriptRule;

    static constexpr bool useScriptedChildSelection = true;

    static constexpr int scratchCapacity           = 256;
    static constexpr int maxConcurrentTraversals   = 128;

    std::vector<int> activeRootIdScratch;
    std::vector<int> restartRootScratch;
    std::vector<int> linkedRootScratch;

    std::uint64_t syncedGraphGeneration = 0;
    std::uint64_t syncedPoolEpoch       = 0;
};
