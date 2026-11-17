#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "EventManager.h"
#include "TraversalSession.h"
#include "ThreadMessaging/AudioUIBridge.h"
#include "../Graph/RTData.h"

#include <atomic>
#include <optional>
#include <vector>

class PlaybackEngine
{
public:

    struct BlockInfo;

    explicit PlaybackEngine(AudioUIBridge& bridge);

    void prepare     (double sampleRate);
    void processBlock(const Snapshot* snapshot, juce::MidiBuffer& midiMessages, const BlockInfo& block) noexcept;
    void releaseNotes ();

    struct TempoInfo
    {
        static constexpr double referenceBpm             = 120.0;
        static constexpr double relocationToleranceBeats = 1.0 / 64.0;
        static constexpr double secondsPerMinute         = 60.0;

        double currentSampleRate = 44100.0;
        double hostBpm           = 0.0;

        bool hostWasPlaying    = false;
        bool hostPositionKnown = false;

        std::optional<double> expectedPpq;
        std::optional<double> pendingRelocationSamples;
    };

    struct BlockInfo
    {
        juce::AudioPlayHead* playHead = nullptr;

        float tempoParameter    = 1.0f;
        float velocityParameter = 0.0f;

        int numSamples = 0;
        int transpose  = 0;

        bool followsHost  = false;
        bool syncedToHost = false;
        bool offline      = false;
    };

    TempoInfo tempoInfo;

    std::atomic<bool> isPlaying            { false };
    std::atomic<bool> resetRequested       { false };
    std::atomic<bool> playbackStateChanged { false };

    bool wasPlaying = false;

    EventManager eventManager;
    EventManager previewEventManager;

    TraversalSession traversalSession;

    std::vector<juce::MidiMessage> pendingNoteOffs;

private:

    struct WalkRequest;

    void followHostTransport(const BlockInfo& block) noexcept;
    void driveWalk          (const Snapshot& snap, juce::MidiBuffer& midiMessages, const BlockInfo& block, const WalkRequest& request) noexcept;

    struct WalkRequest
    {
        bool playing         = false;
        bool resetHit        = false;
        bool replayThisBlock = false;
    };
};
