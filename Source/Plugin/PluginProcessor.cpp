#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../UI/Node/Node.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Util/NodeInfo.h"
#include <algorithm>
#include <cmath>
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
    traversalRuleState.ensureDefaultRule();
    snapshots.publishActiveTraversalRule();
}

juce::AudioProcessorValueTreeState::ParameterLayout SequenceTreeAudioProcessor::createParameterLayout()
{
    juce::NormalisableRange<float> tempoRange { static_cast<float>(RTtraversal::minimumTempoMultiplier),
                                                static_cast<float>(RTtraversal::maximumTempoMultiplier) };
    tempoRange.setSkewForCentre(1.0f);

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { tempoParameterId, parameterVersion },
                                                           "Tempo Multiplier", tempoRange, 1.0f));

    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { velocityParameterId, parameterVersion },
                                                         "Velocity", 0, maximumMidiVelocity, maximumMidiVelocity));

    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { transposeParameterId, parameterVersion },
                                                         "Transpose", minimumGlobalTranspose, maximumGlobalTranspose, 0));

    return layout;
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
        state.addChild(valueTreeState.copyState(),            -1, nullptr);
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

        const juce::ValueTree restoredParameters = restoredTree.getChildWithName(valueTreeState.state.getType());

        if (restoredParameters.isValid()) {
            valueTreeState.replaceState(restoredParameters);
        }
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

void SequenceTreeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) noexcept [[clang::nonblocking]]
{
    juce::ScopedNoDenormals noDenormals;

    struct BlockScope
    {
        AudioSnapshotPublisher&                 publisher;
        const AudioSnapshotPublisher::Snapshot* snapshot = publisher.beginBlock();

        ~BlockScope()
        {
            publisher.endBlock();
        }
    };

    const BlockScope blockScope { snapshots };

    const int numSamples = buffer.getNumSamples();

    buffer.clear();
    midiMessages.clear();

    for (const auto& noteOff : pendingNoteOffs) {
        midiMessages.addEvent(noteOff, 0);
    }

    pendingNoteOffs.clear();

    followHostTransport(numSamples);

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

    const AudioSnapshotPublisher::Snapshot* snap = blockScope.snapshot;

    const RTScript* activeScript = nullptr;

    if (snap != nullptr) {
        activeScript = snap->selectChildScript.get();
    }

    traversalSession.setSelectChildScript(activeScript);

    const bool replayWanted = tempoInfo.pendingRelocationPpq.has_value()
                           || traversalSession.playback == TraversalSession::Playback::Replaying;

    if ((!playing && (resetHit || !replayWanted)) || !snap || !snap->globalNodes) {
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
        tempoParameter.load() * hostTempoScale,
        juce::roundToInt(transposeParameter.load()),
        velocityParameter.load() / static_cast<double>(maximumMidiVelocity)
    };

    eventManager.followTempo(context.tempoMultiplier);

    if (tempoInfo.pendingRelocationPpq) {
        const double targetSamples = std::round(*tempoInfo.pendingRelocationPpq * 60.0 / tempoInfo.hostBpm
                                               * tempoInfo.currentSampleRate);

        traversalSession.beginReplay(context, targetSamples);
        tempoInfo.pendingRelocationPpq.reset();
    }

    if (traversalSession.continueReplay(context, snap->generation, numSamples, playing) == TraversalSession::Playback::Replaying) {
        return;
    }

    if (!playing) {
        return;
    }

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

void SequenceTreeAudioProcessor::followHostTransport(const int numSamples) noexcept
{
    if (wrapperType != wrapperType_Standalone) {
        bool hostPlaying = false;

        if (juce::AudioPlayHead* playHead = getPlayHead()) {
            if (const juce::Optional<juce::AudioPlayHead::PositionInfo> position = playHead->getPosition()) {
                hostPlaying = position->getIsPlaying();

                if (const juce::Optional<double> hostBpm = position->getBpm()) {
                    tempoInfo.hostBpm = *hostBpm;
                }

                const juce::Optional<double> hostPpq = position->getPpqPosition();

                if (hostPpq && tempoInfo.hostBpm > 0.0) {
                    const bool resumed = hostPlaying && !wasPlaying;
                    const bool jumped  = resumed || !tempoInfo.expectedPpq || std::abs(*hostPpq - *tempoInfo.expectedPpq) > TempoInfo::relocationToleranceBeats;

                    if (jumped) {
                        tempoInfo.pendingRelocationPpq = juce::jmax(0.0, *hostPpq);
                    }

                    tempoInfo.expectedPpq = *hostPpq;

                    if (hostPlaying) {
                        tempoInfo.expectedPpq = *hostPpq + numSamples / tempoInfo.currentSampleRate * tempoInfo.hostBpm / 60.0;
                    }
                }
                else {
                    tempoInfo.expectedPpq.reset();
                }
            }
        }

        if (hostPlaying != isPlaying.exchange(hostPlaying)) {
            playbackStateChanged.store(true);
        }
    }
}