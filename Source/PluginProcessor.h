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
    int traversalId = 0;
    int traverserEvent = false;
    RTNode node;
};

struct ActiveNote {
    MidiEvent event;
    int traversalId = 0;
    int remainingSamples;
    bool isActive;
};

struct NodeCount{

    int nodeID = 0;
    int count = 0;
};

struct TraversalInfo {
    int rtTargetId = 0;
    int rtRootId = 1;
    int rtReferenceId = 0;

    bool isStart = true;
    bool isTraversing = false;
    bool isTraversable = false;
    bool isLooping = false;
    bool isInterrupted = false;

    std::unordered_map<int,int> counts;
};

struct TraversalEvent {
    enum class Type { PushNote, Highlight, Unhighlight, None };

    Type type = Type::None;
    RTNode node;
    int pitch = 0;
    int velocity = 0;
    int duration = 0;
    bool highlight = false;
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
    
    void pushNote(RTNode node, int traversalId,juce::MidiBuffer& midiMessages);

    void setNewGraph(std::shared_ptr<RTGraph> graph);
    void scheduleNodeHighlight(const RTNode& node,bool shouldHighlight);

    NodeCanvas* canvas;
    
    using RTGraphs = std::unordered_map<int,std::shared_ptr<RTGraph>>;
    std::shared_ptr<RTGraphs> rtGraphs = nullptr;

    int numGraphs = 0;

    using GraphInfo = std::unordered_map<int,TraversalInfo>;
    std::shared_ptr<GraphInfo> rtGraphInfo = nullptr;

    std::shared_ptr<std::unordered_map<int,RTNode>> globalNodes = nullptr;

    std::shared_ptr<std::unordered_map<int,int>> globalCounts = nullptr;

    std::atomic<bool> isPlaying = false;
    bool isStart = true;
    
    private:
    //==============================================================================
    
    Node* root = nullptr;
    
    double currentSampleRate = 44100.0;
    int samplesPerBeat = 0;
    int currentStep = 0;
    int stepLengthInSamples = 0;
    int samplesIntoStep = 0;
    int bpm = 120;

    std::vector<ActiveNote> activeNotes;
    
    static constexpr int fifoSize = 1024;
    juce::AbstractFifo fifo;
    std::vector<MidiEvent> midiBuffer;



    class TraversalLogic {
    public:

        std::unordered_map<int,int> counts;
        std::vector<int> traversers;

        bool isStart = true;
        bool isTraversing = false;
        bool isTraversable = false;
        bool isLooping = false;
        bool isInterrupted = false;

        bool isOldTraversal = false;

        int rootId = 0;
        int targetId = 0;
        int lastTargetId = 0;
        int referenceTargetId = 0;

        enum class TraversalState{ Idle, Complete, Interrupted, Active};
        TraversalState state = TraversalState::Idle;

        SequenceTreeAudioProcessor* audioProcessor;

        TraversalLogic(int root,SequenceTreeAudioProcessor* processor ) : rootId(root), audioProcessor(processor) {}

        void advance()
        {
            traversers.clear();
            std::shared_ptr<std::unordered_map<int,RTNode>> loadedGlobalNodes = std::atomic_load(&audioProcessor->globalNodes);

            referenceTargetId = lastTargetId;
            lastTargetId = targetId;
            int count = ++counts[targetId];
            auto itTarget = loadedGlobalNodes->find(targetId);

            if (!itTarget->second.children.empty()){

                int maxLimit = 0;

                for (int childIndex : itTarget->second.children) {
                    auto itChild = loadedGlobalNodes->find(childIndex);
                    if (itChild != loadedGlobalNodes->end()) {

                        auto childNode = itChild->second;

                        int limit = childNode.countLimit;

                        if (count % limit == 0 && limit > maxLimit && childNode.isNode)        { targetId = childIndex; maxLimit = limit; }
                        if (count % limit == 0 && !childNode.isNode) { traversers.push_back(childIndex); }
                    }
                }
            }
            else if (isLooping) { state = TraversalState::Idle;     return; }
            else                { state = TraversalState::Complete; return;}

            if (targetId == lastTargetId) {
                if (isLooping) { state = TraversalState::Idle; }
                else { state = TraversalState::Complete; }
            }
        }

        const RTNode& getTargetNode() { return (*std::atomic_load(&audioProcessor->globalNodes))[targetId]; }

        const RTNode& getLastNode() { return (*std::atomic_load(&audioProcessor->globalNodes))[lastTargetId]; }

        const RTNode& getReferenceNode() { return (*std::atomic_load(&audioProcessor->globalNodes))[referenceTargetId]; }

        const RTNode& getRootNode() { return (*std::atomic_load(&audioProcessor->globalNodes))[rootId]; }

        bool shouldTraverse() {
            switch (state) {
                case TraversalState::Idle:     { return true; }
                case TraversalState::Active:   { return true; }
                default:                       { return false;}
            }
        }

        void configureTraversal() {
            case TraversalState::Idle : {}
        }
    };

    std::shared_ptr<std::unordered_map<int,TraversalLogic>> traversals;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessor)
};
