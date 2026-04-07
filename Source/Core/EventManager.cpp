#include "EventManager.h"
#include "PluginProcessor.h"

EventManager::EventManager(SequenceTreeAudioProcessor* p) : processor(p) {}

void EventManager::handleEventStream(int sample, juce::MidiBuffer& midiMessages, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap)
{
    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i) {

        if (traversalMap.empty()) {
            DBG("First Note Not Already Made!");
            handleFirstTraversal(sample, midiMessages, nodes, traversalMap);
            continue;
        }

        ActiveNote& note = activeNotes[i];
        if (--note.remainingSamples > 0) { continue; }

        DBG("Note End Reached at Sample:" + juce::String(sample));
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), sample);

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
            continue;
        }
        TraversalLogic& traversal = it->second;

        switch (tempNote.noteNode.nodeType) {
            case RTNode::NodeType::Node: {
                traversal.handleNodeEvent(nodes);
                if (traversal.shouldTraverse())
                    pushNote(traversal.getTargetNode(nodes), traversalId, midiMessages, sample, nodes, traversalMap);
                break;
            }
            case RTNode::NodeType::RelayNode: {
                traversal.handleRelayNodeEvent(tempNote.noteNode.nodeID, nodes);
                if (traversal.shouldTraverse())
                    pushConnectorNotes(tempNote.noteNode.nodeID, midiMessages, sample, nodes, traversalMap);
                break;
            }
            case RTNode::NodeType::Counter: { break; }
        }
    }
}

void EventManager::handleFirstTraversal(int sample, juce::MidiBuffer& midiMessages, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap)
{
    traversalMap.insert({1, TraversalLogic(1, processor)});
    TraversalLogic& traversal = traversalMap.at(1);
    traversal.targetId  = traversal.rootId;
    traversal.state     = TraversalLogic::TraversalState::Active;
    traversal.isLooping = true;

    RTNode& rootNode   = nodes[traversal.rootId];
    int     traversalId = rootNode.nodeID;

    highlightNode(rootNode, true);
    pushNote(rootNode, traversalId, midiMessages, sample, nodes, traversalMap);
}

void EventManager::pushNote(RTNode node, int traversalId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap)
{
    RTNode* nextTargetNode = nullptr;
    auto it = traversalMap.find(traversalId);
    if (it != traversalMap.end())
        nextTargetNode = it->second.peekNextTarget(nodes);

    int pitch = 63, velocity = 63, duration = 1000;

    if (!node.notes.empty()) {
        const RTNote& note = node.notes[0];

        pitch    = static_cast<int>(note.pitch);
        velocity = static_cast<int>(note.velocity);
        duration = static_cast<int>(note.duration);

        if (node.nodeType == RTNode::NodeType::RelayNode) {
            // Duration comes from the parent node's arrow to this relay node.
            // targetId is still the parent here — advance() hasn't run yet.
            auto tIt = traversalMap.find(traversalId);
            if (tIt != traversalMap.end()) {
                auto parentIt = nodes.find(tIt->second.targetId);
                if (parentIt != nodes.end()) {
                    auto dIt = parentIt->second.durationMap.find(node.nodeID);
                    if (dIt != parentIt->second.durationMap.end())
                        duration = dIt->second;
                }
            }
        } else if (nextTargetNode != nullptr) {
            auto durationIt = node.durationMap.find(nextTargetNode->nodeID);
            if (durationIt != node.durationMap.end())
                duration = durationIt->second;
        }

        if (velocity <= 0) {
            velocity = 63;
            DBG("NOTE OFF MESSAGE SENT TO PUSHNOTE!");
        }
    } else {
        DBG("NO NODE NOTES FOUND!");
    }

    ActiveNote newNote;
    newNote.traversalId        = traversalId;
    newNote.event.pitch        = pitch;
    newNote.event.velocity     = velocity;
    newNote.event.duration     = duration;
    newNote.remainingSamples   = static_cast<int>((duration / 1000.0) * processor->TempoInfo.currentSampleRate / processor->tempoMultiplier.load());
    newNote.noteNode           = node;

    activeNotes.push_back(newNote);
    midiMessages.addEvent(juce::MidiMessage::noteOn(1, pitch, static_cast<juce::uint8>(velocity)), sample);

    // When pushing a regular node, immediately push any relay notes that will fire during it.
    // They use the arrow duration from this node to each relay (independent of this note's length).
    if (node.nodeType == RTNode::NodeType::Node) {
        auto tIt = traversalMap.find(traversalId);
        if (tIt != traversalMap.end()) {
            for (int relayId : tIt->second.peekTraversers(nodes)) {
                auto relayIt = nodes.find(relayId);
                if (relayIt == nodes.end()) continue;
                highlightNode(relayIt->second, true);
                pushNote(relayIt->second, traversalId, midiMessages, sample, nodes, traversalMap);
            }
        }
    }
}

void EventManager::pushConnectorNotes(int traverserId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap)
{
    auto& traverser = nodes[traverserId];

    for (int connectorId : traverser.connectors) {
        auto& connector = nodes[connectorId];

        if (traversalMap.find(connector.graphID) != traversalMap.end()) {
            highlightNode(nodes.at(traversalMap.at(connector.graphID).targetId), false);
            clearOldEvents(connector.graphID);
        } else {
            traversalMap.insert({connector.graphID, TraversalLogic(connector.graphID, processor)});
        }

        auto& newTraversal      = traversalMap.at(connector.graphID);
        newTraversal.targetId   = connectorId;
        newTraversal.state      = TraversalLogic::TraversalState::Active;
        highlightNode(newTraversal.getTargetNode(nodes), true);
        pushNote(connector, connector.graphID, midiMessages, sample, nodes, traversalMap);
    }
}

void EventManager::clearOldEvents(int traversalId)
{
    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i) {
        if (activeNotes[i].traversalId == traversalId)
            activeNotes[i].traversalId = -1;
    }
}

void EventManager::highlightNode(const RTNode& node, bool shouldHighlight)
{
    int nodeID = node.nodeID;
    pendingAsyncCalls.fetch_add(1, std::memory_order_relaxed);

    juce::MessageManager::callAsync([this, nodeID, shouldHighlight]() {
        auto& nodeMap = processor->canvas->nodeMap;
        auto  it      = nodeMap.find(nodeID);

        if (it != nodeMap.end())
            it->second->setHighlightVisual(shouldHighlight);

        pendingAsyncCalls.fetch_sub(1, std::memory_order_relaxed);
    });
}