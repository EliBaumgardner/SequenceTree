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

    // if(loadedGraphs->size() != numGraphs){
    //
    //     numGraphs = loadedGraphs->size();
    //     std::cout<<"num graphs: "<<numGraphs<<std::endl;
    //     for(auto& [graphID,info] : *loadedInfo) {
    //
    //         if (!info.isTraversing && info.isTraversable && info.isStart) { traverse(graphID); std::cout<<"graph: "<<graphID<<" traversal made"<<std::endl;}
    //     }
    // }

    if (loadedTraversals->empty()) {

        loadedTraversals->insert({1,TraversalLogic(1,*this)});
        TraversalLogic& traversal = loadedTraversals->at(1);
        traversal.targetId = traversal.rootId;
        traversal.state = TraversalLogic::TraversalState::Active;
        traversal.isLooping = true;

        RTNode& rootNode = (*loadedGlobalNodes)[traversal.rootId];
        scheduleNodeHighlight(rootNode,true);

        int traversalId = rootNode.nodeID;
        pushNote(rootNode,traversalId);
     }


    for (int sample = 0; sample < numSamples; ++sample){

        for (int i = activeNotes.size() - 1; i >= 0; --i)
        {
            ActiveNote& note = activeNotes[i];

            if (--note.remainingSamples <= 0) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), sample);
                note.isActive = false;
                int traversalId = note.traversalId;
                auto it = loadedTraversals->find(traversalId);
                if (it == loadedTraversals->end()) { continue;  }

                TraversalLogic& traversal = it->second;

                switch (traversal.state) {
                    case TraversalLogic::TraversalState::Idle: {

                        std::cout<<"traversal is idle"<<std::endl;
                        traversal.targetId = traversal.rootId;
                        traversal.state = TraversalLogic::TraversalState::Active;
                        traversal.advance();

                        scheduleNodeHighlight(traversal.getRootNode(),true);
                        pushNote(traversal.getRootNode(),traversalId);
                        break;
                    }

                    case TraversalLogic::TraversalState::Active: {
                        std::cout<<"traversal is active"<<std::endl;
                        traversal.advance();

                        switch (traversal.state) {
                            case TraversalLogic::TraversalState::Idle: {
                                std::cout<<"returning to root"<<std::endl;
                                scheduleNodeHighlight(traversal.getTargetNode(),false);
                                scheduleNodeHighlight(traversal.getRootNode(),true);
                                pushNote(traversal.getRootNode(),traversalId);

                                traversal.targetId = traversal.rootId;
                                traversal.state = TraversalLogic::TraversalState::Active;
                                break;
                            }
                            case TraversalLogic::TraversalState::Active: {
                                std::cout<<"going to child"<<std::endl;

                                scheduleNodeHighlight(traversal.getLastNode(),false);
                                scheduleNodeHighlight(traversal.getTargetNode(),true);
                                pushNote(traversal.getTargetNode(),traversalId);

                                for (int traverserId : traversal.traversers) {
                                    auto& traverser = (*loadedGlobalNodes)[traverserId];
                                    scheduleNodeHighlight(traverser,true);

                                    for (auto& connectorId : traverser.connectors) {
                                        auto& connector = (*loadedGlobalNodes)[connectorId];
                                        pushNote(connector,connector.graphID);
                                    }
                                }
                                break;
                            }
                            case TraversalLogic::TraversalState::Complete: {
                                std::cout<<"finished traversal"<<std::endl;
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

                }

                // auto& noteInfo = (*loadedInfo)[note.event.node.graphID];
                // if (!note.event.traverserEvent) { traverse(note.event.node.graphID); }
                // else {
                //     scheduleNodeHighlight(note.event.node,false);
                //      for (int childIndex : note.event.node.connectors) {
                //
                //          auto child = (*loadedGlobalNodes)[childIndex];
                //
                //          auto& childInfo = (*loadedInfo)[childIndex];
                //          childInfo.isTraversable = true;
                //          childInfo.isStart = true;
                //          if (!childInfo.isTraversing)traverse(child.graphID);
                //      }
                // }

                activeNotes.erase(activeNotes.begin() + i);
            }
        }

        int start1, size1, start2, size2;
        fifo.prepareToRead(fifoSize, start1, size1, start2, size2);

        for (int idx = 0; idx < size1; ++idx)
        {
            const MidiEvent& evt = midiBuffer[start1 + idx];

            ActiveNote newNote;
            newNote.traversalId = evt.traversalId;
            newNote.event = evt;
            newNote.remainingSamples = static_cast<int>((evt.duration / 1000.0) * currentSampleRate);
            newNote.isActive = true;

            activeNotes.push_back(newNote);

            midiMessages.addEvent(juce::MidiMessage::noteOn(1, evt.pitch, (juce::uint8)evt.velocity), sample);
        }

        fifo.finishedRead(size1);
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
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{

    return new SequenceTreeAudioProcessor();
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

void SequenceTreeAudioProcessor::traverse(int nodeID) {

    // //1. Load Graphs,Info and Maps Needed for traversal
    // std::shared_ptr<RTGraphs> loadedGraphs = std::atomic_load(&rtGraphs);
    // std::shared_ptr<GraphInfo> loadedInfo = std::atomic_load(&rtGraphInfo);
    // std::shared_ptr<std::unordered_map<int, RTNode>> rtGlobalNodes = std::atomic_load(&globalNodes);
    //
    // if (!loadedGraphs || !loadedInfo || !rtGlobalNodes) { return; }
    //
    // int graphID = (*rtGlobalNodes)[nodeID].graphID;
    // auto infoIt = loadedInfo->find(graphID);
    // if (infoIt == loadedInfo->end()) { return; }
    //
    // TraversalInfo* info = &infoIt->second;
    //
    // info->isTraversing = true;
    //
    // if (loadedGraphs->find(graphID) == loadedGraphs->end()) {  std::cout<<"no graph found"<<std::endl; return;}
    //
    // auto currentRTGraph = (*loadedGraphs)[graphID];
    // if(!info->isTraversable){ return;}
    // if (!currentRTGraph->traversalRequested.load(std::memory_order_acquire)) { return; }
    //
    //
    //
    // //3. set traversal to root if looping or is traversal start, otherwise return and unhighlight node
    //
    // if ( info->rtTargetId == 0) {
    //     if (info->isLooping) { info->rtTargetId = graphID; }
    //     else if (info->isStart) {info->rtTargetId = graphID;}
    //     info->isStart = false;
    // }
    //
    // //2. unhighlight previous node
    // if (info->rtReferenceId) {
    //     auto it = rtGlobalNodes->find(info->rtReferenceId);
    //     if (it != rtGlobalNodes->end()) {
    //         auto rtNode = it->second;
    //         scheduleNodeHighlight(rtNode, false);
    //     }
    //     if (info->rtTargetId == 0) { info->isTraversing = false; return; }
    // }
    //
    // //if (info->isInterrupted) { info->isInterrupted = false; return; }
    //
    // if (nodeID != graphID) { info->rtTargetId = nodeID;}
    //
    // //4. Highlight current node
    //
    // auto itTarget = rtGlobalNodes->find(info->rtTargetId);
    //
    // if (itTarget != rtGlobalNodes->end()) {
    //
    //     auto rtNode = itTarget->second;
    //     scheduleNodeHighlight(rtNode, true);
    //
    //
    // //5. Push the notes of the current node
    //
    //         pushNote(rtNode);
    //
    //
    // //6. Increment count of current node
    //
    //     int count = ++info->counts[info->rtTargetId];
    //
    //
    // //7. Set the last traversed node to the current node
    //
    //     info->rtReferenceId = info->rtTargetId;
    //
    //
    // //8. Traverse children of current node, and set the current node to traversable child
    //
    //     if (!itTarget->second.children.empty()) {
    //
    //         int maxLimit = 0;
    //         int traverserLimit = 0;
    //
    //         for (int childIndex : itTarget->second.children) {
    //             auto itChild = rtGlobalNodes->find(childIndex);
    //             if (itChild != rtGlobalNodes->end()) {
    //
    //                 auto childNode = itChild->second;
    //
    //                 int limit = childNode.countLimit;
    //
    //                 if (count % limit == 0 && limit > maxLimit && childNode.isNode)        { info->rtTargetId = childIndex; maxLimit = limit; }
    //                 if (count % limit == 0 && limit > traverserLimit && !childNode.isNode) { handleTraverser(childNode); }
    //             }
    //         }
    //     }
    //
    // }
    //
    //
    // //9. If the Node target is invalid then restart the traversal
    //
    // else { info->rtTargetId = 0; traverse(graphID); return; }
    //
    //
    // //10. If the current node is the same as the last node, restart traversal
    //
    // if (info->rtTargetId == info->rtReferenceId ) {
    //     if (info->isLooping) { info->rtTargetId = graphID; }
    //     else info->rtTargetId = 0;
    // }
}


//Handle traverser objects by pushing notes off to processBlock, and using traverse function on children
void SequenceTreeAudioProcessor::handleTraverser(const RTNode& node) {

    // if (!node.connectors.empty()) {
    //     scheduleNodeHighlight(node, true);
    //     if (node.notes.empty()) { pushNote(node); }
    //     else { const auto& note = node.notes[0]; pushNote(node); }
    // }
    // else { scheduleNodeHighlight(node, false); }
}

//Pushes notes into a buffer that is read by processBlock, where a midi event is created
void SequenceTreeAudioProcessor::pushNote(RTNode node, int traversalId){

    int pitch = 63, velocity = 63, duration = 1000;

    if (!node.notes.empty()) {
        RTNote& note = node.notes[0];
        pitch = note.pitch;
        velocity = note.velocity;
        duration = note.duration;
    }

    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0) {
        MidiEvent& event = midiBuffer[start1];
        event.pitch = pitch;
        event.velocity = velocity;
        event.duration = duration;
        event.traversalId = traversalId;
        event.node = node;
    }
    fifo.finishedWrite(size1);
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

    //1. Placeholder structures to modify data and atomic swap
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

    //2. If the graph is the first one then it should loop and be traversable

    if (graph->graphID == 1){graphData.isTraversable = true; graphData.isLooping = true; }


    //3. add new data to copies
    for (const auto& [nodeId, node] : graph->nodeMap) {

        if (!counts.count(nodeId)) { counts[nodeId] = 0; }
        for (auto& [id,traverser]: *newTraversals ) { if (!traverser.counts.count(nodeId)) { traverser.counts[nodeId] = 0; } }
        (*newGlobalNodes)[nodeId] = node;
    }


    //4. remove stale data from copies

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


    //5. atomic swap copy structures

    std::atomic_store(&rtGraphs, newRTGraphs);
    std::atomic_store(&rtGraphInfo, newRTGraphInfo);
    std::atomic_store(&globalNodes, newGlobalNodes);
    std::atomic_store(&traversals, newTraversals);

}
