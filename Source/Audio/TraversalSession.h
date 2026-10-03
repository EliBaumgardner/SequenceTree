#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "TraversalPool.h"
#include "TraversalDispatcher.h"
#include "../Graph/RTData.h"
#include <cstdint>

class EventManager;

class TraversalSession
{
public:

    TraversalSession(EventManager& eventManager, EventManager& previewEventManager);

    void prepare();

    void silenceAllNotes(juce::MidiBuffer& midiMessages);
    void clearTraversals();

    void suspendActiveNotes(juce::MidiBuffer& midiMessages);
    void resumeSuspendedNotes(juce::MidiBuffer& midiMessages);

    void restartActiveTraversals(const DispatchContext& context);

    void syncWithGraph(const DispatchContext& context, std::uint64_t graphGeneration);

    bool startTraversalsFromFirstRoot(const DispatchContext& context);

    void setTraversalScript(const RTScript* script);

    void playPreview(const DispatchContext& context, int numSamples);

    enum class Playback { Live, Suspended, Replaying };

    void     beginReplay   (const DispatchContext& context, double targetSamples);
    Playback continueReplay(const DispatchContext& context, std::uint64_t graphGeneration, int numSamples,
                            bool playing);

    TraversalPool traversals;
    TraversalPool previewTraversals;

    static constexpr int previewRequestCapacity = 16;

    CommandFifo<RTPreviewRequest, previewRequestCapacity> previewRequests;

    Playback playback               = Playback::Live;
    double   replayRemainingSamples = 0.0;

private:

    void startTraversal(const RTNode& rootNode, const RTtraversal& traversal,
                        const DispatchContext& context);

    void syncActiveTraversals   (const NodeMap& nodes);
    void removeDeletedTraversals(const NodeMap& nodes, juce::MidiBuffer& midiMessages);
    void stopTraversalNotes(int runId, juce::MidiBuffer& midiMessages);
    void startMissingTraversals (const DispatchContext& context);
    void syncTraversalLoopLimits(const DispatchContext& context);
    void stopPreview (juce::MidiBuffer& midiMessages);
    void startPreview(const RTPreviewRequest& request, const DispatchContext& context);


    EventManager& eventManager;
    EventManager& previewEventManager;

    static constexpr int scratchCapacity           = 256;
    static constexpr int maxConcurrentTraversals   = 128;
    static constexpr int maxPreviewTraversals      = 16;
    static constexpr int previewRunIdBase          = 1 << 24;

    std::vector<int> activeRootIdScratch;
    std::vector<int> restartRootScratch;
    std::vector<int> removedRunIdScratch;

    static constexpr int    replayChunkSamples      = 1024;
    static constexpr int    replayMidiCapacityBytes = 65536;
    static constexpr double replayShareOfBlock      = 0.25;

    juce::MidiBuffer replayMidi;

    std::uint64_t syncedGraphGeneration = 0;
    std::uint64_t syncedPoolEpoch       = 0;
};
