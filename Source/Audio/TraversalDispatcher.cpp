#include "TraversalDispatcher.h"
#include "AudioUIBridge.h"
#include "../Util/ArrowInfo.h"
#include <algorithm>
#include <functional>

TraversalDispatcher::TraversalDispatcher(NoteScheduler& s, AudioUIBridge& b)
    : flagScheduler(*this, b), scheduler(s), bridge(b)
{
    chordVisitStamps.assign(NodeStateTable::maxNodeIds, 0);
    chordFrontier.reserve(NodeStateTable::maxNodeIds + 1);
    crossTreeScratch.reserve(TraversalLogic::maxCrossTreeTargets);
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
                                         int runId, int typeId)
{
    auto highlight = [&](int nodeId, bool on) {
        auto it = nodes.find(nodeId);

        jassert(it != nodes.end());

        if (it != nodes.end()) {
            bridge.highlightNode(*it->second, on, runId, typeId);
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
        bridge.pushArrowReset(AudioUIBridge::primaryTrail(runId));
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

void TraversalDispatcher::pushNote(const RTNode& node, int runId,
                                   const DispatchContext& context, double sample,
                                   bool isPrimaryRepeat)
{
    const NodeMap& nodes = context.nodes;

    TraversalPool::Instance* const traversalInstance = context.traversalMap.find(runId);

    if (traversalInstance == nullptr) {
        jassertfalse;
        return;
    }

    if (dispatchDepth >= maxDispatchDepth) {
        return;
    }

    ++dispatchDepth;

    TraversalLogic& traversalLogic = traversalInstance->logic;

    const RTNode* modulatorNode            = nullptr;
    const RTNode* nextModulatorTarget      = nullptr;
    const RTNode* alternativeNode          = nullptr;
    const RTNode* alternativeModulatorNode = nullptr;

    dispatchModulator(node, context, traversalLogic, modulatorNode, isPrimaryRepeat);

    if (traversalLogic.mod.walker.alternativeTarget != -1) {
        auto alternativeModulatorIt = nodes.find(traversalLogic.mod.walker.alternativeTarget);

        if (alternativeModulatorIt != nodes.end()) {
            alternativeModulatorNode = alternativeModulatorIt->second.get();
        }
    }

    const RTNode* nextTarget = traversalLogic.peekNextTarget(nodes);

    const double sampleRate = context.sampleRate;

    double traversalMultiplier = traversalLogic.traversal.tempoMultiplier;
    auto rootIt = nodes.find(traversalLogic.rootId);
    if (rootIt != nodes.end()) {
        for (const RTtraversal& t : rootIt->second->traversals) {
            if (t.key == traversalLogic.traversal.key) {
                traversalMultiplier = t.tempoMultiplier;
                break;
            }
        }
    }
    if (traversalMultiplier <= 0.0) {
        traversalMultiplier = 1.0;
    }

    const double tempoMultiplier = juce::jlimit(RTtraversal::minimumTempoMultiplier,
                                                RTtraversal::maximumTempoMultiplier,
                                                context.tempoMultiplier * traversalMultiplier);
    jassert(sampleRate > 0.0);

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

    const TraversalKey activeKey    = traversalLogic.traversal.key;
    const int          activeTypeId = activeKey.typeId;

    auto danglingArrowFor = [&](const RTNode& target) {
        const int count = traversalLogic.nodeState.get(NodeStateSlot::Count, target.nodeID) + 1;

        return traversalLogic.rule->selectDanglingArrow(target, count, activeKey);
    };

    const int nodeCount     = traversalLogic.nodeState.get(NodeStateSlot::Count, node.nodeID) + 1;
    const int danglingIndex = danglingArrowFor(node);

    int modulatorDanglingIndex = -1;

    if (alternativeNode != nullptr) {
        duration = resolveDuration(*alternativeNode, nextTarget, traversalLogic.primary.last, nodes,
                                   danglingArrowFor(*alternativeNode));
    }
    else {
        duration = resolveDuration(node, nextTarget, traversalLogic.primary.last, nodes, danglingIndex);
    }

    int transpose = traversalLogic.traversal.transpose;

    if (modulatorNode != nullptr && traversalLogic.mod.walker.target != -1) {
        const RTNode* modulatorValueNode = modulatorNode;

        if (alternativeModulatorNode != nullptr) {
            modulatorValueNode = alternativeModulatorNode;
        }

        const int modulatorCount = traversalLogic.nodeState.get(NodeStateSlot::ModulatorCount,
                                                                modulatorValueNode->nodeID) + 1;

        modulatorDanglingIndex = traversalLogic.rule->selectDanglingArrow(*modulatorValueNode, modulatorCount, activeKey);

        nextModulatorTarget = traversalLogic.decideNextModulator(nodes);

        int modulatorDuration = resolveDuration(*modulatorValueNode, nextModulatorTarget, traversalLogic.mod.walker.last,
                                                nodes, modulatorDanglingIndex);
        duration = static_cast<int>(juce::jlimit(0.0, ArrowInfo::maximumDurationMs,
                                                 duration * (0.001 * modulatorDuration)));

        transpose += modulatorValueNode->pitchOffset;
    }


    const NoteScheduler::NoteVoicing voicing {
        traversalLogic.traversal.channel,
        transpose,
        traversalLogic.traversal.velocityMultiplier,
        pitchOverride,
        velocityOverride
    };

    scheduler.scheduleNote(node, runId, sample, context.midiMessages,
                           sampleRate, tempoMultiplier, duration, false, voicing);

    pushChordNotes(node, sample, duration, tempoMultiplier, context, nodeCount, traversalLogic, transpose);

    const int wallClockMs = static_cast<int>(juce::jlimit(0.0, ArrowInfo::maximumDurationMs,
                                                          duration / tempoMultiplier));

    if (alternativeNode != nullptr) {

        auto alternativeNodeParentIterator = nodes.find(alternativeNode->parentId);

        if (alternativeNodeParentIterator != nodes.end()) {
            const RTNode* alternativeNodeParent = alternativeNodeParentIterator->second.get();
            dispatchPrimaryArrow(*alternativeNode, alternativeNodeParent, danglingArrowFor(*alternativeNode),
                                 runId, wallClockMs, activeTypeId);
        }
    }

    dispatchPrimaryArrow(node, nextTarget, danglingIndex, runId, wallClockMs, activeTypeId);
    dispatchModulatorArrow(modulatorNode, nextModulatorTarget, modulatorDanglingIndex, runId, wallClockMs, activeTypeId);

    if (alternativeModulatorNode != nullptr) {
        auto alternativeModulatorHostIt = nodes.find(alternativeModulatorNode->parentId);

        if (alternativeModulatorHostIt != nodes.end()) {
            dispatchModulatorArrow(alternativeModulatorNode, alternativeModulatorHostIt->second.get(),
                                   modulatorDanglingIndex, runId, wallClockMs, activeTypeId);
        }
    }

    dispatchCrossTree(node, runId, sample, tempoMultiplier, context, traversalLogic);
    flagScheduler.dispatchFlags(node, runId, activeKey, nodeCount,
                                sample, tempoMultiplier, context);

    --dispatchDepth;
}

void TraversalDispatcher::dispatchPrimaryArrow(const RTNode& node, const RTNode* nextTarget, int danglingIndex,
                                                int runId, int wallClockMs, int colourTypeId)
{
    if (nextTarget == nullptr && danglingIndex < 0) {
        return;
    }

    const int targetId = (nextTarget != nullptr) ? nextTarget->nodeID
                                                 : AudioUIBridge::danglingArrowKey(danglingIndex);

    bridge.pushProgress(node.nodeID, targetId, wallClockMs,
                        AudioUIBridge::primaryTrail(runId), colourTypeId);
}

void TraversalDispatcher::dispatchModulatorArrow(const RTNode* modulatorNode,
                                                  const RTNode* nextModulatorTarget,
                                                  int danglingIndex,
                                                  int runId, int wallClockMs, int colourTypeId)
{
    if (modulatorNode == nullptr) {
        return;
    }

    if (nextModulatorTarget == nullptr && danglingIndex < 0) {
        bridge.pushArrowReset(AudioUIBridge::modulatorTrail(runId));
        return;
    }

    int targetId = AudioUIBridge::danglingArrowKey(danglingIndex);

    if (nextModulatorTarget != nullptr) {
        targetId = nextModulatorTarget->nodeID;
    }

    bridge.pushProgress(modulatorNode->nodeID, targetId, wallClockMs,
                        AudioUIBridge::modulatorTrail(runId), colourTypeId);
}

void TraversalDispatcher::dispatchCrossTree(const RTNode& node, int sourceRunId, double sample,
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

        if (context.traversalMap.hasAllRegisteredRunsOnTree(crossTreeRoot)) {
            continue;
        }

        bridge.highlightNode(crossTreeRoot, true, sourceRunId, traversal.traversal.key.typeId);

        int connectionDuration = 1000;
        int progressSourceId   = node.nodeID;

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
                    progressSourceId   = traversal.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID);
                }
            }
        }

        const NoteScheduler::NoteVoicing crossTreeVoicing {
            traversal.traversal.channel,
            traversal.traversal.transpose,
            traversal.traversal.velocityMultiplier
        };

        scheduler.scheduleNote(crossTreeRoot, sourceRunId, sample, context.midiMessages,
                               context.sampleRate, tempoMultiplier, connectionDuration, true,
                               crossTreeVoicing);

        const int wallClockMs = static_cast<int>(juce::jlimit(0.0, ArrowInfo::maximumDurationMs,
                                                              connectionDuration / tempoMultiplier));
        bridge.pushProgress(progressSourceId, crossTreeRootId, wallClockMs,
                            AudioUIBridge::primaryTrail(sourceRunId),
                            traversal.traversal.key.typeId, true);
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

        bridge.highlightNode(chordNode, true, traversalLogic.runId, traversalLogic.traversal.key.typeId);

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
    const int      runId    = traversalLogic.runId;
    const int      typeId   = traversalLogic.traversal.key.typeId;

    int triggeredRootId = traversalLogic.findActiveModulatorRoot(nodes, node.nodeID);

    if (triggeredRootId != -1) {
        if (mod.walker.target != -1) {
            int previousDisplayedId = mod.walker.target;

            if (mod.walker.alternativeTarget != -1) {
                previousDisplayedId = mod.walker.alternativeTarget;
            }

            auto prevIt = nodes.find(previousDisplayedId);
            if (prevIt != nodes.end()) {
                bridge.highlightNode(*prevIt->second, false, runId, typeId);
            }
        }

        mod.activate(triggeredRootId, node.nodeID);

        traversalLogic.advanceAlternative(nodes, mod.walker.target);

        bridge.pushArrowReset(AudioUIBridge::modulatorTrail(runId));
    }
    else if (mod.isActive()) {
        bool isHost       = (node.nodeID == mod.gate.hostId);
        bool isDescendant = TraversalLogic::isDescendantOf(nodes, node.nodeID, mod.gate.hostId);

        if (!isHost && !isDescendant) {
            int displayedId = mod.walker.target;

            if (mod.walker.alternativeTarget != -1) {
                displayedId = mod.walker.alternativeTarget;
            }

            auto targetIt = nodes.find(displayedId);
            if (targetIt != nodes.end()) {
                bridge.highlightNode(*targetIt->second, false, runId, typeId);
            }

            mod.deactivate();

            bridge.pushArrowReset(AudioUIBridge::modulatorTrail(runId));

            modulatorNode = nullptr;
            return;
        }

        bool advancesOnHost = false;

        if (isHost) {
            const RTConnection* const modRootConnection = node.findConnection(mod.gate.activeRootId);

            if (modRootConnection != nullptr && ! modRootConnection->isSynced) {
                advancesOnHost = true;
            }
        }

        if ((isDescendant || advancesOnHost) && !isPrimaryRepeat) {
            int repeatSourceId = mod.walker.target;

            if (mod.walker.alternativeTarget != -1) {
                repeatSourceId = mod.walker.alternativeTarget;
            }

            auto targetIt             = nodes.find(repeatSourceId);
            int  modulatorRepeatValue = 1;

            if (targetIt != nodes.end()) {
                modulatorRepeatValue = targetIt->second->repeatValue;
            }

            if (mod.tickRepeat(modulatorRepeatValue)) {
                const bool modulatorAdvances = (mod.decidedTarget != -1);

                if (mod.step()) {
                    bridge.pushArrowReset(AudioUIBridge::modulatorTrail(runId));
                }

                if (modulatorAdvances) {
                    traversalLogic.advanceAlternative(nodes, mod.walker.target);
                }
            }
        }
    }

    if (!mod.isActive() || mod.walker.target == -1) {
        modulatorNode = nullptr;
        return;
    }

    auto targetIt = nodes.find(mod.walker.target);
    modulatorNode = nullptr;

    if (targetIt != nodes.end()) {
        modulatorNode = targetIt->second.get();
    }

    int displayedId = mod.walker.target;

    if (mod.walker.alternativeTarget != -1) {
        displayedId = mod.walker.alternativeTarget;
    }

    if (modulatorNode != nullptr) {
        auto displayedIt = nodes.find(displayedId);

        if (displayedIt != nodes.end()) {
            bridge.highlightNode(*displayedIt->second, true, runId, typeId);
        }
    }

    int lastDisplayedId = mod.walker.last;

    if (mod.walker.alternativeLast != -1) {
        lastDisplayedId = mod.walker.alternativeLast;
    }

    if (lastDisplayedId != -1 && lastDisplayedId != displayedId) {
        auto lastIt = nodes.find(lastDisplayedId);
        if (lastIt != nodes.end()) {
            bridge.highlightNode(*lastIt->second, false, runId, typeId);
        }
    }
}

void TraversalDispatcher::handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                                            double expiryTime,
                                            const DispatchContext& context)
{
    const NodeMap& nodes = context.nodes;

    int runId = expiredNote.runId;

    TraversalPool::Instance* const traversalInstance = context.traversalMap.find(runId);

    if (traversalInstance == nullptr) {
        bridge.highlightNode(expiredNote.nodeId, false, runId);
        return;
    }

    TraversalLogic&   traversal = traversalInstance->logic;
    TraversalRuntime& runtime   = traversalInstance->runtime;

    if (runtime.pendingRemoval) {
        bridge.highlightNode(expiredNote.nodeId, false, runId, traversal.traversal.key.typeId);
        bridge.pushArrowReset(AudioUIBridge::primaryTrail(runId));
        bridge.pushArrowReset(AudioUIBridge::modulatorTrail(runId));
        context.traversalMap.erase(runId);
        return;
    }

    auto type = expiredNote.nodeType;

    jassert(nodes.find(traversal.primary.target) != nodes.end());

    if (type == RTNode::NodeType::RootNode && expiredNote.isConnectionTrigger) {
        bridge.highlightNode(expiredNote.nodeId, false, runId, traversal.traversal.key.typeId);

        if (traversal.shouldTraverse()) {
            pushRootNodeConnection(expiredNote.nodeId, context, expiryTime);
        }
    }
    else if (type == RTNode::NodeType::RootNode|| type == RTNode::NodeType::Node) {
        auto currentIt   = nodes.find(traversal.primary.target);
        int  repeatValue = 1;

        if (currentIt != nodes.end()) {
            const RTNode& currentNode = *currentIt->second;
            int           activeAltId = traversal.nodeState.get(NodeStateSlot::ActiveAlternative, currentNode.nodeID);

            if (activeAltId != -1) {
                auto altIt  = nodes.find(activeAltId);
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
            pushNote(traversal.getTargetNode(nodes), runId, context, expiryTime, true);
        } else {
            runtime.repeatCount = 0;

            const TraversalLogic::StepResult step = traversal.handleNodeEvent(nodes);

                applyStepResult(step, nodes, runId, traversal.traversal.key.typeId);
            applyTreeJump(step, traversal, runtime, context);

            if (traversal.shouldTraverse() && nodes.find(traversal.primary.target) != nodes.end()) {
                pushNote(traversal.getTargetNode(nodes), runId, context, expiryTime);
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

    if (context.traversalMap.hasAllRegisteredRunsOnTree(targetRootNode)) {
        return;
    }

    for (const RTtraversal& registeredTraversal : targetRootNode.traversals) {
        startCrossTreeTraversal(targetRootNode, registeredTraversal, sample, context);
    }
}

void TraversalDispatcher::startCrossTreeTraversal(const RTNode& targetRootNode, const RTtraversal& traversal,
                                                  double sample, const DispatchContext& context)
{
    const int rootId = targetRootNode.nodeID;

    int runId = context.traversalMap.findRunFor(rootId, traversal.key);

    if (runId == -1) {
        runId = context.traversalMap.nextRunId();
    }
    else {
        const TraversalPool::Instance* const existingInstance = context.traversalMap.find(runId);

        if (existingInstance != nullptr && existingInstance->logic.shouldTraverse()) {
            return;
        }
    }

    TraversalPool::Instance* instance = prepareTraversal(runId, rootId, rootId, traversal, context);

    if (instance == nullptr) {
        return;
    }

    instance->runtime.asCrossTree = true;

    bridge.highlightNode(targetRootNode, true, runId, traversal.key.typeId);
    pushNote(targetRootNode, runId, context, sample);
}

void TraversalDispatcher::applyGraphLoopLimit(TraversalLogic& traversalLogic, int rootId,
                                              const DispatchContext& context)
{
    auto rootIt = context.nodes.find(rootId);

    if (rootIt != context.nodes.end()) {
        traversalLogic.loop.limit = rootIt->second->graphLoopLimit;
    }
}

TraversalPool::Instance* TraversalDispatcher::prepareTraversal(int runId, int rootId, int startNodeId,
                                                             const RTtraversal& traversal, const DispatchContext& context)
{
    TraversalPool::Instance* instance = context.traversalMap.find(runId);

    if (instance == nullptr) {
        instance = context.traversalMap.acquire(runId, rootId, traversal);
    }

    if (instance == nullptr) {
        return nullptr;
    }

    TraversalLogic& logic = instance->logic;

    logic.runId          = runId;
    logic.primary.target = startNodeId;
    logic.state          = TraversalLogic::TraversalState::Active;
    logic.loop.active    = true;
    logic.loop.count     = 0;
    instance->runtime    = {};

    applyGraphLoopLimit(logic, rootId, context);

    logic.advanceAlternative(context.nodes, startNodeId);

    return instance;
}

