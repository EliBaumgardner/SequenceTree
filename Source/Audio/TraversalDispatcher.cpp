#include "TraversalDispatcher.h"
#include "AudioUIBridge.h"
#include "../Plugin/PluginProcessor.h"

TraversalDispatcher::TraversalDispatcher(SequenceTreeAudioProcessor& p,
                                         NoteScheduler& s,
                                         AudioUIBridge& b)
    : processor(p), scheduler(s), bridge(b)
{
}

void TraversalDispatcher::applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes)
{
    auto highlight = [&](int nodeId, bool on) {
        auto it = nodes.find(nodeId);
        if (it != nodes.end()) bridge.highlightNode(it->second, on);
    };

    if (step.leftId         != -1) highlight(step.leftId, false);
    if (step.enteredId      != -1) highlight(step.enteredId, true);
    if (step.referenceOffId != -1) highlight(step.referenceOffId, false);
    if (step.rootForReset   != -1) bridge.pushArrowReset(step.rootForReset);

    if (step.pushCounts && step.countSourceNodeId != -1) {
        bridge.pushCount(step.countSourceNodeId, 0, 1);

        auto it = nodes.find(step.countSourceNodeId);
        if (it != nodes.end()) {
            for (int childId : it->second.children) {
                auto childIt = nodes.find(childId);
                if (childIt == nodes.end()) continue;

                int limit = childIt->second.countLimit;
                int fill  = (step.countSourceCount % limit == 0)
                              ? limit
                              : (step.countSourceCount % limit);
                bridge.pushCount(childId, fill, limit);
            }
        }
    }
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
    auto traversalIterator = traversalMap.find(traversalId);
    jassert(traversalIterator != traversalMap.end());
    TraversalLogic& traversalLogic = traversalIterator->second;

    RTNode* modulatorNode = nullptr;
    dispatchModulator(node, nodes, traversalLogic, modulatorNode, traversalMap);

    RTNode* nextTarget = traversalLogic.peekNextTarget(nodes);
    int duration = resolveDuration(node, nextTarget, traversalLogic.primary.last, nodes);

    RTNode* nextModulatorTarget = nullptr;

    if (modulatorNode != nullptr && traversalLogic.modulator.target != -1) {
        nextModulatorTarget = traversalLogic.peekModulators(nodes);
        int modulatorDuration = resolveDuration(*modulatorNode, nextModulatorTarget, traversalLogic.modulator.last, nodes);
        duration = static_cast<int>(duration * (0.001 * modulatorDuration));
    }

    const double sampleRate      = processor.TempoInfo.currentSampleRate;
    const double tempoMultiplier = processor.tempoMultiplier.load();
    jassert(sampleRate > 0.0);
    jassert(tempoMultiplier > 0.0);

    scheduler.scheduleNote(node, traversalId, sample, midiMessages,
                           sampleRate, tempoMultiplier, duration);

    pushChordNotes(node, sample, duration, midiMessages, sampleRate, tempoMultiplier, nodes);

    const int wallClockMs = static_cast<int>(duration / tempoMultiplier);

    dispatchPrimaryArrow(node, nextTarget, traversalId, traversalLogic.rootId, wallClockMs);

    dispatchModulatorArrow(modulatorNode, nextModulatorTarget, traversalLogic.activeModulatorRootId, traversalLogic.rootId, wallClockMs);

    dispatchCrossTree(node, traversalId, sample, traversalLogic.rootId, midiMessages, sampleRate, tempoMultiplier, nodes, traversalLogic);
}

void TraversalDispatcher::dispatchPrimaryArrow(const RTNode& node, const RTNode* nextTarget,
                                                int traversalId, int rootId, int wallClockMs)
{
    if (nextTarget != nullptr) {
        bridge.pushProgress(node.nodeID, nextTarget->nodeID, wallClockMs, rootId);
    }
    else {
        bridge.pushArrowReset(traversalId);
    }
}

void TraversalDispatcher::dispatchModulatorArrow(const RTNode* modulatorNode,
                                                  const RTNode* nextModulatorTarget,
                                                  int activeModulatorRootId,
                                                  int rootId, int wallClockMs)
{
    if (modulatorNode == nullptr) return;

    if (nextModulatorTarget != nullptr) {
        bridge.pushProgress(modulatorNode->nodeID, nextModulatorTarget->nodeID, wallClockMs, rootId);
    }
    else {
        bridge.pushArrowReset(activeModulatorRootId);
    }
}

void TraversalDispatcher::dispatchCrossTree(const RTNode& node, int traversalId, int sample, int rootId,
                                             juce::MidiBuffer& midiMessages,
                                             double sampleRate, double tempoMultiplier,
                                             NodeMap& nodes, TraversalLogic& traversal)
{
    if (node.nodeType != RTNode::NodeType::Node && node.nodeType != RTNode::NodeType::RootNode)
        return;

    for (int connectorId : traversal.peekCrossTreeNode(nodes))
    {
        auto connectorIt = nodes.find(connectorId);
        if (connectorIt == nodes.end()) continue;

        const RTNode& connectorNode = connectorIt->second;
        bridge.highlightNode(connectorNode, true);

        auto durIt = node.durationMap.find(connectorId);
        const int connectionDuration = (durIt != node.durationMap.end()) ? durIt->second : 1000;

        scheduler.scheduleNote(connectorNode, traversalId, sample, midiMessages,
                               sampleRate, tempoMultiplier, connectionDuration, true);

        const int connectorWallClockMs = static_cast<int>(connectionDuration / tempoMultiplier);
        bridge.pushProgress(node.nodeID, connectorId, connectorWallClockMs, rootId);
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
        if (traversalLogic.modulator.target != -1) {
            auto prevIt = nodes.find(traversalLogic.modulator.target);
            if (prevIt != nodes.end()) {
                bridge.highlightNode(prevIt->second, false);
            }
        }

        traversalLogic.activeModulatorRootId = newActiveRootId;
        traversalLogic.modulator.target      = newActiveRootId;
        traversalLogic.modulator.last        = -1;
    }
    else if (newActiveRootId != -1) {
        traversalLogic.advanceModulator(nodes);
    }

    if (traversalLogic.activeModulatorRootId == -1 || traversalLogic.modulator.target == -1) {
        modulatorNode = nullptr;
        return;
    }

    auto targetIt = nodes.find(traversalLogic.modulator.target);
    modulatorNode = (targetIt != nodes.end()) ? &targetIt->second : nullptr;

    if (modulatorNode != nullptr) {
        bridge.highlightNode(*modulatorNode, true);
    }

    if (traversalLogic.modulator.last != -1
        && traversalLogic.modulator.last != traversalLogic.modulator.target) {
        auto lastIt = nodes.find(traversalLogic.modulator.last);
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

    jassert(nodes.find(traversal.primary.target) != nodes.end());

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
        applyStepResult(traversal.handleNodeEvent(nodes), nodes);

        if (traversal.shouldTraverse() && nodes.find(traversal.primary.target) != nodes.end())
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
        auto oldTargetIt = nodes.find(existing.primary.target);

        if (oldTargetIt != nodes.end()) {
            bridge.highlightNode(oldTargetIt->second, false);
        }

        scheduler.clearTraversalNotes(graphId);
    }
    else {
        traversalMap.insert({ graphId, TraversalLogic(graphId, bridge) });
    }

    TraversalLogic& traversal = traversalMap.at(graphId);
    traversal.primary.target = newTargetId;
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
    applyStepResult(traversal.handleNodeEvent(nodes), nodes);

    if (traversal.shouldTraverse() && nodes.find(traversal.primary.target) != nodes.end()) {
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
