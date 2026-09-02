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
    graphState.ensureDefaultTraversalRule();
    snapshots.publishActiveTraversalRule();
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
    tempoInfo.currentSampleRate = sampleRate;
    traversalSession.prepare();
}

void SequenceTreeAudioProcessor::releaseResources()
{
    snapshots.releaseRetiredSnapshots();
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
    if (pendingRestoreState.isValid()
        && juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        applyRestoredState();
    }

    juce::ValueTree state;

    if (pendingRestoreState.isValid()) {
        state = pendingRestoreState;
    }
    else {
        state = juce::ValueTree(ValueTreeIdentifiers::PluginState);
        state.addChild(graphState.nodeMap.createCopy(),      -1, nullptr);
        state.addChild(graphState.traversalMap.createCopy(), -1, nullptr);
        state.addChild(graphState.traversalRules.createCopy(), -1, nullptr);
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
    graphState.ensureDefaultTraversalRule();
    rtGraphBuilder.rebuildAllGraphs();

    snapshots.publishActiveTraversalRule();

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
        AudioSnapshotPublisher& publisher;
        ~BlockScope() { publisher.blockCompleted(); }
    };

    const BlockScope blockScope { snapshots };

    const int numSamples = buffer.getNumSamples();

    buffer.clear();
    midiMessages.clear();

    const bool resetHit = resetRequested.exchange(false);

    if (resetHit) {
        traversalSession.silenceAllNotes(midiMessages);
    }

    const bool playing   = isPlaying.load();
    const bool suspended = wasPlaying && !playing;

    if (suspended) {
        traversalSession.suspendActiveNotes(midiMessages);
    }

    wasPlaying = playing;

    const AudioSnapshotPublisher::Snapshot* snap = snapshots.acquireForBlock();

    traversalSession.setSelectChildScript(snap != nullptr ? snap->selectChildScript.get() : nullptr);

    if (!playing || !snap || !snap->globalNodes) {
        if (resetHit) {
            traversalSession.clearTraversals();
        }

        if ((resetHit || suspended) && notifyUi) {
            notifyUi();
        }
        return;
    }

    const DispatchContext context {
        *snap->globalNodes,
        *snap->rtGraphs,
        traversalSession.getTraversals(),
        midiMessages,
        tempoInfo.currentSampleRate,
        tempoMultiplier.load()
    };

    if (resetHit) {
        traversalSession.restartActiveTraversals(context);

        if (notifyUi) {
            notifyUi();
        }
    }

    traversalSession.syncWithGraph(context, snap->generation);

    if (traversalSession.isIdle()
        && !traversalSession.startTraversalsFromFirstRoot(context)) {
        return;
    }

    eventManager.processEvents(numSamples, context);

    if (notifyUi && hasPendingUiCommands()) {
        notifyUi();
    }
}

bool SequenceTreeAudioProcessor::hasPendingUiCommands() const
{
    return eventManager.bridge.hasPendingCommands();
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