#include "TraversalSession.h"
#include "EventManager.h"

#include <unordered_set>

TraversalSession::TraversalSession(EventManager& eventManager) : eventManager(eventManager)
{
}

void TraversalSession::silenceAllNotes(juce::MidiBuffer& midiMessages)
{
    for (auto& note : eventManager.scheduler.activeNotes)
    {
        if (NoteScheduler::isNodeAudible(note.noteNode.nodeType) && !note.isConnectionTrigger) {
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), 0);
        }

        eventManager.bridge.highlightNode(note.noteNode, false);
    }

    eventManager.scheduler.activeNotes.clear();
}

void TraversalSession::clearTraversals()
{
    traversals.clear();
}

void TraversalSession::restartActiveTraversals(const NodeMap& nodes, RTGraphs& rtGraphs,
                                               juce::MidiBuffer& midiMessages)
{
    std::unordered_set<int> rootsToRestart;
    for (const auto& [id, traverser] : traversals) {
        rootsToRestart.insert(traverser.rootId);
    }

    traversals.clear();

    for (int rootId : rootsToRestart) {
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
    updateTraversalCounts(nodes);
    syncActiveTraversals(nodes);
    removeDeletedTraversals(nodes, midiMessages);
    startMissingTraversals(nodes, rtGraphs, midiMessages);
    syncTraversalLoopLimits(nodes, rtGraphs, midiMessages);
}

void TraversalSession::updateTraversalCounts(const NodeMap& nodes)
{
    for (const auto& [nodeId, node] : nodes) {
        for (auto& [id, traverser] : traversals) {
            if (!traverser.primary.counts.count(nodeId)) {
                traverser.primary.counts[nodeId] = 0;
            }
        }
    }

    for (auto& [id, traverser] : traversals) {
        for (auto it = traverser.primary.counts.begin(); it != traverser.primary.counts.end(); ) {
            if (nodes.find(it->first) == nodes.end()) {
                it = traverser.primary.counts.erase(it);
            } else {
                it = std::next(it);
            }
        }
    }
}

void TraversalSession::syncActiveTraversals(const NodeMap& nodes)
{
    for (auto& [id, traverser] : traversals) {
        if (traverser.isFlagSpawned) {
            auto flagIt = nodes.find(traverser.flagSourceNodeId);
            if (flagIt != nodes.end()
                && flagIt->second.flagTraversal.traversalId == traverser.traversal.traversalId) {
                traverser.traversal = flagIt->second.flagTraversal;
            }
            continue;
        }

        auto rootIt = nodes.find(traverser.rootId);
        if (rootIt == nodes.end()) {
            continue;
        }

        for (const RTtraversal& assigned : rootIt->second.traversals) {
            if (assigned.traversalId == traverser.traversal.traversalId) {
                traverser.traversal = assigned;
                break;
            }
        }
    }
}

void TraversalSession::removeDeletedTraversals(const NodeMap& nodes, juce::MidiBuffer& midiMessages)
{
    for (auto it = traversals.begin(); it != traversals.end(); ) {
        const TraversalLogic& traverser = it->second;

        bool stillAssigned = false;
        auto rootIt = nodes.find(traverser.rootId);
        if (rootIt != nodes.end()) {
            if (traverser.isFlagSpawned) {
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
    std::unordered_set<int> activeRootIds;
    for (const auto& [id, traverser] : traversals) {
        if (traverser.isFlagSpawned) {
            continue;
        }
        activeRootIds.insert(traverser.rootId);
    }

    auto isActive = [this](int rootId, int traversalId) {
        for (const auto& [id, traverser] : traversals) {
            if (traverser.rootId == rootId && traverser.traversal.traversalId == traversalId) {
                return true;
            }
        }
        return false;
    };

    for (int rootId : activeRootIds) {
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
    for (auto& [instanceId, traversal] : traversals)
    {
        auto rtGraphIt = rtGraphs.find(traversal.rootId);
        if (rtGraphIt == rtGraphs.end()) {
            continue;
        }

        int newLoopLimit = rtGraphIt->second->loopLimit;
        if (newLoopLimit == traversal.loopLimit) {
            continue;
        }

        traversal.loopLimit = newLoopLimit;

        if (traversal.state == TraversalLogic::TraversalState::End) {
            if (newLoopLimit == 0 || traversal.loopCount < newLoopLimit) {
                traversal.primary.target = traversal.rootId;
                traversal.state          = TraversalLogic::TraversalState::Active;
                traversal.advanceAlternative(nodes, traversal.rootId);

                auto rootIt = nodes.find(traversal.rootId);
                if (rootIt != nodes.end()) {
                    eventManager.bridge.highlightNode(rootIt->second, true);
                    eventManager.dispatcher.pushNote(rootIt->second, instanceId, midiMessages, 0, nodes, traversals);
                }
            }
        }
    }
}

int TraversalSession::findFirstUnlinkedRootId(const NodeMap& nodes) const
{
    std::unordered_set<int> linkedRootIds;
    for (const auto& [nodeId, node] : nodes) {
        for (int childId : node.children) {
            linkedRootIds.insert(childId);
        }
    }

    int rootId = -1;

    for (const auto& [nodeId, node] : nodes) {
        if (node.nodeID == node.graphID && linkedRootIds.find(nodeId) == linkedRootIds.end()) {
            if (rootId == -1 || nodeId < rootId) {
                rootId = nodeId;
            }
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

    const RTNode rootNode = nodes.at(rootId);

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

    traversals.insert({ instanceId, TraversalLogic(rootId, eventManager.bridge, traversal) });
    TraversalLogic& traversalLogic = traversals.at(instanceId);

    traversalLogic.instanceId     = instanceId;
    traversalLogic.isFirstEvent   = true;
    traversalLogic.primary.target = rootId;
    traversalLogic.state          = TraversalLogic::TraversalState::Active;
    traversalLogic.isLooping      = true;

    auto rtGraphIt = rtGraphs.find(rootId);
    if (rtGraphIt != rtGraphs.end()) {
        traversalLogic.loopLimit = rtGraphIt->second->loopLimit;
    }

    traversalLogic.advanceAlternative(nodes, rootId);

    eventManager.bridge.highlightNode(rootNode, true, traversalId);
    eventManager.dispatcher.pushNote(rootNode, instanceId, midiMessages, 0, nodes, traversals);
}

void TraversalSession::stopTraversalNotes(int instanceId, juce::MidiBuffer& midiMessages)
{
    auto& activeNotes = eventManager.scheduler.activeNotes;

    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i) {
        auto& note = activeNotes[i];

        if (note.instanceId != instanceId) {
            continue;
        }

        if (NoteScheduler::isNodeAudible(note.noteNode.nodeType) && !note.isConnectionTrigger) {
            midiMessages.addEvent(juce::MidiMessage::noteOff(note.event.midiChannel, note.event.pitch), 0);
        }

        eventManager.bridge.highlightNode(note.noteNode, false);
        eventManager.scheduler.removeNote(i);
    }
}
