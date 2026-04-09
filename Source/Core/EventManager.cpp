#include "EventManager.h"
#include "PluginProcessor.h"

EventManager::EventManager(SequenceTreeAudioProcessor* p) : processor(p) {}

void EventManager::handleEventStream(int sample, juce::MidiBuffer& midiMessages, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap)
{
    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i) {

        ActiveNote& note = activeNotes[i];

        if (nodes.find(note.noteNode.nodeID) == nodes.end()) {
            if (note.noteNode.nodeType != RTNode::NodeType::Connector
                && note.noteNode.nodeType != RTNode::NodeType::ModulatorRoot) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), sample);
            }

            int deletedTraversalId = note.traversalId;
            activeNotes.erase(activeNotes.begin() + i);

            if (deletedTraversalId != -1) {
                auto traversalIt = traversalMap.find(deletedTraversalId);
                if (traversalIt != traversalMap.end()) {
                    TraversalLogic& traversal = traversalIt->second;
                    auto rootIt = nodes.find(traversal.rootId);
                    if (rootIt != nodes.end()) {
                        traversal.targetId = traversal.rootId;
                        traversal.state    = TraversalLogic::TraversalState::Active;
                        highlightNode(rootIt->second, true);
                        pushNote(rootIt->second, deletedTraversalId, midiMessages, sample, nodes, traversalMap);
                    }
                }
            }
            continue;
        }

        if (--note.remainingSamples > 0) { continue; }


        if (note.noteNode.nodeType != RTNode::NodeType::Connector
            && note.noteNode.nodeType != RTNode::NodeType::ModulatorRoot) {
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), sample);
        }

        if (note.traversalId == -1) {
            activeNotes.erase(activeNotes.begin() + i);
            continue;
        }

        auto tempNote = note;
        activeNotes.erase(activeNotes.begin() + i);

        int traversalId = tempNote.traversalId;
        auto it = traversalMap.find(traversalId);
        if (it == traversalMap.end()) {
            continue;
        }
        TraversalLogic& traversal = it->second;

        switch (tempNote.noteNode.nodeType) {
            case RTNode::NodeType::Node:
            case RTNode::NodeType::Modulator: {
                if (nodes.find(traversal.targetId) == nodes.end())
                    break;
                traversal.handleNodeEvent(nodes);
                if (traversal.shouldTraverse() && nodes.find(traversal.targetId) != nodes.end())
                    pushNote(traversal.getTargetNode(nodes), traversalId, midiMessages, sample, nodes, traversalMap);
                break;
            }
            case RTNode::NodeType::Connector: {
                if (nodes.find(tempNote.noteNode.nodeID) == nodes.end())
                    break;
                traversal.handleConnectorNodeEvent(tempNote.noteNode.nodeID, nodes);
                if (traversal.shouldTraverse())
                    pushConnectorNotes(tempNote.noteNode.nodeID, midiMessages, sample, nodes, traversalMap);
                break;
            }
            case RTNode::NodeType::ModulatorRoot: {
                if (nodes.find(tempNote.noteNode.nodeID) == nodes.end())
                    break;
                traversal.handleConnectorNodeEvent(tempNote.noteNode.nodeID, nodes);
                if (traversal.shouldTraverse())
                    pushModulatorNotes(tempNote.noteNode.nodeID, midiMessages, sample, nodes, traversalMap);
                break;
            }
        }
    }
}


void EventManager::pushNote(RTNode node, int traversalId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap)
{
    RTNode* nextTargetNode = nullptr;

    auto it = traversalMap.find(traversalId);

    if (it != traversalMap.end()) {
        nextTargetNode = it->second.peekNextTarget(nodes);
    }

    int pitch = 63, velocity = 63, duration = 1000;

    if (!node.notes.empty()) {
        const RTNote& note = node.notes[0];

        pitch    = static_cast<int>(note.pitch);
        velocity = static_cast<int>(note.velocity);
        duration = static_cast<int>(note.duration);

        if (node.nodeType == RTNode::NodeType::Connector
            || node.nodeType == RTNode::NodeType::ModulatorRoot) {
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
        }
    }

    ActiveNote newNote;
    newNote.traversalId        = traversalId;
    newNote.event.pitch        = pitch;
    newNote.event.velocity     = velocity;
    newNote.event.duration     = duration;
    newNote.remainingSamples   = static_cast<int>((duration / 1000.0) * processor->TempoInfo.currentSampleRate / processor->tempoMultiplier.load());
    newNote.noteNode           = node;

    activeNotes.push_back(newNote);
    if (node.nodeType != RTNode::NodeType::Connector
        && node.nodeType != RTNode::NodeType::ModulatorRoot) {
        midiMessages.addEvent(juce::MidiMessage::noteOn(1, pitch, static_cast<juce::uint8>(velocity)), sample);
    }

    if (node.nodeType == RTNode::NodeType::Node || node.nodeType == RTNode::NodeType::Modulator) {
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
    auto traverserIt = nodes.find(traverserId);
    if (traverserIt == nodes.end()) return;
    const RTNode& traverser = traverserIt->second;

    for (int connectorId : traverser.connectors) {
        auto connectorIt = nodes.find(connectorId);
        if (connectorIt == nodes.end()) continue;
        const RTNode& connector = connectorIt->second;

        if (traversalMap.find(connector.graphID) != traversalMap.end()) {
            auto& existingTraversal = traversalMap.at(connector.graphID);
            auto  oldTargetIt       = nodes.find(existingTraversal.targetId);
            if (oldTargetIt != nodes.end())
                highlightNode(oldTargetIt->second, false);
            clearOldEvents(connector.graphID);
        } else {
            traversalMap.insert({connector.graphID, TraversalLogic(connector.graphID, processor)});
        }

        auto& newTraversal    = traversalMap.at(connector.graphID);
        newTraversal.targetId = connectorId;
        newTraversal.state    = TraversalLogic::TraversalState::Active;

        auto newTargetIt = nodes.find(newTraversal.targetId);
        if (newTargetIt != nodes.end())
            highlightNode(newTargetIt->second, true);

        pushNote(connector, connector.graphID, midiMessages, sample, nodes, traversalMap);
    }
}

void EventManager::pushModulatorNotes(int modulatorRootId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, std::unordered_map<int, TraversalLogic>& traversalMap)
{
    auto rootIt = nodes.find(modulatorRootId);
    if (rootIt == nodes.end()) return;

    int graphId = rootIt->second.graphID; // equals modulatorRootId (self-rooted)

    if (traversalMap.find(graphId) != traversalMap.end()) {
        auto& existingTraversal = traversalMap.at(graphId);
        auto  oldTargetIt       = nodes.find(existingTraversal.targetId);
        if (oldTargetIt != nodes.end())
            highlightNode(oldTargetIt->second, false);
        clearOldEvents(graphId);
    } else {
        traversalMap.insert({graphId, TraversalLogic(graphId, processor)});
    }

    auto& traversal    = traversalMap.at(graphId);
    traversal.targetId = modulatorRootId;
    traversal.state    = TraversalLogic::TraversalState::Active;

    traversal.handleNodeEvent(nodes); // advances past the structural root to the first child
    if (traversal.shouldTraverse() && nodes.find(traversal.targetId) != nodes.end())
        pushNote(traversal.getTargetNode(nodes), graphId, midiMessages, sample, nodes, traversalMap);
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