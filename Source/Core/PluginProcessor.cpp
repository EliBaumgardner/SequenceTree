/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Node/Node.h"
#include "../Util/ValueTreeIdentifiers.h"



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
, valueTreeState(*this,nullptr,"STATE",createParameterLayout())
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
void SequenceTreeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) { }

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
    juce::ValueTree canvasTree = ValueTreeState::nodeMap;

    std::unique_ptr<juce::XmlElement> xml(canvasTree.createXml());

    copyXmlToBinary(*xml, destData);
}

void SequenceTreeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState == nullptr)     { DBG("INVALID STATE DATA"); return; }

    juce::ValueTree restoredTree = juce::ValueTree::fromXml (*xmlState);

    if (!restoredTree.isValid()) { DBG("INVALID STATE TREE"); return; }

    juce::MessageManager::callAsync([this, restoredTree]() {
        // Suppress the ValueTree listener while rebuilding to avoid double-adding nodes
        ValueTreeState::nodeMap.removeListener(canvas);
        ValueTreeState::nodeMap.removeAllChildren(nullptr);

        for (int i = 0; i < restoredTree.getNumChildren(); ++i)
            ValueTreeState::nodeMap.addChild(restoredTree.getChild(i).createCopy(), -1, nullptr);

        // Advance nodeIdIncrement past all restored IDs to avoid future collisions
        int maxId = 0;
        for (int i = 0; i < ValueTreeState::nodeMap.getNumChildren(); ++i) {
            int id = ValueTreeState::nodeMap.getChild(i).getProperty(ValueTreeIdentifiers::Id);
            if (id > maxId) maxId = id;
        }
        ValueTreeState::nodeIdIncrement = maxId;

        canvas->setValueTreeState(ValueTreeState::nodeMap);
        ValueTreeState::nodeMap.addListener(canvas);
    });
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SequenceTreeAudioProcessor();
}


void SequenceTreeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (resetRequested.exchange(false))
    {
        for (auto& note : eventManager.activeNotes)
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), 0);

        eventManager.activeNotes.clear();

        auto emptyTraversals = std::make_shared<std::unordered_map<int, TraversalLogic>>();
        std::atomic_store(&eventManager.traversals, emptyTraversals);

        juce::MessageManager::callAsync([this]() {
            for (auto& [nodeId, node] : canvas->nodeMap)
                node->setHighlightVisual(false);
        });
    }

    if(isPlaying.load() == false){ return; }

    auto loadedGlobalNodes = std::atomic_load(&globalNodes);
    auto loadedTraversals  = std::atomic_load(&eventManager.traversals);
    NodeMap& nodes         = *loadedGlobalNodes;
    auto& traversals       = *loadedTraversals;

    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();

    buffer.clear();
    midiMessages.clear();

    if (traversals.empty()) {
        traversals.insert({1,TraversalLogic(1,this)});
        TraversalLogic& traversal = traversals.at(1);
        traversal.isFirstEvent = true;
        traversal.targetId = traversal.rootId;
        traversal.state = TraversalLogic::TraversalState::Active;
        traversal.isLooping = true;

        RTNode& rootNode = nodes[traversal.rootId];
        eventManager.highlightNode(rootNode,true);

        int traversalId = rootNode.nodeID;
        eventManager.pushNote(rootNode,traversalId,midiMessages,0,nodes,traversals);
    }

    for (int sample = 0; sample < numSamples; ++sample) { eventManager.handleEventStream(sample, midiMessages, nodes, traversals); }

}

void SequenceTreeAudioProcessor::clearOldEvents(RTNode node, int traversalId)
{
    for (int i = eventManager.activeNotes.size()-1; i >= 0; --i) {
        EventManager::ActiveNote& note = eventManager.activeNotes[i];
        if (note.traversalId == traversalId ) { note.traversalId = -1; }
    }
}

void SequenceTreeAudioProcessor::setNewGraph(std::shared_ptr<RTGraph> graph)
{
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

juce::AudioProcessorValueTreeState::ParameterLayout SequenceTreeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain",
        "Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));

    return { params.begin(), params.end() };
}