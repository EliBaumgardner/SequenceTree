/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "Node/NodeData.h"
#include "Logic/NodeLogic.h"

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
    std::shared_ptr<GraphInfo> loadedInfo = std::atomic_load(&rtGraphInfo);
    std::shared_ptr<std::unordered_map<int,TraversalLogic>> loadedTraversals = std::atomic_load(&traversals);

    RTGraphs* loadedGraphs = std::atomic_load(&rtGraphs).get();
    
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

    for (int sample = 0; sample < numSamples; ++sample){

        for (int i = activeNotes.size() - 1; i >= 0; --i)
        {
            ActiveNote& note = activeNotes[i];

            if (note.traversalId == -1) { continue; }
            if (--note.remainingSamples > 0) { continue; }

                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), sample);
                note.isActive = false;
                int traversalId = note.traversalId;
                auto it = loadedTraversals->find(traversalId);
                if (it == loadedTraversals->end()) { continue;  }

                TraversalLogic& traversal = it->second;

                switch (traversal.state) {
                    case TraversalLogic::TraversalState::Idle: {

                        traversal.targetId = traversal.rootId;
                        traversal.state = TraversalLogic::TraversalState::Active;
                        scheduleNodeHighlight(traversal.getLastNode(),false);
                        scheduleNodeHighlight(traversal.getRootNode(),true);
                        pushNote(traversal.getRootNode(),traversalId,midiMessages);
                        break;
                    }

                    case TraversalLogic::TraversalState::Active: {
                        traversal.advance();

                        for (int traverserId : traversal.traversers) {

                            auto& traverser = (*loadedGlobalNodes)[traverserId];
                            scheduleNodeHighlight(traverser,true);

                            for (auto& connectorId : traverser.connectors) {

                                auto& connector = (*loadedGlobalNodes)[connectorId];


                                if (loadedTraversals->find(connector.graphID) != loadedTraversals->end()) {
                                    TraversalLogic& nextTraversal = loadedTraversals->at(connector.graphID);
                                    scheduleNodeHighlight(nextTraversal.getTargetNode(), false);
                                    nextTraversal.targetId = connectorId;
                                    nextTraversal.lastTargetId = traverserId;
                                    scheduleNodeHighlight(nextTraversal.getTargetNode(), true);
                                    pushNote(nextTraversal.getTargetNode(), nextTraversal.rootId,midiMessages);
                                }
                                else {
                                    loadedTraversals->insert({connector.graphID,TraversalLogic(connector.graphID,this)});
                                    TraversalLogic& nextTraversal = loadedTraversals->at(connector.graphID);
                                    nextTraversal.lastTargetId = traverserId;
                                    pushNote(connector,connector.graphID,midiMessages);
                                }
                            }
                        }

                        switch (traversal.state) {

                            case TraversalLogic::TraversalState::Idle: {

                                scheduleNodeHighlight(traversal.getTargetNode(),false);
                                scheduleNodeHighlight(traversal.getRootNode(),true);
                                pushNote(traversal.getRootNode(),traversalId,midiMessages);

                                traversal.targetId = traversal.rootId;
                                traversal.state = TraversalLogic::TraversalState::Active;
                                break;
                            }
                            case TraversalLogic::TraversalState::Active: {

                                scheduleNodeHighlight(traversal.getLastNode(),false);
                                scheduleNodeHighlight(traversal.getTargetNode(),true);
                                pushNote(traversal.getTargetNode(),traversalId,midiMessages);

                                break;
                            }
                            case TraversalLogic::TraversalState::Complete: {

                                scheduleNodeHighlight(traversal.getTargetNode(),false);
                                break;
                            }
                        }
                        break;
                    }

                    case TraversalLogic::TraversalState::Complete: {
                        std::cout<<"traversal is complete"<<std::endl;
                        scheduleNodeHighlight(traversal.getTargetNode(),false);
                        traversal.targetId = traversal.rootId;
                        traversal.state = TraversalLogic::TraversalState::Idle;
                        break;
                    }

                    case TraversalLogic::TraversalState::Interrupted: {

                    }
                }
                activeNotes.erase(activeNotes.begin() + i);
        }
    }
}

//Pushes notes into a buffer that is read by processBlock, where a midi event is created
void SequenceTreeAudioProcessor::pushNote(RTNode node, int traversalId,juce::MidiBuffer& midiMessages)
{

    if (!node.isNode) {
        for (int i = 0; i < activeNotes.size(); ++i) {
            if (activeNotes[i].traversalId == traversalId) { activeNotes[i].traversalId = -1; }
        }
    }

    int pitch = 63, velocity = 63, duration = 1000;

    if (!node.notes.empty()) {
        const RTNote& note = node.notes[0];
        pitch = note.pitch;
        velocity = note.velocity;
        duration = note.duration;
    }

    ActiveNote newNote;
    newNote.traversalId = traversalId;
    newNote.event.pitch = pitch;
    newNote.event.velocity = velocity;
    newNote.event.duration = duration;
    newNote.remainingSamples = static_cast<int>((duration / 1000.0) * currentSampleRate);
    newNote.isActive = true;
    newNote.event.node = node;

    activeNotes.push_back(newNote);
    midiMessages.addEvent(juce::MidiMessage::noteOn(1, pitch, (juce::uint8)velocity), 0);
}

//Asynchronous node highlighting scheduled for editor
void SequenceTreeAudioProcessor::scheduleNodeHighlight(const RTNode& node, bool shouldHighlight) {

    int nodeID = node.nodeID;
    int graphID = node.graphID;
    pendingAsyncCalls.fetch_add(1, std::memory_order_relaxed);

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

        pendingAsyncCalls.fetch_sub(1, std::memory_order_relaxed);
    });
}

//Adds a new graph to the processor and inserts/deletes nodes based on graph
void SequenceTreeAudioProcessor::setNewGraph(std::shared_ptr<RTGraph> graph) {

    ////////  1. Placeholder structures to modify data and atomic swap  ////////
    std::shared_ptr<std::unordered_map<int,RTNode>> newGlobalNodes;
    std::shared_ptr<RTGraphs> newRTGraphs;
    std::shared_ptr<GraphInfo> newRTGraphInfo;
    std::shared_ptr<std::unordered_map<int,TraversalLogic>> newTraversals;

    if (globalNodes != nullptr) { newGlobalNodes = std::make_shared<std::unordered_map<int,RTNode>>(*globalNodes);}
    else                        { newGlobalNodes = std::make_shared<std::unordered_map<int,RTNode>>(); }

    if (rtGraphs != nullptr)    { newRTGraphs = std::make_shared<RTGraphs>(*rtGraphs); }
    else                        { newRTGraphs = std::make_shared<RTGraphs>(); }

    if (rtGraphInfo != nullptr) { newRTGraphInfo = std::make_shared<GraphInfo>(*rtGraphInfo); }
    else                        {  newRTGraphInfo = std::make_shared<GraphInfo>(); }

    if (traversals != nullptr)  { newTraversals = std::make_shared<std::unordered_map<int,TraversalLogic>>(*traversals);}
    else                        { newTraversals = std::make_shared<std::unordered_map<int,TraversalLogic>>(); }

    (*newRTGraphs)[graph->graphID] = graph;
    auto& graphData = (*newRTGraphInfo)[graph->graphID];
    auto& counts = graphData.counts;

    ////////  2. If the graph is the first one then it should loop and be traversable  ////////

    if (graph->graphID == 1){graphData.isTraversable = true; graphData.isLooping = true; }


    //3. add new data to copies
    for (const auto& [nodeId, node] : graph->nodeMap) {

        if (!counts.count(nodeId)) { counts[nodeId] = 0; }
        for (auto& [id,traverser]: *newTraversals ) { if (!traverser.counts.count(nodeId)) { traverser.counts[nodeId] = 0; } }
        (*newGlobalNodes)[nodeId] = node;
    }


    ////////  4. remove stale data from copies  ////////

    std::vector<int> idsToErase;
    for (const auto& [nodeId, count] : counts) {
        if (!graph->nodeMap.count(nodeId)) { idsToErase.push_back(nodeId); }
    }
    for (int id : idsToErase) {

        counts.erase(id);
        for (auto& [id,traverser] : *newTraversals) { traverser.counts.erase(id); }
    }

    std::vector<int> globalNodesToErase;
    for (const auto& [nodeId,node] : *newGlobalNodes) {
        if (node.graphID == graph->graphID) { if (!graph->nodeMap.count(nodeId)) { globalNodesToErase.push_back(nodeId); } }
    }

    for (int id : globalNodesToErase) { newGlobalNodes->erase(id); }


    ////////  5. atomic swap copy structures  ////////

    std::atomic_store(&rtGraphs, newRTGraphs);
    std::atomic_store(&rtGraphInfo, newRTGraphInfo);
    std::atomic_store(&globalNodes, newGlobalNodes);
    std::atomic_store(&traversals, newTraversals);

}