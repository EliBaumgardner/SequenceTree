#include "PluginProcessor.h"
#include "Editor/PluginEditor.h"

#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/StateJson.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include "Core/Model/ArrowInfo.h"
#include "Core/Model/NodeInfo.h"
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
    snapshots.publishActiveTraversalRule();
}

void SequenceTreeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    playback.prepare(sampleRate);
}

void SequenceTreeAudioProcessor::releaseResources()
{
    playback.releaseNotes();

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

void SequenceTreeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) noexcept [[clang::nonblocking]]
{
    juce::ScopedNoDenormals noDenormals;

    const BlockScope blockScope { snapshots };

    const Snapshot*            snap     = blockScope.snapshot;
    juce::AudioPlayHead* const playHead = getPlayHead();

    const int numSamples = buffer.getNumSamples();
    const int transpose  = juce::roundToInt(transposeParameter.load());

    const float tempo    = tempoParameter.load();
    const float velocity = velocityParameter.load();

    const bool followsHost  = wrapperType != wrapperType_Standalone;
    const bool syncedToHost = hostSyncParameter.load() >= 0.5f;
    const bool offline      = isNonRealtime();

    const PlaybackEngine::BlockInfo block { .playHead = playHead, .tempoParameter = tempo, .velocityParameter = velocity, .numSamples = numSamples,
                                            .transpose = transpose, .followsHost = followsHost, .syncedToHost = syncedToHost, .offline = offline };

    buffer.clear();
    midiMessages.clear();

    playback.processBlock(snap, midiMessages, block);

    virtualMidiOutput.mirrorBlock(midiMessages, playback.tempoInfo.currentSampleRate);
}

juce::AudioProcessorEditor* SequenceTreeAudioProcessor::createEditor()
{
    return new SequenceTreeAudioProcessorEditor (*this);
}

bool SequenceTreeAudioProcessor::hasEditor() const
{
    return true;
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

const juce::String SequenceTreeAudioProcessor::getName() const
{
    return JucePlugin_Name;
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
    return "Default";
}

void SequenceTreeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void SequenceTreeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (pendingRestoreState.isValid()
        && juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        applyRestoredState();
    }

    const PatchProperties patchProperties(document.patch);

    const juce::String storedPatchFile = patchProperties.patchFile;

    const bool stateIsLive = ! pendingRestoreState.isValid();

    const juce::File patchFile(storedPatchFile);

    juce::ValueTree state;

    if (! stateIsLive) {
        state = pendingRestoreState;
    }
    else {
        const juce::ValueTree parameters = valueTreeState.copyState();

        state = document.createState(parameters);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    const juce::String stateText = juce::JSON::toString(StateJson::toVar(*xml));

    destData.replaceAll(stateText.toRawUTF8(), stateText.getNumBytesAsUTF8());

    if (stateIsLive && (document.graphState.nodeMap.getNumChildren() > 0 || patchFile.existsAsFile())) {
        patchFile.replaceWithData(destData.getData(), destData.getSize());
    }
}

void SequenceTreeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes == 0) {
        pendingRestoreState = juce::ValueTree(ValueTreeIdentifiers::NodeMap);
    }
    else {
        std::unique_ptr<juce::XmlElement> xmlState = StateJson::read(data, sizeInBytes);

        if (xmlState == nullptr) {
            xmlState = getXmlFromBinary(data, sizeInBytes);
        }

        if (xmlState == nullptr)     { DBG("INVALID STATE DATA"); return; }

        juce::ValueTree restoredTree = juce::ValueTree::fromXml (*xmlState);

        if (!restoredTree.isValid()) { DBG("INVALID STATE TREE"); return; }

        const int savedSchemaVersion = restoredTree.getProperty(ValueTreeIdentifiers::SchemaVersion, 0);

        if (savedSchemaVersion > PluginDocument::schemaVersion) {
            DBG("STATE FROM A NEWER SCHEMA VERSION");

            return;
        }

        const juce::ValueTree restoredParameters = restoredTree.getChildWithName(valueTreeState.state.getType());

        if (restoredTree.getType() == ValueTreeIdentifiers::PluginState && restoredParameters.isValid()) {
            valueTreeState.replaceState(restoredParameters);
        }

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

void SequenceTreeAudioProcessor::applyRestoredState()
{
    if (!pendingRestoreState.isValid()) {
        return;
    }

    const juce::ValueTree restoredTree = pendingRestoreState;

    document.restore(restoredTree);

    rtGraphBuilder.rebuildAllGraphs();

    document.undoManager.clearUndoHistory();

    snapshots.publishActiveTraversalRule();

    pendingRestoreState = juce::ValueTree();

    if (auto* editor = dynamic_cast<SequenceTreeAudioProcessorEditor*>(getActiveEditor())) {
        editor->nodeCanvas->rebuildFromNodeMap(document.graphState.nodeMap);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout SequenceTreeAudioProcessor::createParameterLayout()
{
    juce::NormalisableRange<float> tempoRange { static_cast<float>(RTtraversal::minimumTempoMultiplier),
                                                static_cast<float>(RTtraversal::maximumTempoMultiplier) };
    tempoRange.setSkewForCentre(1.0f);

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { tempoParameterId, parameterVersion },
                                                           "Tempo Multiplier", tempoRange, 1.0f,
                                                           juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int) {
                                                               return juce::String(value, 2);
                                                           })));

    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { velocityParameterId, parameterVersion },
                                                         "Velocity", NodeInfo::silentMidiVelocity, NodeInfo::maximumMidiVelocity, NodeInfo::maximumMidiVelocity));

    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { transposeParameterId, parameterVersion },
                                                         "Transpose", NodeInfo::minimumGlobalTranspose, NodeInfo::maximumGlobalTranspose, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { hostSyncParameterId, parameterVersion }, "Sync To Host", true));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SequenceTreeAudioProcessor();
}
