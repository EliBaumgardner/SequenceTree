#include "TraversalLogic.h"

#include <algorithm>
#include <ranges>

namespace {

bool isModulatorChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Modulator || t == RTNode::NodeType::ModulatorRoot;
}

bool isAlternativeChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Alternative;
}

bool isAlternativeModulatorChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::AlternativeModulator;
}

bool isAdvanceableChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Node || t == RTNode::NodeType::Modulator
        || t == RTNode::NodeType::RootNode;
}

bool isAudibleChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::Node || t == RTNode::NodeType::RootNode;
}

bool isTreeJumpChild(RTNode::NodeType t) {
    return t == RTNode::NodeType::RootNode;
}

void resetWalker(TraversalLogic::Walker& walker)
{
    walker.target = 0;
    walker.last   = 0;

    walker.subRootNode = -1;

    walker.alternativeTarget = -1;
    walker.alternativeLast   = -1;
}

}

void TraversalLogic::reset(int root, const RTtraversal& newTraversal)
{
    resetWalker(primary);

    nodeState.clear();

    traversal = newTraversal;

    mod  = {};
    loop = {};

    selectionRandom = (static_cast<unsigned int>(root) * 2654435761u
                       ^ static_cast<unsigned int>(newTraversal.key.typeId) * 40503u
                       ^ static_cast<unsigned int>(newTraversal.key.instance) * 2246822519u
                       ^ 2463534242u) | 1u;

    rootId              = root;
    referenceTargetId   = 0;
    pendingJumpTargetId = -1;
    membersScript       = nullptr;

    state = TraversalState::End;
}

void TraversalLogic::begin(const NodeMap& nodes, int startNodeId, int graphLoopLimit)
{
    const int  seededCount   = previewVisitCount - 1;
    const auto seedableNodes = nodes.sortedById | std::views::take(NodeStateTable::maxNodeIds);

    primary.target = startNodeId;

    state = TraversalState::Active;

    loop.active   = true;
    loop.count    = 0;
    loop.limit    = graphLoopLimit;
    loop.returnId = -1;

    if (mode == Mode::Preview) {
        loop.limit = 1;

        for (const RTNode& node : seedableNodes) {
            nodeState.set(NodeStateSlot::ModulatorCount, node.nodeID, seededCount);
            nodeState.set(NodeStateSlot::Chord,          node.nodeID, seededCount);

            if (node.nodeType != RTNode::NodeType::Alternative) {
                nodeState.set(NodeStateSlot::Count, node.nodeID, seededCount);
            }

            if (node.countLimit > 0) {
                nodeState.set(NodeStateSlot::CrossTree, node.nodeID, seededCount % node.countLimit);
            }

            if (node.alternativeRootId != -1 && seededCount > 0 && isModulatorChild(node.nodeType)) {
                nodeState.set(NodeStateSlot::ActiveAlternative, node.nodeID, node.nodeID);
            }
        }

        for (const RTNode& host : seedableNodes) {
            if (host.alternativeRootId == -1 || !isAudibleChild(host.nodeType)) {
                continue;
            }

            for (int visit = 1; visit < previewVisitCount; ++visit) {
                nodeState.set(NodeStateSlot::Count, host.nodeID, visit - 1);
                advanceAlternative(nodes, host.nodeID);
            }

            nodeState.set(NodeStateSlot::Count, host.nodeID, seededCount);
        }
    }

    advanceAlternative(nodes, startNodeId);
}

void TraversalLogic::advanceAlternative(const NodeMap& nodes,int parentId) {
    const RTNode* const parentNode = nodes.find(parentId);
    if (parentNode == nullptr) {
        primary.alternativeTarget = -1;
        return;
    }

    const RTNode& parent = *parentNode;

    const bool modulatorWalkerEntered = mod.isActive() && parentId == mod.walker.target
                                     && (parent.nodeType == RTNode::NodeType::Modulator
                                      || parent.nodeType == RTNode::NodeType::ModulatorRoot);

    Walker*        walker        = &primary;
    ChildPredicate isAlternative = &isAlternativeChild;
    NodeStateSlot  countSlot     = NodeStateSlot::Count;

    if (modulatorWalkerEntered) {
        walker        = &mod.walker;
        isAlternative = &isAlternativeModulatorChild;
        countSlot     = NodeStateSlot::ModulatorCount;
    }

    if (parent.alternativeRootId == -1) {
        nodeState.set(NodeStateSlot::ActiveAlternative, parentId, -1);
        walker->alternativeTarget  = -1;
        return;
    }

    const int currentAltId = nodeState.get(NodeStateSlot::ActiveAlternative, parentId);

    if (currentAltId == -1) {
        nodeState.set(NodeStateSlot::ActiveAlternative, parentId, parentId);
        walker->alternativeTarget  = -1;
        return;
    }

    if (currentAltId != parentId) {
        const RTNode* const currentAltNode = nodes.find(currentAltId);

        if (currentAltNode != nullptr) {
            const int switchCountLimit = currentAltNode->switchCountLimit;
            const int switchCount      = nodeState.increment(NodeStateSlot::SwitchCount, currentAltId);

            if (switchCount < switchCountLimit && switchCountLimit > 1) {
                walker->alternativeTarget = currentAltId;
                return;
            }
            else {
                nodeState.set(NodeStateSlot::SwitchCount, currentAltId, 0);
            }
        }
    }

    int count;
    if (currentAltId == parentId) {
        count = nodeState.get(countSlot, parentId);
    } else {
        count = nodeState.increment(countSlot, currentAltId);
    }

    const int chosen = selectNextChild(nodes,currentAltId, count, isAlternative);

    if (currentAltId != parentId) {
        nodeState.set(NodeStateSlot::LastNode, currentAltId, chosen);
    }

    if (chosen == -1) {
        nodeState.set(NodeStateSlot::ActiveAlternative, parentId, parentId);
        walker->alternativeTarget  = -1;
    } else {
        nodeState.set(NodeStateSlot::ActiveAlternative, parentId, chosen);
        walker->alternativeTarget  = chosen;
    }
}

int TraversalLogic::selectNextChild(const NodeMap& nodes, int parentId, int parentCount,
                                    ChildPredicate isEligible) const
{
    const RTNode* const parentNode = nodes.find(parentId);
    if (parentNode == nullptr) {
        return -1;
    }

    const RuleContext context{nodes, *parentNode, parentCount,
                              traversal.key, isEligible, nodeState,
                              static_cast<int>(selectionRandom >> 1)};

    return rule->selectChild(context);
}

void TraversalLogic::beginPreview(const NodeMap& nodes, int startNodeId, int alternativeId)
{
    begin(nodes, startNodeId, 1);

    loop.returnId = startNodeId;

    if (alternativeId == -1) {
        return;
    }

    nodeState.set(NodeStateSlot::ActiveAlternative, startNodeId, alternativeId);
    primary.alternativeTarget = alternativeId;
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

TraversalLogic::StepResult TraversalLogic::stepActive(const NodeMap& nodes, int steps)
{
    advance(nodes, steps);

    selectionRandom ^= selectionRandom << 13;
    selectionRandom ^= selectionRandom >> 17;
    selectionRandom ^= selectionRandom << 5;

    StepResult result;

    const int leftId            = primary.last;
    const int leftAlternativeId = primary.alternativeLast;

    result.pushCounts        = mode == Mode::Live;
    result.countSourceNodeId = leftId;
    result.countSourceCount  = nodeState.get(NodeStateSlot::Count, leftId);

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

void TraversalLogic::advance(const NodeMap& nodes, int steps)
{
    const int targetId     = primary.target;
    int       chosenNodeId = -1;

    referenceTargetId       = primary.last;
    primary.last            = targetId;
    primary.alternativeLast = primary.alternativeTarget;

    TraversalState deadEndState = TraversalState::End;

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
        nodeState.increment(NodeStateSlot::Count, targetId);
        state = deadEndState;
        return;
    }

    const int jumpTargetId = selectTreeJumpChild(nodes, *targetNode);

    if (jumpTargetId != -1) {
        nodeState.increment(NodeStateSlot::Count, targetId);
        nodeState.set(NodeStateSlot::SwitchCandidate, targetId, -1);

        pendingJumpTargetId = jumpTargetId;
        state               = TraversalState::Jump;
        return;
    }

    chosenNodeId = chooseChild(nodes, primary, NodeStateSlot::Count, &isAdvanceableChild);

    if (chosenNodeId  != -1) {
        const RTNode* const nextTargetNode = nodes.find(chosenNodeId);

        if (nextTargetNode == nullptr) {
            primary.alternativeTarget = -1;
        }
        else {
            primary.target = chosenNodeId;
            advanceAlternative(nodes,chosenNodeId);

            armSubLoop(primary, *nextTargetNode);
        }
    }

    if (primary.target == primary.last) {
        state = deadEndState;
        return;
    }

    skipAhead(nodes, steps);
}

void TraversalLogic::advanceByScript(const NodeMap& nodes, const RTNode& leavingNode, int steps,
                                     TraversalState deadEndState)
{
    const int scriptArguments[] = { std::min(steps, maxAdvanceSteps) };

    ScriptRun run(makeScriptContext(nodes, Walk::Primary, ScriptWrites::Commit, nullptr));

    const int                 chosenId   = run.call(script->advanceFunction, scriptArguments);
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
    advanceAlternative(nodes, chosenId);
}

ScriptRunContext TraversalLogic::makeScriptContext(const NodeMap& nodes, Walk walk, ScriptWrites writes,
                                                   ScriptHost* host)
{
    ScriptRunContext context { *script, nodes, nodeState, scriptMembers, traversal.key,
                               static_cast<int>(selectionRandom >> 1), primary.target, NodeStateSlot::Count,
                               &isAdvanceableChild, writes, host };

    const std::size_t memberCount = std::min(script->memberDefaults.size(), scriptMembers.size());

    if (membersScript != script) {
        scriptMembers.fill(0);
        std::copy_n(script->memberDefaults.begin(), memberCount, scriptMembers.begin());

        membersScript = script;
    }

    if (walk == Walk::Modulator) {
        context.currentNodeId = mod.walker.target;
        context.countSlot     = NodeStateSlot::ModulatorCount;
        context.isEligible    = &isModulatorChild;
    }

    return context;
}

int TraversalLogic::selectTreeJumpChild(const NodeMap& nodes, const RTNode& host) const
{
    const RTNode* parent = &host;

    const RTNode* const voicedAlternative = nodes.find(primary.alternativeTarget);

    if (voicedAlternative != nullptr) {
        parent = voicedAlternative;
    }

    const int parentCount = nodeState.get(NodeStateSlot::Count, parent->nodeID) + 1;

    const RuleContext context { nodes, *parent, parentCount,
                                traversal.key, &isTreeJumpChild, nodeState,
                                static_cast<int>(selectionRandom >> 1), true };

    int chosen   = -1;
    int maxLimit = 0;

    for (const RTConnection& connection : parent->connections) {
        if (!connection.isTreeJump) {
            continue;
        }

        const int childId = connection.childId;

        const RTNode* child = context.eligibleChild(childId);

        if (child == nullptr) {
            continue;
        }

        if (parentCount % child->countLimit == 0 && child->countLimit > maxLimit) {
            chosen   = childId;
            maxLimit = child->countLimit;
        }
    }

    return chosen;
}

int TraversalLogic::chooseChild(const NodeMap& nodes, Walker& walker, NodeStateSlot countSlot,
                                ChildPredicate isEligible)
{
    const int targetId     = walker.target;
    int       chosenNodeId = -1;

    selectSwitchNode(nodes, targetId, chosenNodeId);

    if (chosenNodeId != -1) {
        return chosenNodeId;
    }

    const int count = nodeState.increment(countSlot, targetId);

    chosenNodeId = selectNextChild(nodes, targetId, count, isEligible);

    nodeState.set(NodeStateSlot::SwitchCandidate, targetId, chosenNodeId);
    nodeState.set(NodeStateSlot::LastNode, targetId, chosenNodeId);

    if (chosenNodeId == -1) {
        return -1;
    }

    const int encapsulationEntryId = encapsulationLoopTarget(nodes, walker, targetId, chosenNodeId);

    if (encapsulationEntryId != -1) {
        chosenNodeId = encapsulationEntryId;
    }

    registerTrigger(nodes, chosenNodeId);

    return chosenNodeId;
}

void TraversalLogic::selectSwitchNode(const NodeMap& nodes,int targetId, int& chosenNodeId) {
    if (nodeState.get(NodeStateSlot::SwitchCandidate, targetId) != -1) {

        const int switchCount = nodeState.increment(NodeStateSlot::SwitchCount, targetId);

        const RTNode* const switchEntry = nodes.find(nodeState.get(NodeStateSlot::SwitchCandidate, targetId));

        if (switchEntry != nullptr) {
            const RTNode& switchNode       = *switchEntry;
            const int     switchCountLimit = switchNode.switchCountLimit;

            if (switchCount < switchCountLimit && switchCountLimit > 1) {
                chosenNodeId = switchNode.nodeID;
            }
            else {
                nodeState.set(NodeStateSlot::SwitchCount, targetId, 0);
            }
        }
    }
}

int TraversalLogic::encapsulationLoopTarget(const NodeMap& nodes, Walker& walker, int leavingNodeId, int chosenNodeId)
{
    const RTNode* const leavingNode = nodes.find(leavingNodeId);

    if (leavingNode == nullptr) {
        return -1;
    }

    const int entryId = leavingNode->encapsulationEntryId;

    if (entryId == -1 || walker.subRootNode != entryId) {
        return -1;
    }

    const RTNode* const chosenNode = nodes.find(chosenNodeId);

    if (chosenNode != nullptr && chosenNode->encapsulationEntryId == entryId) {
        return -1;
    }

    const RTNode* const entryNode = nodes.find(entryId);

    if (entryNode == nullptr) {
        return -1;
    }

    const int subLoopLimit = entryNode->subLoopCountLimit;
    const int subLoopCount = nodeState.increment(NodeStateSlot::SubRootCount, entryId);

    if (subLoopLimit > 0 && subLoopCount >= subLoopLimit) {
        nodeState.set(NodeStateSlot::SubRootCount, entryId, 0);
        walker.subRootNode = -1;
        return -1;
    }

    return entryId;
}

void TraversalLogic::registerTrigger(const NodeMap& nodes, int nodeId)
{
    const RTNode* const node = nodes.find(nodeId);

    if (node == nullptr) {
        return;
    }

    if (node->triggerLimit <= 0) {
        return;
    }

    nodeState.increment(NodeStateSlot::Trigger, nodeId);
}

void TraversalLogic::armSubLoop(Walker& walker, const RTNode& enteredNode)
{
    if (walker.subRootNode != -1) {
        return;
    }

    const bool nodeSubLoops = enteredNode.subLoopCountLimit != 1 && mode == Mode::Live;

    if (! nodeSubLoops) {
        return;
    }

    walker.subRootNode = enteredNode.nodeID;
    nodeState.set(NodeStateSlot::SubRootCount, walker.subRootNode, 0);
}

void TraversalLogic::skipAhead(const NodeMap& nodes, int steps)
{
    const int stepCount = std::min(steps, maxAdvanceSteps);

    for (int skipped = 1; skipped < stepCount; ++skipped) {
        const RTNode* const skipTarget = peekNextTarget(nodes);

        if (skipTarget == nullptr || skipTarget->nodeType == RTNode::NodeType::RootNode) {
            return;
        }

        primary.target = skipTarget->nodeID;
        advanceAlternative(nodes, primary.target);
    }
}

const RTNode* TraversalLogic::peekNextTarget(const NodeMap& nodes)
{
    const int count = nodeState.get(NodeStateSlot::Count, primary.target) + 1;

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
        const int jumpTargetId = selectTreeJumpChild(nodes, *targetNode);

        if (jumpTargetId != -1) {
            const RTNode* const jumpTargetNode = nodes.find(jumpTargetId);

            if (jumpTargetNode != nullptr) {
                return jumpTargetNode;
            }
        }
    }

    const RTNode* const switchCandidate = nodes.find(nodeState.get(NodeStateSlot::SwitchCandidate, primary.target));

    if (switchCandidate != nullptr && switchCandidate->nodeID != primary.target) {
        const int switchCountLimit = switchCandidate->switchCountLimit;
        const int switchCount      = nodeState.get(NodeStateSlot::SwitchCount, primary.target) + 1;

        if (switchCount < switchCountLimit && switchCountLimit > 1) {
            return switchCandidate;
        }
    }

    const int peekTargetId = selectNextChild(nodes,primary.target, count, &isAudibleChild);

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

    const int           entryId      = targetNode->encapsulationEntryId;
    const RTNode* const entryNode    = nodes.find(entryId);
    const int           subLoopCount = nodeState.get(NodeStateSlot::SubRootCount, entryId) + 1;

    if (entryId != primary.subRootNode || peekNode->encapsulationEntryId == entryId || entryNode == nullptr) {
        return peekNode;
    }

    if (entryNode->subLoopCountLimit > 0 && subLoopCount >= entryNode->subLoopCountLimit) {
        return peekNode;
    }

    return entryNode;
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
        const int subRootTarget = advanceSubRoot(nodes, primary);

        if (subRootTarget != -1) {
            primary.target = subRootTarget;
        }
    }

    advanceAlternative(nodes, primary.target);

    result.enteredId            = primary.target;
    result.enteredAlternativeId = primary.alternativeTarget;

    result.clearTrail = true;

    const RTNode* const enteredNode = nodes.find(primary.target);

    if (enteredNode != nullptr && enteredNode->encapsulationEntryId == primary.target) {
        armSubLoop(primary, *enteredNode);
    }

    state = TraversalState::Active;
}

int TraversalLogic::advanceSubRoot(const NodeMap& nodes, Walker& walker)
{
    const RTNode* const subRoot = nodes.find(walker.subRootNode);

    int subRootLimit = 0;

    if (subRoot != nullptr) {
        subRootLimit = subRoot->subLoopCountLimit;
    }

    const int  subRootCount        = nodeState.increment(NodeStateSlot::SubRootCount, walker.subRootNode);
    const bool subRootLoopsForever = (subRootLimit == 0);

    if (!subRootLoopsForever && subRootCount >= subRootLimit) {
        nodeState.set(NodeStateSlot::SubRootCount, walker.subRootNode, 0);
        walker.subRootNode = -1;
        return -1;
    }

    return walker.subRootNode;
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

    advanceAlternative(nodes, rootId);

    result.enteredId            = rootId;
    result.enteredAlternativeId = primary.alternativeTarget;
    result.clearTrail           = true;

    loop.active = true;
    loop.count  = 0;

    if (mode == Mode::Live) {
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

void TraversalLogic::peekCrossTreeNode(const NodeMap& nodes, std::vector<int>& traverserIds)
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

            if (childNode.nodeType != RTNode::NodeType::RootNode) {
                continue;
            }
            if (childNode.nodeID == rootId) {
                continue;
            }
            if (childNode.countLimit <= 0) {
                continue;
            }

            int& count       = nodeState.ref(NodeStateSlot::CrossTree, childId);
            int& switchCount = nodeState.ref(NodeStateSlot::CrossTreeSwitch, childId);

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

const RTNode* TraversalLogic::decideNextModulator(const NodeMap& nodes)
{
    mod.decide(nodes, *this);

    if (mod.decidedRestart) {
        return nullptr;
    }

    const RTNode* const decidedNode = nodes.find(mod.decidedTarget);

    if (decidedNode == nullptr) {
        return nullptr;
    }

    return decidedNode;
}

int TraversalLogic::findActiveModulatorRoot(const NodeMap& nodes, int regularNodeId) const
{
    const RTNode* const hostNode = nodes.find(regularNodeId);

    if (hostNode == nullptr) {
        return -1;
    }

    const RTNode& host = *hostNode;

    const int hostCount = nodeState.get(NodeStateSlot::Count, regularNodeId) + 1;

    int maxLimit    = 0;
    int totalWeight = 0;

    for (const RTConnection& connection : host.connections) {
        const RTNode* const modRoot = eligibleModulatorRoot(nodes, connection, hostCount);

        if (modRoot == nullptr) {
            continue;
        }

        if (modRoot->countLimit > maxLimit) {
            maxLimit    = modRoot->countLimit;
            totalWeight = modRoot->probability;
        }
        else if (modRoot->countLimit == maxLimit) {
            totalWeight += modRoot->probability;
        }
    }

    if (totalWeight <= 0) {
        return -1;
    }

    int selectionSpan = totalWeight;

    if (selectionSpan < RTNode::probabilityScale) {
        selectionSpan = RTNode::probabilityScale;
    }

    const int pick = static_cast<int>(selectionRandom >> 1) % selectionSpan;

    int runningWeight = 0;

    const RTNode* chosenRoot = nullptr;

    for (const RTConnection& connection : host.connections) {
        const RTNode* const modRoot = eligibleModulatorRoot(nodes, connection, hostCount);

        if (modRoot == nullptr || modRoot->countLimit != maxLimit) {
            continue;
        }

        runningWeight += modRoot->probability;

        if (pick < runningWeight) {
            chosenRoot = modRoot;
            break;
        }
    }

    if (chosenRoot == nullptr) {
        return -1;
    }

    const RTConnection* const modRootConnection = host.findConnection(chosenRoot->nodeID);

    const bool alreadyWalking = mod.gate.activeRootId == chosenRoot->nodeID
                             && mod.gate.hostId == regularNodeId;

    if (modRootConnection != nullptr && ! modRootConnection->isSynced && alreadyWalking) {
        return -1;
    }

    return chosenRoot->nodeID;
}

const RTNode* TraversalLogic::eligibleModulatorRoot(const NodeMap& nodes, const RTConnection& connection,
                                                   int hostCount) const
{
    const RTNode* const childNode = nodes.find(connection.childId);

    if (childNode == nullptr) {
        return nullptr;
    }

    if (childNode->nodeType != RTNode::NodeType::ModulatorRoot) {
        return nullptr;
    }

    const std::vector<TraversalKey>& disabled = connection.disabledTraversals;

    if (std::ranges::find(disabled, traversal.key) != disabled.end()) {
        return nullptr;
    }

    if (childNode->countLimit <= 0) {
        return nullptr;
    }

    if (hostCount % childNode->countLimit != 0) {
        return nullptr;
    }

    return childNode;
}

bool TraversalLogic::isDescendantOf(const NodeMap& nodes, int nodeId, int ancestorId)
{
    if (ancestorId == -1 || nodeId == ancestorId) {
        return false;
    }

    int current = nodeId;
    int guard   = 0;

    while (current != 0 && guard++ < 10000) {
        const RTNode* const currentNode = nodes.find(current);
        if (currentNode == nullptr) {
            return false;
        }

        const int parent = currentNode->parentId;
        if (parent == ancestorId) {
            return true;
        }

        current = parent;
    }

    return false;
}

bool TraversalLogic::shouldTraverse() const
{
    return state != TraversalState::End;
}

void TraversalLogic::ModulatorWalk::decide(const NodeMap& nodes, TraversalLogic& owner)
{
    const bool scriptAdvances = owner.script != nullptr && owner.script->advanceFunction != -1;

    if (decidedTarget != -1) {
        return;
    }

    decidedRestart = false;

    if (gate.activeRootId == -1 || walker.target == -1) {
        return;
    }

    if (nodes.find(walker.target) == nullptr) {
        return;
    }

    int chosen = -1;

    if (scriptAdvances) {
        ScriptRun run(owner.makeScriptContext(nodes, Walk::Modulator, ScriptWrites::Commit, nullptr));

        const int scriptArguments[] = { 1 };

        chosen = run.call(owner.script->advanceFunction, scriptArguments);

        if (nodes.find(chosen) == nullptr) {
            chosen = -1;
        }
    }
    else {
        chosen = owner.chooseChild(nodes, walker, NodeStateSlot::ModulatorCount, &isModulatorChild);
    }

    if (chosen == -1) {
        decidedRestart = true;
        decidedTarget  = gate.activeRootId;

        if (walker.subRootNode != -1) {
            const int subRootTarget = owner.advanceSubRoot(nodes, walker);

            if (subRootTarget != -1) {
                decidedTarget = subRootTarget;
            }
        }

        const RTNode* const restartNode = nodes.find(decidedTarget);

        if (restartNode != nullptr && restartNode->encapsulationEntryId == decidedTarget) {
            owner.armSubLoop(walker, *restartNode);
        }

        return;
    }

    decidedTarget = chosen;

    const RTNode* const chosenNode = nodes.find(chosen);

    if (chosenNode != nullptr && !scriptAdvances) {
        owner.armSubLoop(walker, *chosenNode);
    }
}

bool TraversalLogic::ModulatorWalk::step()
{
    if (decidedTarget == -1) {
        return false;
    }

    walker.last            = walker.target;
    walker.alternativeLast = walker.alternativeTarget;
    walker.target          = decidedTarget;

    const bool restarted = decidedRestart;

    decidedTarget  = -1;
    decidedRestart = false;

    return restarted;
}
