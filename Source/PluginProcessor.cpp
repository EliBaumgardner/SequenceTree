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
    
    RTGraphs* loadedGraphs = std::atomic_load(&rtGraphs).get();
    
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();
    buffer.clear();

    if(loadedGraphs->size() != numGraphs){

        numGraphs = loadedGraphs->size();
        std::cout<<"num graphs: "<<numGraphs<<std::endl;
        for(auto& [graphID,graph] : *loadedGraphs) {

            if (!graph->isTraversing && graph->isTraversable) { traverse(graphID,true); }
        }
    }


    for (int sample = 0; sample < numSamples; ++sample){

        for (int i = activeNotes.size() - 1; i >= 0; --i)
        {
            ActiveNote& note = activeNotes[i];

            if (--note.remainingSamples <= 0) {

                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), sample);
                note.isActive = false;

                if (note.event.traverserEvent) { traverse(note.graphID,false); }
                else                           { traverse(note.graphID,true); }

                activeNotes.erase(activeNotes.begin() + i);
            }
        }

        int start1, size1, start2, size2;
        fifo.prepareToRead(fifoSize, start1, size1, start2, size2);

        for (int idx = 0; idx < size1; ++idx)
        {
            const MidiEvent& evt = midiBuffer[start1 + idx];

            ActiveNote newNote;
            newNote.event = evt;
            newNote.remainingSamples = static_cast<int>((evt.duration / 1000.0) * currentSampleRate);
            newNote.isActive = true;
            newNote.graphID = evt.graphID;

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
// This creates new instances of the plugin..
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

void SequenceTreeAudioProcessor::traverse(int graphID, bool isNode) {

    std::shared_ptr<RTGraphs> loadedGraphs = std::atomic_load(&rtGraphs);
    std::shared_ptr<GraphInfo> loadedInfo = nullptr;

    if (isNode) { loadedInfo = std::atomic_load(&rtGraphInfo);}
    else        { loadedInfo = std::atomic_load(&traverserInfo); std::cout<<"is traversal"<<std::endl;}
    if (!loadedGraphs || !loadedInfo)
        return;

    auto currentRTGraph = (*loadedGraphs)[graphID];
    auto infoIt = loadedInfo->find(graphID);
    if (infoIt == loadedInfo->end()) { return; }

    TraversalInfo* info = &infoIt->second;
    auto& loadedMap = currentRTGraph->nodeMap;

    if (loadedMap.empty()) { return; }
    if (!currentRTGraph->traversalRequested.load(std::memory_order_acquire)) { return; }
    if ( info->rtTargetId == 0) {
        if (isNode) { info->rtTargetId = graphID; }
        else        { std::cout<<"traversal target is 0"<<std::endl; return; }
    }
    //notify processBlock that graph is traversing
    currentRTGraph->isTraversing = true;

    // Unhighlight previous node
    if (info->rtReferenceId ) {
        auto it = loadedMap.find(info->rtReferenceId);
        if (it != loadedMap.end()) {
            auto rtNode = it->second;
            std::shared_ptr<RTNode> highlightNode = std::make_shared<RTNode>(rtNode);
            scheduleNodeHighlight(highlightNode, false);
        }
    }

    // Highlight current node
    auto itTarget = loadedMap.find(info->rtTargetId);
    if (itTarget != loadedMap.end()) {
        auto rtNode = itTarget->second;
        std::shared_ptr<RTNode> highlightNode = std::make_shared<RTNode>(rtNode);
        scheduleNodeHighlight(highlightNode, true);

        // Push first note if any
        if (!itTarget->second.notes.empty()) {
            const auto& note = rtNode.notes[0];
            pushNote(note.pitch, note.velocity, note.duration,rtNode);
        } else {
            pushNote(64, 64, 1000,rtNode);
        }

        // Increment count
        int count = ++info->counts[info->rtTargetId];

        info->rtReferenceId = info->rtTargetId;

        // Traverse children
        int maxLimit = 0;
        int traverserLimit = 0;
        for (int childIndex : itTarget->second.children) {
            auto itChild = loadedMap.find(childIndex);
            auto childNode = itChild->second;
            if (itChild != loadedMap.end()) {
                int limit = childNode.countLimit;
                if (isNode) {
                    if (count % limit == 0 && limit > maxLimit && childNode.isNode)        { info->rtTargetId = childIndex; maxLimit = limit; }
                    if (count % limit == 0 && limit > traverserLimit && !childNode.isNode) { handleTraverser(childNode); std::cout<<"handling traversal"<<std::endl; }
                }
                 else {
                     std::cout<<"traverser found child"<<std::endl;
                     if (childNode.isNode) { info->rtTargetId = childIndex; std::cout<<"traverser found connector"<<std::endl; }
                     else if (count % limit == 0 && limit > traverserLimit) {info->rtTargetId = childIndex; traverserLimit = limit;} }
              }
           }

         } else {
             if (isNode) { info->rtTargetId = 0; traverse(graphID,isNode); return; }
             std::cout<<"traverser found no child"<<std::endl;
             return;
         }

    if (info->rtTargetId == info->rtReferenceId ) { info->rtTargetId = 0; }
}

void SequenceTreeAudioProcessor::handleTraverser(RTNode node) {
    std::shared_ptr<RTNode> traverser = std::make_shared<RTNode>(node);

    scheduleNodeHighlight(traverser, true);

    if (node.notes.empty()) {
        pushNote(0, 0, 1000, node);
    } else {
        const auto& note = node.notes[0];
        pushNote(0, 0, note.duration, node);
    }
}

void SequenceTreeAudioProcessor::pushNote(int pitch, int velocity, int duration,RTNode node){

    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);

    int graphID = node.graphID;
    if (size1 > 0)
        midiBuffer[start1] = { pitch, velocity, duration,graphID,!node.isNode};


    fifo.finishedWrite(size1);
}

void SequenceTreeAudioProcessor::scheduleNodeHighlight(std::shared_ptr<RTNode> node, bool shouldHighlight) {

    if (node == nullptr) {
        return;
    }

    int nodeID = node.get()->nodeID;
    int graphID = node.get()->graphID;
    pendingAsyncCalls.fetch_add(1, std::memory_order_relaxed);

    juce::MessageManager::callAsync([this, nodeID, shouldHighlight, graphID]() {

        if(canvas->nodeMaps.find(graphID) == canvas->nodeMaps.end()){
            return;
        }

        auto& nodeMap = canvas->nodeMaps[graphID];

        Node* foundNode = nullptr;

        auto it = nodeMap.find(nodeID);
        if (it != nodeMap.end()) {
            foundNode = it->second;
            foundNode->setHighlightVisual(shouldHighlight);
        }

        pendingAsyncCalls.fetch_sub(1, std::memory_order_relaxed);
    });
}

void SequenceTreeAudioProcessor::setNewGraph(std::shared_ptr<RTGraph> graph) {
    std::shared_ptr<GraphInfo> newRTGraphInfo;
    std::shared_ptr<GraphInfo> newTraverserInfo;
    std::shared_ptr<RTGraphs> newRTGraphs;

    if(rtGraphs != nullptr)
        newRTGraphs = std::make_shared<RTGraphs>(*rtGraphs);
    else
        newRTGraphs = std::make_shared<RTGraphs>();

    (*newRTGraphs)[graph->graphID] = graph;

    if(rtGraphInfo != nullptr) {
        newRTGraphInfo = std::make_shared<GraphInfo>(*rtGraphInfo);
        newTraverserInfo = std::make_shared<GraphInfo>(*traverserInfo);
    }
    else {
        newRTGraphInfo = std::make_shared<GraphInfo>();
        newTraverserInfo = std::make_shared<GraphInfo>();
    }

    // Safe node insertion
    auto& graphData = (*newRTGraphInfo)[graph->graphID];// creates in-place safely
    auto& traverserData = (*newTraverserInfo)[graph->graphID];
    auto& counts = graphData.counts;
    auto& traverserCounts =traverserData.counts;


    for (const auto& [nodeId, node] : graph->nodeMap) {
        if (node.isNode) {
            if (!counts.count(nodeId)) {
                counts[nodeId] = 0;
            }
        }
        else {
            if (!traverserCounts.count(nodeId)) {
                traverserCounts[nodeId] = 0;
            }
        }
    }

    // Remove stale nodes
    std::vector<int> idsToErase;
    for (const auto& [nodeId, count] : counts) {
        if (!graph->nodeMap.count(nodeId)) {
            idsToErase.push_back(nodeId);
            std::cout<<"id erased"<<std::endl;
        }
    }
    for (int id : idsToErase) {
        counts.erase(id);
    }

    std::vector<int> traverserIdsToErase;
    for (const auto& [nodeId, count] : traverserCounts) {
        if (!graph->nodeMap.count(nodeId)) {
            idsToErase.push_back(nodeId);
            std::cout<<"id erased"<<std::endl;
        }
    }
    for (int id : traverserIdsToErase) {
        traverserCounts.erase(id);
    }


    std::atomic_store(&rtGraphs, newRTGraphs);
    std::atomic_store(&rtGraphInfo, newRTGraphInfo);
    std::atomic_store(&traverserInfo, newTraverserInfo);
}
