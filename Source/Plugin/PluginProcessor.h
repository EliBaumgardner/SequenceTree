/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../Util/PluginModules.h"
#include <memory>
#include <atomic>
#include <functional>
#include "../Graph/RTData.h"
#include "../Graph/ValueTreeState.h"
#include "../Audio/EventManager.h"
#include "../Audio/TraversalSession.h"

class SequenceTreeAudioProcessorEditor;

class SequenceTreeAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SequenceTreeAudioProcessor();
    ~SequenceTreeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void clearOldEvents(int traversalId);
    void setNewGraph(std::shared_ptr<RTGraph> graph);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::function<void()>                  notifyUi;
    std::function<void(juce::ValueTree)>   applyStateToUi;

    juce::ValueTree                        pendingRestoreState;

    struct AudioSnapshot
    {
        std::shared_ptr<NodeMap>      globalNodes;
        std::shared_ptr<RTGraphs>     rtGraphs;
    };

    std::shared_ptr<AudioSnapshot> audioSnapshot;

    void publishAudioSnapshot(std::shared_ptr<AudioSnapshot> snapshot);

    std::atomic<bool>   isPlaying       = false;
    std::atomic<bool>   resetRequested  = false;
    std::atomic<double> tempoMultiplier { 1.0 };

    std::vector<std::shared_ptr<AudioSnapshot>> retiredSnapshots;

    juce::AudioProcessorValueTreeState valueTreeState;

    ValueTreeState graphState;


    struct {
        double currentSampleRate;
        int samplesPerBeat;
        int currentStep;
        int stepLengthInSamples;
        int samplesIntoStep;
        int bpm;
    } TempoInfo { 44100, 0, 0, 0, 0, 120 };

    EventManager     eventManager     { this };
    TraversalSession traversalSession { eventManager };

    bool hasPendingUiCommands() const;

    int numTraversals = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessor)
};