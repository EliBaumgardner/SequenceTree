#include "TraversalSession.h"
#include "EventManager.h"

#include <algorithm>

static int homeRootId(const TraversalPool::Instance& instance)
{
    if (instance.runtime.originRootId != -1) {
        return instance.runtime.originRootId;
    }

    return instance.logic.rootId;
}

TraversalSession::TraversalSession(EventManager& eventManager) : eventManager(eventManager)
{
    activeRootIdScratch.reserve(scratchCapacity);
    restartRootScratch.reserve(scratchCapacity);
}

void TraversalSession::prepare()
{
    selectChildScript = makeNativeSelectChildScript();
    scriptRule.setScript(&selectChildScript);

    if (useScriptedChildSelection) {
        traversals.prepare(maxConcurrentTraversals, scriptRule);
    }
    else {
        traversals.prepare(maxConcurrentTraversals, NativeTraversalRule::instance());
    }
}

void TraversalSession::silenceAllNotes(juce::MidiBuffer& midiMessages)
{
    for (auto& note : eventManager.scheduler.activeNotes)
    {
        if (NoteScheduler::isNodeAudible(note.nodeType) && !note.isConnectionTrigger) {
            midiMessages.addEvent(juce::MidiMessage::noteOff(note.event.midiChannel, note.event.pitch), 0);
        }
    }

    eventManager.bridge.clearAllHighlights();
    eventManager.scheduler.activeNotes.clear();
}

void TraversalSession::clearTraversals()
{
    eventManager.dispatcher.clearPendingFlags();
    traversals.clear();
}

void TraversalSession::suspendActiveNotes(juce::MidiBuffer& midiMessages)
{
    for (const auto& note : eventManager.scheduler.activeNotes) {
        eventManager.scheduler.sendNoteOff(note, midiMessages, 0);
    }

    eventManager.bridge.clearAllHighlights();
}

void TraversalSession::restartActiveTraversals(const NodeMap& nodes, RTGraphs& rtGraphs,
                                               juce::MidiBuffer& midiMessages)
{
    restartRootScratch.clear();

    eventManager.dispatcher.clearPendingFlags();

    for (const auto& [id, instance] : traversals) {
        if (instance.runtime.isSpawned()) {
            continue;
        }

        const int rootId = homeRootId(instance);

        if (std::find(restartRootScratch.begin(), restartRootScratch.end(), rootId)
            == restartRootScratch.end()) {
            restartRootScratch.push_back(rootId);
        }
    }

    traversals.clear();

    for (int rootId : restartRootScratch) {
        auto rootIt = nodes.find(rootId);
        if (rootIt == nodes.end()) {
            continue;
        }

        for (const RTtraversal& assigned : rootIt->second.traversals) {
            startTraversal(rootIt->second, assigned, nodes, rtGraphs, midiMessages);
        }
    }
}

void TraversalSession::syncWithGraph(const NodeMap& nodes, RTGraphs& rtGraphs,
                                     juce::MidiBuffer& midiMessages)
{
    syncActiveTraversals(nodes);
    removeDeletedTraversals(nodes, midiMessages);
    startMissingTraversals(nodes, rtGraphs, midiMessages);
    syncTraversalLoopLimits(nodes, rtGraphs, midiMessages);
}

void TraversalSession::syncActiveTraversals(const NodeMap& nodes)
{
    for (auto& [id, instance] : traversals) {
        TraversalLogic& logic = instance.logic;

        if (instance.runtime.asFlag) {
            auto flagIt = nodes.find(instance.runtime.sourceNodeId);
            if (flagIt != nodes.end()
                && flagIt->second.flagTraversal.traversalId == logic.traversal.traversalId) {
                logic.traversal = flagIt->second.flagTraversal;
            }
            continue;
        }

        auto rootIt = nodes.find(homeRootId(instance));
        if (rootIt == nodes.end()) {
            continue;
        }

        for (const RTtraversal& assigned : rootIt->second.traversals) {
            if (assigned.traversalId == logic.traversal.traversalId) {
                logic.traversal = assigned;
                break;
            }
        }
    }
}

void TraversalSession::removeDeletedTraversals(const NodeMap& nodes, juce::MidiBuffer& midiMessages)
{
    for (auto it = traversals.begin(); it != traversals.end(); ) {
        const TraversalPool::Instance& instance = it->second;
        const TraversalLogic&          traverser = instance.logic;

        bool stillAssigned = false;
        auto rootIt = nodes.find(homeRootId(instance));

        if (rootIt != nodes.end()) {
            if (instance.runtime.asFlag) {
                stillAssigned = true;
            }
            else {
                for (const RTtraversal& assigned : rootIt->second.traversals) {
                    if (assigned.traversalId == traverser.traversal.traversalId) {
                        stillAssigned = true;
                        break;
                    }
                }
            }
        }

        if (stillAssigned) {
            it = std::next(it);
            continue;
        }

        stopTraversalNotes(it->first, midiMessages);
        it = traversals.erase(it);
    }
}

void TraversalSession::startMissingTraversals(const NodeMap& nodes, RTGraphs& rtGraphs,
                                              juce::MidiBuffer& midiMessages)
{
    activeRootIdScratch.clear();

    for (const auto& [id, instance] : traversals) {
        if (instance.runtime.isSpawned()) {
            continue;
        }

        if (std::find(activeRootIdScratch.begin(), activeRootIdScratch.end(), homeRootId(instance))
            == activeRootIdScratch.end()) {
            activeRootIdScratch.push_back(homeRootId(instance));
        }
    }

    auto isActive = [this](int rootId, int traversalId) {
        for (const auto& [id, instance] : traversals) {
            if (homeRootId(instance) == rootId && instance.logic.traversal.traversalId == traversalId) {
                return true;
            }
        }
        return false;
    };

    for (int rootId : activeRootIdScratch) {
        auto rootIt = nodes.find(rootId);
        if (rootIt == nodes.end()) {
            continue;
        }

        for (const RTtraversal& assigned : rootIt->second.traversals) {
            if (isActive(rootId, assigned.traversalId)) {
                continue;
            }

            startTraversal(rootIt->second, assigned, nodes, rtGraphs, midiMessages);
        }
    }
}

void TraversalSession::syncTraversalLoopLimits(const NodeMap& nodes, RTGraphs& rtGraphs,
                                               juce::MidiBuffer& midiMessages)
{
    for (auto& [instanceId, instance] : traversals)
    {
        TraversalLogic& traversal = instance.logic;

        auto rtGraphIt = rtGraphs.find(traversal.rootId);
        if (rtGraphIt == rtGraphs.end()) {
            continue;
        }

        int newLoopLimit = rtGraphIt->second->loopLimit;
        if (newLoopLimit == traversal.loop.limit) {
            continue;
        }

        traversal.loop.limit = newLoopLimit;

        if (traversal.state == TraversalLogic::TraversalState::End) {
            if (newLoopLimit == 0 || traversal.loop.count < newLoopLimit) {
                traversal.primary.target = traversal.rootId;
                traversal.state          = TraversalLogic::TraversalState::Active;
                traversal.advanceAlternative(nodes, traversal.rootId);

                auto rootIt = nodes.find(traversal.rootId);
                if (rootIt != nodes.end()) {
                    eventManager.bridge.highlightNode(rootIt->second, true);
                    eventManager.dispatcher.pushNote(rootIt->second, instanceId, { nodes, traversals, midiMessages }, 0);
                }
            }
        }
    }
}

bool TraversalSession::isLinkedAsChild(const NodeMap& nodes, int nodeId)
{
    for (const auto& [otherId, other] : nodes) {
        for (int childId : other.children) {
            if (childId == nodeId) {
                return true;
            }
        }
    }

    return false;
}

int TraversalSession::findFirstUnlinkedRootId(const NodeMap& nodes) const
{
    int rootId = -1;

    for (const auto& [nodeId, node] : nodes) {
        if (node.nodeID != node.graphID) {
            continue;
        }

        if (rootId != -1 && nodeId >= rootId) {
            continue;
        }

        if (!isLinkedAsChild(nodes, nodeId)) {
            rootId = nodeId;
        }
    }

    return rootId;
}

bool TraversalSession::startTraversalsFromFirstRoot(const NodeMap& nodes, RTGraphs& rtGraphs,
                                                    juce::MidiBuffer& midiMessages)
{
    const int rootId = findFirstUnlinkedRootId(nodes);

    if (rootId == -1) {
        return false;
    }

    const RTNode& rootNode = nodes.at(rootId);

    for (const RTtraversal& traversal : rootNode.traversals) {
        startTraversal(rootNode, traversal, nodes, rtGraphs, midiMessages);
    }

    return true;
}

void TraversalSession::startTraversal(const RTNode& rootNode, const RTtraversal& traversal,
                                      const NodeMap& nodes, RTGraphs& rtGraphs,
                                      juce::MidiBuffer& midiMessages)
{
    const int rootId      = rootNode.nodeID;
    const int traversalId = traversal.traversalId;
    const int instanceId  = nextTraversalInstanceId();

    TraversalPool::Instance* acquired = traversals.acquire(instanceId, rootId, traversal);

    if (acquired == nullptr) {
        return;
    }

    TraversalLogic& traversalLogic = acquired->logic;

    traversalLogic.instanceId     = instanceId;
    traversalLogic.primary.target = rootId;
    traversalLogic.state          = TraversalLogic::TraversalState::Active;
    traversalLogic.loop.active    = true;

    auto rtGraphIt = rtGraphs.find(rootId);
    if (rtGraphIt != rtGraphs.end()) {
        traversalLogic.loop.limit = rtGraphIt->second->loopLimit;
    }

    traversalLogic.advanceAlternative(nodes, rootId);

    eventManager.bridge.highlightNode(rootNode, true, traversalId);
    eventManager.dispatcher.pushNote(rootNode, instanceId, { nodes, traversals, midiMessages }, 0);
}

void TraversalSession::stopTraversalNotes(int instanceId, juce::MidiBuffer& midiMessages)
{
    auto& activeNotes = eventManager.scheduler.activeNotes;

    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i) {
        auto& note = activeNotes[i];

        if (note.instanceId != instanceId) {
            continue;
        }

        if (NoteScheduler::isNodeAudible(note.nodeType) && !note.isConnectionTrigger) {
            midiMessages.addEvent(juce::MidiMessage::noteOff(note.event.midiChannel, note.event.pitch), 0);
        }

        eventManager.bridge.highlightNode(note.nodeId, false);
        eventManager.scheduler.removeNote(i);
    }
}
