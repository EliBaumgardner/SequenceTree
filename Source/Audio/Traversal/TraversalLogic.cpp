#include "TraversalLogic.h"

#include <algorithm>
#include <optional>
#include <ranges>

void TraversalLogic::reset(int root, const RTtraversal& newTraversal)
{
    childSelector.nodeState.clear();

    primary                    = {};
    traversal                  = newTraversal;
    childSelector.traversalKey = newTraversal.key;
    mod                        = {};
    loop                       = {};
    rootId                     = root;
    referenceTargetId          = -1;
    pendingJumpTargetId        = -1;
    membersScript              = nullptr;
    state                      = TraversalState::End;

    childSelector.selectionRandom = (static_cast<unsigned int>(root) * seedRootMultiplier
                                     ^ static_cast<unsigned int>(newTraversal.key.typeId) * seedTypeMultiplier
                                     ^ static_cast<unsigned int>(newTraversal.key.instance) * seedInstanceMultiplier
                                     ^ seedOffset) | 1u;
}

void TraversalLogic::begin(const NodeMap& nodes, int startNodeId, int graphLoopLimit)
{
    const int seededCount = previewVisitCount - 1;

    const auto seedableNodes = nodes.sortedById | std::views::take(NodeStateTable::maxNodeIds);

    primary.target = startNodeId;
    state = TraversalState::Active;
    loop.active   = true;
    loop.count    = 0;
    loop.limit    = graphLoopLimit;
    loop.returnId = -1;

    if (mode == TraversalMode::Preview) {
        loop.limit = 1;

        for (const RTNode& node : seedableNodes) {
            childSelector.nodeState.set(NodeStateSlot::ModulatorCount, node.nodeID, seededCount);
            childSelector.nodeState.set(NodeStateSlot::Chord,          node.nodeID, seededCount);

            if (node.nodeType != NodeInfo::Kind::Alternative) {
                childSelector.nodeState.set(NodeStateSlot::Count, node.nodeID, seededCount);
            }

            if (node.countLimit > 0) {
                const int seededCrossTreeCount = seededCount % node.countLimit;

                childSelector.nodeState.set(NodeStateSlot::CrossTree, node.nodeID, seededCrossTreeCount);
            }

            if (node.alternativeRootId != -1 && seededCount > 0 && ChildSelector::isModulatorChild(node.nodeType)) {
                childSelector.nodeState.set(NodeStateSlot::ActiveAlternative, node.nodeID, NodeStateTable::hostPlays);
            }
        }

        for (const RTNode& host : seedableNodes) {
            if (host.alternativeRootId == -1 || !ChildSelector::isAudibleChild(host.nodeType)) {
                continue;
            }

            for (int visit = 1; visit < previewVisitCount; ++visit) {
                const int countedVisits = visit - 1;

                childSelector.nodeState.set(NodeStateSlot::Count, host.nodeID, countedVisits);

                childSelector.advanceAlternative(nodes, primary, Walk::Primary, host.nodeID);
            }

            childSelector.nodeState.set(NodeStateSlot::Count, host.nodeID, seededCount);
        }
    }

    childSelector.advanceAlternative(nodes, primary, Walk::Primary, startNodeId);
}

void TraversalLogic::beginPreview(const NodeMap& nodes, int startNodeId, int alternativeId)
{
    begin(nodes, startNodeId, 1);

    loop.returnId = startNodeId;

    if (alternativeId == -1) {
        return;
    }

    childSelector.nodeState.set(NodeStateSlot::ActiveAlternative, startNodeId, alternativeId);

    primary.alternativeTarget = alternativeId;
}

void TraversalLogic::advance(const NodeMap& nodes, int steps)
{
    const int targetId = primary.target;

    int chosenNodeId = -1;

    TraversalState deadEndState = TraversalState::End;

    referenceTargetId       = primary.last;
    primary.last            = targetId;
    primary.alternativeLast = primary.alternativeTarget;

    if (loop.active) {
        deadEndState = TraversalState::Reset;
    }

    const RTNode* const targetNode = nodes.find(targetId);

    if (targetNode == nullptr) {
        state = deadEndState;

        return;
    }

    if (script != nullptr && script->advanceFunction != -1) {
        advanceByScript(nodes, *targetNode, steps, deadEndState);

        return;
    }

    if (targetNode->connections.empty()) {
        childSelector.nodeState.increment(NodeStateSlot::Count, targetId);

        state = deadEndState;

        return;
    }

    const int jumpTargetId = childSelector.selectTreeJumpChild(nodes, primary, *targetNode);

    if (jumpTargetId != -1) {
        childSelector.nodeState.increment(NodeStateSlot::Count, targetId);
        childSelector.nodeState.set(NodeStateSlot::SwitchCandidate, targetId, -1);

        pendingJumpTargetId = jumpTargetId;
        state               = TraversalState::Jump;

        return;
    }

    chosenNodeId = childSelector.chooseChild(nodes, primary, NodeStateSlot::Count, &ChildSelector::isAdvanceableChild);

    if (chosenNodeId  != -1) {
        const RTNode* const nextTargetNode = nodes.find(chosenNodeId);

        if (nextTargetNode == nullptr) {
            primary.alternativeTarget = -1;
        }
        else {
            primary.target = chosenNodeId;

            childSelector.advanceAlternative(nodes, primary, Walk::Primary, chosenNodeId);

            childSelector.armSubLoop(primary, *nextTargetNode, mode);
        }
    }

    if (primary.target == primary.last) {
        state = deadEndState;

        return;
    }

    skipAhead(nodes, steps);
}

void TraversalLogic::advanceCrossTreeCounts(const NodeMap& nodes, std::vector<int>& traverserIds)
{
    traverserIds.clear();

    auto scanHost = [&](int hostId) {
        const RTNode* const hostNode = nodes.find(hostId);

        if (hostNode == nullptr) {
            return;
        }

        for (const RTConnection& connection : hostNode->connections) {
            if (static_cast<int>(traverserIds.size()) >= maxCrossTreeTargets) {
                return;
            }

            if (!connection.isCrossRoot) {
                continue;
            }

            const std::vector<TraversalKey>& disabled = connection.disabledTraversals;

            if (std::ranges::find(disabled, traversal.key) != disabled.end()) {
                continue;
            }

            const int childId = connection.childId;

            const RTNode* const childEntry = nodes.find(childId);

            if (childEntry == nullptr) {
                continue;
            }

            const RTNode& childNode = *childEntry;

            if (childNode.nodeType != NodeInfo::Kind::Root) {
                continue;
            }

            if (childNode.nodeID == rootId) {
                continue;
            }

            if (childNode.countLimit <= 0) {
                continue;
            }

            int& count       = childSelector.nodeState.ref(NodeStateSlot::CrossTree, childId);
            int& switchCount = childSelector.nodeState.ref(NodeStateSlot::CrossTreeSwitch, childId);

            if (switchCount > 0) {
                traverserIds.push_back(childId);

                switchCount++;

                if (switchCount >= childNode.switchCountLimit) {
                    switchCount = 0;
                    count       = 0;
                }
            }
            else {
                count++;

                if (count >= childNode.countLimit) {
                    traverserIds.push_back(childId);

                    if (childNode.switchCountLimit > 1) {
                        switchCount = 1;
                    }
                    else {
                        count = 0;
                    }
                }
            }
        }
    };

    scanHost(primary.target);

    if (primary.alternativeTarget != -1) {
        scanHost(primary.alternativeTarget);
    }
}

TraversalLogic::StepResult TraversalLogic::handleNodeEvent(const NodeMap& nodes, int steps) {
    switch (state) {
        case TraversalState::Active:
            return stepActive(nodes, steps);

        case TraversalState::End: {
            StepResult result;
            fillEndedResult(result);
            return result;
        }

        default:
            return {};
    }
}

ScriptRunContext TraversalLogic::makeScriptContext(const NodeMap& nodes, Walk walk, ScriptWrites writes,
                                                   ScriptHost* host)
{
    ScriptRunContext context { *script, nodes, childSelector.nodeState, scriptMembers, traversal.key,
                               static_cast<int>(childSelector.selectionRandom >> 1), primary.target, NodeStateSlot::Count,
                               &ChildSelector::isAdvanceableChild, writes, host, sharedGlobals };

    const std::size_t memberCount = std::min(script->memberDefaults.size(), scriptMembers.size());
    const std::size_t globalCount = std::min(script->globalDefaults.size(), scriptMembers.size());

    if (walk == Walk::Modulator) {
        context.currentNodeId = mod.walker.target;
        context.countSlot     = NodeStateSlot::ModulatorCount;
        context.isEligible    = &ChildSelector::isModulatorChild;
    }

    ScriptRunContext initialContext = context;

    initialContext.writes = ScriptWrites::Commit;
    initialContext.host   = nullptr;

    if (sharedGlobals != nullptr && sharedGlobals->script != script) {
        sharedGlobals->values.fill(0);

        std::copy_n(script->globalDefaults.begin(), globalCount, sharedGlobals->values.begin());

        sharedGlobals->script = script;

        ScriptRun(initialContext).call(script->globalInitFunction, {});
    }

    if (membersScript != script) {
        scriptMembers.fill(0);

        std::copy_n(script->memberDefaults.begin(), memberCount, scriptMembers.begin());

        membersScript = script;

        ScriptRun(initialContext).call(script->memberInitFunction, {});
    }

    return context;
}

const RTNode* TraversalLogic::peekNextTarget(const NodeMap& nodes)
{
    const int count = childSelector.nodeState.get(NodeStateSlot::Count, primary.target) + 1;

    const RTNode* const targetNode = nodes.find(primary.target);

    if (script != nullptr && script->advanceFunction != -1) {
        ScriptRun run(makeScriptContext(nodes, Walk::Primary, ScriptWrites::Trial, nullptr));

        const int scriptArguments[] = { 1 };

        const RTNode* const scriptTarget = nodes.find(run.call(script->advanceFunction, scriptArguments));

        if (scriptTarget == nullptr || scriptTarget->nodeID == primary.target) {
            return nullptr;
        }

        return scriptTarget;
    }

    if (targetNode != nullptr) {
        const int jumpTargetId = childSelector.selectTreeJumpChild(nodes, primary, *targetNode);

        if (jumpTargetId != -1) {
            const RTNode* const jumpTargetNode = nodes.find(jumpTargetId);

            if (jumpTargetNode != nullptr) {
                return jumpTargetNode;
            }
        }
    }

    const RTNode* const switchCandidate = nodes.find(childSelector.nodeState.get(NodeStateSlot::SwitchCandidate, primary.target));

    if (switchCandidate != nullptr && switchCandidate->nodeID != primary.target) {
        const int switchCountLimit = switchCandidate->switchCountLimit;
        const int switchCount      = childSelector.nodeState.get(NodeStateSlot::SwitchCount, primary.target) + 1;

        if (switchCount < switchCountLimit && switchCountLimit > 1) {
            return switchCandidate;
        }
    }

    const int peekTargetId = childSelector.selectNextChild(nodes, primary.target, count, &ChildSelector::isAudibleChild);

    if (peekTargetId == -1 || peekTargetId == primary.target) {
        return nullptr;
    }

    const RTNode* const peekNode = nodes.find(peekTargetId);

    if (peekNode == nullptr) {
        return nullptr;
    }

    if (targetNode == nullptr || targetNode->encapsulationEntryId == -1) {
        return peekNode;
    }

    const int entryId      = targetNode->encapsulationEntryId;
    const int subLoopCount = childSelector.nodeState.get(NodeStateSlot::SubRootCount, entryId) + 1;

    const RTNode* const entryNode = nodes.find(entryId);

    if (entryId != primary.subRootNode || peekNode->encapsulationEntryId == entryId || entryNode == nullptr) {
        return peekNode;
    }

    if (entryNode->subLoopCountLimit > 0 && subLoopCount >= entryNode->subLoopCountLimit) {
        return peekNode;
    }

    return entryNode;
}

const RTNode* TraversalLogic::decideNextModulator(const NodeMap& nodes)
{
    const bool scriptAdvances = script != nullptr && script->advanceFunction != -1;

    const bool needsDecision = mod.decidedTarget == -1 && mod.isActive() && mod.walker.target != -1
                            && nodes.find(mod.walker.target) != nullptr;

    std::optional<int> scriptChoice;

    if (needsDecision && scriptAdvances) {
        ScriptRun run(makeScriptContext(nodes, Walk::Modulator, ScriptWrites::Commit, nullptr));

        const int scriptArguments[] = { 1 };

        scriptChoice = run.call(script->advanceFunction, scriptArguments);
    }

    if (needsDecision) {
        mod.decide(nodes, childSelector, mode, scriptChoice);
    }

    if (mod.decidedRestart) {
        return nullptr;
    }

    return nodes.find(mod.decidedTarget);
}

bool TraversalLogic::shouldTraverse() const
{
    return state != TraversalState::End;
}

TraversalLogic::StepResult TraversalLogic::stepActive(const NodeMap& nodes, int steps)
{
    StepResult result;

    advance(nodes, steps);

    childSelector.selectionRandom ^= childSelector.selectionRandom << xorshiftFirstShift;
    childSelector.selectionRandom ^= childSelector.selectionRandom >> xorshiftSecondShift;
    childSelector.selectionRandom ^= childSelector.selectionRandom << xorshiftThirdShift;

    const int leftId            = primary.last;
    const int leftAlternativeId = primary.alternativeLast;

    result.pushCounts        = mode == TraversalMode::Live;
    result.countSourceNodeId = leftId;
    result.countSourceCount  = childSelector.nodeState.get(NodeStateSlot::Count, leftId);

    switch (state) {
        case TraversalState::Active:
            result.kind                 = StepResult::Kind::Advanced;
            result.leftId               = leftId;
            result.leftAlternativeId    = leftAlternativeId;
            result.enteredId            = primary.target;
            result.enteredAlternativeId = primary.alternativeTarget;
            break;

        case TraversalState::Reset:
            handleLoopReset(nodes, result);
            break;

        case TraversalState::Jump:
            handleTreeJump(nodes, result);
            break;

        case TraversalState::End:
            fillEndedResult(result);
            break;

        default:
            break;
    }

    return result;
}

void TraversalLogic::handleLoopReset(const NodeMap& nodes, StepResult& result)
{
    const RTNode* const returnNode = nodes.find(loop.returnId);

    loop.count++;

    if (loop.limit > 0 && loop.count >= loop.limit) {
        state                    = TraversalState::End;
        result.kind              = StepResult::Kind::Ended;
        result.leftId            = primary.target;
        result.leftAlternativeId = primary.alternativeTarget;
        result.clearTrail        = true;

        return;
    }

    result.kind              = StepResult::Kind::LoopedToRoot;
    result.leftId            = primary.target;
    result.leftAlternativeId = primary.alternativeTarget;
    primary.target = rootId;

    if (returnNode != nullptr) {
        rootId         = returnNode->graphID;
        primary.target = returnNode->nodeID;
    }

    if (primary.subRootNode != -1) {
        const int subRootTarget = childSelector.advanceSubRoot(nodes, primary);

        if (subRootTarget != -1) {
            primary.target = subRootTarget;
        }
    }

    childSelector.advanceAlternative(nodes, primary, Walk::Primary, primary.target);

    result.enteredId            = primary.target;
    result.enteredAlternativeId = primary.alternativeTarget;
    result.clearTrail = true;

    const RTNode* const enteredNode = nodes.find(primary.target);

    if (enteredNode != nullptr && enteredNode->encapsulationEntryId == primary.target) {
        childSelector.armSubLoop(primary, *enteredNode, mode);
    }

    state = TraversalState::Active;
}

void TraversalLogic::handleTreeJump(const NodeMap& nodes, StepResult& result)
{
    const int jumpTargetId = pendingJumpTargetId;

    pendingJumpTargetId    = -1;
    state = TraversalState::Active;

    const RTNode* const jumpTargetNode = nodes.find(jumpTargetId);

    if (jumpTargetNode == nullptr) {
        return;
    }

    result.kind              = StepResult::Kind::JumpedToTree;
    result.leftId            = primary.target;
    result.leftAlternativeId = primary.alternativeLast;
    result.jumpedFromRootId  = rootId;

    rootId = jumpTargetNode->nodeID;

    primary.target            = rootId;
    primary.subRootNode       = -1;
    primary.alternativeTarget = -1;
    primary.alternativeLast   = -1;

    childSelector.advanceAlternative(nodes, primary, Walk::Primary, rootId);

    result.enteredId            = rootId;
    result.enteredAlternativeId = primary.alternativeTarget;
    result.clearTrail           = true;

    loop.active = true;
    loop.count  = 0;

    if (mode == TraversalMode::Live) {
        loop.limit = 0;
    }
}

void TraversalLogic::fillEndedResult(StepResult& result) const
{
    result.kind              = StepResult::Kind::Ended;
    result.leftId            = primary.target;
    result.leftAlternativeId = primary.alternativeTarget;
    result.referenceOffId    = referenceTargetId;
    result.clearTrail        = true;
}

void TraversalLogic::advanceByScript(const NodeMap& nodes, const RTNode& leavingNode, int steps,
                                     TraversalState deadEndState)
{
    const int scriptArguments[] = { std::min(steps, maxAdvanceSteps) };

    ScriptRun run(makeScriptContext(nodes, Walk::Primary, ScriptWrites::Commit, nullptr));

    const int chosenId = run.call(script->advanceFunction, scriptArguments);

    const RTNode*       const chosenNode = nodes.find(chosenId);
    const RTConnection* const connection = leavingNode.findConnection(chosenId);

    if (chosenNode == nullptr || chosenId == leavingNode.nodeID) {
        state = deadEndState;

        return;
    }

    if (connection != nullptr && connection->isTreeJump) {
        pendingJumpTargetId = chosenId;
        state               = TraversalState::Jump;

        return;
    }

    primary.target = chosenId;

    childSelector.advanceAlternative(nodes, primary, Walk::Primary, chosenId);
}

void TraversalLogic::skipAhead(const NodeMap& nodes, int steps)
{
    const int stepCount = std::min(steps, maxAdvanceSteps);

    for (int skipped = 1; skipped < stepCount; ++skipped) {
        const RTNode* const skipTarget = peekNextTarget(nodes);

        if (skipTarget == nullptr || skipTarget->nodeType == NodeInfo::Kind::Root) {
            return;
        }

        primary.target = skipTarget->nodeID;

        childSelector.advanceAlternative(nodes, primary, Walk::Primary, primary.target);
    }
}
