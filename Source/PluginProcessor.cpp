/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Node/Node.h"
#include "Node/NodeData.h"

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
                       )
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
        
    TempoInfo.currentSampleRate = sampleRate;

    double beatsPerSecond = TempoInfo.bpm / 60.0;
    TempoInfo.samplesPerBeat = static_cast<int> (sampleRate / beatsPerSecond);

    int stepsPerBeat = 4; // 16th notes
    TempoInfo.stepLengthInSamples = TempoInfo.samplesPerBeat / stepsPerBeat;

    TempoInfo.currentStep = 0;
    TempoInfo.samplesIntoStep = 0;
    
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
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{

    return new SequenceTreeAudioProcessor();
}


void SequenceTreeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if(isPlaying.load() == false){ return; }

    std::shared_ptr<std::unordered_map<int,RTNode>>loadedGlobalNodes = std::atomic_load(&globalNodes);
    std::shared_ptr<std::unordered_map<int,TraversalLogic>> loadedTraversals = std::atomic_load(&eventManager.traversals);

    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();
    buffer.clear();

    if (loadedTraversals->empty()) {

        loadedTraversals->insert({1,TraversalLogic(1,this)});
        TraversalLogic& traversal = loadedTraversals->at(1);
        traversal.targetId = traversal.rootId;
        traversal.state = TraversalLogic::TraversalState::Active;
        traversal.isLooping = true;

        RTNode& rootNode = (*loadedGlobalNodes)[traversal.rootId];
        scheduleNodeHighlight(rootNode,true);

        int traversalId = rootNode.nodeID;
        pushNote(rootNode,traversalId,midiMessages);
     }

    for (int sample = 0; sample < numSamples; ++sample) {
        eventManager.handleEventStream(sample,midiMessages);
    }
}


//Pushes notes into a buffer that is read by processBlock, where a midi event is created
void SequenceTreeAudioProcessor::pushNote(RTNode node, int traversalId,juce::MidiBuffer& midiMessages)
{

    int pitch = 63, velocity = 63, duration = 1000;

    if (!node.notes.empty()) {
        const RTNote& note = node.notes[0];
        pitch = note.pitch;
        velocity = note.velocity;
        duration = note.duration;
    }

    EventManager::ActiveNote newNote;
    newNote.traversalId = traversalId;
    newNote.event.pitch = pitch;
    newNote.event.velocity = velocity;
    newNote.event.duration = duration;
    newNote.remainingSamples = static_cast<int>((duration / 1000.0) * TempoInfo.currentSampleRate);
    newNote.noteNode = node;

    eventManager.activeNotes.push_back(newNote);
    midiMessages.addEvent(juce::MidiMessage::noteOn(1, pitch, (juce::uint8)velocity), 0);
}

void SequenceTreeAudioProcessor::clearOldEvents(RTNode node, int traversalId) {

    for (int i = eventManager.activeNotes.size()-1; i >= 0; --i) {
        EventManager::ActiveNote& note = eventManager.activeNotes[i];
        if (note.traversalId == traversalId ) { note.traversalId = -1; std::cout<<"disabling old event"<<std::endl;}
    }
}

//Asynchronous node highlighting scheduled for editor
void SequenceTreeAudioProcessor::scheduleNodeHighlight(const RTNode& node, bool shouldHighlight) {

    int nodeID = node.nodeID;
    int graphID = node.graphID;
    //pendingAsyncCalls.fetch_add(1, std::memory_order_relaxed);

    juce::MessageManager::callAsync([this, nodeID, shouldHighlight, graphID]() {

        if(canvas->nodeMaps.find(graphID) == canvas->nodeMaps.end()){
            return;
        }

        auto& nodeMap = canvas->nodeMaps[graphID];

        auto it = nodeMap.find(nodeID);
        if (it != nodeMap.end()) {
            Node* foundNode = nullptr;
            foundNode = it->second;
            foundNode->setHighlightVisual(shouldHighlight);
        }

        //pendingAsyncCalls.fetch_sub(1, std::memory_order_relaxed);
    });
}

//Adds a new graph to the processor and inserts/deletes nodes based on graph
void SequenceTreeAudioProcessor::setNewGraph(std::shared_ptr<RTGraph> graph) {

    ////////  1. Placeholder structures to modify data and atomic swap  ////////
    std::shared_ptr<std::unordered_map<int,RTNode>> newGlobalNodes;
    std::shared_ptr<RTGraphs> newRTGraphs;
    std::shared_ptr<std::unordered_map<int,TraversalLogic>> newTraversals;

    if (globalNodes != nullptr) { newGlobalNodes = std::make_shared<std::unordered_map<int,RTNode>>(*globalNodes);}
    else                        { newGlobalNodes = std::make_shared<std::unordered_map<int,RTNode>>(); }

    if (rtGraphs != nullptr)    { newRTGraphs = std::make_shared<RTGraphs>(*rtGraphs); }
    else                        { newRTGraphs = std::make_shared<RTGraphs>(); }

    if (eventManager.traversals != nullptr)  { newTraversals = std::make_shared<std::unordered_map<int,TraversalLogic>>(*eventManager.traversals);}
    else                        { newTraversals = std::make_shared<std::unordered_map<int,TraversalLogic>>(); }

    (*newRTGraphs)[graph->graphID] = graph;

    for (const auto& [nodeId, node] : graph->nodeMap) {
        for (auto& [id,traverser]: *newTraversals ) { if (!traverser.counts.count(nodeId)) { traverser.counts[nodeId] = 0; } }
        (*newGlobalNodes)[nodeId] = node;
    }

    std::vector<int> staleIds;
    for (const auto& [nodeId,node] : *newGlobalNodes) {
        if (node.graphID == graph->graphID) { if (!graph->nodeMap.count(nodeId)) { staleIds.push_back(nodeId); } }
    }

    for (int id : staleIds) {
        newGlobalNodes->erase(id);
        for (auto& [id,traverser] : *newTraversals) { traverser.counts.erase(id); }
    }

    std::atomic_store(&rtGraphs, newRTGraphs);
    std::atomic_store(&globalNodes, newGlobalNodes);
    std::atomic_store(&eventManager.traversals, newTraversals);

}