#include "FlagScheduler.h"
#include "TraversalDispatcher.h"
#include "AudioUIBridge.h"

namespace {

int flagStartDelayMs(const RTNode& hostNode, const RTNode& flagNode)
{
    auto it = hostNode.durationMap.find(flagNode.nodeID);

    if (it == hostNode.durationMap.end()) {
        return 0;
    }

    return it->second;
}

}

FlagScheduler::FlagScheduler(TraversalDispatcher& owner, AudioUIBridge& bridgeRef)
    : dispatcher(owner), bridge(bridgeRef)
{
}

void FlagScheduler::dispatchFlags(const RTNode& node, int hostInstanceId, int hostTypeId,
                                  int parentCount, double sample, double tempoMultiplier,
                                  const DispatchContext& context)
{
    for (int childId : node.children) {
        auto childIt = context.nodes.find(childId);
        if (childIt == context.nodes.end()) {
            continue;
        }

        const RTNode& flagNode = *childIt->second;
        if (flagNode.nodeType != RTNode::NodeType::TraversalFlagData) {
            continue;
        }

        if (flagNode.countLimit <= 0 || parentCount % flagNode.countLimit != 0) {
            continue;
        }

        if (isChildDisabledForTraversal(node, childId, hostTypeId)) {
            continue;
        }

        if (flagNode.flagRemovesTraversal) {
            queueRemoval(flagNode, hostInstanceId, hostTypeId, context.traversalMap);
            continue;
        }

        const int delayMs = flagStartDelayMs(node, flagNode);

        if (delayMs <= 0) {
            startFlagTraversal(flagNode, hostTypeId, sample, context);
            continue;
        }

        queueStart(flagNode, hostTypeId, delayMs, sample, tempoMultiplier, context);
    }
}

void FlagScheduler::queueStart(const RTNode& flagNode, int hostTypeId,
                               int delayMs, double sample, double tempoMultiplier,
                               const DispatchContext& context)
{
    const double delaySamples = (delayMs / 1000.0) * context.sampleRate / tempoMultiplier;

    PendingStart* slot = nullptr;

    for (PendingStart& pending : pendingStarts) {
        if (!pending.active) {
            slot = &pending;
            break;
        }
    }

    if (slot == nullptr) {
        jassertfalse;
        startFlagTraversal(flagNode, hostTypeId, sample, context);
        return;
    }

    slot->flagNodeId       = flagNode.nodeID;
    slot->hostTypeId       = hostTypeId;
    slot->remainingSamples = delaySamples + sample;
    slot->active           = true;
}

bool FlagScheduler::startNextDue(double before, const DispatchContext& context)
{
    PendingStart* earliest = nullptr;

    for (PendingStart& pending : pendingStarts) {
        if (!pending.active || pending.remainingSamples >= before) {
            continue;
        }

        if (earliest == nullptr || pending.remainingSamples < earliest->remainingSamples) {
            earliest = &pending;
        }
    }

    if (earliest == nullptr) {
        return false;
    }

    const PendingStart due = *earliest;
    earliest->active = false;

    auto flagIt = context.nodes.find(due.flagNodeId);

    if (flagIt == context.nodes.end()) {
        return true;
    }

    const RTNode& flagNode = *flagIt->second;

    if (flagNode.nodeType != RTNode::NodeType::TraversalFlagData || flagNode.flagRemovesTraversal) {
        return true;
    }

    startFlagTraversal(flagNode, due.hostTypeId, juce::jmax(0.0, due.remainingSamples), context);

    return true;
}

void FlagScheduler::advance(int numSamples)
{
    for (PendingStart& pending : pendingStarts) {
        if (pending.active) {
            pending.remainingSamples -= numSamples;
        }
    }
}

void FlagScheduler::clear()
{
    for (PendingStart& pending : pendingStarts) {
        pending.active = false;
    }
}

void FlagScheduler::queueRemoval(const RTNode& flagNode, int hostInstanceId, int hostTypeId,
                                 TraversalPool& traversalMap)
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

    traversalIt->second.runtime.pendingRemoval = true;
}

void FlagScheduler::startFlagTraversal(const RTNode& flagNode, int hostTypeId, double sample,
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

    const RTNode& startNode = *startIt->second;
    const int rootId = startNode.graphID;

    int instanceId = context.traversalMap.findInstanceFor(rootId, spawnTypeId);

    if (instanceId == -1) {
        instanceId = context.traversalMap.nextInstanceId();
    }
    else if (context.traversalMap.find(instanceId)->second.logic.shouldTraverse()) {
        return;
    }

    TraversalPool::Instance* instance = dispatcher.prepareTraversal(instanceId, rootId, startNode.nodeID,
                                                                    flagNode.flagTraversal, context);

    if (instance == nullptr) {
        return;
    }

    instance->runtime.asFlag       = true;
    instance->runtime.sourceNodeId = flagNode.nodeID;

    bridge.highlightNode(startNode, true, instance->logic.traversal.traversalId);
    dispatcher.pushNote(startNode, instanceId, context, sample);
}
