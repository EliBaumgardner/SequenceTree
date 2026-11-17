#include "TraversalDispatcher.h"
#include "Processor/Channels/AudioUIBridge.h"
#include "Core/Model/ArrowInfo.h"
#include <algorithm>
#include <functional>

TraversalDispatcher::TraversalDispatcher(NoteScheduler& s, AudioUIBridge& b)
    : flagScheduler(*this, b),
      scheduler    (s),
      bridge       (b)
{
    const int chordFrontierCapacity = NodeStateTable::maxNodeIds + 1;

    chordVisits.prepare();

    chordFrontier   .reserve(chordFrontierCapacity);
    crossTreeScratch.reserve(TraversalLogic::maxCrossTreeTargets);
}

int TraversalDispatcher::advance(int steps)
{
    if (mainRun.instance == nullptr) {
        return -1;
    }

    TraversalLogic&   traversal = mainRun.instance->logic;
    TraversalRuntime& runtime   = mainRun.instance->runtime;

    const DispatchContext& context = *mainRun.context;

    if (steps < 1 && traversal.shouldTraverse()) {
        return traversal.primary.target;
    }

    const TraversalLogic::StepResult step = traversal.handleNodeEvent(context.nodes, steps);

    applyStepResult(step, context.nodes, mainRun.runId, traversal.traversal.key.typeId);

    applyTreeJump(step, traversal, runtime, context);

    if (!traversal.shouldTraverse()) {
        return -1;
    }

    return traversal.primary.target;
}

int TraversalDispatcher::noteDuration(int nodeId)
{
    if (mainRun.instance == nullptr) {
        return 0;
    }

    TraversalLogic& traversal = mainRun.instance->logic;

    const NodeMap& nodes = mainRun.context->nodes;

    const RTNode* const node        = nodes.find(nodeId);
    const RTNode* const alternative = nodes.find(traversal.childSelector.nodeState.get(NodeStateSlot::ActiveAlternative, nodeId));
    const RTNode*       voiced      = node;
    const RTNode*       nextTarget  = nullptr;

    if (node == nullptr) {
        return 0;
    }

    if (alternative != nullptr && !node->notes.empty()) {
        voiced = alternative;
    }

    if (nodeId == traversal.primary.target) {
        nextTarget = traversal.peekNextTarget(nodes);
    }

    const int visitCount    = traversal.childSelector.nodeState.get(NodeStateSlot::Count, voiced->nodeID) + 1;
    const int danglingIndex = traversal.childSelector.rule->selectDanglingArrow(*voiced, visitCount, traversal.traversal.key);

    return resolveDuration(*voiced, nextTarget, traversal.primary.last, nodes, danglingIndex);
}

void TraversalDispatcher::playNote(const ScriptNote& note)
{
    if (mainRun.context == nullptr || mainRun.context->nodes.find(note.nodeId) == nullptr) {
        return;
    }

    mainRun.note = note;
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

    const NotePlan plan = planNote(node, runId, context, traversalLogic, traversalInstance->runtime.scriptNote, isPrimaryRepeat);

    const TraversalKey activeKey = traversalLogic.traversal.key;

    BridgeCommands::TrailSource trailSource = BridgeCommands::TrailSource::Live;

    if (traversalLogic.mode == TraversalMode::Preview) {
        trailSource = BridgeCommands::TrailSource::Preview;
    }

    const double wallClockDuration = plan.duration / plan.tempoMultiplier;

    const int wallClockMs    = static_cast<int>(juce::jlimit(0.0, ArrowInfo::maximumDurationMs, wallClockDuration));
    const int chordTranspose = plan.transpose + plan.scriptTranspose;

    const NoteDispatch dispatch {
        .node            = node,
        .traversal       = traversalLogic,
        .runId           = runId,
        .duration        = plan.duration,
        .wallClockMs     = wallClockMs,
        .typeId          = activeKey.typeId,
        .sample          = sample,
        .tempoMultiplier = plan.tempoMultiplier,
        .source          = trailSource
    };

    const NoteScheduler::NoteTiming stepTiming { .runId = runId, .duration = plan.duration, .sample = sample, .tempoMultiplier = plan.tempoMultiplier };

    scheduler.scheduleNote(node, NoteScheduler::NoteRole::Stepping, NoteScheduler::NoteTrigger::Step, stepTiming, context, plan.voicing);

    if (plan.voicing.sound == NoteScheduler::Sound::Audible) {
        pushChordNotes(dispatch, context, plan.nodeCount, chordTranspose);
    }

    dispatchVisualProgress(dispatch, plan, nodes);
    dispatchCrossTree(dispatch, context);

    flagScheduler.dispatchFlags(node, activeKey, plan.nodeCount, stepTiming, context);

    --dispatchDepth;
}

void TraversalDispatcher::handleExpiredNote(const NoteScheduler::ActiveNote& expiredNote,
                                            double expiryTime,
                                            const DispatchContext& context)
{
    const NodeMap& nodes = context.nodes;

    int runId = expiredNote.runId;

    TraversalPool::Instance* const traversalInstance = context.traversalMap.find(runId);

    if (traversalInstance == nullptr) {
        bridge.highlightNode(expiredNote.nodeId, BridgeCommands::HighlightKind::Hide, runId);

        return;
    }

    TraversalLogic&   traversal = traversalInstance->logic;
    TraversalRuntime& runtime   = traversalInstance->runtime;

    if (runtime.pendingRemoval) {
        const int removedTypeId = traversal.traversal.key.typeId;

        bridge.highlightNode(expiredNote.nodeId, BridgeCommands::HighlightKind::Hide, runId, removedTypeId);

        if (traversal.primary.alternativeTarget != -1) {
            bridge.highlightNode(traversal.primary.alternativeTarget, BridgeCommands::HighlightKind::Hide, runId, removedTypeId);
        }

        if (traversal.mod.isActive()) {
            hideModulatorHighlight(traversal.mod.walker, runId, removedTypeId);
        }

        bridge.pushArrowReset(BridgeCommands::primaryTrail(runId));
        bridge.pushArrowReset(BridgeCommands::modulatorTrail(runId));

        context.traversalMap.erase(runId);

        return;
    }

    auto type = expiredNote.nodeType;

    jassert(nodes.find(traversal.primary.target) != nullptr);

    if (type == NodeInfo::Kind::Root && expiredNote.isConnectionTrigger) {
        bridge.highlightNode(expiredNote.nodeId, BridgeCommands::HighlightKind::Hide, runId, traversal.traversal.key.typeId);

        if (traversal.shouldTraverse()) {
            pushRootNodeConnection(expiredNote.nodeId, context, expiryTime);
        }
    }
    else if (type == NodeInfo::Kind::Root|| type == NodeInfo::Kind::Node) {
        const RTNode* currentEntry = nodes.find(traversal.primary.target);

        int repeatValue = 1;

        if (currentEntry != nullptr) {
            const RTNode& currentNode = *currentEntry;

            int activeAltId = traversal.childSelector.nodeState.get(NodeStateSlot::ActiveAlternative, currentNode.nodeID);

            if (activeAltId != -1 && activeAltId != NodeStateTable::hostPlays) {
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

        if (currentEntry != nullptr && runtime.repeatCount < repeatValue) {
            pushNote(*currentEntry, runId, context, expiryTime, true);
        } else {
            runtime.repeatCount = 0;

            stepTraversal(*traversalInstance, runId, context, expiryTime);
        }
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

    int graphLoopLimit = instance->logic.loop.limit;

    const RTNode* const rootNode = context.nodes.find(rootId);

    if (rootNode != nullptr) {
        graphLoopLimit = rootNode->graphLoopLimit;
    }

    instance->runtime = {};

    instance->logic.begin(context.nodes, startNodeId, graphLoopLimit);

    return instance;
}

void TraversalDispatcher::applyStepResult(const TraversalLogic::StepResult& step, const NodeMap& nodes,
                                         int runId, int typeId)
{
    auto highlight = [&](int nodeId, BridgeCommands::HighlightKind kind) {
        const RTNode* highlightedNode = nodes.find(nodeId);

        jassert(highlightedNode != nullptr);

        if (highlightedNode != nullptr) {
            bridge.highlightNode(*highlightedNode, kind, runId, typeId);
        }
    };

    if (step.leftAlternativeId != -1) {
        highlight(step.leftAlternativeId, BridgeCommands::HighlightKind::Hide);
    } else if (step.leftId != -1) {
        highlight(step.leftId, BridgeCommands::HighlightKind::Hide);
    }

    if (step.enteredAlternativeId != -1) {
        highlight(step.enteredAlternativeId, BridgeCommands::HighlightKind::Show);
    } else if (step.enteredId != -1) {
        highlight(step.enteredId, BridgeCommands::HighlightKind::Show);
    }

    if (step.referenceOffId != -1) {
        highlight(step.referenceOffId, BridgeCommands::HighlightKind::Hide);
    }

    if (step.clearTrail) {
        bridge.pushArrowReset(BridgeCommands::primaryTrail(runId));
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

void TraversalDispatcher::applyTreeJump(const TraversalLogic::StepResult& step, TraversalLogic& traversal, TraversalRuntime& runtime,
                                        const DispatchContext& context)
{
    if (step.kind != TraversalLogic::StepResult::Kind::JumpedToTree) {
        return;
    }

    if (runtime.originRootId == -1) {
        runtime.originRootId = step.jumpedFromRootId;
    }

    const RTNode* const rootNode = context.nodes.find(traversal.rootId);

    if (traversal.mode == TraversalMode::Preview) {
        return;
    }

    if (rootNode != nullptr) {
        traversal.loop.limit = rootNode->graphLoopLimit;
    }
}

void TraversalDispatcher::stepTraversal(TraversalPool::Instance& instance, int runId,
                                        const DispatchContext& context, double expiryTime)
{
    TraversalLogic&   traversal = instance.logic;
    TraversalRuntime& runtime   = instance.runtime;

    const bool scriptHasMain = traversal.script != nullptr && traversal.script->mainFunction != -1;

    mainRun = { &instance, &context, runId, {} };

    if (scriptHasMain) {
        ScriptRun run(traversal.makeScriptContext(context.nodes, Walk::Primary, ScriptWrites::Commit, this));

        run.call(traversal.script->mainFunction, {});

        if (mainRun.note.kind == ScriptNote::Kind::Default) {
            mainRun.note = { ScriptNote::Kind::Silent, traversal.primary.target };
        }
    }
    else {
        advance(1);
    }

    runtime.scriptNote = mainRun.note;
    mainRun            = {};

    const RTNode* nextEntry = context.nodes.find(traversal.primary.target);

    if (runtime.scriptNote.kind != ScriptNote::Kind::Default) {
        nextEntry = context.nodes.find(runtime.scriptNote.nodeId);
    }

    if (traversal.shouldTraverse()) {
        if (nextEntry != nullptr) {
            pushNote(*nextEntry, runId, context, expiryTime);
        }

        return;
    }

    if (traversal.mod.isActive()) {
        hideModulatorHighlight(traversal.mod.walker, runId, traversal.traversal.key.typeId);

        bridge.pushArrowReset(BridgeCommands::modulatorTrail(runId));
    }
}

void TraversalDispatcher::dispatchModulator(const RTNode& node, int runId, const DispatchContext& context,
                                            TraversalLogic& traversalLogic, const RTNode*& modulatorNode,
                                            bool isPrimaryRepeat)
{
    const NodeMap& nodes = context.nodes;

    ModulatorWalk& mod = traversalLogic.mod;

    const int typeId = traversalLogic.traversal.key.typeId;

    const Walker shownWalker = mod.walker;

    const ModulatorWalk::Change change = mod.followMainWalk(nodes, traversalLogic.childSelector, node, isPrimaryRepeat);

    switch (change) {
        case ModulatorWalk::Change::Started:
            if (shownWalker.target != -1) {
                hideModulatorHighlight(shownWalker, runId, typeId);
            }

            bridge.pushArrowReset(BridgeCommands::modulatorTrail(runId));
            break;

        case ModulatorWalk::Change::Stopped:
            hideModulatorHighlight(shownWalker, runId, typeId);

            bridge.pushArrowReset(BridgeCommands::modulatorTrail(runId));
            break;

        case ModulatorWalk::Change::Restarted:
            bridge.pushArrowReset(BridgeCommands::modulatorTrail(runId));
            break;

        case ModulatorWalk::Change::None:
        case ModulatorWalk::Change::Advanced:
            break;
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

    int displayedId     = mod.walker.target;
    int lastDisplayedId = mod.walker.last;

    if (mod.walker.alternativeTarget != -1) {
        displayedId = mod.walker.alternativeTarget;
    }

    if (modulatorNode != nullptr) {
        const RTNode* displayedNode = nodes.find(displayedId);

        if (displayedNode != nullptr) {
            bridge.highlightNode(*displayedNode, BridgeCommands::HighlightKind::Show, runId, typeId);
        }
    }

    if (mod.walker.alternativeLast != -1) {
        lastDisplayedId = mod.walker.alternativeLast;
    }

    if (lastDisplayedId != -1 && lastDisplayedId != displayedId) {
        const RTNode* lastNode = nodes.find(lastDisplayedId);

        if (lastNode != nullptr) {
            bridge.highlightNode(*lastNode, BridgeCommands::HighlightKind::Hide, runId, typeId);
        }
    }
}

void TraversalDispatcher::hideModulatorHighlight(const Walker& walker, int runId, int typeId)
{
    int displayedId = walker.target;

    if (walker.alternativeTarget != -1) {
        displayedId = walker.alternativeTarget;
    }

    if (displayedId != -1) {
        bridge.highlightNode(displayedId, BridgeCommands::HighlightKind::Hide, runId, typeId);
    }
}

void TraversalDispatcher::scaleByModulator(NotePlan& plan, TraversalLogic& traversalLogic, const NodeMap& nodes)
{
    const TraversalKey activeKey = traversalLogic.traversal.key;

    const RTNode* modulatorValueNode = plan.modulatorNode;

    if (plan.alternativeModulatorNode != nullptr) {
        modulatorValueNode = plan.alternativeModulatorNode;
    }

    const int modulatorCount = traversalLogic.childSelector.nodeState.get(NodeStateSlot::ModulatorCount, modulatorValueNode->nodeID) + 1;

    plan.modulatorDanglingIndex = traversalLogic.childSelector.rule->selectDanglingArrow(*modulatorValueNode, modulatorCount, activeKey);
    plan.nextModulatorTarget    = traversalLogic.decideNextModulator(nodes);

    const int modulatorDuration = resolveDuration(*modulatorValueNode, plan.nextModulatorTarget, traversalLogic.mod.walker.last, nodes,
                                                  plan.modulatorDanglingIndex);

    const double modulatorScale = modulatorDuration / ArrowInfo::millisecondsPerSecond;
    const double scaledDuration = plan.duration * modulatorScale;

    plan.duration = static_cast<int>(juce::jlimit(0.0, ArrowInfo::maximumDurationMs, scaledDuration));
    plan.transpose += modulatorValueNode->pitchOffset;
}

void TraversalDispatcher::overrideVoicing(NotePlan& plan, const RTNode& node, const TraversalLogic& traversalLogic, const DispatchContext& context,
                                          const ScriptNote& scriptNote, ScriptNote::Kind scriptKind)
{
    const int activeAlternativeId = traversalLogic.childSelector.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID);

    if (activeAlternativeId != -1 && !node.notes.empty()) {
        const RTNode* altNode = context.nodes.find(activeAlternativeId);

        if (altNode != nullptr) {
            plan.alternativeNode = altNode;

            if (!altNode->notes.empty()) {
                plan.voicing.pitchOverride    = altNode->notes[0].pitch;
                plan.voicing.velocityOverride = altNode->notes[0].velocity;
            }
        }
    }

    if (scriptKind == ScriptNote::Kind::Played) {
        int voicedPitch = RTNote::fallbackValue;

        if (!node.notes.empty()) {
            voicedPitch = node.notes[0].pitch;
        }

        if (plan.voicing.pitchOverride >= 0) {
            voicedPitch = plan.voicing.pitchOverride;
        }

        plan.voicing.pitchOverride    = juce::jlimit(NodeInfo::minimumMidiPitch, NodeInfo::maximumMidiPitch, scriptNote.pitch);
        plan.voicing.velocityOverride = juce::jlimit(NodeInfo::silentMidiVelocity, NodeInfo::maximumMidiVelocity, scriptNote.velocity);
        plan.scriptTranspose          = plan.voicing.pitchOverride - voicedPitch;
    }
}

TraversalDispatcher::NotePlan TraversalDispatcher::planNote(const RTNode& node, int runId, const DispatchContext& context, TraversalLogic& traversalLogic,
                                                            const ScriptNote& scriptNote, bool isPrimaryRepeat)
{
    const NodeMap& nodes = context.nodes;

    const TraversalKey activeKey = traversalLogic.traversal.key;

    ScriptNote::Kind scriptKind = ScriptNote::Kind::Default;

    NotePlan plan;

    auto danglingArrowFor = [&](const RTNode& target) {
        const int count = traversalLogic.childSelector.nodeState.get(NodeStateSlot::Count, target.nodeID) + 1;

        return traversalLogic.childSelector.rule->selectDanglingArrow(target, count, activeKey);
    };

    if (scriptNote.nodeId == node.nodeID) {
        scriptKind = scriptNote.kind;
    }

    if (scriptKind == ScriptNote::Kind::Silent) {
        plan.voicing.sound = NoteScheduler::Sound::Silent;
    }

    dispatchModulator(node, runId, context, traversalLogic, plan.modulatorNode, isPrimaryRepeat);

    if (traversalLogic.mod.walker.alternativeTarget != -1) {
        plan.alternativeModulatorNode = nodes.find(traversalLogic.mod.walker.alternativeTarget);
    }

    plan.nextTarget      = traversalLogic.peekNextTarget(nodes);
    plan.tempoMultiplier = traversalTempoMultiplier(traversalLogic, context);

    jassert(context.sampleRate > 0.0);

    overrideVoicing(plan, node, traversalLogic, context, scriptNote, scriptKind);

    plan.nodeCount     = traversalLogic.childSelector.nodeState.get(NodeStateSlot::Count, node.nodeID) + 1;
    plan.danglingIndex = danglingArrowFor(node);
    plan.transpose     = traversalLogic.traversal.transpose;

    if (plan.alternativeNode != nullptr) {
        plan.alternativeDanglingIndex = danglingArrowFor(*plan.alternativeNode);
        plan.duration = resolveDuration(*plan.alternativeNode, plan.nextTarget, traversalLogic.primary.last, nodes, plan.alternativeDanglingIndex);
    }
    else {
        plan.duration = resolveDuration(node, plan.nextTarget, traversalLogic.primary.last, nodes, plan.danglingIndex);
    }

    if (scriptKind == ScriptNote::Kind::Played) {
        const double scriptDuration = scriptNote.duration;

        plan.duration = static_cast<int>(juce::jlimit(1.0, ArrowInfo::maximumDurationMs, scriptDuration));
    }

    if (plan.modulatorNode != nullptr && traversalLogic.mod.walker.target != -1) {
        scaleByModulator(plan, traversalLogic, nodes);
    }

    applyBaseVoicing(plan.voicing, traversalLogic.traversal, context, plan.transpose);

    return plan;
}

void TraversalDispatcher::pushChordNotes(const NoteDispatch& dispatch, const DispatchContext& context, int parentCount, int transpose)
{
    const NodeMap& nodes = context.nodes;

    const NoteScheduler::NoteTiming chordTiming { .runId = dispatch.runId, .duration = dispatch.duration, .sample = dispatch.sample,
                                                  .tempoMultiplier = dispatch.tempoMultiplier };

    chordVisits.clear();
    chordFrontier.clear();
    chordFrontier.push_back({ dispatch.node.nodeID, parentCount });

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
        && chordNode.nodeType != NodeInfo::Kind::Root)) {
            return;
        }

        if (chordNode.countLimit <= 0 || chainCount % chordNode.countLimit != 0) {
            return;
        }

        NoteScheduler::NoteVoicing chordVoicing;

        applyBaseVoicing(chordVoicing, dispatch.traversal.traversal, context, transpose);

        scheduler.scheduleNote(chordNode, NoteScheduler::NoteRole::ChordVoice, NoteScheduler::NoteTrigger::Step, chordTiming, context, chordVoicing);

        bridge.highlightNode(chordNode, BridgeCommands::HighlightKind::Show, dispatch.runId, dispatch.typeId);

        int chordPlayCount = dispatch.traversal.childSelector.nodeState.increment(NodeStateSlot::Chord, chordNode.nodeID);

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

        if (NodeInfo::kindTraits[static_cast<int>(chainNode.nodeType)].isAlternative && chainNode.alternativeArrowDuration == 0) {
            scheduleChordMember(chainNode.parentId, chainCount);
        }
    }
}

void TraversalDispatcher::dispatchVisualProgress(const NoteDispatch& dispatch, const NotePlan& plan, const NodeMap& nodes)
{
    if (plan.alternativeNode != nullptr) {
        const RTNode* alternativeNodeParent = nodes.find(plan.alternativeNode->parentId);

        if (alternativeNodeParent != nullptr) {
            dispatchPrimaryArrow(dispatch, *plan.alternativeNode, nullptr, alternativeNodeParent, plan.alternativeDanglingIndex);
        }
    }

    dispatchPrimaryArrow(dispatch, dispatch.node, plan.alternativeNode, plan.nextTarget, plan.danglingIndex);

    dispatchModulatorArrow(dispatch, plan.modulatorNode, plan.nextModulatorTarget, plan.modulatorDanglingIndex);

    if (plan.alternativeModulatorNode != nullptr) {
        const RTNode* alternativeModulatorHost = nodes.find(plan.alternativeModulatorNode->parentId);

        if (alternativeModulatorHost != nullptr) {
            dispatchModulatorArrow(dispatch, plan.alternativeModulatorNode, alternativeModulatorHost, plan.modulatorDanglingIndex);
        }
    }
}

void TraversalDispatcher::dispatchPrimaryArrow(const NoteDispatch& dispatch, const RTNode& arrowNode, const RTNode* voicedAlternative,
                                               const RTNode* nextTarget, int danglingIndex)
{
    if (nextTarget == nullptr && danglingIndex < 0) {
        return;
    }

    int sourceId = arrowNode.nodeID;
    int targetId = (-(danglingIndex + 1));

    if (voicedAlternative != nullptr && nextTarget != nullptr) {
        const RTConnection* const alternativeJump = voicedAlternative->findConnection(nextTarget->nodeID);

        if (alternativeJump != nullptr && alternativeJump->isTreeJump) {
            sourceId = voicedAlternative->nodeID;
        }
    }

    if (nextTarget != nullptr) {
        targetId = nextTarget->nodeID;
    }

    bridge.send(BridgeCommands::ArrowCommand { .parentNodeId = sourceId, .childNodeId = targetId, .durationMs = dispatch.wallClockMs,
                                               .trailId = BridgeCommands::primaryTrail(dispatch.runId), .traversalId = dispatch.typeId,
                                               .source = dispatch.source });
}

void TraversalDispatcher::dispatchModulatorArrow(const NoteDispatch& dispatch, const RTNode* modulatorNode, const RTNode* nextModulatorTarget,
                                                 int danglingIndex)
{
    if (modulatorNode == nullptr) {
        return;
    }

    if (nextModulatorTarget == nullptr && danglingIndex < 0) {
        bridge.pushArrowReset(BridgeCommands::modulatorTrail(dispatch.runId));

        return;
    }

    int targetId = (-(danglingIndex + 1));

    if (nextModulatorTarget != nullptr) {
        targetId = nextModulatorTarget->nodeID;
    }

    bridge.send(BridgeCommands::ArrowCommand { .parentNodeId = modulatorNode->nodeID, .childNodeId = targetId, .durationMs = dispatch.wallClockMs,
                                               .trailId = BridgeCommands::modulatorTrail(dispatch.runId), .traversalId = dispatch.typeId,
                                               .source = dispatch.source });
}

void TraversalDispatcher::dispatchCrossTree(const NoteDispatch& dispatch, const DispatchContext& context)
{
    const RTNode&  node  = dispatch.node;
    const NodeMap& nodes = context.nodes;

    TraversalLogic& traversal = dispatch.traversal;

    if (node.nodeType != NodeInfo::Kind::Node && node.nodeType != NodeInfo::Kind::Root) {
        return;
    }

    traversal.advanceCrossTreeCounts(nodes, crossTreeScratch);

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

        bridge.highlightNode(crossTreeRoot, BridgeCommands::HighlightKind::Show, dispatch.runId, dispatch.typeId);

        int connectionDuration = NodeInfo::defaultNoteDurationMs;
        int progressSourceId   = node.nodeID;

        const RTConnection* const crossTreeConnection = node.findConnection(crossTreeRootId);

        if (crossTreeConnection != nullptr && crossTreeConnection->duration >= 0) {
            connectionDuration = crossTreeConnection->duration;
        }
        else if (traversal.childSelector.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID) != -1) {
            const RTNode* altNode = nodes.find(traversal.childSelector.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID));

            if (altNode != nullptr) {
                const RTConnection* const alternativeConnection = altNode->findConnection(crossTreeRootId);

                if (alternativeConnection != nullptr && alternativeConnection->duration >= 0) {
                    connectionDuration = alternativeConnection->duration;
                    progressSourceId   = traversal.childSelector.nodeState.get(NodeStateSlot::ActiveAlternative, node.nodeID);
                }
            }
        }

        const NoteScheduler::NoteTiming connectionTiming { .runId = dispatch.runId, .duration = connectionDuration, .sample = dispatch.sample,
                                                           .tempoMultiplier = dispatch.tempoMultiplier };

        NoteScheduler::NoteVoicing crossTreeVoicing;

        applyBaseVoicing(crossTreeVoicing, traversal.traversal, context, traversal.traversal.transpose);

        scheduler.scheduleNote(crossTreeRoot, NoteScheduler::NoteRole::Stepping, NoteScheduler::NoteTrigger::Connection, connectionTiming, context,
                               crossTreeVoicing);

        const double wallClockDuration = connectionDuration / dispatch.tempoMultiplier;

        const int wallClockMs = static_cast<int>(juce::jlimit(0.0, ArrowInfo::maximumDurationMs, wallClockDuration));

        bridge.send(BridgeCommands::ArrowCommand { .kind = BridgeCommands::ArrowKind::Connection, .parentNodeId = progressSourceId,
                                                   .childNodeId = crossTreeRootId, .durationMs = wallClockMs,
                                                   .trailId = BridgeCommands::primaryTrail(dispatch.runId), .traversalId = dispatch.typeId,
                                                   .source = dispatch.source });
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

    bridge.highlightNode(targetRootNode, BridgeCommands::HighlightKind::Show, runId, traversal.key.typeId);

    pushNote(targetRootNode, runId, context, sample);
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

int TraversalDispatcher::resolveDuration(const RTNode& node, const RTNode* nextTarget,
                                          int lastTargetId, const NodeMap& nodes, int danglingIndex)
{
    int duration = NodeInfo::defaultNoteDurationMs;

    if (!node.notes.empty() && node.notes[0].duration > 0) {
        duration = node.notes[0].duration;
    }

    if (nextTarget != nullptr) {
        int connectionDuration = -1;

        if (NodeInfo::kindTraits[static_cast<int>(node.nodeType)].isAlternative) {
            connectionDuration = node.alternativeArrowDuration;
        }
        else {
            if (nextTarget->nodeType != NodeInfo::Kind::TraversalFlag) {
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
    else if (NodeInfo::kindTraits[static_cast<int>(node.nodeType)].isAlternative) {
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

bool TraversalDispatcher::markChordVisited(int nodeId)
{
    if (chordVisits.contains(nodeId)) {
        return false;
    }

    return chordVisits.claim(nodeId) >= 0;
}

void TraversalDispatcher::applyBaseVoicing(NoteScheduler::NoteVoicing& voicing, const RTtraversal& traversal,
                                          const DispatchContext& context, int transpose)
{
    const int totalTranspose = transpose + context.transpose;

    const double scaledVelocity = traversal.velocityMultiplier * context.velocityScale;

    voicing.channel            = traversal.channel;
    voicing.transpose          = totalTranspose;
    voicing.velocityMultiplier = scaledVelocity;
}

double TraversalDispatcher::traversalTempoMultiplier(const TraversalLogic& traversalLogic, const DispatchContext& context)
{
    const RTNode* const rootNode = context.nodes.find(traversalLogic.rootId);

    double traversalMultiplier = traversalLogic.traversal.tempoMultiplier;

    if (rootNode != nullptr) {
        for (const RTtraversal& rootTraversal : rootNode->traversals) {
            if (rootTraversal.key == traversalLogic.traversal.key) {
                traversalMultiplier = rootTraversal.tempoMultiplier;

                break;
            }
        }
    }

    if (traversalMultiplier <= 0.0) {
        traversalMultiplier = 1.0;
    }

    const double combinedMultiplier = context.tempoMultiplier * traversalMultiplier;

    return juce::jlimit(RTtraversal::minimumTempoMultiplier, RTtraversal::maximumTempoMultiplier, combinedMultiplier);
}
