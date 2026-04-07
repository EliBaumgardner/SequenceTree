/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../Util/PluginModules.h"
#include "../Util/PluginContext.h"
#include <memory>
#include <atomic>
#include "../Util/RTData.h"
#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "TraversalLogic.h"
#include "EventManager.h"

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

    void clearOldEvents(RTNode node, int traversalId);
    void setNewGraph(std::shared_ptr<RTGraph> graph);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    NodeCanvas* canvas;

    using RTGraphs = std::unordered_map<int, std::shared_ptr<RTGraph>>;
    std::shared_ptr<RTGraphs> rtGraphs     = nullptr;
    std::shared_ptr<NodeMap>  globalNodes  = nullptr;

    int numGraphs = 0;

    std::atomic<bool>   isPlaying       = false;
    std::atomic<bool>   resetRequested  = false;
    std::atomic<double> tempoMultiplier { 1.0 };

    juce::AudioProcessorValueTreeState valueTreeState;


    struct {
        double currentSampleRate;
        int samplesPerBeat;
        int currentStep;
        int stepLengthInSamples;
        int samplesIntoStep;
        int bpm;
    } TempoInfo { 44100, 0, 0, 0, 0, 120 };

    EventManager eventManager { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessor)
};