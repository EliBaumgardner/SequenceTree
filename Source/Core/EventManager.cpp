#include "EventManager.h"
#include "PluginProcessor.h"

EventManager::EventManager(SequenceTreeAudioProcessor* p)
    : processor(p)
{
    jassert(processor != nullptr);
}


// ── Predicates ──

bool EventManager::isNodeAudible(RTNode::NodeType nodeType)
{
    return nodeType != RTNode::NodeType::Connector
        && nodeType != RTNode::NodeType::ModulatorRoot;
}

void EventManager::resetTraversal(int graphId, int newTargetId, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto existingIt = traversalMap.find(graphId);

    if (existingIt != traversalMap.end())
    {
        TraversalLogic& existingTraversal = existingIt->second;
        auto oldTargetIt = nodes.find(existingTraversal.targetId);

        if (oldTargetIt != nodes.end())
            highlightNode(oldTargetIt->second, false);

        clearOldEvents(graphId);
    }
    else
    {
        traversalMap.insert({ graphId, TraversalLogic(graphId, processor) });
    }

    TraversalLogic& traversal = traversalMap.at(graphId);
    traversal.targetId        = newTargetId;
    traversal.state           = TraversalLogic::TraversalState::Active;
}


// ── processEvents ──
//
// Event-driven scheduling: instead of iterating every sample and decrementing
// every note, we find the next-expiring note and jump directly to it.
// This turns O(samples * notes) into O(events_in_block * notes).

void EventManager::processEvents(int numSamples, juce::MidiBuffer& midiMessages, NodeMap& nodes, TraversalMap& traversalMap)
{
    // First pass: handle orphaned notes (node deleted from graph) at sample 0
    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        ActiveNote& activeNote = activeNotes[i];
        if (nodes.find(activeNote.noteNode.nodeID) != nodes.end())
            continue;

        if (isNodeAudible(activeNote.noteNode.nodeType))
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, activeNote.event.pitch), 0);

        int orphanedTraversalId = activeNote.traversalId;
        activeNotes[i] = std::move(activeNotes.back());
        activeNotes.pop_back();

        if (orphanedTraversalId == -1)
            continue;

        auto traversalIt = traversalMap.find(orphanedTraversalId);
        if (traversalIt == traversalMap.end())
            continue;

        TraversalLogic& traversal = traversalIt->second;
        auto rootIt = nodes.find(traversal.rootId);
        if (rootIt == nodes.end())
            continue;

        traversal.targetId = traversal.rootId;
        traversal.state    = TraversalLogic::TraversalState::Active;
        highlightNode(rootIt->second, true);
        pushNote(rootIt->second, orphanedTraversalId, midiMessages, 0, nodes, traversalMap);
    }

    // Event loop: process expiring notes in order until no more expire in this buffer
    while (true)
    {
        // Find the note that expires soonest within this buffer
        int bestIdx    = -1;
        int bestExpiry = numSamples + 1;

        for (int i = 0; i < static_cast<int>(activeNotes.size()); ++i)
        {
            if (activeNotes[i].remainingSamples < bestExpiry)
            {
                bestExpiry = activeNotes[i].remainingSamples;
                bestIdx    = i;
            }
        }

        if (bestIdx == -1 || bestExpiry > numSamples)
            break;

        int sample = bestExpiry;
        ActiveNote& activeNote = activeNotes[bestIdx];

        // Send note-off
        if (isNodeAudible(activeNote.noteNode.nodeType))
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, activeNote.event.pitch), sample);

        if (activeNote.traversalId == -1)
        {
            activeNotes[bestIdx] = std::move(activeNotes.back());
            activeNotes.pop_back();
            continue;
        }

        // Copy before swap — the reference is invalidated by swap-and-pop.
        ActiveNote expiredNote = activeNote;
        activeNotes[bestIdx] = std::move(activeNotes.back());
        activeNotes.pop_back();

        int traversalId = expiredNote.traversalId;
        auto traversalIt = traversalMap.find(traversalId);
        if (traversalIt == traversalMap.end())
            continue;

        TraversalLogic& traversal = traversalIt->second;

        switch (expiredNote.noteNode.nodeType)
        {
            case RTNode::NodeType::RootNode:
            case RTNode::NodeType::Node:
            case RTNode::NodeType::Modulator:
            {
                if (nodes.find(traversal.targetId) == nodes.end())
                    break;

                traversal.handleNodeEvent(nodes);

                if (traversal.shouldTraverse() && nodes.find(traversal.targetId) != nodes.end())
                    pushNote(traversal.getTargetNode(nodes), traversalId, midiMessages, sample, nodes, traversalMap);

                break;
            }
            case RTNode::NodeType::Connector:
            {
                if (nodes.find(expiredNote.noteNode.nodeID) == nodes.end())
                    break;

                traversal.handleConnectorNodeEvent(expiredNote.noteNode.nodeID, nodes);

                if (traversal.shouldTraverse())
                    pushConnectorNotes(expiredNote.noteNode.nodeID, midiMessages, sample, nodes, traversalMap);

                break;
            }
            case RTNode::NodeType::ModulatorRoot:
            {
                if (nodes.find(expiredNote.noteNode.nodeID) == nodes.end())
                    break;

                traversal.handleConnectorNodeEvent(expiredNote.noteNode.nodeID, nodes);

                if (traversal.shouldTraverse())
                    pushModulatorNotes(expiredNote.noteNode.nodeID, midiMessages, sample, nodes, traversalMap);

                break;
            }
        }
    }

    // Subtract elapsed time from all surviving notes
    for (auto& note : activeNotes)
        note.remainingSamples -= numSamples;
}

void EventManager::pushNote(const RTNode& node, int traversalId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto traversalIt = traversalMap.find(traversalId);
    jassert(traversalIt != traversalMap.end());

    RTNode* nextTargetNode = traversalIt->second.peekNextTarget(nodes);

    int pitch    = 63;
    int velocity = 63;
    int duration = 1000;

    if (!node.notes.empty())
    {
        const RTNote& noteData = node.notes[0];

        pitch    = static_cast<int>(noteData.pitch);
        velocity = static_cast<int>(noteData.velocity);
        duration = static_cast<int>(noteData.duration);

        if (nextTargetNode != nullptr)
        {
            auto durationIt = node.durationMap.find(nextTargetNode->nodeID);
            if (durationIt != node.durationMap.end())
                duration = durationIt->second;
        }

        if (nextTargetNode == nullptr && node.nodeType == RTNode::NodeType::Connector
            && !node.children.empty())
        {
            auto durationIt = node.durationMap.find(node.children[0]);
            if (durationIt != node.durationMap.end())
                duration = durationIt->second;
        }

        if (velocity <= 0)
            velocity = 63;
    }

    double sampleRate      = processor->TempoInfo.currentSampleRate;
    double tempoMultiplier = processor->tempoMultiplier.load();

    jassert(sampleRate > 0.0);
    jassert(tempoMultiplier > 0.0);

    ActiveNote newNote;
    newNote.traversalId      = traversalId;
    newNote.event.pitch      = pitch;
    newNote.event.velocity   = velocity;
    newNote.event.duration   = duration;
    newNote.remainingSamples = static_cast<int>((duration / 1000.0) * sampleRate / tempoMultiplier);
    newNote.noteNode         = node;

    activeNotes.push_back(newNote);

    if (isNodeAudible(node.nodeType))
        midiMessages.addEvent(juce::MidiMessage::noteOn(1, pitch, static_cast<juce::uint8>(velocity)), sample);

    if (node.nodeType == RTNode::NodeType::Node)
    {
        for (int relayId : traversalIt->second.peekTraversers(nodes))
        {
            auto relayIt = nodes.find(relayId);
            if (relayIt == nodes.end())
                continue;

            highlightNode(relayIt->second, true);
            pushNote(relayIt->second, traversalId, midiMessages, sample, nodes, traversalMap);
        }
    }
}

void EventManager::pushConnectorNotes(int traverserId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto traverserIt = nodes.find(traverserId);
    if (traverserIt == nodes.end())
        return;

    const RTNode& traverser = traverserIt->second;

    for (int connectorId : traverser.children)
    {
        auto connectorIt = nodes.find(connectorId);
        if (connectorIt == nodes.end())
            continue;

        const RTNode& connector = connectorIt->second;

        resetTraversal(connector.graphID, connectorId, nodes, traversalMap);

        auto targetIt = nodes.find(connectorId);
        if (targetIt != nodes.end())
            highlightNode(targetIt->second, true);

        pushNote(connector, connector.graphID, midiMessages, sample, nodes, traversalMap);
    }
}

void EventManager::pushModulatorNotes(int modulatorRootId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto rootIt = nodes.find(modulatorRootId);
    if (rootIt == nodes.end())
        return;

    int graphId = rootIt->second.graphID;

    resetTraversal(graphId, modulatorRootId, nodes, traversalMap);

    TraversalLogic& traversal = traversalMap.at(graphId);
    traversal.handleNodeEvent(nodes);

    if (traversal.shouldTraverse() && nodes.find(traversal.targetId) != nodes.end())
        pushNote(traversal.getTargetNode(nodes), graphId, midiMessages, sample, nodes, traversalMap);
}

void EventManager::clearOldEvents(int traversalId)
{
    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        if (activeNotes[i].traversalId == traversalId)
            activeNotes[i].traversalId = -1;
    }
}

void EventManager::highlightNode(const RTNode& node, bool shouldHighlight)
{
    const auto scope = highlightFifo.write(1);
    if (scope.blockSize1 > 0)
        highlightBuffer[static_cast<size_t>(scope.startIndex1)] = { node.nodeID, shouldHighlight };
    else if (scope.blockSize2 > 0)
        highlightBuffer[static_cast<size_t>(scope.startIndex2)] = { node.nodeID, shouldHighlight };
}