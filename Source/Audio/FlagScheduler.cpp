#include "FlagScheduler.h"
#include "TraversalDispatcher.h"
#include "AudioUIBridge.h"

#include <algorithm>

FlagScheduler::FlagScheduler(TraversalDispatcher& owner, AudioUIBridge& bridgeRef)
    : dispatcher(owner), bridge(bridgeRef)
{
}

void FlagScheduler::dispatchFlags(const RTNode& node, int hostRunId, const TraversalKey& hostKey,
                                  int parentCount, double sample, double tempoMultiplier,
                                  const DispatchContext& context)
{
    for (const RTConnection& connection : node.connections) {
        const int childId = connection.childId;

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

        const std::vector<TraversalKey>& disabled = connection.disabledTraversals;
        if (std::find(disabled.begin(), disabled.end(), hostKey) != disabled.end()) {
            continue;
        }

        if (flagNode.flagRemovesTraversal) {
            queueRemoval(flagNode, hostRunId, hostKey, context.traversalMap);
            continue;
        }

        const int delayMs = connection.duration;

        if (delayMs <= 0) {
            startFlagTraversal(flagNode, hostKey, sample, context);
            continue;
        }

        queueStart(flagNode, hostKey, delayMs, sample, tempoMultiplier, context);
    }
}

void FlagScheduler::queueStart(const RTNode& flagNode, const TraversalKey& hostKey,
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
        startFlagTraversal(flagNode, hostKey, sample, context);
        return;
    }

    slot->flagNodeId       = flagNode.nodeID;
    slot->hostKey          = hostKey;
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
    earliest->active       = false;

    auto flagIt = context.nodes.find(due.flagNodeId);

    if (flagIt == context.nodes.end()) {
        return true;
    }

    const RTNode& flagNode = *flagIt->second;

    if (flagNode.nodeType != RTNode::NodeType::TraversalFlagData || flagNode.flagRemovesTraversal) {
        return true;
    }

    startFlagTraversal(flagNode, due.hostKey, juce::jmax(0.0, due.remainingSamples), context);

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

void FlagScheduler::queueRemoval(const RTNode& flagNode, int hostRunId, const TraversalKey& hostKey,
                                 TraversalPool& traversalMap)
{
    const TraversalKey targetKey = flagNode.flagTraversal.key;

    if (targetKey.typeId <= 0) {
        return;
    }

    if (!(hostKey == targetKey)) {
        return;
    }

    TraversalPool::Instance* const hostInstance = traversalMap.find(hostRunId);
    if (hostInstance == nullptr) {
        return;
    }

    hostInstance->runtime.pendingRemoval = true;
}

void FlagScheduler::startFlagTraversal(const RTNode& flagNode, const TraversalKey& hostKey, double sample,
                                       const DispatchContext& context)
{
    const TraversalKey spawnKey = flagNode.flagTraversal.key;

    if (spawnKey.typeId <= 0) {
        return;
    }

    if (hostKey == spawnKey) {
        return;
    }

    auto startIt = context.nodes.find(flagNode.flagTargetId);
    if (startIt == context.nodes.end()) {
        return;
    }

    const RTNode& startNode = *startIt->second;
    const int     rootId    = startNode.graphID;

    int runId = context.traversalMap.findRunFor(rootId, spawnKey);

    if (runId == -1) {
        runId = context.traversalMap.nextRunId();
    }
    else {
        const TraversalPool::Instance* const existingInstance = context.traversalMap.find(runId);

        if (existingInstance != nullptr && existingInstance->logic.shouldTraverse()) {
            return;
        }
    }

    TraversalPool::Instance* instance = dispatcher.prepareTraversal(runId, rootId, startNode.nodeID,
                                                                    flagNode.flagTraversal, context);

    if (instance == nullptr) {
        return;
    }

    instance->runtime.asFlag       = true;
    instance->runtime.sourceNodeId = flagNode.nodeID;

    bridge.highlightNode(startNode, true, runId, instance->logic.traversal.key.typeId);
    dispatcher.pushNote(startNode, runId, context, sample);
}
