/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../UI/Node/Node.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include <unordered_set>



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
    return 1;
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
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SequenceTreeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

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
    return true;
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

    if (applyStateToUi)
        applyStateToUi(restoredTree);
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SequenceTreeAudioProcessor();
}

void SequenceTreeAudioProcessor::updateTraversalCounts(NodeMap &nodes, TraversalMap &traversals) {
    for (const auto& [nodeId, node] : nodes) {
        for (auto& [id, traverser] : traversals) {
            if (!traverser.primary.counts.count(nodeId)) {
                traverser.primary.counts[nodeId] = 0;
            }
        }
    }

    for (auto& [id, traverser] : traversals) {
        for (auto it = traverser.primary.counts.begin(); it != traverser.primary.counts.end(); ) {
            if (nodes.find(it->first) == nodes.end()) {
                it = traverser.primary.counts.erase(it);
            } else {
                it = std::next(it);
            }
        }
    }
}

void SequenceTreeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    buffer.clear();
    midiMessages.clear();

    if (resetRequested.exchange(false))
    {
        for (auto& note : eventManager.scheduler.activeNotes)
        {
            if (NoteScheduler::isNodeAudible(note.noteNode.nodeType) && !note.isConnectionTrigger)
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), 0);

            eventManager.bridge.highlightNode(note.noteNode, false);
        }

        eventManager.scheduler.activeNotes.clear();
        eventManager.traversals.clear();

        if (notifyUi) notifyUi();
    }

    if(isPlaying.load() == false) {
        return;
    }

    auto snap = std::atomic_load(&audioSnapshot);

    if (!snap || !snap->globalNodes) {
        return;
    }

    NodeMap& nodes       = *snap->globalNodes;
    TraversalMap& traversals = eventManager.traversals;

    updateTraversalCounts(nodes, traversals);
    syncTraversalLoopLimits(traversals, *snap->rtGraphs, midiMessages, nodes);

    if (traversals.empty()) {
        if (initializeTraversalForRootNode(midiMessages, nodes, traversals, *snap->rtGraphs) == false) {
            return;
        }
    }

    eventManager.processEvents(numSamples, midiMessages, nodes, traversals);

    if (notifyUi
        && (eventManager.bridge.highlightFifo.getNumReady() > 0
         || eventManager.bridge.progressFifo.getNumReady() > 0
         || eventManager.bridge.arrowResetFifo.getNumReady() > 0
         || eventManager.bridge.countFifo.getNumReady() > 0)) {
        notifyUi();
    }

}

bool SequenceTreeAudioProcessor::initializeTraversalForRootNode(juce::MidiBuffer &midiMessages, NodeMap &nodes, TraversalMap &traversals, RTGraphs &rtGraphs) {
    int rootId = -1;
    bool rootFound;

    std::unordered_set<int> linkedRootIds;
    for (const auto& [nodeId, node] : nodes) {
        for (int childId : node.children) {
            linkedRootIds.insert(childId);
        }
    }

    for (const auto& [nodeId, node] : nodes) {
        if (node.nodeID == node.graphID && linkedRootIds.find(nodeId) == linkedRootIds.end()) {
            if (rootId == -1 || nodeId < rootId) {
                rootId = nodeId;
            }
        }
    }

    if (rootId != -1) {
        rootFound = true;
    }
    else {
        rootFound = false;
        return rootFound;
    }

    traversals.insert({rootId, TraversalLogic(rootId, eventManager.bridge)});
    TraversalLogic& traversal = traversals.at(rootId);
    traversal.isFirstEvent  = true;
    traversal.primary.target = rootId;
    traversal.state         = TraversalLogic::TraversalState::Active;
    traversal.isLooping     = true;

    auto rtGraphIt = rtGraphs.find(rootId);
    if (rtGraphIt != rtGraphs.end()) {
        traversal.loopLimit = rtGraphIt->second->loopLimit;
    }

    RTNode& rootNode    = nodes.at(rootId);
    int     traversalId = rootNode.nodeID;
    eventManager.bridge.highlightNode(rootNode, true);
    eventManager.dispatcher.pushNote(rootNode, traversalId, midiMessages, 0, nodes, traversals);

    return rootFound;
}

void SequenceTreeAudioProcessor::syncTraversalLoopLimits(TraversalMap &traversals, RTGraphs &rtGraphs, juce::MidiBuffer &midiMessages, NodeMap &nodes)
{
    for (auto& [traversalId, traversal] : traversals)
    {
        auto rtGraphIt = rtGraphs.find(traversalId);
        if (rtGraphIt == rtGraphs.end()) continue;

        int newLoopLimit = rtGraphIt->second->loopLimit;
        if (newLoopLimit == traversal.loopLimit) continue;

        traversal.loopLimit = newLoopLimit;

        if (traversal.state == TraversalLogic::TraversalState::End) {
            if (newLoopLimit == 0 || traversal.loopCount < newLoopLimit) {
                traversal.primary.target = traversal.rootId;
                traversal.state          = TraversalLogic::TraversalState::Active;

                auto rootIt = nodes.find(traversal.rootId);
                if (rootIt != nodes.end()) {
                    eventManager.bridge.highlightNode(rootIt->second, true);
                    eventManager.dispatcher.pushNote(rootIt->second, traversalId, midiMessages, 0, nodes, traversals);
                }
            }
        }
    }
}

void SequenceTreeAudioProcessor::clearOldEvents(int traversalId)
{
    eventManager.scheduler.clearTraversalNotes(traversalId);
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