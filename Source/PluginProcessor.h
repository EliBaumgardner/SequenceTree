/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "Util/ProjectModules.h"

//==============================================================================
/**
*/
#include <memory>
#include <atomic>
#include "Util/RTData.h"

class SequenceTreeAudioProcessorEditor;

class NodeCanvas;

class Node;

class NodeData;

class NodeLogic;

struct MidiEvent {
    
    int pitch;
    int velocity;
    int duration;
    int graphID;
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
    
    void pushNote(int pitch, int velocity, int duration, int graphID);
    void setNewGraph(std::shared_ptr<RTGraph> graph);
    
    void scheduleTraversal();
    void traverse(int graphID);
    void handleTraverser(int nodeID);
    
    void scheduleNodeHighlight(std::shared_ptr<RTNode> node,bool shouldHighlight, int graphID);
    
    struct TraversalInfo {
        int rtTargetId = 0;
        int rtRootId = 1;
        int rtReferenceId = 0;
        std::unordered_map<int,int> counts;
    };
    
    NodeCanvas* canvas;
    
    using RTGraphs = std::unordered_map<int,std::shared_ptr<RTGraph>>;
    std::shared_ptr<RTGraphs> rtGraphs = nullptr;
    
    int numGraphs = 0;
    
    using GraphInfo = std::unordered_map<int,TraversalInfo>;
    std::shared_ptr<GraphInfo> rtGraphInfo = nullptr;

    std::atomic<bool> isPlaying = false;
    
    
    private:
    //==============================================================================
    
    Node* root = nullptr;
    
    double currentSampleRate = 44100.0;
    int samplesPerBeat = 0;
    
    int currentStep = 0;
    int stepLengthInSamples = 0;
    int samplesIntoStep = 0;
    
    int bpm = 120;
    
    struct ActiveNote {
        MidiEvent event;
        int remainingSamples;
        bool isActive;
        int graphID;
    };
    
    std::vector<ActiveNote> activeNotes;
    int maxPolyphony = 128;
    //activeNotes.reserve(maxPolyphony);
    
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
