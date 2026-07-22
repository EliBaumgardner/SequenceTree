#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../UI/Node/Node.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include <algorithm>
#include <unordered_set>



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
void SequenceTreeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    traversalSession.prepare();
}

void SequenceTreeAudioProcessor::releaseResources()
{
    retiredSnapshots.clear();
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
        state.addChild(graphState.nodeMap.createCopy(),      -1, nullptr);
        state.addChild(graphState.traversalMap.createCopy(), -1, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    copyXmlToBinary(*xml, destData);
}

void SequenceTreeAudioProcessor::applyRestoredState()
{
    if (!pendingRestoreState.isValid()) {
        return;
    }

    const juce::ValueTree restoredTree = pendingRestoreState;

    if (suspendStateListeners) {
        suspendStateListeners();
    }

    graphState.replaceState(restoredTree);
    rtGraphBuilder.rebuildAllGraphs();

    pendingRestoreState = juce::ValueTree();

    if (resumeStateListeners) {
        resumeStateListeners();
    }
}

void SequenceTreeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes == 0) {
        pendingRestoreState = juce::ValueTree(ValueTreeIdentifiers::NodeMap);
    }
    else {
        std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

        if (xmlState == nullptr) { DBG("INVALID STATE DATA"); return; }

        juce::ValueTree restoredTree = juce::ValueTree::fromXml (*xmlState);

        if (!restoredTree.isValid()) { DBG("INVALID STATE TREE"); return; }

        pendingRestoreState = restoredTree;
    }

    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        applyRestoredState();
        return;
    }

    juce::WeakReference<SequenceTreeAudioProcessor> safeThis (this);

    juce::MessageManager::callAsync([safeThis]() mutable {
        if (safeThis != nullptr) {
            safeThis->applyRestoredState();
        }
    });
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SequenceTreeAudioProcessor();
}

void SequenceTreeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    struct BlockScope
    {
        std::atomic<std::uint64_t>& counter;
        ~BlockScope() { counter.fetch_add(1, std::memory_order_release); }
    };

    const BlockScope blockScope { blocksCompleted };

    const int numSamples = buffer.getNumSamples();

    buffer.clear();
    midiMessages.clear();

    const bool resetHit = resetRequested.exchange(false);

    if (resetHit) {
        traversalSession.silenceAllNotes(midiMessages);
    }

    AudioSnapshot* snap = currentSnapshot.load(std::memory_order_acquire);

    if (isPlaying.load() == false || !snap || !snap->globalNodes) {
        if (resetHit) {
            traversalSession.clearTraversals();
            if (notifyUi) {
                notifyUi();
            }
        }
        return;
    }

    NodeMap&  nodes    = *snap->globalNodes;
    RTGraphs& rtGraphs = *snap->rtGraphs;

    if (resetHit) {
        traversalSession.restartActiveTraversals(nodes, rtGraphs, midiMessages);

        if (notifyUi) {
            notifyUi();
        }
    }

    traversalSession.syncWithGraph(nodes, rtGraphs, midiMessages);

    if (traversalSession.isIdle()
        && !traversalSession.startTraversalsFromFirstRoot(nodes, rtGraphs, midiMessages)) {
        return;
    }

    eventManager.processEvents(numSamples, midiMessages, nodes, traversalSession.getTraversals());

    if (notifyUi && hasPendingUiCommands()) {
        notifyUi();
    }
}

bool SequenceTreeAudioProcessor::hasPendingUiCommands() const
{
    return eventManager.bridge.highlightFifo.getNumReady()  > 0
        || eventManager.bridge.progressFifo.getNumReady()   > 0
        || eventManager.bridge.arrowResetFifo.getNumReady() > 0
        || eventManager.bridge.countFifo.getNumReady()      > 0;
}

void SequenceTreeAudioProcessor::clearOldEvents(int traversalId)
{
    eventManager.scheduler.clearTraversalNotes(traversalId);
}

void SequenceTreeAudioProcessor::setNewGraph(std::shared_ptr<RTGraph> graph)
{
    const AudioSnapshot* oldSnap = publishedSnapshot.get();

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

    publishAudioSnapshot(newSnap);
}

void SequenceTreeAudioProcessor::publishAudioSnapshot(std::shared_ptr<AudioSnapshot> snapshot)
{
    static_assert(std::atomic<AudioSnapshot*>::is_always_lock_free,
                  "the audio thread must be able to read the snapshot without a lock");

    AudioSnapshot* raw = snapshot.get();

    auto retired      = std::move(publishedSnapshot);
    publishedSnapshot = std::move(snapshot);

    currentSnapshot.store(raw, std::memory_order_release);

    std::atomic_thread_fence(std::memory_order_seq_cst);

    if (retired != nullptr) {
        retiredSnapshots.push_back({ std::move(retired),
                                     blocksCompleted.load(std::memory_order_acquire) });
    }

    collectRetiredSnapshots();
}

void SequenceTreeAudioProcessor::collectRetiredSnapshots()
{
    const std::uint64_t completed = blocksCompleted.load(std::memory_order_acquire);

    auto isUnreachableByAudioThread = [completed](const RetiredSnapshot& entry) {
        return completed > entry.retiredAtBlock;
    };

    retiredSnapshots.erase(std::remove_if(retiredSnapshots.begin(),
                                          retiredSnapshots.end(),
                                          isUnreachableByAudioThread),
                           retiredSnapshots.end());
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