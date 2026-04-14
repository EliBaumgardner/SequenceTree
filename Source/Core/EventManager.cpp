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
        {
            highlightNode(oldTargetIt->second, false);
        }

        clearOldEvents(graphId);
        existingTraversal.resetCounts();
    }
    else
    {
        traversalMap.insert({ graphId, TraversalLogic(graphId, processor) });
    }

    TraversalLogic& traversal = traversalMap.at(graphId);
    traversal.targetId = newTargetId;
    traversal.state    = TraversalLogic::TraversalState::Active;
}


void EventManager::handleOrphanNotes(juce::MidiBuffer &midiMessages, NodeMap &nodes, TraversalMap &traversalMap)
{

    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        ActiveNote& activeNote = activeNotes[i];

        // 1. if node exists continue iterating to find orphan//
        if (nodes.find(activeNote.noteNode.nodeID) != nodes.end()){
            continue;
        }

        // 2. if node is audible send note off//
        if (isNodeAudible(activeNote.noteNode.nodeType) && !activeNote.isConnectionTrigger){
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, activeNote.event.pitch), 0);
        }

        // 3. remove note
        int orphanedTraversalId = activeNote.traversalId;
        activeNotes[i] = std::move(activeNotes.back());
        activeNotes.pop_back();


        // 4.if traversal has been deleted continue to next note
        if (orphanedTraversalId == -1){
            continue;
        }

        // 5. if traversal does not exist continue to next note
        auto traversalIt = traversalMap.find(orphanedTraversalId);
        if (traversalIt == traversalMap.end()){
            continue;
        }

        // 6. if root node does not exist continue to next note
        TraversalLogic& traversal = traversalIt->second;
        auto rootIt = nodes.find(traversal.rootId);
        if (rootIt == nodes.end()){
            continue;
        }

        // 7. reset traversal to root and push note on root
        traversal.targetId = traversal.rootId;
        traversal.state    = TraversalLogic::TraversalState::Active;
        highlightNode(rootIt->second, true);
        pushNote(rootIt->second, orphanedTraversalId, midiMessages, 0, nodes, traversalMap);
    }
}

void EventManager::handleNoteCreation(juce::MidiBuffer &midiMessages, NodeMap &nodes, TraversalMap &traversalMap, int priorityNoteDuration, EventManager::ActiveNote expiredNote, int traversalId, TraversalLogic &traversal, RTNode::NodeType type) {
    if (type == RTNode::NodeType::RootNode && expiredNote.isConnectionTrigger)
    {
        traversal.handleConnectorNodeEvent(expiredNote.noteNode.nodeID, nodes);

        if (traversal.shouldTraverse())
            pushRootNodeConnection(expiredNote.noteNode.nodeID, midiMessages, priorityNoteDuration, nodes, traversalMap);
    }
    else if (type == RTNode::NodeType::RootNode
        || type == RTNode::NodeType::Node
        || type == RTNode::NodeType::Modulator)
    {
        traversal.handleNodeEvent(nodes);

        if (traversal.shouldTraverse() && nodes.find(traversal.targetId) != nodes.end()){
            pushNote(traversal.getTargetNode(nodes), traversalId, midiMessages, priorityNoteDuration, nodes, traversalMap);
        }

    }
    else if (type == RTNode::NodeType::Connector)
    {
        traversal.handleConnectorNodeEvent(expiredNote.noteNode.nodeID, nodes);

        if (traversal.shouldTraverse()){
            pushConnectorNotes(expiredNote.noteNode.nodeID, midiMessages, priorityNoteDuration, nodes, traversalMap);
        }

    }
    else if (type == RTNode::NodeType::ModulatorRoot)
    {
        traversal.handleConnectorNodeEvent(expiredNote.noteNode.nodeID, nodes);

        if (traversal.shouldTraverse()){
            pushModulatorNotes(expiredNote.noteNode.nodeID, midiMessages, priorityNoteDuration, nodes, traversalMap);
        }
    }
}

void EventManager::processEvents(int numSamples, juce::MidiBuffer& midiMessages, NodeMap& nodes, TraversalMap& traversalMap)
{

    handleOrphanNotes(midiMessages, nodes, traversalMap);

    while (true)
    {
        int smallestNoteIndex    = -1;
        int smallestNoteSamples = numSamples + 1;

        // 1. find smallest note
        for (int i = 0; i < static_cast<int>(activeNotes.size()); ++i)
        {
            if (activeNotes[i].remainingSamples < smallestNoteSamples)
            {
                smallestNoteSamples = activeNotes[i].remainingSamples;
                smallestNoteIndex    = i;
            }
        }

        // 2. if no note is found within block continue decrementing note values as usual
        if (smallestNoteIndex == -1 || smallestNoteSamples > numSamples){
            break;
        }

        int priorityNoteDuration = smallestNoteSamples;
        ActiveNote& activeNote = activeNotes[smallestNoteIndex];

        // 3.  Send note-off in the future, when the note expires
        if (isNodeAudible(activeNote.noteNode.nodeType) && !activeNote.isConnectionTrigger){
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, activeNote.event.pitch), priorityNoteDuration);
        }

        // 4. if traversal has been removed, remove note and continue to next smallest note
        if (activeNote.traversalId == -1)
        {
            activeNotes[smallestNoteIndex] = std::move(activeNotes.back());
            activeNotes.pop_back();
            continue;
        }

        ActiveNote expiredNote = activeNote;
        activeNotes[smallestNoteIndex] = std::move(activeNotes.back());
        activeNotes.pop_back();

        int traversalId = expiredNote.traversalId;
        auto traversalIt = traversalMap.find(traversalId);

        // 5. if traversal does not exist continue to next smallest note
        if (traversalIt == traversalMap.end()){
            continue;
        }

        TraversalLogic& traversal = traversalIt->second;

        auto type = expiredNote.noteNode.nodeType;

        jassert(nodes.find(traversal.targetId) != nodes.end());

        // 6. handle note creation depending the node type and the priority note duration left
        handleNoteCreation(midiMessages, nodes, traversalMap, priorityNoteDuration, expiredNote, traversalId, traversal, type);
    }

    // 7. decrement remaining samples of all active notes
    for (auto& note : activeNotes)
        note.remainingSamples -= numSamples;
}

void EventManager::pushNote(const RTNode& node, int traversalId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto traversalIt = traversalMap.find(traversalId);
    jassert(traversalIt != traversalMap.end());

    RTNode* nextTargetNode = traversalIt->second.peekNextTarget(nodes);

    // 1. set note values from RTNode
    int pitch    = 63;
    int velocity = 63;
    int duration = 1000;

    if (!node.notes.empty())
    {
        const RTNote& noteData = node.notes[0];

        pitch    = static_cast<int>(noteData.pitch);
        velocity = static_cast<int>(noteData.velocity);
        duration = static_cast<int>(noteData.duration);

        if (velocity <= 0){
            velocity = 63;
        }
    }
    // 2. If next target node exists apply its associated duration value, if the node is a connector it should find the root node
    if (node.nodeType == RTNode::NodeType::Connector)
    {
        if (!node.children.empty())
        {
            auto durationIt = node.durationMap.find(node.children[0]);
            if (durationIt != node.durationMap.end()){
                duration = durationIt->second;
            }
        }
    }
    else if (nextTargetNode != nullptr)
    {
        auto durationIt = node.durationMap.find(nextTargetNode->nodeID);
        if (durationIt != node.durationMap.end()){
            duration = durationIt->second;
        }
    }

    double sampleRate      = processor->TempoInfo.currentSampleRate;
    double tempoMultiplier = processor->tempoMultiplier.load();

    jassert(sampleRate > 0.0);
    jassert(tempoMultiplier > 0.0);

    // 3. create an ActiveNote to store information and push to FIFO to be processed in processEvents
    ActiveNote newNote;
    newNote.traversalId      = traversalId;
    newNote.event.pitch      = pitch;
    newNote.event.velocity   = velocity;
    newNote.event.duration   = duration;
    newNote.remainingSamples = static_cast<int>((duration / 1000.0) * sampleRate / tempoMultiplier);
    newNote.noteNode         = node;

    activeNotes.push_back(newNote);

    if (nextTargetNode != nullptr
        && (node.nodeType == RTNode::NodeType::Node
            || node.nodeType == RTNode::NodeType::RootNode
            || node.nodeType == RTNode::NodeType::Modulator))
    {
        const int wallClockDurationMs = static_cast<int>(duration / tempoMultiplier);
        pushProgress(node.nodeID, nextTargetNode->nodeID, wallClockDurationMs);
    }

    // 4. if node should produce MIDI then turn on here
    if (isNodeAudible(node.nodeType)){
        midiMessages.addEvent(juce::MidiMessage::noteOn(1, pitch, static_cast<juce::uint8>(velocity)), sample);
    }

    if (node.nodeType == RTNode::NodeType::Node)
    {
        for (int connectorId : traversalIt->second.peekTraversers(nodes))
        {
            auto connectorIterator = nodes.find(connectorId);
            if (connectorIterator == nodes.end()){
                continue;
            }

            const RTNode& connectorNode = connectorIterator->second;
            highlightNode(connectorNode, true);

            if (connectorNode.nodeType == RTNode::NodeType::RootNode)
            {
                // Push a silent connection trigger — its duration is the arrow length from
                // this node to the root node, after which traversal starts on that root's graph.
                auto durIt = node.durationMap.find(connectorId);
                int connectionDuration = (durIt != node.durationMap.end()) ? durIt->second : 1000;

                ActiveNote triggerNote;
                triggerNote.traversalId         = traversalId;
                triggerNote.noteNode            = connectorNode;
                triggerNote.isConnectionTrigger = true;
                triggerNote.event.duration      = connectionDuration;
                triggerNote.remainingSamples    = static_cast<int>((connectionDuration / 1000.0) * sampleRate / tempoMultiplier);
                activeNotes.push_back(triggerNote);
            }
            else
            {
                pushNote(connectorNode, traversalId, midiMessages, sample, nodes, traversalMap);
            }
        }
    }
}

void EventManager::pushConnectorNotes(int traverserId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto traverserIt = nodes.find(traverserId);

    if (traverserIt == nodes.end()){
        return;
    }

    const RTNode& traverser = traverserIt->second;

    for (int connectorId : traverser.children)
    {
        auto connectorIt = nodes.find(connectorId);
        if (connectorIt == nodes.end()){
            continue;
        }

        const RTNode& connector = connectorIt->second;

        resetTraversal(connector.graphID, connectorId, nodes, traversalMap);

        auto targetIt = nodes.find(connectorId);
        if (targetIt != nodes.end()){
            highlightNode(targetIt->second, true);
        }

        pushNote(connector, connector.graphID, midiMessages, sample, nodes, traversalMap);
    }
}

void EventManager::pushModulatorNotes(int modulatorRootId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto rootIt = nodes.find(modulatorRootId);
    if (rootIt == nodes.end()){
        return;
    }

    int graphId = rootIt->second.graphID;

    resetTraversal(graphId, modulatorRootId, nodes, traversalMap);

    TraversalLogic& traversal = traversalMap.at(graphId);
    traversal.handleNodeEvent(nodes);

    if (traversal.shouldTraverse() && nodes.find(traversal.targetId) != nodes.end()){
        pushNote(traversal.getTargetNode(nodes), graphId, midiMessages, sample, nodes, traversalMap);
    }
}

void EventManager::clearOldEvents(int traversalId)
{
    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        if (activeNotes[i].traversalId == traversalId){
            activeNotes[i].traversalId = -1;
        }
    }
}

void EventManager::pushRootNodeConnection(int rootNodeId, juce::MidiBuffer& midiMessages, int sample, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto rootIt = nodes.find(rootNodeId);
    if (rootIt == nodes.end()) return;

    const RTNode& rootNode = rootIt->second;

    resetTraversal(rootNode.graphID, rootNodeId, nodes, traversalMap);

    // Apply the same loop settings a standalone root gets, so the connected tree
    // loops as configured by its root rectangle.
    TraversalLogic& traversal = traversalMap.at(rootNode.graphID);
    traversal.isLooping = true;
    traversal.loopCount = 0;

    auto snap = std::atomic_load(&processor->audioSnapshot);
    if (snap && snap->rtGraphs) {
        auto rtGraphIt = snap->rtGraphs->find(rootNode.graphID);
        if (rtGraphIt != snap->rtGraphs->end())
            traversal.loopLimit = rtGraphIt->second->loopLimit;
    }

    highlightNode(rootNode, true);
    pushNote(rootNode, rootNode.graphID, midiMessages, sample, nodes, traversalMap);
}

void EventManager::highlightNode(const RTNode& node, bool shouldHighlight)
{
    const auto scope = highlightFifo.write(1);
    if (scope.blockSize1 > 0)
    {
        highlightBuffer[static_cast<size_t>(scope.startIndex1)] = { node.nodeID, shouldHighlight };
    }
    else if (scope.blockSize2 > 0)
    {
        highlightBuffer[static_cast<size_t>(scope.startIndex2)] = { node.nodeID, shouldHighlight };
    }
    else
    {
        jassertfalse;
    }
}

void EventManager::pushProgress(int parentNodeId, int childNodeId, int durationMs)
{
    const auto scope = progressFifo.write(1);
    if (scope.blockSize1 > 0)
    {
        progressBuffer[static_cast<size_t>(scope.startIndex1)] = { parentNodeId, childNodeId, durationMs };
    }
    else if (scope.blockSize2 > 0)
    {
        progressBuffer[static_cast<size_t>(scope.startIndex2)] = { parentNodeId, childNodeId, durationMs };
    }
    else
    {
        jassertfalse;
    }
}