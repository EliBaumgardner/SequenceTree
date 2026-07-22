#include "TraversalDispatcher.h"
#include "AudioUIBridge.h"
#include "../Plugin/PluginProcessor.h"
#include <unordered_set>
#include <functional>

TraversalDispatcher::TraversalDispatcher(SequenceTreeAudioProcessor& p,
                                         NoteScheduler& s,
                                         AudioUIBridge& b)
    : processor(p), scheduler(s), bridge(b)
{
    chordVisited.reserve(scratchCapacity);
    chordFrontier.reserve(scratchCapacity);
    crossTreeScratch.reserve(scratchCapacity);
}

void TraversalDispatcher::applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes, int traversalId)
{
    auto highlight = [&](int nodeId, bool on) {
        auto it = nodes.find(nodeId);

        jassert(it != nodes.end());

        if (it != nodes.end()) {
            bridge.highlightNode(it->second, on, traversalId);
        }
    };


    if (step.leftAlternativeId != -1) {
        highlight(step.leftAlternativeId, false);
    } else if (step.leftId != -1) {
        highlight(step.leftId, false);
    }

    if (step.enteredAlternativeId != -1) {
        highlight(step.enteredAlternativeId, true);
    } else if (step.enteredId != -1) {
        highlight(step.enteredId, true);
    }

    if (step.referenceOffId != -1) {
        highlight(step.referenceOffId, false);
    }

    if (step.rootForReset   != -1) {
        bridge.pushArrowReset(step.rootForReset, traversalId);
    }

    if (step.pushCounts && step.countSourceNodeId != -1) {
        bridge.pushCount(step.countSourceNodeId, 0, 1);

        auto it = nodes.find(step.countSourceNodeId);

        if (it != nodes.end()) {
            for (int childId : it->second.children) {
                auto childIt = nodes.find(childId);
                if (childIt == nodes.end()) {
                    continue;
                }

                int limit = childIt->second.countLimit;
                int fill  = (step.countSourceCount % limit == 0)
                              ? limit
                              : (step.countSourceCount % limit);
                bridge.pushCount(childId, fill, limit);
            }
        }
    }
}

static bool isDanglingTraversalDisabled(const RTNode& node, int traversalId)
{
    auto it = node.disabledTraversalsByChild.find(node.nodeID);
    return it != node.disabledTraversalsByChild.end() && it->second.count(traversalId) > 0;
}

int TraversalDispatcher::resolveDuration(const RTNode& node, const RTNode* nextTarget,
                                          int lastTargetId, const NodeMap& nodes, int traversalId)
{
    int duration = 1000;

    if (!node.notes.empty()) {
        duration = static_cast<int>(node.notes[0].duration);
    }

    if (nextTarget != nullptr) {

        std::unordered_map<int, int>::const_iterator it = node.durationMap.end();

        if (node.isAlternativeNode) {
            it = node.durationMap.find(node.parentId);
        }
        else {
            if (nextTarget->nodeType != RTNode::NodeType::TraversalFlagData) {
                it = node.durationMap.find(nextTarget->nodeID);
            }
        }


        if (it != node.durationMap.end() && it->second > 0) {
            duration = it->second;
        }
    }
    else if (node.isAlternativeNode) {
        auto it = node.durationMap.find(node.parentId);
        if (it != node.durationMap.end() && it->second > 0) {
            duration = it->second;
        }
    }
    else {
        auto danglingIt = node.durationMap.find(node.nodeID);
        if (danglingIt != node.durationMap.end() && danglingIt->second > 0
            && !isDanglingTraversalDisabled(node, traversalId)) {
            duration = danglingIt->second;
        }
        else {
            auto parentIt = nodes.find(lastTargetId);
            if (parentIt != nodes.end()) {
                auto durIt = parentIt->second.durationMap.find(node.nodeID);

                if (durIt != parentIt->second.durationMap.end() && durIt->second > 0) {
                    duration = durIt->second;
                }
            }
        }
    }

    return duration;
}

void TraversalDispatcher::pushNote(const RTNode& node, int instanceId,
                                   const DispatchContext& context, int sample,
                                   bool isPrimaryRepeat)
{
    const NodeMap& nodes = context.nodes;

    auto traversalIterator = context.traversalMap.find(instanceId);
    jassert(traversalIterator != context.traversalMap.end());
    TraversalLogic& traversalLogic = traversalIterator->second;

    const RTNode* modulatorNode       = nullptr;
    const RTNode* nextModulatorTarget = nullptr;
    const RTNode* alternativeNode     = nullptr;

    dispatchModulator(node, context, traversalLogic, modulatorNode, isPrimaryRepeat);

    const RTNode* nextTarget = traversalLogic.peekNextTarget(nodes);

    const double sampleRate = processor.TempoInfo.currentSampleRate;

    double traversalMultiplier = traversalLogic.traversal.tempoMultiplier;
    auto rootIt = nodes.find(traversalLogic.rootId);
    if (rootIt != nodes.end()) {
        for (const RTtraversal& t : rootIt->second.traversals) {
            if (t.traversalId == traversalLogic.traversal.traversalId) {
                traversalMultiplier = t.tempoMultiplier;
                break;
            }
        }
    }
    if (traversalMultiplier <= 0.0) {
        traversalMultiplier = 1.0;
    }

    const double tempoMultiplier = processor.tempoMultiplier.load() * traversalMultiplier;
    jassert(sampleRate > 0.0);
    jassert(tempoMultiplier > 0.0);

    int duration;

    int pitchOverride    = -1;
    int velocityOverride = -1;

    if (traversalLogic.nodeStates[node.nodeID].activeAlternativeId != -1 && !node.notes.empty()) {
        auto altIt = nodes.find(traversalLogic.nodeStates[node.nodeID].activeAlternativeId);

        if (altIt != nodes.end()) {
            alternativeNode = &altIt->second;

            if (!alternativeNode->notes.empty()) {
                pitchOverride    = static_cast<int>(alternativeNode->notes[0].pitch);
                velocityOverride = static_cast<int>(alternativeNode->notes[0].velocity);
            }
        }
    }

    const int activeTraversalId = traversalLogic.traversal.traversalId;

    if (alternativeNode != nullptr) {
        duration = resolveDuration(*alternativeNode, nextTarget, traversalLogic.primary.last, nodes, activeTraversalId);
    }
    else {
        duration = resolveDuration(node, nextTarget, traversalLogic.primary.last, nodes, activeTraversalId);
    }

    if (modulatorNode != nullptr && traversalLogic.modulator.target != -1) {
        nextModulatorTarget = traversalLogic.peekModulators(nodes);
        int modulatorDuration = resolveDuration(*modulatorNode, nextModulatorTarget, traversalLogic.modulator.last, nodes, activeTraversalId);
        duration = static_cast<int>(duration * (0.001 * modulatorDuration));
    }


    scheduler.scheduleNote(node, instanceId, sample, context.midiMessages, sampleRate, tempoMultiplier, duration, false, traversalLogic.traversal.channel, traversalLogic.traversal.transpose, traversalLogic.traversal.velocityMultiplier, pitchOverride, velocityOverride);

    int chordParentCount = traversalLogic.primary.counts[node.nodeID] + 1;
    pushChordNotes(node, sample, duration, sampleRate, tempoMultiplier, context, chordParentCount, traversalLogic);

    const int wallClockMs = static_cast<int>(duration / tempoMultiplier);

    if (alternativeNode != nullptr) {

        auto alternativeNodeParentIterator = nodes.find(alternativeNode->parentId);

        if (alternativeNodeParentIterator != nodes.end()) {
            const RTNode* alternativeNodeParent = &alternativeNodeParentIterator->second;
            dispatchPrimaryArrow(*alternativeNode, alternativeNodeParent, traversalLogic.rootId, wallClockMs, traversalLogic.traversal.traversalId);
        }
    }

    dispatchPrimaryArrow(node, nextTarget, traversalLogic.rootId, wallClockMs, traversalLogic.traversal.traversalId);
    dispatchModulatorArrow(modulatorNode, nextModulatorTarget, traversalLogic.activeModulatorRootId, traversalLogic.rootId, wallClockMs, traversalLogic.traversal.traversalId);
    dispatchCrossTree(node, instanceId, sample, traversalLogic.rootId, sampleRate, tempoMultiplier, context, traversalLogic);
    dispatchFlag(node, instanceId, traversalLogic.traversal.traversalId, chordParentCount, sample, context);
}

void TraversalDispatcher::dispatchPrimaryArrow(const RTNode& node, const RTNode* nextTarget,
                                                int rootId, int wallClockMs, int colourTraversalId)
{
    if (nextTarget != nullptr) {
        bridge.pushProgress(node.nodeID, nextTarget->nodeID, wallClockMs, rootId, colourTraversalId);
    }
    else {
        auto danglingIt = node.durationMap.find(node.nodeID);
        if (danglingIt != node.durationMap.end() && danglingIt->second > 0
            && !isDanglingTraversalDisabled(node, colourTraversalId)) {
            bridge.pushProgress(node.nodeID, node.nodeID, wallClockMs, rootId, colourTraversalId);
        }
    }
}

void TraversalDispatcher::dispatchModulatorArrow(const RTNode* modulatorNode,
                                                  const RTNode* nextModulatorTarget,
                                                  int activeModulatorRootId,
                                                  int rootId, int wallClockMs, int colourTraversalId)
{
    if (modulatorNode == nullptr) {
        return;
    }

    if (nextModulatorTarget != nullptr) {
        bridge.pushProgress(modulatorNode->nodeID, nextModulatorTarget->nodeID, wallClockMs, rootId, colourTraversalId);
    }
    else {
        bridge.pushArrowReset(activeModulatorRootId, colourTraversalId);
    }
}

void TraversalDispatcher::dispatchCrossTree(const RTNode& node, int sourceInstanceId, int sample, int rootId,
                                             double sampleRate, double tempoMultiplier,
                                             const DispatchContext& context, TraversalLogic& traversal)
{
    if (node.nodeType != RTNode::NodeType::Node && node.nodeType != RTNode::NodeType::RootNode) {
        return;
    }

    const NodeMap& nodes = context.nodes;

    traversal.peekCrossTreeNode(nodes, crossTreeScratch);

    for (int crossTreeRootId : crossTreeScratch)
    {
        auto crossTreeIt = nodes.find(crossTreeRootId);
        if (crossTreeIt == nodes.end()) {
            continue;
        }

        const RTNode& crossTreeRoot = crossTreeIt->second;

        if (hasActiveTraversalOnTree(crossTreeRoot.nodeID, context.traversalMap)) {
            continue;
        }

        bridge.highlightNode(crossTreeRoot, true, traversal.traversal.traversalId);

        int connectionDuration = 1000;
        int progressSourceId = node.nodeID;

        auto durIt = node.durationMap.find(crossTreeRootId);
        if (durIt != node.durationMap.end()) {
            connectionDuration = durIt->second;
        }
        else if (traversal.nodeStates[node.nodeID].activeAlternativeId != -1) {
            auto altIt = nodes.find(traversal.nodeStates[node.nodeID].activeAlternativeId);
            if (altIt != nodes.end()) {
                auto altDurIt = altIt->second.durationMap.find(crossTreeRootId);
                if (altDurIt != altIt->second.durationMap.end()) {
                    connectionDuration = altDurIt->second;
                    progressSourceId = traversal.nodeStates[node.nodeID].activeAlternativeId;
                }
            }
        }

        scheduler.scheduleNote(crossTreeRoot, sourceInstanceId, sample, context.midiMessages,
                               sampleRate, tempoMultiplier, connectionDuration, true, traversal.traversal.channel, traversal.traversal.transpose, traversal.traversal.velocityMultiplier);

        const int wallClockMs = static_cast<int>(connectionDuration / tempoMultiplier);
        bridge.pushProgress(progressSourceId, crossTreeRootId, wallClockMs, rootId, traversal.traversal.traversalId, true);
    }
}

void TraversalDispatcher::dispatchFlag(const RTNode& node, int hostInstanceId, int hostTypeId,
                                       int parentCount, int sample, const DispatchContext& context)
{
    for (int childId : node.children) {
        auto childIt = context.nodes.find(childId);
        if (childIt == context.nodes.end()) {
            continue;
        }

        const RTNode& flagNode = childIt->second;
        if (flagNode.nodeType != RTNode::NodeType::TraversalFlagData) {
            continue;
        }

        if (flagNode.countLimit <= 0 || parentCount % flagNode.countLimit != 0) {
            continue;
        }

        if (flagNode.flagRemovesTraversal) {
            queueFlagRemoval(flagNode, hostInstanceId, hostTypeId, context.traversalMap);
        }
        else {
            startFlagTraversal(flagNode, hostTypeId, sample, context);
        }
    }
}

void TraversalDispatcher::queueFlagRemoval(const RTNode& flagNode, int hostInstanceId, int hostTypeId, TraversalMap& traversalMap)
{
    const int targetTypeId = flagNode.flagTraversal.traversalId;

    if (targetTypeId <= 0) {
        return;
    }

    if (hostTypeId != targetTypeId) {
        return;
    }

    auto traversalIt = traversalMap.find(hostInstanceId);
    if (traversalIt == traversalMap.end()) {
        return;
    }

    traversalIt->second.pendingRemoval = true;
}

void TraversalDispatcher::startFlagTraversal(const RTNode& flagNode, int hostTypeId, int sample,
                                              const DispatchContext& context)
{
    const int spawnTypeId = flagNode.flagTraversal.traversalId;

    if (spawnTypeId <= 0) {
        return;
    }

    if (hostTypeId == spawnTypeId) {
        return;
    }

    auto startIt = context.nodes.find(flagNode.flagTargetId);
    if (startIt == context.nodes.end()) {
        return;
    }

    const RTNode& startNode = startIt->second;
    const int rootId = startNode.graphID;

    if (findTraversalInstance(rootId, spawnTypeId, context.traversalMap) != -1) {
        return;
    }

    const int instanceId = processor.traversalSession.nextTraversalInstanceId();

    TraversalLogic* traversalLogic = prepareTraversal(instanceId, rootId, startNode.nodeID,
                                                      flagNode.flagTraversal, context);

    if (traversalLogic == nullptr) {
        return;
    }

    traversalLogic->isFlagSpawned    = true;
    traversalLogic->flagSourceNodeId = flagNode.nodeID;

    bridge.highlightNode(startNode, true, traversalLogic->traversal.traversalId);
    pushNote(startNode, instanceId, context, sample);
}

void TraversalDispatcher::pushChordNotes(const RTNode& node, int sample, int duration,
                                          double sampleRate, double tempoMultiplier,
                                          const DispatchContext& context, int parentCount,
                                          TraversalLogic& traversalLogic)
{
    const NodeMap& nodes = context.nodes;

    chordVisited.clear();
    chordFrontier.clear();

    chordFrontier.push_back({ node.nodeID, parentCount });

    while (!chordFrontier.empty())
    {
        const auto [chainNodeId, chainCount] = chordFrontier.back();
        chordFrontier.pop_back();

        auto chainIt = nodes.find(chainNodeId);
        if (chainIt == nodes.end()) {
            continue;
        }

        for (const auto& [childId, connDuration] : chainIt->second.durationMap)
        {
            if (connDuration != 0) {
                continue;
            }

            if (!chordVisited.insert(childId).second) {
                continue;
            }

            auto childIt = nodes.find(childId);

            if (childIt == nodes.end()) {
                continue;
            }

            const RTNode& chordNode = childIt->second;

            if (!NoteScheduler::isNodeAudible(chordNode.nodeType)) {
                continue;
            }

            if (chordNode.countLimit <= 0 || chainCount % chordNode.countLimit != 0) {
                continue;
            }

            scheduler.scheduleNote(chordNode, -1, sample, context.midiMessages,
                                   sampleRate, tempoMultiplier, duration, false, traversalLogic.traversal.channel, traversalLogic.traversal.transpose, traversalLogic.traversal.velocityMultiplier);

            bridge.highlightNode(chordNode, true, traversalLogic.traversal.traversalId);

            int chordPlayCount = ++traversalLogic.chordCounts[chordNode.nodeID];
            chordFrontier.push_back({ chordNode.nodeID, chordPlayCount });
        }
    }
}

void TraversalDispatcher::dispatchModulator(const RTNode& node, const DispatchContext& context,
                                            TraversalLogic& traversalLogic, const RTNode*& modulatorNode,
                                            bool isPrimaryRepeat)
{
    const NodeMap& nodes = context.nodes;

    int triggeredRootId = traversalLogic.findActiveModulatorRoot(nodes, node.nodeID);

    if (triggeredRootId != -1) {
        if (traversalLogic.modulator.target != -1) {
            auto prevIt = nodes.find(traversalLogic.modulator.target);
            if (prevIt != nodes.end()) {
                bridge.highlightNode(prevIt->second, false, traversalLogic.traversal.traversalId);
            }
        }

        traversalLogic.activeModulatorRootId  = triggeredRootId;
        traversalLogic.modulator.target       = triggeredRootId;
        traversalLogic.modulator.last         = -1;
        traversalLogic.modulatorRepeatCount   = 0;
        traversalLogic.modulatorHostId        = node.nodeID;
    }
    else if (traversalLogic.activeModulatorRootId != -1) {
        bool isHost       = (node.nodeID == traversalLogic.modulatorHostId);
        bool isDescendant = TraversalLogic::isDescendantOf(nodes, node.nodeID, traversalLogic.modulatorHostId);

        if (!isHost && !isDescendant) {
            auto targetIt = nodes.find(traversalLogic.modulator.target);
            if (targetIt != nodes.end()) {
                bridge.highlightNode(targetIt->second, false, traversalLogic.traversal.traversalId);
            }

            traversalLogic.activeModulatorRootId = -1;
            traversalLogic.modulator.target      = -1;
            traversalLogic.modulator.last        = -1;
            traversalLogic.modulatorRepeatCount  = 0;
            traversalLogic.modulatorHostId       = -1;

            modulatorNode = nullptr;
            return;
        }

        if (isDescendant && !isPrimaryRepeat) {
            auto targetIt = nodes.find(traversalLogic.modulator.target);
            int modulatorRepeatValue = (targetIt != nodes.end()) ? targetIt->second.repeatValue : 1;

            traversalLogic.modulatorRepeatCount++;

            if (traversalLogic.modulatorRepeatCount >= modulatorRepeatValue) {
                traversalLogic.modulatorRepeatCount = 0;
                traversalLogic.advanceModulator(nodes);
            }
        }
    }

    if (traversalLogic.activeModulatorRootId == -1 || traversalLogic.modulator.target == -1) {
        modulatorNode = nullptr;
        return;
    }

    auto targetIt = nodes.find(traversalLogic.modulator.target);
    modulatorNode = (targetIt != nodes.end()) ? &targetIt->second : nullptr;

    if (modulatorNode != nullptr) {
        bridge.highlightNode(*modulatorNode, true, traversalLogic.traversal.traversalId);
    }

    if (traversalLogic.modulator.last != -1
        && traversalLogic.modulator.last != traversalLogic.modulator.target) {
        auto lastIt = nodes.find(traversalLogic.modulator.last);
        if (lastIt != nodes.end()) {
            bridge.highlightNode(lastIt->second, false, traversalLogic.traversal.traversalId);
        }
    }
}

void TraversalDispatcher::handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                                            int priorityNoteDuration,
                                            const DispatchContext& context)
{
    const NodeMap& nodes = context.nodes;

    int instanceId = expiredNote.instanceId;

    auto traversalIt = context.traversalMap.find(instanceId);

    if (traversalIt == context.traversalMap.end()) {
        bridge.highlightNode(expiredNote.nodeId, false);
        return;
    }

    TraversalLogic& traversal = traversalIt->second;

    if (traversal.pendingRemoval) {
        bridge.highlightNode(expiredNote.nodeId, false, traversal.traversal.traversalId);
        bridge.pushArrowReset(traversal.rootId, traversal.traversal.traversalId);
        context.traversalMap.erase(traversalIt);
        return;
    }

    auto type = expiredNote.nodeType;

    jassert(nodes.find(traversal.primary.target) != nodes.end());

    if (type == RTNode::NodeType::RootNode && expiredNote.isConnectionTrigger) {
        bridge.highlightNode(expiredNote.nodeId, false, traversal.traversal.traversalId);

        if (traversal.shouldTraverse()) {
            pushRootNodeConnection(expiredNote.nodeId, context, priorityNoteDuration);
        }
    }
    else if (type == RTNode::NodeType::RootNode|| type == RTNode::NodeType::Node) {
        auto currentIt = nodes.find(traversal.primary.target);
        int  repeatValue = 1;

        if (currentIt != nodes.end()) {
            const RTNode& currentNode = currentIt->second;
            int activeAltId = traversal.nodeStates[currentNode.nodeID].activeAlternativeId;

            if (activeAltId != -1) {
                auto altIt = nodes.find(activeAltId);
                repeatValue = (altIt != nodes.end()) ? altIt->second.repeatValue : 1;
            } else {
                repeatValue = currentNode.repeatValue;
            }
        }

        traversal.repeatCount++;

        if (traversal.repeatCount < repeatValue) {
            pushNote(traversal.getTargetNode(nodes), instanceId, context, priorityNoteDuration, true);
        } else {
            traversal.repeatCount = 0;
            applyStepResult(traversal.handleNodeEvent(nodes), nodes, traversal.traversal.traversalId);

            if (traversal.shouldTraverse() && nodes.find(traversal.primary.target) != nodes.end()) {
                pushNote(traversal.getTargetNode(nodes), instanceId, context, priorityNoteDuration);
            }
        }
    }
}

void TraversalDispatcher::pushRootNodeConnection(int rootNodeId, const DispatchContext& context, int sample)
{
    auto rootIt = context.nodes.find(rootNodeId);
    if (rootIt == context.nodes.end()) {
        return;
    }

    const RTNode& targetRootNode = rootIt->second;

    if (hasActiveTraversalOnTree(targetRootNode.nodeID, context.traversalMap)) {
        return;
    }

    for (const RTtraversal& registeredTraversal : targetRootNode.traversals) {
        startCrossTreeTraversal(targetRootNode, registeredTraversal, sample, context);
    }
}

bool TraversalDispatcher::hasActiveTraversalOnTree(int treeRootId, const TraversalMap& traversalMap) const
{
    for (const auto& [traversalId, traversal] : traversalMap) {
        if (traversal.rootId == treeRootId && traversal.shouldTraverse()) {
            return true;
        }
    }

    return false;
}

void TraversalDispatcher::startCrossTreeTraversal(const RTNode& targetRootNode, const RTtraversal& traversal,
                                                  int sample, const DispatchContext& context)
{
    const int rootId      = targetRootNode.nodeID;
    const int traversalId = traversal.traversalId;

    int instanceId = findTraversalInstance(rootId, traversalId, context.traversalMap);

    if (instanceId == -1) {
        instanceId = processor.traversalSession.nextTraversalInstanceId();
    }

    if (prepareTraversal(instanceId, rootId, rootId, traversal, context) == nullptr) {
        return;
    }

    bridge.highlightNode(targetRootNode, true, traversalId);
    pushNote(targetRootNode, instanceId, context, sample);
}

int TraversalDispatcher::findTraversalInstance(int rootId, int typeId, const TraversalMap& traversalMap) const
{
    for (const auto& [instanceId, traversal] : traversalMap) {
        if (traversal.rootId == rootId && traversal.traversal.traversalId == typeId) {
            return instanceId;
        }
    }

    return -1;
}

void TraversalDispatcher::applyGraphLoopLimit(TraversalLogic& traversalLogic, int rootId)
{
    const auto* snapshot = processor.currentSnapshot.load(std::memory_order_relaxed);

    if (snapshot == nullptr || snapshot->rtGraphs == nullptr) {
        return;
    }

    auto rtGraphIt = snapshot->rtGraphs->find(rootId);

    if (rtGraphIt != snapshot->rtGraphs->end()) {
        traversalLogic.loopLimit = rtGraphIt->second->loopLimit;
    }
}

TraversalLogic* TraversalDispatcher::prepareTraversal(int instanceId, int rootId, int startNodeId,
                                                      const RTtraversal& traversal, const DispatchContext& context)
{
    auto existingIt = context.traversalMap.find(instanceId);

    TraversalLogic* traversalLogic = (existingIt != context.traversalMap.end())
                                       ? &existingIt->second
                                       : context.traversalMap.acquire(instanceId, rootId, traversal);

    if (traversalLogic == nullptr) {
        return nullptr;
    }

    traversalLogic->instanceId     = instanceId;
    traversalLogic->isFirstEvent   = true;
    traversalLogic->primary.target = startNodeId;
    traversalLogic->state          = TraversalLogic::TraversalState::Active;
    traversalLogic->isLooping      = true;
    traversalLogic->loopCount      = 0;
    traversalLogic->repeatCount    = 0;

    applyGraphLoopLimit(*traversalLogic, rootId);

    traversalLogic->advanceAlternative(context.nodes, startNodeId);

    return traversalLogic;
}

