#include "TraversalDispatcher.h"
#include "AudioUIBridge.h"
#include "../Plugin/PluginProcessor.h"

TraversalDispatcher::TraversalDispatcher(SequenceTreeAudioProcessor& p,
                                         NoteScheduler& s,
                                         AudioUIBridge& b)
    : processor(p), scheduler(s), bridge(b)
{
}

int TraversalDispatcher::resolveDuration(const RTNode& node, const RTNode* nextTarget,
                                          int lastTargetId, const NodeMap& nodes)
{
    int duration = 1000;

    if (!node.notes.empty())
        duration = static_cast<int>(node.notes[0].duration);

    if (node.nodeType == RTNode::NodeType::Connector)
    {
        if (!node.children.empty())
        {
            auto it = node.durationMap.find(node.children[0]);
            if (it != node.durationMap.end())
                duration = it->second;
        }
    }
    else if (nextTarget != nullptr)
    {
        auto it = node.durationMap.find(nextTarget->nodeID);
        if (it != node.durationMap.end() && it->second > 0)
            duration = it->second;
    }
    else
    {
        auto parentIt = nodes.find(lastTargetId);
        if (parentIt != nodes.end())
        {
            auto durIt = parentIt->second.durationMap.find(node.nodeID);
            if (durIt != parentIt->second.durationMap.end() && durIt->second > 0)
                duration = durIt->second;
        }
    }

    return duration;
}

void TraversalDispatcher::pushNote(const RTNode& node, int traversalId,
                                   juce::MidiBuffer& midiMessages, int sample,
                                   NodeMap& nodes, TraversalMap& traversalMap)
{
    std::unordered_map<int,TraversalLogic>::iterator traversalIterator = traversalMap.find(traversalId);
    
    jassert(traversalIterator != traversalMap.end());

    TraversalLogic& traversalLogic = traversalIterator->second;

    RTNode::NodeType nodeType = node.nodeType;

    RTNode* modulatorNode = nullptr;

    dispatchModulator(node, nodes, traversalLogic, modulatorNode, traversalMap);

    RTNode* nextTarget = traversalLogic.peekNextTarget(nodes);
    int duration = resolveDuration(node, nextTarget, traversalLogic.lastTargetId, nodes);

    RTNode* nextModulatorTarget = nullptr;

    if (modulatorNode != nullptr && traversalLogic.targetModulatorId != -1) {
        nextModulatorTarget = traversalLogic.peekModulators(nodes);
        int modulatorDuration = resolveDuration(*modulatorNode, nextModulatorTarget,
                                                traversalLogic.lastModulatorId, nodes);
        double modulationAmount = 0.001 * modulatorDuration;
        duration = static_cast<int>(duration * modulationAmount);
    }

    double sampleRate      = processor.TempoInfo.currentSampleRate;
    double tempoMultiplier = processor.tempoMultiplier.load();

    jassert(sampleRate > 0.0);
    jassert(tempoMultiplier > 0.0);

    scheduler.scheduleNote(node, traversalId, sample, midiMessages, sampleRate, tempoMultiplier, duration);

    pushChordNotes(node, sample, duration, midiMessages, sampleRate, tempoMultiplier, nodes);

    const int wallClockMs = static_cast<int>(duration / tempoMultiplier);

    if (nextTarget != nullptr){
        bridge.pushProgress(node.nodeID, nextTarget->nodeID, wallClockMs, traversalLogic.rootId);
    }
    else {
        bridge.pushArrowReset(traversalId);
    }

    if (modulatorNode != nullptr) {
        if (nextModulatorTarget != nullptr) {
            bridge.pushProgress(modulatorNode->nodeID, nextModulatorTarget->nodeID, wallClockMs, traversalLogic.rootId);
        }
        else {
            bridge.pushArrowReset(traversalLogic.activeModulatorRootId);
        }
    }


    if (nodeType == RTNode::NodeType::Node || nodeType == RTNode::NodeType::RootNode)
    {
        for (int connectorId : traversalLogic.peekCrossTreeNode(nodes))
        {
            auto connectorIt = nodes.find(connectorId);

            if (connectorIt == nodes.end()) {
                continue;
            }

            const RTNode& connectorNode = connectorIt->second;
            bridge.highlightNode(connectorNode, true);

            auto durIt = node.durationMap.find(connectorId);

            int connectionDuration;

            if (durIt != node.durationMap.end()) {
                connectionDuration = durIt->second;
            }
            else {
                connectionDuration = 1000;
            }

            scheduler.scheduleNote(connectorNode, traversalId, sample, midiMessages,
                                   sampleRate, tempoMultiplier, connectionDuration, true);

            const int wallClockMs = static_cast<int>(connectionDuration / tempoMultiplier);
            bridge.pushProgress(node.nodeID, connectorId, wallClockMs, traversalLogic.rootId);

        }
    }
}

void TraversalDispatcher::pushChordNotes(const RTNode& node, int sample, int duration,
                                          juce::MidiBuffer& midiMessages,
                                          double sampleRate, double tempoMultiplier,
                                          const NodeMap& nodes)
{
    for (const auto& [childId, connDuration] : node.durationMap)
    {
        if (connDuration != 0)
            continue;

        auto childIt = nodes.find(childId);
        if (childIt == nodes.end())
            continue;

        const RTNode& chordNode = childIt->second;

        if (!NoteScheduler::isNodeAudible(chordNode.nodeType))
            continue;

        scheduler.scheduleNote(chordNode, -1, sample, midiMessages,
                               sampleRate, tempoMultiplier, duration);
    }
}

void TraversalDispatcher::dispatchModulator(const RTNode &node, NodeMap &nodes, TraversalLogic &traversalLogic, RTNode *&modulatorNode, TraversalMap& traversalMap)
{
    int newActiveRootId = traversalLogic.findActiveModulatorRoot(nodes, node.nodeID);

    if (newActiveRootId != traversalLogic.activeModulatorRootId) {
        if (traversalLogic.targetModulatorId != -1) {
            auto prevIt = nodes.find(traversalLogic.targetModulatorId);
            if (prevIt != nodes.end()) {
                bridge.highlightNode(prevIt->second, false);
            }
        }

        traversalLogic.activeModulatorRootId = newActiveRootId;
        traversalLogic.targetModulatorId     = newActiveRootId;
        traversalLogic.lastModulatorId       = -1;
    }
    else if (newActiveRootId != -1) {
        traversalLogic.advanceModulator(nodes);
    }

    if (traversalLogic.activeModulatorRootId == -1 || traversalLogic.targetModulatorId == -1) {
        modulatorNode = nullptr;
        return;
    }

    auto targetIt = nodes.find(traversalLogic.targetModulatorId);
    modulatorNode = (targetIt != nodes.end()) ? &targetIt->second : nullptr;

    if (modulatorNode != nullptr) {
        bridge.highlightNode(*modulatorNode, true);
    }

    if (traversalLogic.lastModulatorId != -1
        && traversalLogic.lastModulatorId != traversalLogic.targetModulatorId) {
        auto lastIt = nodes.find(traversalLogic.lastModulatorId);
        if (lastIt != nodes.end()) {
            bridge.highlightNode(lastIt->second, false);
        }
    }
}

void TraversalDispatcher::handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                                            int priorityNoteDuration,
                                            juce::MidiBuffer& midiMessages,
                                            NodeMap& nodes, TraversalMap& traversalMap)
{
    int traversalId = expiredNote.traversalId;
    auto traversalIt = traversalMap.find(traversalId);

    if (traversalIt == traversalMap.end())
        return;

    TraversalLogic& traversal = traversalIt->second;
    auto type = expiredNote.noteNode.nodeType;

    jassert(nodes.find(traversal.targetId) != nodes.end());

    if (type == RTNode::NodeType::RootNode && expiredNote.isConnectionTrigger)
    {
        bridge.highlightNode(expiredNote.noteNode, false);

        if (traversal.shouldTraverse()) {
            pushRootNodeConnection(expiredNote.noteNode.nodeID, midiMessages,priorityNoteDuration, nodes, traversalMap);
        }
    }
    else if (type == RTNode::NodeType::RootNode
          || type == RTNode::NodeType::Node
          )
    {
        traversal.handleNodeEvent(nodes);

        if (traversal.shouldTraverse() && nodes.find(traversal.targetId) != nodes.end())
            pushNote(traversal.getTargetNode(nodes), traversalId, midiMessages,
                     priorityNoteDuration, nodes, traversalMap);
    }
}

void TraversalDispatcher::resetTraversal(int graphId, int newTargetId,
                                          NodeMap& nodes, TraversalMap& traversalMap)
{
    auto existingIt = traversalMap.find(graphId);

    if (existingIt != traversalMap.end())
    {
        TraversalLogic& existing = existingIt->second;
        auto oldTargetIt = nodes.find(existing.targetId);

        if (oldTargetIt != nodes.end()) {
            bridge.highlightNode(oldTargetIt->second, false);
        }

        scheduler.clearTraversalNotes(graphId);
    }
    else {
        traversalMap.insert({ graphId, TraversalLogic(graphId, &processor) });
    }

    TraversalLogic& traversal = traversalMap.at(graphId);
    traversal.targetId = newTargetId;
    traversal.state    = TraversalLogic::TraversalState::Active;
}

void TraversalDispatcher::pushConnectorNotes(int traverserId, juce::MidiBuffer& midiMessages,
                                              int sample, NodeMap& nodes, TraversalMap& traversalMap)
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
            bridge.highlightNode(targetIt->second, true);

        pushNote(connector, connector.graphID, midiMessages, sample, nodes, traversalMap);
    }
}

void TraversalDispatcher::pushModulatorNotes(int modulatorRootId, juce::MidiBuffer& midiMessages,
                                              int sample, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto rootIt = nodes.find(modulatorRootId);

    if (rootIt == nodes.end()) {
        return;
    }

    int graphId = rootIt->second.graphID;

    resetTraversal(graphId, modulatorRootId, nodes, traversalMap);

    TraversalLogic& traversal = traversalMap.at(graphId);
    traversal.handleNodeEvent(nodes);

    if (traversal.shouldTraverse() && nodes.find(traversal.targetId) != nodes.end()) {
        pushNote(traversal.getTargetNode(nodes), graphId, midiMessages, sample, nodes, traversalMap);
    }
}

void TraversalDispatcher::pushRootNodeConnection(int rootNodeId, juce::MidiBuffer& midiMessages,
                                                  int sample, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto rootIt = nodes.find(rootNodeId);
    if (rootIt == nodes.end())
        return;

    const RTNode& rootNode = rootIt->second;

    resetTraversal(rootNode.graphID, rootNodeId, nodes, traversalMap);

    TraversalLogic& traversal = traversalMap.at(rootNode.graphID);
    traversal.isLooping = true;
    traversal.loopCount = 0;

    auto snap = std::atomic_load(&processor.audioSnapshot);
    if (snap && snap->rtGraphs)
    {
        auto rtGraphIt = snap->rtGraphs->find(rootNode.graphID);
        if (rtGraphIt != snap->rtGraphs->end())
            traversal.loopLimit = rtGraphIt->second->loopLimit;
    }

    bridge.highlightNode(rootNode, true);
    pushNote(rootNode, rootNode.graphID, midiMessages, sample, nodes, traversalMap);
}
