#include "TraversalDispatcher.h"
#include "AudioUIBridge.h"
#include "../Util/ArrowInfo.h"
#include <algorithm>
#include <functional>

TraversalDispatcher::TraversalDispatcher(NoteScheduler& s, AudioUIBridge& b)
    : flagScheduler(*this, b), scheduler(s), bridge(b)
{
    chordVisits.prepare();
    chordFrontier.reserve(NodeStateTable::maxNodeIds + 1);
    crossTreeScratch.reserve(TraversalLogic::maxCrossTreeTargets);
}

bool TraversalDispatcher::markChordVisited(int nodeId)
{
    if (chordVisits.find(nodeId) >= 0) {
        return false;
    }

    return chordVisits.claim(nodeId) >= 0;
}

void TraversalDispatcher::applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes,
                                         int runId, int typeId)
{
    auto highlight = [&](int nodeId, AudioUIBridge::HighlightKind kind) {
        const RTNode* highlightedNode = nodes.find(nodeId);

        jassert(highlightedNode != nullptr);

        if (highlightedNode != nullptr) {
            bridge.highlightNode(*highlightedNode, kind, runId, typeId);
        }
    };

    if (step.leftAlternativeId != -1) {
        highlight(step.leftAlternativeId, AudioUIBridge::HighlightKind::Hide);
    } else if (step.leftId != -1) {
        highlight(step.leftId, AudioUIBridge::HighlightKind::Hide);
    }

    if (step.enteredAlternativeId != -1) {
        highlight(step.enteredAlternativeId, AudioUIBridge::HighlightKind::Show);
    } else if (step.enteredId != -1) {
        highlight(step.enteredId, AudioUIBridge::HighlightKind::Show);
    }

    if (step.referenceOffId != -1) {
        highlight(step.referenceOffId, AudioUIBridge::HighlightKind::Hide);
    }

    if (step.clearTrail) {
        bridge.pushArrowReset(AudioUIBridge::primaryTrail(runId));
    }

    if (step.pushCounts && step.countSourceNodeId != -1) {
        bridge.pushCount(step.countSourceNodeId, 0, 1);

        const RTNode* countSourceNode = nodes.find(step.countSourceNodeId);

        if (countSourceNode != nullptr) {
            for (const RTConnection& connection : countSourceNode->connections) {
                const int childId = connection.childId;

                const RTNode* childNode = nodes.find(childId);
                if (childNode == nullptr) {
                    continue;
                }

                int limit = childNode->countLimit;
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

    const RTNode* const rootNode = context.nodes.find(traversal.rootId);

    if (rootNode != nullptr) {
        traversal.loop.limit = rootNode->graphLoopLimit;
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
            const RTNode* parentNode = nodes.find(lastTargetId);
            if (parentNode != nullptr) {
                const RTConnection* const connection = parentNode->findConnection(node.nodeID);

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

    dispatchModulator(node, runId, context, traversalLogic, modulatorNode, isPrimaryRepeat);

    if (traversalLogic.mod.walker.alternativeTarget != -1) {
        alternativeModulatorNode = nodes.find(traversalLogic.mod.walker.alternativeTarget);
    }

    const RTNode* nextTarget = traversalLogic.peekNextTarget(nodes);

    const double sampleRate = context.sampleRate;

    double traversalMultiplier = traversalLogic.traversal.tempoMultiplier;
    const RTNode* rootNode            = nodes.find(traversalLogic.rootId);
    if (rootNode != nullptr) {
        for (const RTtraversal& t : rootNode->traversals) {
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
        const RTNode* altNode = nodes.find(traversalLogic.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID));

        if (altNode != nullptr) {
            alternativeNode = altNode;

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
        transpose + context.transpose,
        traversalLogic.traversal.velocityMultiplier * context.velocityScale,
        pitchOverride,
        velocityOverride
    };

    scheduler.scheduleNote(node, NoteScheduler::NoteRole::Stepping, runId, sample,
                           context.midiMessages,
                           sampleRate, tempoMultiplier, duration, false, voicing);

    pushChordNotes(node, runId, sample, duration, tempoMultiplier, context, nodeCount, traversalLogic, transpose);

    const int wallClockMs = static_cast<int>(juce::jlimit(0.0, ArrowInfo::maximumDurationMs,
                                                          duration / tempoMultiplier));

    if (alternativeNode != nullptr) {

        const RTNode* alternativeNodeParent = nodes.find(alternativeNode->parentId);

        if (alternativeNodeParent != nullptr) {
            dispatchPrimaryArrow(*alternativeNode, alternativeNodeParent, danglingArrowFor(*alternativeNode),
                                 runId, wallClockMs, activeTypeId);
        }
    }

    dispatchPrimaryArrow(node, nextTarget, danglingIndex, runId, wallClockMs, activeTypeId);
    dispatchModulatorArrow(modulatorNode, nextModulatorTarget, modulatorDanglingIndex, runId, wallClockMs, activeTypeId);

    if (alternativeModulatorNode != nullptr) {
        const RTNode* alternativeModulatorHost = nodes.find(alternativeModulatorNode->parentId);

        if (alternativeModulatorHost != nullptr) {
            dispatchModulatorArrow(alternativeModulatorNode, alternativeModulatorHost,
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
                                                 : (-(danglingIndex + 1));

    bridge.pushProgress(node.nodeID, targetId, wallClockMs, AudioUIBridge::primaryTrail(runId), colourTypeId);
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

    int targetId = (-(danglingIndex + 1));

    if (nextModulatorTarget != nullptr) {
        targetId = nextModulatorTarget->nodeID;
    }

    bridge.pushProgress(modulatorNode->nodeID, targetId, wallClockMs, AudioUIBridge::modulatorTrail(runId), colourTypeId);
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
        const RTNode* crossTreeNode = nodes.find(crossTreeRootId);
        if (crossTreeNode == nullptr) {
            continue;
        }

        const RTNode& crossTreeRoot = *crossTreeNode;

        if (context.traversalMap.hasAllRegisteredRunsOnTree(crossTreeRoot)) {
            continue;
        }

        bridge.highlightNode(crossTreeRoot, AudioUIBridge::HighlightKind::Show, sourceRunId,
                             traversal.traversal.key.typeId);

        int connectionDuration = 1000;
        int progressSourceId   = node.nodeID;

        const RTConnection* const crossTreeConnection = node.findConnection(crossTreeRootId);

        if (crossTreeConnection != nullptr && crossTreeConnection->duration >= 0) {
            connectionDuration = crossTreeConnection->duration;
        }
        else if (traversal.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID) != -1) {
            const RTNode* altNode = nodes.find(traversal.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID));
            if (altNode != nullptr) {
                const RTConnection* const alternativeConnection = altNode->findConnection(crossTreeRootId);

                if (alternativeConnection != nullptr && alternativeConnection->duration >= 0) {
                    connectionDuration = alternativeConnection->duration;
                    progressSourceId   = traversal.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID);
                }
            }
        }

        const NoteScheduler::NoteVoicing crossTreeVoicing {
            traversal.traversal.channel,
            traversal.traversal.transpose + context.transpose,
            traversal.traversal.velocityMultiplier * context.velocityScale
        };

        scheduler.scheduleNote(crossTreeRoot, NoteScheduler::NoteRole::Stepping, sourceRunId, sample,
                               context.midiMessages,
                               context.sampleRate, tempoMultiplier, connectionDuration, true,
                               crossTreeVoicing);

        const int wallClockMs = static_cast<int>(juce::jlimit(0.0, ArrowInfo::maximumDurationMs,
                                                              connectionDuration / tempoMultiplier));
        bridge.pushProgress(progressSourceId, crossTreeRootId, wallClockMs, AudioUIBridge::primaryTrail(sourceRunId),
                            traversal.traversal.key.typeId, AudioUIBridge::ArrowKind::Connection);
    }
}

void TraversalDispatcher::pushChordNotes(const RTNode& node, int runId, double sample, int duration,
                                          double tempoMultiplier, const DispatchContext& context,
                                          int parentCount, TraversalLogic& traversalLogic, int transpose)
{
    const NodeMap& nodes = context.nodes;

    chordVisits.clear();
    chordFrontier.clear();

    chordFrontier.push_back({ node.nodeID, parentCount });

    auto scheduleChordMember = [&](int childId, int chainCount) {
        if (!markChordVisited(childId)) {
            return;
        }

        const RTNode* childNode = nodes.find(childId);

        if (childNode == nullptr) {
            return;
        }

        const RTNode& chordNode = *childNode;

        if (!(NoteScheduler::isNodeAudible(chordNode.nodeType)
        && chordNode.nodeType != RTNode::NodeType::RootNode)) {
            return;
        }

        if (chordNode.countLimit <= 0 || chainCount % chordNode.countLimit != 0) {
            return;
        }

        const NoteScheduler::NoteVoicing chordVoicing {
            traversalLogic.traversal.channel,
            transpose + context.transpose,
            traversalLogic.traversal.velocityMultiplier * context.velocityScale
        };

        scheduler.scheduleNote(chordNode, NoteScheduler::NoteRole::ChordVoice, runId, sample,
                               context.midiMessages,
                               context.sampleRate, tempoMultiplier, duration, false, chordVoicing);

        bridge.highlightNode(chordNode, AudioUIBridge::HighlightKind::Show, runId, traversalLogic.traversal.key.typeId);

        int chordPlayCount = traversalLogic.nodeState.increment(NodeStateSlot::Chord, chordNode.nodeID);
        chordFrontier.push_back({ chordNode.nodeID, chordPlayCount });
    };

    while (!chordFrontier.empty())
    {
        const auto [chainNodeId, chainCount] = chordFrontier.back();
        chordFrontier.pop_back();

        const RTNode* chainEntry = nodes.find(chainNodeId);
        if (chainEntry == nullptr) {
            continue;
        }

        const RTNode& chainNode = *chainEntry;

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

void TraversalDispatcher::dispatchModulator(const RTNode& node, int runId, const DispatchContext& context,
                                            TraversalLogic& traversalLogic, const RTNode*& modulatorNode,
                                            bool isPrimaryRepeat)
{
    const NodeMap& nodes    = context.nodes;
    auto&          mod      = traversalLogic.mod;
    const int      typeId   = traversalLogic.traversal.key.typeId;

    int triggeredRootId = traversalLogic.findActiveModulatorRoot(nodes, node.nodeID);

    if (triggeredRootId != -1) {
        if (mod.walker.target != -1) {
            int previousDisplayedId = mod.walker.target;

            if (mod.walker.alternativeTarget != -1) {
                previousDisplayedId = mod.walker.alternativeTarget;
            }

            const RTNode* prevNode = nodes.find(previousDisplayedId);
            if (prevNode != nullptr) {
                bridge.highlightNode(*prevNode, AudioUIBridge::HighlightKind::Hide, runId, typeId);
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

            const RTNode* targetNode = nodes.find(displayedId);
            if (targetNode != nullptr) {
                bridge.highlightNode(*targetNode, AudioUIBridge::HighlightKind::Hide, runId, typeId);
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

            const RTNode* targetNode           = nodes.find(repeatSourceId);
            int  modulatorRepeatValue = 1;

            if (targetNode != nullptr) {
                modulatorRepeatValue = targetNode->repeatValue;
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

    const RTNode* targetNode = nodes.find(mod.walker.target);
    modulatorNode = nullptr;

    if (targetNode != nullptr) {
        modulatorNode = targetNode;
    }

    int displayedId = mod.walker.target;

    if (mod.walker.alternativeTarget != -1) {
        displayedId = mod.walker.alternativeTarget;
    }

    if (modulatorNode != nullptr) {
        const RTNode* displayedNode = nodes.find(displayedId);

        if (displayedNode != nullptr) {
            bridge.highlightNode(*displayedNode, AudioUIBridge::HighlightKind::Show, runId, typeId);
        }
    }

    int lastDisplayedId = mod.walker.last;

    if (mod.walker.alternativeLast != -1) {
        lastDisplayedId = mod.walker.alternativeLast;
    }

    if (lastDisplayedId != -1 && lastDisplayedId != displayedId) {
        const RTNode* lastNode = nodes.find(lastDisplayedId);
        if (lastNode != nullptr) {
            bridge.highlightNode(*lastNode, AudioUIBridge::HighlightKind::Hide, runId, typeId);
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
        bridge.highlightNode(expiredNote.nodeId, AudioUIBridge::HighlightKind::Hide, runId);
        return;
    }

    TraversalLogic&   traversal = traversalInstance->logic;
    TraversalRuntime& runtime   = traversalInstance->runtime;

    if (runtime.pendingRemoval) {
        const int removedTypeId = traversal.traversal.key.typeId;

        bridge.highlightNode(expiredNote.nodeId, AudioUIBridge::HighlightKind::Hide, runId, removedTypeId);

        if (traversal.primary.alternativeTarget != -1) {
            bridge.highlightNode(traversal.primary.alternativeTarget, AudioUIBridge::HighlightKind::Hide, runId,
                                 removedTypeId);
        }

        if (traversal.mod.isActive()) {
            int displayedModulatorId = traversal.mod.walker.target;

            if (traversal.mod.walker.alternativeTarget != -1) {
                displayedModulatorId = traversal.mod.walker.alternativeTarget;
            }

            if (displayedModulatorId != -1) {
                bridge.highlightNode(displayedModulatorId, AudioUIBridge::HighlightKind::Hide, runId, removedTypeId);
            }
        }

        bridge.pushArrowReset(AudioUIBridge::primaryTrail(runId));
        bridge.pushArrowReset(AudioUIBridge::modulatorTrail(runId));
        context.traversalMap.erase(runId);
        return;
    }

    auto type = expiredNote.nodeType;

    jassert(nodes.find(traversal.primary.target) != nullptr);

    if (type == RTNode::NodeType::RootNode && expiredNote.isConnectionTrigger) {
        bridge.highlightNode(expiredNote.nodeId, AudioUIBridge::HighlightKind::Hide, runId,
                             traversal.traversal.key.typeId);

        if (traversal.shouldTraverse()) {
            pushRootNodeConnection(expiredNote.nodeId, context, expiryTime);
        }
    }
    else if (type == RTNode::NodeType::RootNode|| type == RTNode::NodeType::Node) {
        const RTNode* currentEntry = nodes.find(traversal.primary.target);
        int  repeatValue = 1;

        if (currentEntry != nullptr) {
            const RTNode& currentNode = *currentEntry;
            int           activeAltId = traversal.nodeState.get(NodeStateSlot::ActiveAlternative, currentNode.nodeID);

            if (activeAltId != -1) {
                const RTNode* altNode = nodes.find(activeAltId);
                repeatValue = 1;

                if (altNode != nullptr) {
                    repeatValue = altNode->repeatValue;
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

            if (!traversal.shouldTraverse()) {
                if (traversal.mod.isActive()) {
                    int displayedModulatorId = traversal.mod.walker.target;

                    if (traversal.mod.walker.alternativeTarget != -1) {
                        displayedModulatorId = traversal.mod.walker.alternativeTarget;
                    }

                    if (displayedModulatorId != -1) {
                        bridge.highlightNode(displayedModulatorId, AudioUIBridge::HighlightKind::Hide, runId,
                                             traversal.traversal.key.typeId);
                    }

                    bridge.pushArrowReset(AudioUIBridge::modulatorTrail(runId));
                }
            }
            else if (nodes.find(traversal.primary.target) != nullptr) {
                pushNote(traversal.getTargetNode(nodes), runId, context, expiryTime);
            }
        }
    }
}

void TraversalDispatcher::pushRootNodeConnection(int rootNodeId, const DispatchContext& context, double sample)
{
    const RTNode* rootNode = context.nodes.find(rootNodeId);
    if (rootNode == nullptr) {
        return;
    }

    const RTNode& targetRootNode = *rootNode;

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

    bridge.highlightNode(targetRootNode, AudioUIBridge::HighlightKind::Show, runId, traversal.key.typeId);
    pushNote(targetRootNode, runId, context, sample);
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

    int graphLoopLimit = instance->logic.loop.limit;

    const RTNode* const rootNode = context.nodes.find(rootId);

    if (rootNode != nullptr) {
        graphLoopLimit = rootNode->graphLoopLimit;
    }

    instance->runtime = {};

    instance->logic.begin(context.nodes, startNodeId, graphLoopLimit);

    return instance;
}

