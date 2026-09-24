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
, valueTreeState(*this,nullptr,"STATE",TraversalParameters::createParameterLayout())
{
    traversalRuleState.ensureDefaultRule();
    snapshots.publishActiveTraversalRule();
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
    pendingNoteOffs.clear();

    for (const auto& note : eventManager.scheduler.activeNotes) {
        if (NoteScheduler::isNoteSounding(note)) {
            pendingNoteOffs.push_back(
                juce::MidiMessage::noteOff(note.event.midiChannel, note.event.pitch));
        }
    }

    eventManager.scheduler.activeNotes.clear();

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
        state.addChild(graphState.nodeMap.createCopy(),       -1, nullptr);
        state.addChild(graphState.traversals.map.createCopy(),-1, nullptr);
        state.addChild(traversalRuleState.rules.createCopy(), -1, nullptr);
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

    juce::ValueTree restoredNodeMap = restoredTree;
    juce::ValueTree restoredTraversalMap;
    juce::ValueTree restoredRules;

    if (restoredTree.getType() == ValueTreeIdentifiers::PluginState) {
        restoredNodeMap      = restoredTree.getChildWithName(ValueTreeIdentifiers::NodeMap);
        restoredTraversalMap = restoredTree.getChildWithName(ValueTreeIdentifiers::TraversalMap);
        restoredRules        = restoredTree.getChildWithName(ValueTreeIdentifiers::TraversalRules);
    }

    graphState.replaceState(restoredNodeMap, restoredTraversalMap);

    traversalRuleState.replaceState(restoredRules);
    traversalRuleState.ensureDefaultRule();

    rtGraphBuilder.rebuildAllGraphs();

    undoManager.clearUndoHistory();

    snapshots.publishActiveTraversalRule();

    pendingRestoreState = juce::ValueTree();

    if (auto* editor = dynamic_cast<SequenceTreeAudioProcessorEditor*>(getActiveEditor())) {
        editor->canvas->rebuildFromNodeMap(graphState.nodeMap);
    }
}

void SequenceTreeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes == 0) {
        pendingRestoreState = juce::ValueTree(ValueTreeIdentifiers::NodeMap);
    }
    else {
        std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

        if (xmlState == nullptr)     { DBG("INVALID STATE DATA"); return; }

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
        AudioSnapshotPublisher&     publisher;
        SequenceTreeAudioProcessor& processor;

        ~BlockScope()
        {
            publisher.blockCompleted();

            const bool uiWorkPending = processor.eventManager.bridge.hasPendingCommands()
                                    || processor.playbackStateChanged.load();

            if (uiWorkPending) {
                processor.triggerAsyncUpdate();
            }
        }
    };

    const BlockScope blockScope { snapshots, *this };

    const int numSamples = buffer.getNumSamples();

    buffer.clear();
    midiMessages.clear();

    for (const auto& noteOff : pendingNoteOffs) {
        midiMessages.addEvent(noteOff, 0);
    }

    pendingNoteOffs.clear();

    if (wrapperType != wrapperType_Standalone) {
        bool hostPlaying = false;

        if (juce::AudioPlayHead* playHead = getPlayHead()) {
            if (const juce::Optional<juce::AudioPlayHead::PositionInfo> position = playHead->getPosition()) {
                hostPlaying = position->getIsPlaying();

                if (const juce::Optional<double> hostBpm = position->getBpm()) {
                    tempoInfo.hostBpm = *hostBpm;
                }
            }
        }

        if (hostPlaying != isPlaying.exchange(hostPlaying)) {
            playbackStateChanged.store(true);
        }
    }

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

    const RTScript* activeScript = nullptr;

    if (snap != nullptr) {
        activeScript = snap->selectChildScript.get();
    }

    traversalSession.setSelectChildScript(activeScript);

    if (!playing || !snap || !snap->globalNodes) {
        if (resetHit) {
            traversalSession.clearTraversals();
        }

        return;
    }

    double hostTempoScale = 1.0;

    if (tempoInfo.hostBpm > 0.0) {
        hostTempoScale = tempoInfo.hostBpm / TempoInfo::referenceBpm;
    }

    const DispatchContext context {
        *snap->globalNodes,
        traversalSession.traversals,
        midiMessages,
        tempoInfo.currentSampleRate,
        tempoMultiplier.load() * hostTempoScale
    };

    if (resetHit) {
        traversalSession.restartActiveTraversals(context);
    }

    traversalSession.syncWithGraph(context, snap->generation);

    if ((traversalSession.traversals.empty()) && !traversalSession.startTraversalsFromFirstRoot(context)) {
        if (!eventManager.scheduler.activeNotes.empty()) {
            traversalSession.silenceAllNotes(midiMessages);
        }

        return;
    }

    eventManager.processEvents(numSamples, context);
}

void SequenceTreeAudioProcessor::handleAsyncUpdate()
{
    auto* editor = dynamic_cast<SequenceTreeAudioProcessorEditor*>(getActiveEditor());

    const bool transportMoved = playbackStateChanged.exchange(false);

    if (editor == nullptr) {
        return;
    }

    if (transportMoved) {
        editor->titleBar->applyPlaybackState(isPlaying.load());
    }

    editor->canvas->handleAsyncUpdate();
}