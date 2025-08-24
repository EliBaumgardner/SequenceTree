/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "NodeCanvas.h"
#include "Node.h"
#include "NodeData.h"
#include "NodeLogic.h"

static std::atomic<int> pendingAsyncCalls{0};
//==============================================================================
SequenceTreeAudioProcessor::SequenceTreeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), fifo(fifoSize),midiBuffer(fifoSize)
#endif
{
}

SequenceTreeAudioProcessor::~SequenceTreeAudioProcessor()
{
    
}

//==============================================================================
const juce::String SequenceTreeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SequenceTreeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SequenceTreeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SequenceTreeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SequenceTreeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SequenceTreeAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SequenceTreeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SequenceTreeAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SequenceTreeAudioProcessor::getProgramName (int index)
{
    return {};
}

void SequenceTreeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SequenceTreeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
        
    currentSampleRate = sampleRate;

    double beatsPerSecond = bpm / 60.0;
    samplesPerBeat = static_cast<int> (sampleRate / beatsPerSecond);

    int stepsPerBeat = 4; // 16th notes
    stepLengthInSamples = samplesPerBeat / stepsPerBeat;

    currentStep = 0;
    samplesIntoStep = 0;
    
}

void SequenceTreeAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SequenceTreeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SequenceTreeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();
    buffer.clear();

    static bool noteActive = false;
    static MidiEvent currentEvent = {};
    static int remainingSamples = 0;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // If no note is active, try to grab one from the FIFO
        if (!noteActive)
        {
            //scheduleTraversal();
            traverse();
            
            int start1, size1, start2, size2;
            fifo.prepareToRead(1, start1, size1, start2, size2);

            if (size1 > 0)
            {
                
                currentEvent = midiBuffer[start1];
                remainingSamples = static_cast<int>((currentEvent.duration / 1000.0) * currentSampleRate);
                fifo.finishedRead(size1);
                
                // Send noteOn
                midiMessages.addEvent(juce::MidiMessage::noteOn(1, currentEvent.pitch, (juce::uint8)currentEvent.velocity), sample);
                noteActive = true;
            }
        }
        else
        {
            // Countdown and send noteOff when time is up
            if (--remainingSamples <= 0)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentEvent.pitch), sample);
                noteActive = false;
            }
        }
    }
}


//==============================================================================
bool SequenceTreeAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SequenceTreeAudioProcessor::createEditor()
{
    return new SequenceTreeAudioProcessorEditor (*this);
}

//==============================================================================
void SequenceTreeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SequenceTreeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
                
    return new SequenceTreeAudioProcessor();
}

void SequenceTreeAudioProcessor::pushNote(int pitch, int velocity, int duration){
    
    std::cout << "Pushing note: " << pitch << ", " << velocity << ", " << duration << std::endl;

    int start1, size1, start2, size2;
        fifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0)
            midiBuffer[start1] = { pitch, velocity, duration };

        fifo.finishedWrite(size1);
}

void SequenceTreeAudioProcessor::scheduleTraversal(){
    
    pendingAsyncCalls.fetch_add(1, std::memory_order_relaxed);
    juce::MessageManager::callAsync([this]() {
        pendingAsyncCalls.fetch_sub(1, std::memory_order_relaxed);
        if (canvas && canvas->root)
        {
            this->root = canvas->root;
            NodeLogic::start = true;
            canvas->root->nodeLogic.traverse();
        }
    });
}

void SequenceTreeAudioProcessor::traverse(){
    
    std::shared_ptr<RTGraph> loadedGraph = std::atomic_load(&rtGraph);
    
    if(loadedGraph == nullptr)
        return;
    
    std::unordered_map<int, RTNode>& loadedMap = loadedGraph->nodeMap;
    
    if(loadedGraph->traversalRequested.load(std::memory_order_acquire) != true) { return; }
    
    if(rtTargetId == 0){ rtTargetId = rtRootId; }
    
    if(rtReferenceId){
        scheduleNodeHighlight(&loadedMap[rtReferenceId], false);
    }
    
    scheduleNodeHighlight(&loadedMap[rtTargetId], true);
    
    if(!loadedMap[rtTargetId].notes.empty()){
        auto note = loadedMap[rtTargetId].notes[0];
        pushNote(note.pitch,note.velocity,note.duration);
    }
    else {
        pushNote(64,64,1000);
    }
    
    int count = ++(*nodeCounts)[rtTargetId];
    std::cout<<count<<std::endl;
    
    rtReferenceId = rtTargetId;
    
    int maxLimit = 0;
    for(auto childIndex : loadedMap[rtTargetId].children){
        int limit = loadedMap[childIndex].countLimit;
        if(count % limit == 0 && limit > maxLimit){
            maxLimit = limit;
            rtTargetId = childIndex;
        }
    }
    
    if(rtTargetId == rtReferenceId){
        rtTargetId = rtRootId;
    }
}

void SequenceTreeAudioProcessor::scheduleNodeHighlight(RTNode* node, bool shouldHighlight) {
    if (node == nullptr) {
        return;
    }

    int nodeID = node->nodeID;
    pendingAsyncCalls.fetch_add(1, std::memory_order_relaxed);

    juce::MessageManager::callAsync([this, nodeID, shouldHighlight]() {
        auto& nodeMap = canvas->nodeMap;
        Node* foundNode = nullptr;
        
        for (const auto& pair : nodeMap) {
            if (pair.second == nodeID) {
                foundNode = pair.first;
                break;
            }
        }
            foundNode->setHighlightVisual(shouldHighlight);
        
    });
}

void SequenceTreeAudioProcessor::setNewGraph(std::shared_ptr<RTGraph> graph) {
    std::shared_ptr<std::unordered_map<int,int>> newCounts;

    if (nodeCounts != nullptr) {
        newCounts = std::make_shared<std::unordered_map<int,int>>(*nodeCounts);
    } else {
        newCounts = std::make_shared<std::unordered_map<int,int>>();
    }

    std::atomic_store(&rtGraph, graph);

    for (const auto& [nodeId, node] : rtGraph->nodeMap) {
        if (!newCounts->count(nodeId)) {
            (*newCounts)[nodeId] = 0;
        }
    }

    // safe erase pass
    std::vector<int> idsToErase;
    for (const auto& [nodeId, _] : *newCounts) {
        if (!rtGraph->nodeMap.count(nodeId)) {
            idsToErase.push_back(nodeId);
        }
    }
    for (int id : idsToErase) {
        newCounts->erase(id);
    }

    std::atomic_store(&nodeCounts, newCounts);
}

