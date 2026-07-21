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
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) {
        return false;
    }
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
    juce::ValueTree state;

    if (pendingRestoreState.isValid()) {
        state = pendingRestoreState;
    }
    else {
        state = juce::ValueTree(ValueTreeIdentifiers::PluginState);
        state.addChild(ValueTreeState::nodeMap.createCopy(),      -1, nullptr);
        state.addChild(ValueTreeState::traversalMap.createCopy(), -1, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    copyXmlToBinary(*xml, destData);
}

void SequenceTreeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes == 0) {
        pendingRestoreState = juce::ValueTree();

        if (applyStateToUi) {
            applyStateToUi(juce::ValueTree(ValueTreeIdentifiers::NodeMap));
        }
        else {
            ValueTreeState::nodeMap.removeAllChildren(nullptr);
        }
        return;
    }

    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState == nullptr) { DBG("INVALID STATE DATA"); return; }

    juce::ValueTree restoredTree = juce::ValueTree::fromXml (*xmlState);

    if (!restoredTree.isValid()) { DBG("INVALID STATE TREE"); return; }

    pendingRestoreState = restoredTree;

    if (applyStateToUi) {
        applyStateToUi(restoredTree);
    }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SequenceTreeAudioProcessor();
}

void SequenceTreeAudioProcessor::updateTraversalCounts(const NodeMap &nodes, TraversalMap &traversals) {
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

void SequenceTreeAudioProcessor::syncActiveTraversals(const NodeMap &nodes, TraversalMap &traversals) {
    for (auto& [id, traverser] : traversals) {
        if (traverser.isFlagSpawned) {
            auto flagIt = nodes.find(traverser.flagSourceNodeId);
            if (flagIt != nodes.end()
                && flagIt->second.flagTraversal.traversalId == traverser.traversal.traversalId) {
                traverser.traversal = flagIt->second.flagTraversal;
            }
            continue;
        }

        auto rootIt = nodes.find(traverser.rootId);
        if (rootIt == nodes.end()) {
            continue;
        }

        for (const RTtraversal& assigned : rootIt->second.traversals) {
            if (assigned.traversalId == traverser.traversal.traversalId) {
                traverser.traversal = assigned;
                break;
            }
        }
    }
}

void SequenceTreeAudioProcessor::removeDeletedTraversals(const NodeMap &nodes, TraversalMap &traversals, juce::MidiBuffer &midiMessages) {
    for (auto it = traversals.begin(); it != traversals.end(); ) {
        const TraversalLogic& traverser = it->second;

        bool stillAssigned = false;
        auto rootIt = nodes.find(traverser.rootId);
        if (rootIt != nodes.end()) {
            if (traverser.isFlagSpawned) {
                stillAssigned = true;
            }
            else {
                for (const RTtraversal& assigned : rootIt->second.traversals) {
                    if (assigned.traversalId == traverser.traversal.traversalId) {
                        stillAssigned = true;
                        break;
                    }
                }
            }
        }

        if (stillAssigned) {
            it = std::next(it);
            continue;
        }

        stopTraversalNotes(it->first, midiMessages);
        it = traversals.erase(it);
    }
}

void SequenceTreeAudioProcessor::stopTraversalNotes(int instanceId, juce::MidiBuffer &midiMessages) {
    auto& activeNotes = eventManager.scheduler.activeNotes;

    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i) {
        auto& note = activeNotes[i];

        if (note.instanceId != instanceId) {
            continue;
        }

        if (NoteScheduler::isNodeAudible(note.noteNode.nodeType) && !note.isConnectionTrigger) {
            midiMessages.addEvent(juce::MidiMessage::noteOff(note.event.midiChannel, note.event.pitch), 0);
        }

        eventManager.bridge.highlightNode(note.noteNode, false);
        eventManager.scheduler.removeNote(i);
    }
}

void SequenceTreeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    buffer.clear();
    midiMessages.clear();

    const bool resetHit = resetRequested.exchange(false);

    if (resetHit) {
        for (auto& note : eventManager.scheduler.activeNotes)
        {
            if (NoteScheduler::isNodeAudible(note.noteNode.nodeType) && !note.isConnectionTrigger) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), 0);
            }

            eventManager.bridge.highlightNode(note.noteNode, false);
        }

        eventManager.scheduler.activeNotes.clear();
    }

    if(isPlaying.load() == false) {
        if (resetHit) {
            eventManager.traversals.clear();
            if (notifyUi) {
                notifyUi();
            }
        }
        return;
    }

    auto snap = std::atomic_load(&audioSnapshot);

    if (!snap || !snap->globalNodes) {
        if (resetHit) {
            eventManager.traversals.clear();
            if (notifyUi) {
                notifyUi();
            }
        }
        return;
    }

    NodeMap& nodes           = *snap->globalNodes;
    TraversalMap& traversals = eventManager.traversals;

    if (resetHit) {
        std::unordered_set<int> rootsToRestart;
        for (const auto& [id, traverser] : traversals) {
            rootsToRestart.insert(traverser.rootId);
        }

        traversals.clear();

        for (int rootId : rootsToRestart) {
            auto rootIt = nodes.find(rootId);
            if (rootIt == nodes.end()) {
                continue;
            }

            for (const RTtraversal& assigned : rootIt->second.traversals) {
                startTraversal(rootIt->second, assigned, nodes, traversals, *snap->rtGraphs, midiMessages);
            }
        }

        if (notifyUi) {
            notifyUi();
        }
    }

    updateTraversalCounts(nodes, traversals);
    syncActiveTraversals(nodes, traversals);
    removeDeletedTraversals(nodes, traversals, midiMessages);
    startMissingTraversals(nodes, traversals, *snap->rtGraphs, midiMessages);
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

bool SequenceTreeAudioProcessor::initializeTraversalForRootNode(juce::MidiBuffer &midiMessages, const NodeMap &nodes, TraversalMap &traversals, RTGraphs &rtGraphs) {
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

    RTNode rootNode = nodes.at(rootId);

    //RTtraversal traversal = rootNode.traversals.empty() ? RTtraversal{} : rootNode.traversals[0];

    for (const RTtraversal& traversal : rootNode.traversals) {
        startTraversal(rootNode, traversal, nodes, traversals, rtGraphs, midiMessages);
    }

    return rootFound;
}

void SequenceTreeAudioProcessor::startTraversal(const RTNode &rootNode, const RTtraversal &traversal,
                                                const NodeMap &nodes, TraversalMap &traversals,
                                                RTGraphs &rtGraphs, juce::MidiBuffer &midiMessages)
{
    const int rootId      = rootNode.nodeID;
    const int traversalId = traversal.traversalId;
    const int instanceId  = nextTraversalInstanceId();

    traversals.insert({ instanceId, TraversalLogic(rootId, eventManager.bridge, traversal) });
    TraversalLogic& traversalLogic = traversals.at(instanceId);

    traversalLogic.instanceId     = instanceId;
    traversalLogic.isFirstEvent   = true;
    traversalLogic.primary.target = rootId;
    traversalLogic.state          = TraversalLogic::TraversalState::Active;
    traversalLogic.isLooping      = true;

    auto rtGraphIt = rtGraphs.find(rootId);
    if (rtGraphIt != rtGraphs.end()) {
        traversalLogic.loopLimit = rtGraphIt->second->loopLimit;
    }

    traversalLogic.advanceAlternative(nodes, rootId);

    eventManager.bridge.highlightNode(rootNode, true, traversalId);
    eventManager.dispatcher.pushNote(rootNode, instanceId, midiMessages, 0, nodes, traversals);
}

void SequenceTreeAudioProcessor::startMissingTraversals(const NodeMap &nodes, TraversalMap &traversals,
                                                        RTGraphs &rtGraphs, juce::MidiBuffer &midiMessages)
{
    std::unordered_set<int> activeRootIds;
    for (const auto& [id, traverser] : traversals) {
        if (traverser.isFlagSpawned) {
            continue;
        }
        activeRootIds.insert(traverser.rootId);
    }

    auto isActive = [&traversals](int rootId, int traversalId) {
        for (const auto& [id, traverser] : traversals) {
            if (traverser.rootId == rootId && traverser.traversal.traversalId == traversalId) {
                return true;
            }
        }
        return false;
    };

    for (int rootId : activeRootIds) {
        auto rootIt = nodes.find(rootId);
        if (rootIt == nodes.end()) {
            continue;
        }

        for (const RTtraversal& assigned : rootIt->second.traversals) {
            if (isActive(rootId, assigned.traversalId)) {
                continue;
            }

            startTraversal(rootIt->second, assigned, nodes, traversals, rtGraphs, midiMessages);
        }
    }
}

void SequenceTreeAudioProcessor::syncTraversalLoopLimits(TraversalMap &traversals, RTGraphs &rtGraphs, juce::MidiBuffer &midiMessages, const NodeMap &nodes)
{
    for (auto& [instanceId, traversal] : traversals)
    {
        auto rtGraphIt = rtGraphs.find(traversal.rootId);
        if (rtGraphIt == rtGraphs.end()) {
            continue;
        }

        int newLoopLimit = rtGraphIt->second->loopLimit;
        if (newLoopLimit == traversal.loopLimit) {
            continue;
        }

        traversal.loopLimit = newLoopLimit;

        if (traversal.state == TraversalLogic::TraversalState::End) {
            if (newLoopLimit == 0 || traversal.loopCount < newLoopLimit) {
                traversal.primary.target = traversal.rootId;
                traversal.state          = TraversalLogic::TraversalState::Active;
                traversal.advanceAlternative(nodes, traversal.rootId);

                auto rootIt = nodes.find(traversal.rootId);
                if (rootIt != nodes.end()) {
                    eventManager.bridge.highlightNode(rootIt->second, true);
                    eventManager.dispatcher.pushNote(rootIt->second, instanceId, midiMessages, 0, nodes, traversals);
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


    for (const auto& [nodeId, node] : graph->nodeMap) {
        (*newSnap->globalNodes)[nodeId] = node;
    }

    std::vector<int> staleIds;

    for (const auto& [nodeId, node] : *newSnap->globalNodes) {
        if (node.graphID == graph->graphID && !graph->nodeMap.count(nodeId)) {
            staleIds.push_back(nodeId);
        }
    }

    for (int id : staleIds) {
        newSnap->globalNodes->erase(id);
    }

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