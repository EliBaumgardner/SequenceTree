/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "Util/PluginModules.h"

//==============================================================================
/**
*/
#include <memory>
#include <atomic>
#include "Util/RTData.h"
#include "Node/NodeCanvas.h"

class SequenceTreeAudioProcessorEditor;

class Node;

class NodeData;

class SequenceTreeAudioProcessor  : public juce::AudioProcessor
{
    public:
    //==============================================================================
    SequenceTreeAudioProcessor();
    ~SequenceTreeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif
    
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void pushNote(RTNode node, int traversalId,juce::MidiBuffer& midiMessages);

    void clearOldEvents(RTNode node, int traversalId);

    void setNewGraph(std::shared_ptr<RTGraph> graph);
    void scheduleNodeHighlight(const RTNode& node,bool shouldHighlight);

    NodeCanvas* canvas;
    
    using RTGraphs = std::unordered_map<int,std::shared_ptr<RTGraph>>;
    std::shared_ptr<RTGraphs> rtGraphs = nullptr;

    int numGraphs = 0;

    std::shared_ptr<std::unordered_map<int,RTNode>> globalNodes = nullptr;

    std::atomic<bool> isPlaying = false;


    /////                        TRAVERSAL LOGIC                       /////


    class TraversalLogic {
    public:

        std::unordered_map<int,int> counts;
        std::vector<int> traversers;

        bool isLooping = false;
        int rootId = 0;
        int targetId = 0;
        int lastTargetId = 0;
        int referenceTargetId = 0;

        enum class TraversalState{ Start, Active, End, Reset};
        enum class EventType { Node, RelayNode, Counter};

        TraversalState state = TraversalState::Start;


        SequenceTreeAudioProcessor* audioProcessor;

        TraversalLogic(int root,SequenceTreeAudioProcessor* processor ) : rootId(root), audioProcessor(processor) {}

        void advance()
        {
            traversers.clear();
            std::shared_ptr<std::unordered_map<int,RTNode>> loadedGlobalNodes = std::atomic_load(&audioProcessor->globalNodes);

            referenceTargetId = lastTargetId;
            lastTargetId = targetId;
            int count = ++counts[targetId];
            auto itTarget = loadedGlobalNodes->find(targetId);

            if (itTarget->second.children.empty()) {
                if (isLooping)  { state = TraversalState::Reset; return; }
                state = TraversalState::End; return;
            }

            int maxLimit = 0;

            for (int childIndex : itTarget->second.children) {

                auto itChild = loadedGlobalNodes->find(childIndex);
                auto childNode = itChild->second;
                int limit = childNode.countLimit;

                switch (childNode.nodeType) {
                    case(RTNode::NodeType::Node) : { if (count % limit == 0 && limit > maxLimit) { targetId = childIndex; maxLimit = limit; } break; }
                    case(RTNode::NodeType::RelayNode) : { if (count % limit == 0) { traversers.push_back(childIndex); } break; }
                }
            }

            if (targetId == lastTargetId) {
                if (isLooping) { state = TraversalState::Reset; }
                else { state = TraversalState::End; }
            }
        }

        const RTNode& getTargetNode()    { return ( *std::atomic_load(&audioProcessor->globalNodes) )[targetId];          }
        const RTNode& getLastNode()      { return ( *std::atomic_load(&audioProcessor->globalNodes) )[lastTargetId];      }
        const RTNode& getReferenceNode() { return ( *std::atomic_load(&audioProcessor->globalNodes) )[referenceTargetId]; }
        const RTNode& getRootNode()      { return ( *std::atomic_load(&audioProcessor->globalNodes) )[rootId];            }

        const RTNode& getRelayNode(int relayNodeId) { return ( *std::atomic_load(&audioProcessor->globalNodes) )[relayNodeId]; }


        bool shouldTraverse()
        {
            switch (state) {
                case TraversalState::Start  :   { return true; }
                case TraversalState::Active :   { return true; }
                case TraversalState::Reset  :   { return true; }
                case TraversalState::End    :   { return false;}
            }
        }

        void handleNodeEvent() {

            switch (state) {
                case TraversalState::Start : {
                    state = TraversalState::Active;
                    targetId = rootId;
                    audioProcessor->scheduleNodeHighlight(getTargetNode(), true);
                    break;
                }
                case TraversalState::Active: {
                    advance();

                    for (int traverserId : traversers) {
                        auto& traverserNode = (*std::atomic_load(&audioProcessor->globalNodes))[traverserId];
                        audioProcessor->scheduleNodeHighlight(traverserNode,   true);
                    }

                    switch (state) {
                        case TraversalState::Active: {
                            audioProcessor->scheduleNodeHighlight(getLastNode()  ,  false);
                            audioProcessor->scheduleNodeHighlight(getTargetNode(),   true);
                            break;
                        }
                        case TraversalState::Reset: {
                            audioProcessor->scheduleNodeHighlight(getTargetNode(),   false);
                            audioProcessor->scheduleNodeHighlight(getRootNode()  ,    true);
                            targetId = rootId;
                            state = TraversalState::Active;
                            break;
                        }
                        case TraversalState::End: {
                            audioProcessor->scheduleNodeHighlight(getTargetNode()   ,false);
                            audioProcessor->scheduleNodeHighlight(getReferenceNode(),false);
                        }
                    }
                    break;
                }
                case TraversalState::End: {
                    audioProcessor->scheduleNodeHighlight(getReferenceNode(),false);
                    audioProcessor->scheduleNodeHighlight(getTargetNode(),   false);
                    break;
                }
            }
        }

        void handleRelayNodeEvent(int relayNodeId) {

            audioProcessor->scheduleNodeHighlight((*std::atomic_load(&audioProcessor->globalNodes) )[relayNodeId], false);
        }


    };


    /////                        EVENT MANAGER                       /////


    class EventManager {
    public:

        std::shared_ptr<std::unordered_map<int,TraversalLogic>> traversals;

        std::atomic<int> pendingAsyncCalls{0};
        struct MidiEvent { int pitch; int velocity; int duration; };
        struct ActiveNote{ MidiEvent event; int traversalId = 0; int remainingSamples; RTNode noteNode; };

        std::vector<ActiveNote> activeNotes;
        SequenceTreeAudioProcessor* processor;
        EventManager(SequenceTreeAudioProcessor* p) : processor(p) {}

        void handleEventStream(int sample, juce::MidiBuffer& midiMessages) {

            auto& loadedTraversals = *std::atomic_load(&traversals);
            auto& loadedGlobalNodes = *std::atomic_load(&processor->globalNodes);

            for (int i = activeNotes.size() - 1; i >= 0; --i) {
                ActiveNote& note = activeNotes[i];

                if (--note.remainingSamples > 0) { continue; }
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), sample);
                if (note.traversalId == -1)      { activeNotes.erase(activeNotes.begin() + i); continue; }

                int traversalId = note.traversalId;

                auto it = loadedTraversals.find(traversalId);
                if (it == loadedTraversals.end()) { continue;  }

                TraversalLogic& traversal = it->second;

                switch (note.noteNode.nodeType) {

                    case RTNode::NodeType::Node: {
                        traversal.handleNodeEvent();
                        if (traversal.shouldTraverse()) {
                            pushNote(traversal.getTargetNode(), traversalId, midiMessages);
                            for (int traverserId : traversal.traversers) { pushNote(loadedGlobalNodes.at(traverserId), traversalId, midiMessages); }
                        }
                        break;
                    }
                    case RTNode::NodeType::RelayNode : {
                        traversal.handleRelayNodeEvent(note.noteNode.nodeID);
                        if (traversal.shouldTraverse()) { pushConnectorNotes(note.noteNode.nodeID, midiMessages); }
                        break;
                    }
                    case RTNode::NodeType::Counter : { break; }
                }

                activeNotes.erase(activeNotes.begin() + i);
            }
        }

        void pushNote(RTNode node, int traversalId,juce::MidiBuffer& midiMessages)
        {

            int pitch = 63, velocity = 63, duration = 1000;

            if (!node.notes.empty()) {
                const RTNote& note = node.notes[0];
                pitch = note.pitch;
                velocity = note.velocity;
                duration = note.duration;
            }

            ActiveNote newNote;
            newNote.traversalId = traversalId;
            newNote.event.pitch = pitch;
            newNote.event.velocity = velocity;
            newNote.event.duration = duration;
            newNote.remainingSamples = static_cast<int>((duration / 1000.0) * processor->TempoInfo.currentSampleRate);
            newNote.noteNode = node;

            activeNotes.push_back(newNote);
            midiMessages.addEvent(juce::MidiMessage::noteOn(1, pitch, (juce::uint8)velocity), 0);
        }

        void pushConnectorNotes(int traverserId, juce::MidiBuffer& midiMessages) {

            auto& loadedGlobalNodes = *std::atomic_load(&processor->globalNodes);
            auto& loadedTraversals = *std::atomic_load(&traversals);

            auto& traverser = (loadedGlobalNodes)[traverserId];

            for (int connectorId : traverser.connectors){

                auto& connector = (loadedGlobalNodes)[connectorId];
                if (loadedTraversals.find(connector.graphID) != loadedTraversals.end()) {
                    highlightNode(loadedGlobalNodes.at(loadedTraversals.at(connector.graphID).targetId), false);
                    clearOldEvents(connector.graphID);
                }
                else { loadedTraversals.insert({connector.graphID,TraversalLogic(connector.graphID,processor)}); }

                auto& newTraversal = loadedTraversals.at(connector.graphID);
                newTraversal.targetId = connectorId;
                newTraversal.state = TraversalLogic::TraversalState::Active;
                highlightNode(newTraversal.getTargetNode(),true);
                pushNote(connector,connector.graphID,midiMessages);
            }
        }

        void clearOldEvents(int traversalId) {
            for (int i = activeNotes.size()-1; i >= 0; --i) {
                ActiveNote& note = activeNotes[i];
                if (note.traversalId == traversalId ) { note.traversalId = -1;}
            }
        }

        void highlightNode(const RTNode& node, bool shouldHighlight) {
            int nodeID = node.nodeID;
            int graphID = node.graphID;
            pendingAsyncCalls.fetch_add(1, std::memory_order_relaxed);

            juce::MessageManager::callAsync([this, nodeID, shouldHighlight, graphID]() {

                if(processor->canvas->nodeMaps.find(graphID) == processor->canvas->nodeMaps.end()){
                    return;
                }

                auto& nodeMap = processor->canvas->nodeMaps[graphID];

                auto it = nodeMap.find(nodeID);
                if (it != nodeMap.end()) {
                    Node* foundNode = nullptr;
                    foundNode = it->second;
                    foundNode->setHighlightVisual(shouldHighlight);
                }

                pendingAsyncCalls.fetch_sub(1, std::memory_order_relaxed);
            });
        }
    };

    struct {
        double currentSampleRate;
        int samplesPerBeat;
        int currentStep;
        int stepLengthInSamples;
        int samplesIntoStep;
        int bpm;
    } TempoInfo {44100,0,0,0,0,120};

    EventManager eventManager {this};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessor)
};
