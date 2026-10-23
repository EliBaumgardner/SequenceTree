#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <optional>
#include <atomic>
#include <vector>
#include <functional>
#include "../Graph/GraphState.h"
#include "../Graph/TraversalRuleState.h"
#include "../Graph/OrnamentLibraries.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Theme/InterfaceIdentifiers.h>
#include "../Graph/RTGraphBuilder.h"
#include "../Audio/EventManager.h"
#include "../Audio/TraversalSession.h"
#include "../Audio/ThreadMessaging/VirtualMidiOutput.h"
#include "AudioSnapshotPublisher.h"

/*
    The plugin itself, as the host sees it, and the root that owns everything the sound depends on:
    the document (graphState, traversalRuleState, ornamentLibraries, undoManager, the host
    parameters in valueTreeState, patch), the audio model (rtGraphBuilder, snapshots), playback
    (traversalSession and the two EventManagers) and the output channels (bridge,
    virtualMidiOutput). The editor window is separate and optional, so closing it loses nothing.

    processBlock runs on the audio thread and outputs MIDI only; the audio buffer is cleared. Each
    block it takes the current snapshot, follows the host's play state, tempo and position
    (followHostTransport; a jump to a new position becomes pendingRelocationSamples, which makes
    TraversalSession replay silently to it), sends any pending note-offs, then driveWalk builds the
    DispatchContexts for playback and note preview and runs TraversalSession and EventManager. It
    ends by copying the block's MIDI to virtualMidiOutput.

    getStateInformation and setStateInformation save and restore the graph, traversal map, rules,
    colour presets, parameters and patch as one ValueTree. A load is applied on the message thread
    by applyRestoredState, which rebuilds the audio model and refreshes an open editor's canvas.
    The editor controls playback through the atomics isPlaying and resetRequested.
*/

class SequenceTreeAudioProcessorEditor;

class SequenceTreeAudioProcessor  : public juce::AudioProcessor
{
public:
    SequenceTreeAudioProcessor();
    ~SequenceTreeAudioProcessor() override = default;

    void prepareToPlay   (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock       (juce::AudioBuffer<float>&, juce::MidiBuffer&) noexcept [[clang::nonblocking]] override;
    void followHostTransport(const int numSamples) noexcept;

    void driveWalk(const AudioSnapshotPublisher::Snapshot& snap, juce::MidiBuffer& midiMessages, const int numSamples, const bool playing, const bool resetHit,
                   const bool replayThisBlock) noexcept;

    juce::AudioProcessorEditor* createEditor() override;
    bool                        hasEditor   () const override;

    bool acceptsMidi () const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;

    const juce::String getName() const override;

    double getTailLengthSeconds() const override;

    int                getNumPrograms   () override;
    int                getCurrentProgram() override;
    void               setCurrentProgram(int index) override;
    const juce::String getProgramName   (int index) override;
    void               changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    void applyRestoredState ();

    struct TempoInfo
    {
        static constexpr double referenceBpm             = 120.0;
        static constexpr double relocationToleranceBeats = 1.0 / 64.0;

        double currentSampleRate = 44100.0;
        double hostBpm           = 0.0;
        bool   hostWasPlaying    = false;
        bool   hostPositionKnown = false;

        std::optional<double> expectedPpq;
        std::optional<double> pendingRelocationSamples;
    };

    struct BlockScope
    {
        AudioSnapshotPublisher&                 publisher;
        const AudioSnapshotPublisher::Snapshot* snapshot = publisher.beginBlock();

        ~BlockScope()
        {
            publisher.endBlock();
        }
    };

    juce::ValueTree pendingRestoreState;

    std::atomic<bool>   isPlaying       = false;
    std::atomic<bool>   resetRequested  = false;
    std::atomic<bool>   playbackStateChanged { false };
    bool                wasPlaying      = false;

    static constexpr const char* tempoParameterId     = "tempoMultiplier";
    static constexpr const char* velocityParameterId  = "velocity";
    static constexpr const char* transposeParameterId = "transpose";
    static constexpr const char* hostSyncParameterId  = "hostSync";
    static constexpr int         parameterVersion     = 1;

    juce::AudioProcessorValueTreeState valueTreeState;

    std::atomic<float>& tempoParameter     = *valueTreeState.getRawParameterValue(tempoParameterId);
    std::atomic<float>& velocityParameter  = *valueTreeState.getRawParameterValue(velocityParameterId);
    std::atomic<float>& transposeParameter = *valueTreeState.getRawParameterValue(transposeParameterId);
    std::atomic<float>& hostSyncParameter  = *valueTreeState.getRawParameterValue(hostSyncParameterId);

    juce::UndoManager undoManager;

    GraphState graphState;

    TraversalRuleState traversalRuleState;

    juce::ValueTree colourPresets { ArsNovaUI::InterfaceIdentifiers::ColourPresets };

    juce::File userFolder  { juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile(JucePlugin_Name) };
    juce::File patchFolder { userFolder.getChildFile("Patches") };

    juce::ValueTree patch { ValueTreeIdentifiers::Patch };

    static constexpr const char* patchExtension = ".sqtree";

    juce::UndoManager ornamentUndoManager;

    OrnamentLibraries ornamentLibraries;

    AudioSnapshotPublisher snapshots { traversalRuleState };

    RTGraphBuilder rtGraphBuilder { *this, graphState };

    TempoInfo tempoInfo;

    AudioUIBridge    bridge;
    EventManager     eventManager        { bridge };
    EventManager     previewEventManager { bridge };
    TraversalSession traversalSession    { eventManager, previewEventManager };

    VirtualMidiOutput virtualMidiOutput;

    std::vector<juce::MidiMessage> pendingNoteOffs;

    JUCE_DECLARE_WEAK_REFERENCEABLE (SequenceTreeAudioProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessor)

private:

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
};