#pragma once

#include "../Util/PluginModules.h"
#include <memory>
#include <atomic>
#include <functional>
#include "../Graph/RTData.h"
#include "../Graph/ValueTreeState.h"
#include "../Graph/RTGraphBuilder.h"
#include "../Audio/EventManager.h"
#include "../Audio/TraversalSession.h"

class SequenceTreeAudioProcessorEditor;

class SequenceTreeAudioProcessor  : public juce::AudioProcessor
{
public:
    SequenceTreeAudioProcessor();
    ~SequenceTreeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void clearOldEvents(int traversalId);
    void setNewGraph(std::shared_ptr<RTGraph> graph);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::function<void()>                  notifyUi;

    std::function<void()>                  suspendStateListeners;
    std::function<void()>                  resumeStateListeners;

    juce::ValueTree                        pendingRestoreState;

    void applyRestoredState();

    struct AudioSnapshot
    {
        std::shared_ptr<NodeMap>      globalNodes;
        std::shared_ptr<RTGraphs>     rtGraphs;
    };

    std::atomic<AudioSnapshot*> currentSnapshot { nullptr };
    std::atomic<std::uint64_t>  blocksCompleted { 0 };

    void publishAudioSnapshot(std::shared_ptr<AudioSnapshot> snapshot);

    std::atomic<bool>   isPlaying       = false;
    std::atomic<bool>   resetRequested  = false;
    std::atomic<double> tempoMultiplier { 1.0 };

    juce::AudioProcessorValueTreeState valueTreeState;

    ValueTreeState graphState;

    RTGraphBuilder rtGraphBuilder { *this, graphState };


    struct {
        double currentSampleRate;
        int samplesPerBeat;
        int currentStep;
        int stepLengthInSamples;
        int samplesIntoStep;
        int bpm;
    } TempoInfo { 44100, 0, 0, 0, 0, 120 };

    EventManager     eventManager     { this };
    TraversalSession traversalSession { eventManager };

    bool hasPendingUiCommands() const;

private:

    struct RetiredSnapshot
    {
        std::shared_ptr<AudioSnapshot> snapshot;
        std::uint64_t                  retiredAtBlock = 0;
    };

    void collectRetiredSnapshots();

    std::shared_ptr<AudioSnapshot> publishedSnapshot;
    std::vector<RetiredSnapshot>   retiredSnapshots;

public:

    const AudioSnapshot* getPublishedSnapshot() const { return publishedSnapshot.get(); }

    JUCE_DECLARE_WEAK_REFERENCEABLE (SequenceTreeAudioProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessor)
};