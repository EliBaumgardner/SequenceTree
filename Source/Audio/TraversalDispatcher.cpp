#include "TraversalDispatcher.h"
#include "AudioUIBridge.h"
#include <algorithm>
#include <functional>

TraversalDispatcher::TraversalDispatcher(NoteScheduler& s, AudioUIBridge& b)
    : flagScheduler(*this, b), scheduler(s), bridge(b)
{
    chordVisitStamps.assign(NodeStateTable::maxNodeIds, 0);
    chordFrontier.reserve(NodeStateTable::maxNodeIds + 1);
    crossTreeScratch.reserve(scratchCapacity);
}

bool TraversalDispatcher::markChordVisited(int nodeId)
{
    if (nodeId < 0 || nodeId >= NodeStateTable::maxNodeIds) {
        return false;
    }

    if (chordVisitStamps[static_cast<std::size_t>(nodeId)] == chordVisitToken) {
        return false;
    }

    chordVisitStamps[static_cast<std::size_t>(nodeId)] = chordVisitToken;

    return true;
}

void TraversalDispatcher::applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes,
                                         int instanceId, int traversalId)
{
    auto highlight = [&](int nodeId, bool on) {
        auto it = nodes.find(nodeId);

        jassert(it != nodes.end());

        if (it != nodes.end()) {
            bridge.highlightNode(*it->second, on, traversalId);
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

    if (step.clearTrail) {
        bridge.pushArrowReset(AudioUIBridge::primaryTrail(instanceId));
    }

    if (step.pushCounts && step.countSourceNodeId != -1) {
        bridge.pushCount(step.countSourceNodeId, 0, 1);

        auto it = nodes.find(step.countSourceNodeId);

        if (it != nodes.end()) {
            for (const RTConnection& connection : it->second->connections) {
                const int childId = connection.childId;

                auto childIt = nodes.find(childId);
                if (childIt == nodes.end()) {
                    continue;
                }

                int limit = childIt->second->countLimit;
                if (limit <= 0) {
                    continue;
                }

                int fill = step.countSourceCount % limit;

                if (fill == 0) {
                    fill = limit;
                }
                bridge.pushCount(childId, fill, limit);
            }
        }
    }
}

void TraversalDispatcher::applyTreeJump(const TraversalLogic::StepResult& step,
                                        TraversalLogic& traversal, TraversalRuntime& runtime,
                                        const DispatchContext& context)
{
    if (step.kind != TraversalLogic::StepResult::Kind::JumpedToTree) {
        return;
    }

    if (runtime.originRootId == -1) {
        runtime.originRootId = step.jumpedFromRootId;
    }

    applyGraphLoopLimit(traversal, traversal.rootId, context);
}

namespace {

bool isChordMember(const RTNode& node)
{
    return NoteScheduler::isNodeAudible(node.nodeType)
        && node.nodeType != RTNode::NodeType::RootNode;
}

}

int TraversalDispatcher::resolveDuration(const RTNode& node, const RTNode* nextTarget,
                                          int lastTargetId, const NodeMap& nodes, int danglingIndex)
{
    int duration = 1000;

    if (!node.notes.empty() && node.notes[0].duration > 0) {
        duration = node.notes[0].duration;
    }

    if (nextTarget != nullptr) {

        int connectionDuration = -1;

        if (node.isAlternativeNode) {
            connectionDuration = node.alternativeArrowDuration;
        }
        else {
            if (nextTarget->nodeType != RTNode::NodeType::TraversalFlagData) {
                const RTConnection* const connection = node.findConnection(nextTarget->nodeID);

                if (connection != nullptr) {
                    connectionDuration = connection->duration;
                }
            }
        }

        if (connectionDuration > 0) {
            duration = connectionDuration;
        }
    }
    else if (node.isAlternativeNode) {
        if (node.alternativeArrowDuration > 0) {
            duration = node.alternativeArrowDuration;
        }
    }
    else {
        int danglingDuration = 0;

        if (danglingIndex >= 0 && danglingIndex < static_cast<int>(node.danglingArrows.size())) {
            danglingDuration = node.danglingArrows[static_cast<std::size_t>(danglingIndex)].duration;
        }

        if (danglingDuration > 0) {
            duration = danglingDuration;
        }
        else {
            auto parentIt = nodes.find(lastTargetId);
            if (parentIt != nodes.end()) {
                const RTConnection* const connection = parentIt->second->findConnection(node.nodeID);

                if (connection != nullptr && connection->duration > 0) {
                    duration = connection->duration;
                }
            }
        }
    }

    return duration;
}

void TraversalDispatcher::pushNote(const RTNode& node, int instanceId,
                                   const DispatchContext& context, double sample,
                                   bool isPrimaryRepeat)
{
    const NodeMap& nodes = context.nodes;

    auto traversalIterator = context.traversalMap.find(instanceId);
    jassert(traversalIterator != context.traversalMap.end());
    TraversalLogic& traversalLogic = traversalIterator->second.logic;

    const RTNode* modulatorNode       = nullptr;
    const RTNode* nextModulatorTarget = nullptr;
    const RTNode* alternativeNode     = nullptr;

    dispatchModulator(node, context, traversalLogic, modulatorNode, isPrimaryRepeat);

    const RTNode* nextTarget = traversalLogic.peekNextTarget(nodes);

    const double sampleRate = context.sampleRate;

    double traversalMultiplier = traversalLogic.traversal.tempoMultiplier;
    auto rootIt = nodes.find(traversalLogic.rootId);
    if (rootIt != nodes.end()) {
        for (const RTtraversal& t : rootIt->second->traversals) {
            if (t.traversalId == traversalLogic.traversal.traversalId) {
                traversalMultiplier = t.tempoMultiplier;
                break;
            }
        }
    }
    if (traversalMultiplier <= 0.0) {
        traversalMultiplier = 1.0;
    }

    const double tempoMultiplier = context.tempoMultiplier * traversalMultiplier;
    jassert(sampleRate > 0.0);
    jassert(tempoMultiplier > 0.0);

    int duration;

    int pitchOverride    = -1;
    int velocityOverride = -1;

    if (traversalLogic.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID) != -1 && !node.notes.empty()) {
        auto altIt = nodes.find(traversalLogic.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID));

        if (altIt != nodes.end()) {
            alternativeNode = altIt->second.get();

            if (!alternativeNode->notes.empty()) {
                pitchOverride    = alternativeNode->notes[0].pitch;
                velocityOverride = alternativeNode->notes[0].velocity;
            }
        }
    }

    const int activeTraversalId = traversalLogic.traversal.traversalId;

    auto danglingArrowFor = [&](const RTNode& target) {
        const int count = traversalLogic.nodeState.get(NodeStateSlot::Count, target.nodeID) + 1;

        return traversalLogic.rule->selectDanglingArrow(target, count, activeTraversalId);
    };

    const int nodeCount     = traversalLogic.nodeState.get(NodeStateSlot::Count, node.nodeID) + 1;
    const int danglingIndex = danglingArrowFor(node);

    if (alternativeNode != nullptr) {
        duration = resolveDuration(*alternativeNode, nextTarget, traversalLogic.primary.last, nodes,
                                   danglingArrowFor(*alternativeNode));
    }
    else {
        duration = resolveDuration(node, nextTarget, traversalLogic.primary.last, nodes, danglingIndex);
    }

    int transpose = traversalLogic.traversal.transpose;

    if (modulatorNode != nullptr && traversalLogic.mod.walker.target != -1) {
        nextModulatorTarget = traversalLogic.peekModulators(nodes);
        int modulatorDuration = resolveDuration(*modulatorNode, nextModulatorTarget, traversalLogic.mod.walker.last,
                                                nodes, danglingArrowFor(*modulatorNode));
        duration = static_cast<int>(duration * (0.001 * modulatorDuration));

        transpose += modulatorNode->pitchOffset;
    }


    const NoteScheduler::NoteVoicing voicing {
        traversalLogic.traversal.channel,
        transpose,
        traversalLogic.traversal.velocityMultiplier,
        pitchOverride,
        velocityOverride
    };

    scheduler.scheduleNote(node, instanceId, sample, context.midiMessages,
                           sampleRate, tempoMultiplier, duration, false, voicing);

    pushChordNotes(node, sample, duration, tempoMultiplier, context, nodeCount, traversalLogic, transpose);

    const int wallClockMs = static_cast<int>(duration / tempoMultiplier);

    if (alternativeNode != nullptr) {

        auto alternativeNodeParentIterator = nodes.find(alternativeNode->parentId);

        if (alternativeNodeParentIterator != nodes.end()) {
            const RTNode* alternativeNodeParent = alternativeNodeParentIterator->second.get();
            dispatchPrimaryArrow(*alternativeNode, alternativeNodeParent, danglingArrowFor(*alternativeNode),
                                 instanceId, wallClockMs, activeTraversalId);
        }
    }

    dispatchPrimaryArrow(node, nextTarget, danglingIndex, instanceId, wallClockMs, activeTraversalId);
    dispatchModulatorArrow(modulatorNode, nextModulatorTarget, instanceId, wallClockMs, activeTraversalId);
    dispatchCrossTree(node, instanceId, sample, tempoMultiplier, context, traversalLogic);
    flagScheduler.dispatchFlags(node, instanceId, activeTraversalId, nodeCount,
                                sample, tempoMultiplier, context);
}

void TraversalDispatcher::dispatchPrimaryArrow(const RTNode& node, const RTNode* nextTarget, int danglingIndex,
                                                int instanceId, int wallClockMs, int colourTraversalId)
{
    if (nextTarget == nullptr && danglingIndex < 0) {
        return;
    }

    const int targetId = (nextTarget != nullptr) ? nextTarget->nodeID
                                                 : AudioUIBridge::danglingArrowKey(danglingIndex);

    bridge.pushProgress(node.nodeID, targetId, wallClockMs,
                        AudioUIBridge::primaryTrail(instanceId), colourTraversalId);
}

void TraversalDispatcher::dispatchModulatorArrow(const RTNode* modulatorNode,
                                                  const RTNode* nextModulatorTarget,
                                                  int instanceId, int wallClockMs, int colourTraversalId)
{
    if (modulatorNode == nullptr) {
        return;
    }

    if (nextModulatorTarget == nullptr) {
        bridge.pushArrowReset(AudioUIBridge::modulatorTrail(instanceId));
        return;
    }

    bridge.pushProgress(modulatorNode->nodeID, nextModulatorTarget->nodeID, wallClockMs,
                        AudioUIBridge::modulatorTrail(instanceId), colourTraversalId);
}

void TraversalDispatcher::dispatchCrossTree(const RTNode& node, int sourceInstanceId, double sample,
                                             double tempoMultiplier, const DispatchContext& context,
                                             TraversalLogic& traversal)
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

        const RTNode& crossTreeRoot = *crossTreeIt->second;

        if (context.traversalMap.hasActiveTraversalOnTree(crossTreeRoot.nodeID)) {
            continue;
        }

        bridge.highlightNode(crossTreeRoot, true, traversal.traversal.traversalId);

        int connectionDuration = 1000;
        int progressSourceId = node.nodeID;

        const RTConnection* const crossTreeConnection = node.findConnection(crossTreeRootId);

        if (crossTreeConnection != nullptr && crossTreeConnection->duration >= 0) {
            connectionDuration = crossTreeConnection->duration;
        }
        else if (traversal.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID) != -1) {
            auto altIt = nodes.find(traversal.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID));
            if (altIt != nodes.end()) {
                const RTConnection* const alternativeConnection = altIt->second->findConnection(crossTreeRootId);

                if (alternativeConnection != nullptr && alternativeConnection->duration >= 0) {
                    connectionDuration = alternativeConnection->duration;
                    progressSourceId = traversal.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID);
                }
            }
        }

        const NoteScheduler::NoteVoicing crossTreeVoicing {
            traversal.traversal.channel,
            traversal.traversal.transpose,
            traversal.traversal.velocityMultiplier
        };

        scheduler.scheduleNote(crossTreeRoot, sourceInstanceId, sample, context.midiMessages,
                               context.sampleRate, tempoMultiplier, connectionDuration, true,
                               crossTreeVoicing);

        const int wallClockMs = static_cast<int>(connectionDuration / tempoMultiplier);
        bridge.pushProgress(progressSourceId, crossTreeRootId, wallClockMs,
                            AudioUIBridge::primaryTrail(sourceInstanceId),
                            traversal.traversal.traversalId, true);
    }
}

void TraversalDispatcher::pushChordNotes(const RTNode& node, double sample, int duration,
                                          double tempoMultiplier, const DispatchContext& context,
                                          int parentCount, TraversalLogic& traversalLogic, int transpose)
{
    const NodeMap& nodes = context.nodes;

    if (++chordVisitToken == 0) {
        std::fill(chordVisitStamps.begin(), chordVisitStamps.end(), 0);
        chordVisitToken = 1;
    }

    chordFrontier.clear();

    chordFrontier.push_back({ node.nodeID, parentCount });

    auto scheduleChordMember = [&](int childId, int chainCount) {
        if (!markChordVisited(childId)) {
            return;
        }

        auto childIt = nodes.find(childId);

        if (childIt == nodes.end()) {
            return;
        }

        const RTNode& chordNode = *childIt->second;

        if (!isChordMember(chordNode)) {
            return;
        }

        if (chordNode.countLimit <= 0 || chainCount % chordNode.countLimit != 0) {
            return;
        }

        const NoteScheduler::NoteVoicing chordVoicing {
            traversalLogic.traversal.channel,
            transpose,
            traversalLogic.traversal.velocityMultiplier
        };

        scheduler.scheduleNote(chordNode, -1, sample, context.midiMessages,
                               context.sampleRate, tempoMultiplier, duration, false, chordVoicing);

        bridge.highlightNode(chordNode, true, traversalLogic.traversal.traversalId);

        int chordPlayCount = traversalLogic.nodeState.increment(NodeStateSlot::Chord, chordNode.nodeID);
        chordFrontier.push_back({ chordNode.nodeID, chordPlayCount });
    };

    while (!chordFrontier.empty())
    {
        const auto [chainNodeId, chainCount] = chordFrontier.back();
        chordFrontier.pop_back();

        auto chainIt = nodes.find(chainNodeId);
        if (chainIt == nodes.end()) {
            continue;
        }

        const RTNode& chainNode = *chainIt->second;

        for (const RTConnection& connection : chainNode.connections)
        {
            if (connection.duration == 0) {
                scheduleChordMember(connection.childId, chainCount);
            }
        }

        if (chainNode.isAlternativeNode && chainNode.alternativeArrowDuration == 0) {
            scheduleChordMember(chainNode.parentId, chainCount);
        }
    }
}

void TraversalDispatcher::dispatchModulator(const RTNode& node, const DispatchContext& context,
                                            TraversalLogic& traversalLogic, const RTNode*& modulatorNode,
                                            bool isPrimaryRepeat)
{
    const NodeMap& nodes    = context.nodes;
    auto&          mod      = traversalLogic.mod;
    const int      colourId = traversalLogic.traversal.traversalId;

    int triggeredRootId = traversalLogic.findActiveModulatorRoot(nodes, node.nodeID);

    if (triggeredRootId != -1) {
        if (mod.walker.target != -1) {
            auto prevIt = nodes.find(mod.walker.target);
            if (prevIt != nodes.end()) {
                bridge.highlightNode(*prevIt->second, false, colourId);
            }
        }

        mod.activate(triggeredRootId, node.nodeID);
    }
    else if (mod.isActive()) {
        bool isHost       = (node.nodeID == mod.gate.hostId);
        bool isDescendant = TraversalLogic::isDescendantOf(nodes, node.nodeID, mod.gate.hostId);

        if (!isHost && !isDescendant) {
            auto targetIt = nodes.find(mod.walker.target);
            if (targetIt != nodes.end()) {
                bridge.highlightNode(*targetIt->second, false, colourId);
            }

            mod.deactivate();

            modulatorNode = nullptr;
            return;
        }

        if (isDescendant && !isPrimaryRepeat) {
            auto targetIt = nodes.find(mod.walker.target);
            int  modulatorRepeatValue = (targetIt != nodes.end()) ? targetIt->second->repeatValue : 1;

            if (mod.tickRepeat(modulatorRepeatValue)) {
                const int modulatorRootToReset = traversalLogic.advanceModulator(nodes);

                if (modulatorRootToReset != -1) {
                    bridge.pushArrowReset(AudioUIBridge::modulatorTrail(traversalLogic.instanceId));
                }
            }
        }
    }

    if (!mod.isActive() || mod.walker.target == -1) {
        modulatorNode = nullptr;
        return;
    }

    auto targetIt = nodes.find(mod.walker.target);
    modulatorNode = (targetIt != nodes.end()) ? targetIt->second.get() : nullptr;

    if (modulatorNode != nullptr) {
        bridge.highlightNode(*modulatorNode, true, colourId);
    }

    if (mod.walker.last != -1 && mod.walker.last != mod.walker.target) {
        auto lastIt = nodes.find(mod.walker.last);
        if (lastIt != nodes.end()) {
            bridge.highlightNode(*lastIt->second, false, colourId);
        }
    }
}

void TraversalDispatcher::handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                                            double expiryTime,
                                            const DispatchContext& context)
{
    const NodeMap& nodes = context.nodes;

    int instanceId = expiredNote.instanceId;

    auto traversalIt = context.traversalMap.find(instanceId);

    if (traversalIt == context.traversalMap.end()) {
        bridge.highlightNode(expiredNote.nodeId, false);
        return;
    }

    TraversalLogic&   traversal = traversalIt->second.logic;
    TraversalRuntime& runtime   = traversalIt->second.runtime;

    if (runtime.pendingRemoval) {
        bridge.highlightNode(expiredNote.nodeId, false, traversal.traversal.traversalId);
        bridge.pushArrowReset(AudioUIBridge::primaryTrail(instanceId));
        bridge.pushArrowReset(AudioUIBridge::modulatorTrail(instanceId));
        context.traversalMap.erase(traversalIt);
        return;
    }

    auto type = expiredNote.nodeType;

    jassert(nodes.find(traversal.primary.target) != nodes.end());

    if (type == RTNode::NodeType::RootNode && expiredNote.isConnectionTrigger) {
        bridge.highlightNode(expiredNote.nodeId, false, traversal.traversal.traversalId);

        if (traversal.shouldTraverse()) {
            pushRootNodeConnection(expiredNote.nodeId, context, expiryTime);
        }
    }
    else if (type == RTNode::NodeType::RootNode|| type == RTNode::NodeType::Node) {
        auto currentIt = nodes.find(traversal.primary.target);
        int  repeatValue = 1;

        if (currentIt != nodes.end()) {
            const RTNode& currentNode = *currentIt->second;
            int activeAltId = traversal.nodeState.get(NodeStateSlot::ActiveAlternative, currentNode.nodeID);

            if (activeAltId != -1) {
                auto altIt = nodes.find(activeAltId);
                repeatValue = 1;

                if (altIt != nodes.end()) {
                    repeatValue = altIt->second->repeatValue;
                }

            } else {
                repeatValue = currentNode.repeatValue;
            }
        }

        runtime.repeatCount++;

        if (runtime.repeatCount < repeatValue) {
            pushNote(traversal.getTargetNode(nodes), instanceId, context, expiryTime, true);
        } else {
            runtime.repeatCount = 0;

            const TraversalLogic::StepResult step = traversal.handleNodeEvent(nodes);

            applyStepResult(step, nodes, instanceId, traversal.traversal.traversalId);
            applyTreeJump(step, traversal, runtime, context);

            if (traversal.shouldTraverse() && nodes.find(traversal.primary.target) != nodes.end()) {
                pushNote(traversal.getTargetNode(nodes), instanceId, context, expiryTime);
            }
        }
    }
}

void TraversalDispatcher::pushRootNodeConnection(int rootNodeId, const DispatchContext& context, double sample)
{
    auto rootIt = context.nodes.find(rootNodeId);
    if (rootIt == context.nodes.end()) {
        return;
    }

    const RTNode& targetRootNode = *rootIt->second;

    if (context.traversalMap.hasActiveTraversalOnTree(targetRootNode.nodeID)) {
        return;
    }

    for (const RTtraversal& registeredTraversal : targetRootNode.traversals) {
        startCrossTreeTraversal(targetRootNode, registeredTraversal, sample, context);
    }
}

void TraversalDispatcher::startCrossTreeTraversal(const RTNode& targetRootNode, const RTtraversal& traversal,
                                                  double sample, const DispatchContext& context)
{
    const int rootId      = targetRootNode.nodeID;
    const int traversalId = traversal.traversalId;

    int instanceId = context.traversalMap.findInstanceFor(rootId, traversalId);

    if (instanceId == -1) {
        instanceId = context.traversalMap.nextInstanceId();
    }

    TraversalPool::Instance* instance = prepareTraversal(instanceId, rootId, rootId, traversal, context);

    if (instance == nullptr) {
        return;
    }

    instance->runtime.asCrossTree = true;

    bridge.highlightNode(targetRootNode, true, traversalId);
    pushNote(targetRootNode, instanceId, context, sample);
}

void TraversalDispatcher::applyGraphLoopLimit(TraversalLogic& traversalLogic, int rootId,
                                              const DispatchContext& context)
{
    auto rootIt = context.nodes.find(rootId);

    if (rootIt != context.nodes.end()) {
        traversalLogic.loop.limit = rootIt->second->graphLoopLimit;
    }
}

TraversalPool::Instance* TraversalDispatcher::prepareTraversal(int instanceId, int rootId, int startNodeId,
                                                             const RTtraversal& traversal, const DispatchContext& context)
{
    auto existingIt = context.traversalMap.find(instanceId);

    TraversalPool::Instance* instance = nullptr;

    if (existingIt != context.traversalMap.end()) {
        instance = &existingIt->second;
    } else {
        instance = context.traversalMap.acquire(instanceId, rootId, traversal);
    }

    if (instance == nullptr) {
        return nullptr;
    }

    TraversalLogic& logic = instance->logic;

    logic.instanceId     = instanceId;
    logic.primary.target = startNodeId;
    logic.state          = TraversalLogic::TraversalState::Active;
    logic.loop.active    = true;
    logic.loop.count     = 0;
    instance->runtime = {};

    applyGraphLoopLimit(logic, rootId, context);

    logic.advanceAlternative(context.nodes, startNodeId);

    return instance;
}

