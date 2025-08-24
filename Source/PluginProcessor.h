/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
#include <memory>
#include <atomic>
#include "RTData.h"

class SequenceTreeAudioProcessorEditor;

class NodeCanvas;

class Node;

class NodeData;

class NodeLogic;

struct MidiEvent {
    
    int pitch;
    int velocity;
    int duration;
};

struct NodeCount{
    
    int nodeID = 0;
    int count = 0;
};


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
    
    void pushNote(int pitch, int velocity, int duration);
    void setNewGraph(std::shared_ptr<RTGraph> graph);
    
    void scheduleTraversal();
    void traverse();
    
    void scheduleNodeHighlight(RTNode* node,bool shouldHighlight);
    
    NodeCanvas* canvas;
    std::shared_ptr<RTGraph> rtGraph;
    std::shared_ptr<std::unordered_map<int,int>> nodeCounts;

private:
    //==============================================================================
    
    Node* root = nullptr;
    
    double currentSampleRate = 44100.0;
    int samplesPerBeat = 0;

    int currentStep = 0;
    int stepLengthInSamples = 0;
    int samplesIntoStep = 0;
    
    int bpm = 120;

    std::vector<int> midiNotes = { 60, 62, 64, 65, 67, 69, 71, 72 };
    
    static constexpr int fifoSize = 1024;
    juce::AbstractFifo fifo;
    std::vector<MidiEvent> midiBuffer;
    
    RTNode* rtTarget = nullptr;
    int rtTargetId = 0;
    
    RTNode* rtRoot = nullptr;
    int rtRootId = 1;
    
    RTNode* rtReference = nullptr;
    int rtReferenceId = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessor)
};
