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
        // Push highlight-off before clearing, so the FIFO gets the commands
        for (auto& note : eventManager.activeNotes)
        {
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), 0);
            eventManager.highlightNode(note.noteNode, false);
        }

        eventManager.activeNotes.clear();
        eventManager.traversals.clear();

        canvas->triggerAsyncUpdate();
    }

    if(isPlaying.load() == false){ return; }

    auto snap = std::atomic_load(&audioSnapshot);
    if (!snap || !snap->globalNodes) return;

    NodeMap& nodes       = *snap->globalNodes;
    TraversalMap& traversals = eventManager.traversals;

    // Reconcile traversal counts with current node set
    for (const auto& [nodeId, node] : nodes)
        for (auto& [id, traverser] : traversals)
            if (!traverser.counts.count(nodeId))
                traverser.counts[nodeId] = 0;

    for (auto& [id, traverser] : traversals)
        for (auto it = traverser.counts.begin(); it != traverser.counts.end(); )
            it = (nodes.find(it->first) == nodes.end()) ? traverser.counts.erase(it) : std::next(it);

    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();

    buffer.clear();
    midiMessages.clear();

    if (traversals.empty()) {
        int rootId = -1;
        for (const auto& [nodeId, node] : nodes) {
            if (node.nodeID == node.graphID) {
                if (rootId == -1 || nodeId < rootId)
                    rootId = nodeId;
            }
        }
        if (rootId == -1) return;

        traversals.insert({rootId, TraversalLogic(rootId, this)});
        TraversalLogic& traversal = traversals.at(rootId);
        traversal.isFirstEvent = true;
        traversal.targetId  = rootId;
        traversal.state     = TraversalLogic::TraversalState::Active;
        traversal.isLooping = true;

        RTNode& rootNode    = nodes.at(rootId);
        int     traversalId = rootNode.nodeID;
        eventManager.highlightNode(rootNode, true);
        eventManager.pushNote(rootNode, traversalId, midiMessages, 0, nodes, traversals);
    }

    eventManager.processEvents(numSamples, midiMessages, nodes, traversals);

    if (eventManager.highlightFifo.getNumReady() > 0)
        canvas->triggerAsyncUpdate();

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
    auto oldSnap = std::atomic_load(&audioSnapshot);

    auto newSnap = std::make_shared<AudioSnapshot>();
    newSnap->globalNodes = (oldSnap && oldSnap->globalNodes)
        ? std::make_shared<NodeMap>(*oldSnap->globalNodes)
        : std::make_shared<NodeMap>();
    newSnap->rtGraphs = (oldSnap && oldSnap->rtGraphs)
        ? std::make_shared<RTGraphs>(*oldSnap->rtGraphs)
        : std::make_shared<RTGraphs>();

    (*newSnap->rtGraphs)[graph->graphID] = graph;

    for (const auto& [nodeId, node] : graph->nodeMap)
        (*newSnap->globalNodes)[nodeId] = node;

    std::vector<int> staleIds;
    for (const auto& [nodeId, node] : *newSnap->globalNodes)
        if (node.graphID == graph->graphID && !graph->nodeMap.count(nodeId))
            staleIds.push_back(nodeId);

    for (int id : staleIds)
        newSnap->globalNodes->erase(id);

    std::atomic_store(&audioSnapshot, newSnap);
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