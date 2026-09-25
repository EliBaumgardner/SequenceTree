#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
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

    void setSelectChildScript(const RTScript* script);

    enum class Playback { Live, Replaying };

    void     beginReplay   (const DispatchContext& context, double targetSamples);
    Playback continueReplay(const DispatchContext& context, std::uint64_t graphGeneration, int numSamples,
                            bool playing);

    TraversalPool traversals;

    Playback playback               = Playback::Live;
    double   replayRemainingSamples = 0.0;

private:

    void syncActiveTraversals   (const NodeMap& nodes);
    void removeDeletedTraversals(const NodeMap& nodes, juce::MidiBuffer& midiMessages);

    void startMissingTraversals (const DispatchContext& context);

    void syncTraversalLoopLimits(const DispatchContext& context);

    void startTraversal(const RTNode& rootNode, const RTtraversal& traversal,
                        const DispatchContext& context);

    void stopTraversalNotes(int runId, juce::MidiBuffer& midiMessages);

    int findFirstUnlinkedRootId(const NodeMap& nodes);


    EventManager& eventManager;

    RTScript            nativeFallbackScript;
    ScriptTraversalRule scriptRule;

    static constexpr bool useScriptedChildSelection = true;

    static constexpr int scratchCapacity           = 256;
    static constexpr int maxConcurrentTraversals   = 128;

    std::vector<int> activeRootIdScratch;
    std::vector<int> restartRootScratch;
    std::vector<int> linkedRootScratch;
    std::vector<int> removedRunIdScratch;

    static constexpr int    replayChunkSamples      = 1024;
    static constexpr int    replayMidiCapacityBytes = 65536;
    static constexpr double replayShareOfBlock      = 0.25;

    juce::MidiBuffer replayMidi;

    std::uint64_t syncedGraphGeneration = 0;
    std::uint64_t syncedPoolEpoch       = 0;
};
