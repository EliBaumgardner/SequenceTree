#pragma once

#include "../Util/PluginModules.h"
#include <memory>
#include <atomic>
#include <vector>
#include <functional>
#include "../Graph/GraphState.h"
#include "../Graph/TraversalRuleState.h"
#include "../Graph/RTGraphBuilder.h"
#include "../Audio/EventManager.h"
#include "../Audio/TraversalSession.h"
#include "AudioSnapshotPublisher.h"

class SequenceTreeAudioProcessorEditor;

class SequenceTreeAudioProcessor  : public juce::AudioProcessor
{
public:
    SequenceTreeAudioProcessor();
    ~SequenceTreeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::function<void()>                  notifyUi;

    std::function<void()>                  suspendStateListeners;
    std::function<void()>                  resumeStateListeners;

    juce::ValueTree                        pendingRestoreState;

    void applyRestoredState();

    std::atomic<bool>   isPlaying       = false;
    std::atomic<bool>   resetRequested  = false;
    bool                wasPlaying      = false;
    std::atomic<double> tempoMultiplier { 1.0 };

    juce::AudioProcessorValueTreeState valueTreeState;

    GraphState graphState;

    TraversalRuleState traversalRuleState;

    AudioSnapshotPublisher snapshots { traversalRuleState };

    RTGraphBuilder rtGraphBuilder { *this, graphState };


    struct TempoInfo
    {
        double currentSampleRate = 44100.0;
    };

    TempoInfo tempoInfo;

    EventManager     eventManager;
    TraversalSession traversalSession { eventManager };

    std::vector<juce::MidiMessage> pendingNoteOffs;

    bool hasPendingUiCommands() const;

    JUCE_DECLARE_WEAK_REFERENCEABLE (SequenceTreeAudioProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessor)
};