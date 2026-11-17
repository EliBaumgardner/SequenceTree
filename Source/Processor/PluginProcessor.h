#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

#include <atomic>
#include <vector>
#include <functional>
#include "Processor/AudioModel/RTGraphBuilder.h"
#include "Processor/Playback/PlaybackEngine.h"
#include "Processor/Channels/VirtualMidiOutput.h"
#include "Processor/AudioModel/AudioSnapshotPublisher.h"
#include "PluginDocument.h"

/*
    The plugin itself, as the host sees it, and the root that owns everything the sound depends on:
    the document (a PluginDocument holding graphState, traversalRuleState, ornamentLibraries,
    undoManager, patch, and in canvasSettings the grid multipliers and the settings new arrows
    start from, beside the host parameters in valueTreeState), the audio model (rtGraphBuilder,
    snapshots), playback (playback, a PlaybackEngine holding traversalSession and the two
    EventManagers) and the output channels (bridge, virtualMidiOutput). The editor window is
    separate and optional, so closing it loses nothing.

    processBlock runs on the audio thread and outputs MIDI only; the audio buffer is cleared. Each
    block it takes the current snapshot, reads the parameters and the host's play head into a
    PlaybackEngine::BlockInfo, hands both to PlaybackEngine::processBlock, which follows the host's
    play state, tempo and position (a jump to a new position becomes pendingRelocationSamples, which
    makes TraversalSession replay silently to it), sends any pending note-offs and drives
    TraversalSession and EventManager, and ends by copying the block's MIDI to virtualMidiOutput.

    getStateInformation and setStateInformation save and restore the graph, traversal map, rules,
    colour presets, canvas settings, parameters and patch as one ValueTree, which PluginDocument
    marks with its schemaVersion; a state marked with a newer version is not loaded.
    setStateInformation restores the parameters at once, on whichever thread the host calls it
    from, so the host reads them back straight away; the rest of a load is applied by
    applyRestoredState, on the message thread only, which rebuilds the audio model and refreshes
    an open editor's canvas. The editor controls playback through the atomics playback.isPlaying
    and playback.resetRequested.
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

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) noexcept [[clang::nonblocking]] override;

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

    struct BlockScope
    {
        AudioSnapshotPublisher& publisher;

        const Snapshot* snapshot = publisher.beginBlock();

        ~BlockScope()
        {
            publisher.endBlock();
        }
    };

    juce::ValueTree pendingRestoreState;

    static constexpr const char* tempoParameterId     = "tempoMultiplier";
    static constexpr const char* velocityParameterId  = "velocity";
    static constexpr const char* transposeParameterId = "transpose";
    static constexpr const char* hostSyncParameterId  = "hostSync";

    static constexpr int parameterVersion = 1;

    juce::AudioProcessorValueTreeState valueTreeState;

    std::atomic<float>& tempoParameter     = *valueTreeState.getRawParameterValue(tempoParameterId);
    std::atomic<float>& velocityParameter  = *valueTreeState.getRawParameterValue(velocityParameterId);
    std::atomic<float>& transposeParameter = *valueTreeState.getRawParameterValue(transposeParameterId);
    std::atomic<float>& hostSyncParameter  = *valueTreeState.getRawParameterValue(hostSyncParameterId);

    PluginDocument document;

    AudioSnapshotPublisher snapshots { document.traversalRuleState };

    RTGraphBuilder rtGraphBuilder { document.graphState, document.ornamentLibraries, snapshots };

    AudioUIBridge bridge;

    PlaybackEngine playback { bridge };

    VirtualMidiOutput virtualMidiOutput;

    JUCE_DECLARE_WEAK_REFERENCEABLE (SequenceTreeAudioProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessor)

private:

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
};