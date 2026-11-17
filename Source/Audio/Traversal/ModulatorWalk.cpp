#include "ModulatorWalk.h"
#include "WeightedPick.h"

#include <algorithm>

void ModulatorWalk::activate(int rootId, int hostId)
{
    gate.activeRootId = rootId;
    gate.hostId       = hostId;
    gate.repeatCount  = 0;
    walker            = { .target = rootId };
    decidedTarget     = -1;
    decidedRestart    = false;
}

void ModulatorWalk::deactivate()
{
    gate           = {};
    walker         = {};
    decidedTarget  = -1;
    decidedRestart = false;
}

void ModulatorWalk::decide(const NodeMap& nodes, ChildSelector& selector, TraversalMode mode, std::optional<int> scriptChoice)
{
    int chosen = -1;

    decidedRestart = false;

    if (scriptChoice.has_value()) {
        if (nodes.find(*scriptChoice) != nullptr) {
            chosen = *scriptChoice;
        }
    }
    else {
        chosen = selector.chooseChild(nodes, walker, NodeStateSlot::ModulatorCount, &ChildSelector::isModulatorChild);
    }

    if (chosen == -1) {
        decidedRestart = true;
        decidedTarget  = gate.activeRootId;

        if (walker.subRootNode != -1) {
            const int subRootTarget = selector.advanceSubRoot(nodes, walker);

            if (subRootTarget != -1) {
                decidedTarget = subRootTarget;
            }
        }

        const RTNode* const restartNode = nodes.find(decidedTarget);

        if (restartNode != nullptr && restartNode->encapsulationEntryId == decidedTarget) {
            selector.armSubLoop(walker, *restartNode, mode);
        }

        return;
    }

    decidedTarget = chosen;

    const RTNode* const chosenNode = nodes.find(chosen);

    if (chosenNode != nullptr && ! scriptChoice.has_value()) {
        selector.armSubLoop(walker, *chosenNode, mode);
    }
}

ModulatorWalk::Change ModulatorWalk::followMainWalk(const NodeMap& nodes, ChildSelector& selector, const RTNode& enteredNode,
                                                    bool isPrimaryRepeat)
{
    const int triggeredRootId = findActiveModulatorRoot(nodes, selector, enteredNode.nodeID);

    const bool isHost       = enteredNode.nodeID == gate.hostId;
    const bool isDescendant = isDescendantOf(nodes, enteredNode.nodeID, gate.hostId);

    bool advancesOnHost = false;

    int repeatSourceId       = walker.target;
    int modulatorRepeatValue = 1;

    if (triggeredRootId != -1) {
        activate(triggeredRootId, enteredNode.nodeID);

        selector.advanceAlternative(nodes, walker, Walk::Modulator, walker.target);

        return Change::Started;
    }

    if (! isActive()) {
        return Change::None;
    }

    if (! isHost && ! isDescendant) {
        deactivate();

        return Change::Stopped;
    }

    if (isHost) {
        const RTConnection* const modRootConnection = enteredNode.findConnection(gate.activeRootId);

        if (modRootConnection != nullptr && ! modRootConnection->isSynced) {
            advancesOnHost = true;
        }
    }

    if ((! isDescendant && ! advancesOnHost) || isPrimaryRepeat) {
        return Change::None;
    }

    if (walker.alternativeTarget != -1) {
        repeatSourceId = walker.alternativeTarget;
    }

    const RTNode* const repeatSource = nodes.find(repeatSourceId);

    if (repeatSource != nullptr) {
        modulatorRepeatValue = repeatSource->repeatValue;
    }

    if (! tickRepeat(modulatorRepeatValue)) {
        return Change::None;
    }

    const bool advances  = decidedTarget != -1;
    const bool restarted = step();

    if (advances) {
        selector.advanceAlternative(nodes, walker, Walk::Modulator, walker.target);
    }

    if (restarted) {
        return Change::Restarted;
    }

    if (advances) {
        return Change::Advanced;
    }

    return Change::None;
}

int ModulatorWalk::findActiveModulatorRoot(const NodeMap& nodes, const ChildSelector& selector, int regularNodeId) const
{
    const RTNode* const hostNode = nodes.find(regularNodeId);

    if (hostNode == nullptr) {
        return -1;
    }

    const RTNode& host = *hostNode;

    const int hostCount   = selector.nodeState.get(NodeStateSlot::Count, regularNodeId) + 1;
    const int randomValue = static_cast<int>(selector.selectionRandom >> 1);

    const RTNode* chosenRoot = nullptr;

    WeightedPick weightedPick;

    for (const RTConnection& connection : host.connections) {
        const RTNode* const modRoot = eligibleModulatorRoot(nodes, selector.traversalKey, connection, hostCount);

        if (modRoot == nullptr) {
            continue;
        }

        weightedPick.tally(modRoot->countLimit, modRoot->probability);
    }

    if (! weightedPick.draw(randomValue)) {
        return -1;
    }

    for (const RTConnection& connection : host.connections) {
        const RTNode* const modRoot = eligibleModulatorRoot(nodes, selector.traversalKey, connection, hostCount);

        if (modRoot == nullptr) {
            continue;
        }

        if (weightedPick.accepts(modRoot->countLimit, modRoot->probability)) {
            chosenRoot = modRoot;

            break;
        }
    }

    if (chosenRoot == nullptr) {
        return -1;
    }

    const RTConnection* const modRootConnection = host.findConnection(chosenRoot->nodeID);

    const bool alreadyWalking = gate.activeRootId == chosenRoot->nodeID && gate.hostId == regularNodeId;

    if (modRootConnection != nullptr && ! modRootConnection->isSynced && alreadyWalking) {
        return -1;
    }

    return chosenRoot->nodeID;
}

bool ModulatorWalk::isActive() const
{
    return gate.activeRootId != -1;
}

bool ModulatorWalk::step()
{
    const bool restarted = decidedRestart;

    if (decidedTarget == -1) {
        return false;
    }

    walker.last            = walker.target;
    walker.alternativeLast = walker.alternativeTarget;
    walker.target          = decidedTarget;

    decidedTarget  = -1;
    decidedRestart = false;

    return restarted;
}

bool ModulatorWalk::tickRepeat(int repeatLimit)
{
    gate.repeatCount++;

    if (gate.repeatCount >= repeatLimit) {
        gate.repeatCount = 0;

        return true;
    }

    return false;
}

const RTNode* ModulatorWalk::eligibleModulatorRoot(const NodeMap& nodes, const TraversalKey& traversalKey, const RTConnection& connection,
                                                   int hostCount) const
{
    const RTNode* const childNode = nodes.find(connection.childId);

    const std::vector<TraversalKey>& disabled = connection.disabledTraversals;

    if (childNode == nullptr) {
        return nullptr;
    }

    if (childNode->nodeType != NodeInfo::Kind::ModulatorRoot) {
        return nullptr;
    }

    if (std::ranges::find(disabled, traversalKey) != disabled.end()) {
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

bool ModulatorWalk::isDescendantOf(const NodeMap& nodes, int nodeId, int ancestorId)
{
    int current = nodeId;
    int guard   = 0;

    if (ancestorId == -1 || nodeId == ancestorId) {
        return false;
    }

    while (current != 0 && guard++ < maximumAncestorSteps) {
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
