/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../Util/PluginModules.h"
#include "../Util/PluginContext.h"

//==============================================================================
/**
*/
#include <memory>
#include <atomic>
#include "../Util/RTData.h"
#include "Node/NodeCanvas.h"
#include "Node/Node.h"

class SequenceTreeAudioProcessorEditor;

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

    void clearOldEvents(RTNode node, int traversalId);

    void setNewGraph(std::shared_ptr<RTGraph> graph);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    NodeCanvas* canvas;

    using RTGraphs = std::unordered_map<int,std::shared_ptr<RTGraph>>;
    std::shared_ptr<RTGraphs> rtGraphs = nullptr;

    using NodeMap = std::unordered_map<int, RTNode>;

    int numGraphs = 0;

    std::shared_ptr<NodeMap> globalNodes = nullptr;

    std::atomic<bool>   isPlaying      = false;
    std::atomic<bool>   resetRequested = false;
    std::atomic<double> tempoMultiplier { 1.0 };

    juce::AudioProcessorValueTreeState valueTreeState;




    /////                        TRAVERSAL LOGIC                       /////


    class TraversalLogic {
    public:

        std::unordered_map<int,int> counts;
        std::vector<int> traversers;

        bool isFirstEvent = false;
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

        void advance(NodeMap& nodes)
        {
            traversers.clear();

            referenceTargetId = lastTargetId;
            lastTargetId = targetId;
            int count = ++counts[targetId];
            auto itTarget = nodes.find(targetId);

            if (itTarget->second.children.empty() && itTarget->second.connectors.empty()) {
                if (isLooping) { state = TraversalState::Reset; return; }
                state = TraversalState::End; return;
            }

            int maxLimit = 0;

            for (int childIndex : itTarget->second.children) {
                auto itChild = nodes.find(childIndex);
                auto childNode = itChild->second;
                int limit = childNode.countLimit;

                switch (childNode.nodeType) {
                    case(RTNode::NodeType::Node): {
                        if (count % limit == 0 && limit > maxLimit) {
                            targetId = childIndex;
                            maxLimit = limit;
                        }
                        break;
                    }
                    case(RTNode::NodeType::RelayNode): {
                        if (count % limit == 0) {
                            traversers.push_back(childIndex);
                        }
                        break;
                    }
                }
            }

            for (int connectorIndex : itTarget->second.connectors) {
                auto itConnector = nodes.find(connectorIndex);
                if (itConnector == nodes.end()) { continue; }
                auto connectorNode = itConnector->second;
                int limit = connectorNode.countLimit;
                if (count % limit == 0) {
                    traversers.push_back(connectorIndex);
                }
            }

            if (targetId == lastTargetId) {
                if (isLooping) { state = TraversalState::Reset; }
                else { state = TraversalState::End; }
            }
        }

        const RTNode& getTargetNode(const NodeMap& nodes)    { return nodes.at(targetId);          }
        const RTNode& getLastNode(const NodeMap& nodes)      { return nodes.at(lastTargetId);      }
        const RTNode& getReferenceNode(const NodeMap& nodes) { return nodes.at(referenceTargetId); }
        const RTNode& getRootNode(const NodeMap& nodes)      { return nodes.at(rootId);            }

        const RTNode& getRelayNode(int relayNodeId, const NodeMap& nodes) { return nodes.at(relayNodeId); }

        bool shouldTraverse()
        {
            switch (state) {
                case TraversalState::Start  :   { return true; }
                case TraversalState::Active :   { return true; }
                case TraversalState::Reset  :   { return true; }
                case TraversalState::End    :   { return false;}
            }
        }

        void handleNodeEvent(NodeMap& nodes)
        {

            switch (state) {
                case TraversalState::Start : {
                    state = TraversalState::Active;
                    targetId = rootId;
                    audioProcessor->eventManager.highlightNode(getTargetNode(nodes), true);
                    break;
                }
                case TraversalState::Active: {
                    advance(nodes);

                    for (int traverserId : traversers) {
                        audioProcessor->eventManager.highlightNode(nodes.at(traverserId), true);
                    }

                    switch (state) {
                        case TraversalState::Active: {
                            audioProcessor->eventManager.highlightNode(getLastNode(nodes)  ,  false);
                            audioProcessor->eventManager.highlightNode(getTargetNode(nodes),   true);
                            break;
                        }
                        case TraversalState::Reset: {
                            audioProcessor->eventManager.highlightNode(getTargetNode(nodes),   false);
                            audioProcessor->eventManager.highlightNode(getRootNode(nodes)  ,    true);
                            targetId = rootId;
                            state = TraversalState::Active;
                            break;
                        }
                        case TraversalState::End: {
                            audioProcessor->eventManager.highlightNode(getTargetNode(nodes)   ,false);
                            audioProcessor->eventManager.highlightNode(getReferenceNode(nodes),false);
                        }
                    }
                    break;
                }
                case TraversalState::End: {
                    audioProcessor->eventManager.highlightNode(getReferenceNode(nodes),false);
                    audioProcessor->eventManager.highlightNode(getTargetNode(nodes),   false);
                    break;
                }
            }
        }

        void handleRelayNodeEvent(int relayNodeId, const NodeMap& nodes)
        {
           audioProcessor->eventManager.highlightNode(nodes.at(relayNodeId), false);
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

        void handleEventStream(int sample, juce::MidiBuffer& midiMessages, NodeMap& nodes, std::unordered_map<int,TraversalLogic>& traversalMap)
        {
            for (int i = activeNotes.size() - 1; i >= 0; --i) {

                if (traversalMap.empty()){
                  DBG("First Note Not Already Made!");
                   handleFirstTraversal(sample, midiMessages, nodes, traversalMap); continue;
                }

                ActiveNote& note = activeNotes[i];

                if (--note.remainingSamples > 0) { continue; }

                DBG("Note End Reached at Sample:" + juce::String(sample));

                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch),sample);

                if (note.traversalId == -1) {
                    DBG("Note Discarded at Sample:" + juce::String(sample));
                    activeNotes.erase(activeNotes.begin() + i);
                    continue;
                }

                auto tempNote = note;
                activeNotes.erase(activeNotes.begin() + i);

                int traversalId = tempNote.traversalId;

                auto it = traversalMap.find(traversalId);
                if (it == traversalMap.end()) {
                    DBG("TRAVERSAL NOT FOUND! SKIPPING TRAVERSAL");
                    continue;  }
                TraversalLogic& traversal = it->second;

                switch (tempNote.noteNode.nodeType) {

                    case RTNode::NodeType::Node: {

                        traversal.handleNodeEvent(nodes);
                        if (traversal.shouldTraverse()) {
                            pushNote(traversal.getTargetNode(nodes), traversalId, midiMessages, sample);
                            for (int traverserId : traversal.traversers) {
                                pushNote(nodes.at(traverserId), traversalId, midiMessages, sample);
                            }
                        }
                        break;
                    }
                    case RTNode::NodeType::RelayNode : {
                        traversal.handleRelayNodeEvent(tempNote.noteNode.nodeID, nodes);
                        if (traversal.shouldTraverse()) { pushConnectorNotes(tempNote.noteNode.nodeID, midiMessages, sample, nodes, traversalMap); }
                        break;
                    }
                    case RTNode::NodeType::Counter : { break; }
                }
            }
        }

        void handleFirstTraversal(int sample, juce::MidiBuffer& midiMessages, NodeMap& nodes, std::unordered_map<int,TraversalLogic>& traversalMap)
        {
            traversalMap.insert({1,TraversalLogic(1,processor)});
            TraversalLogic& traversal = traversalMap.at(1);
            traversal.targetId = traversal.rootId;
            traversal.state = TraversalLogic::TraversalState::Active;
            traversal.isLooping = true;

            RTNode& rootNode = nodes[traversal.rootId];
            highlightNode(rootNode,true);

            int traversalId = rootNode.nodeID;
            pushNote(rootNode,traversalId,midiMessages,sample);
        }

        void pushNote(RTNode node, int traversalId,juce::MidiBuffer& midiMessages,int sample)
        {
            int pitch = 63, velocity = 63, duration = 1000;


            if (!node.notes.empty())
            {   const RTNote& note = node.notes[0];
                pitch = note.pitch;
                velocity = note.velocity;
                duration = note.duration;

                if (velocity <= 0)
                {   velocity = 63;

                    DBG("NOTE OFF MESSAGE SENT TO PUSHNOTE!");
                }

            }
            else
            {
                DBG("NO NODE NOTES FOUND!");
            }

            ActiveNote newNote;
            newNote.traversalId = traversalId;
            newNote.event.pitch = pitch;
            newNote.event.velocity = velocity;
            newNote.event.duration = duration;
            newNote.remainingSamples = static_cast<int>((duration / 1000.0) * processor->TempoInfo.currentSampleRate / processor->tempoMultiplier.load());
            newNote.noteNode = node;

            activeNotes.push_back(newNote);
            midiMessages.addEvent(juce::MidiMessage::noteOn(1, pitch, (juce::uint8)velocity), sample);
        }

        void pushConnectorNotes(int traverserId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, std::unordered_map<int,TraversalLogic>& traversalMap)
        {
            auto& traverser = nodes[traverserId];

            for (int connectorId : traverser.connectors){

                auto& connector = nodes[connectorId];
                if (traversalMap.find(connector.graphID) != traversalMap.end()) {
                    highlightNode(nodes.at(traversalMap.at(connector.graphID).targetId), false);
                    clearOldEvents(connector.graphID);
                }
                else { traversalMap.insert({connector.graphID,TraversalLogic(connector.graphID,processor)}); }

                auto& newTraversal = traversalMap.at(connector.graphID);
                newTraversal.targetId = connectorId;
                newTraversal.state = TraversalLogic::TraversalState::Active;
                highlightNode(newTraversal.getTargetNode(nodes),true);
                pushNote(connector,connector.graphID,midiMessages, sample);
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
                auto& nodeMap = processor->canvas->nodeMap;

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