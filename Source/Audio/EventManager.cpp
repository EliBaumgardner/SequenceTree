#include "EventManager.h"

void EventManager::handleOrphanNotes(const DispatchContext& context)
{
    auto& activeNotes = scheduler.activeNotes;

    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        auto& activeNote = activeNotes[i];

        if (context.nodes.find(activeNote.nodeId) != context.nodes.end()) {
            continue;
        }

        scheduler.handleOrphanNoteOff(activeNote, context.midiMessages);

        int orphanedInstanceId = activeNote.instanceId;
        scheduler.removeNote(i);

        if (orphanedInstanceId == -1) {
            continue;
        }

        auto traversalIt = context.traversalMap.find(orphanedInstanceId);

        if (traversalIt == context.traversalMap.end()) {
            continue;
        }

        TraversalLogic& traversal = traversalIt->second.logic;
        auto rootIt = context.nodes.find(traversal.rootId);

        if (rootIt == context.nodes.end()) {
            continue;
        }

        traversal.primary.target = traversal.rootId;
        traversal.state          = TraversalLogic::TraversalState::Active;
        traversal.advanceAlternative(context.nodes, traversal.rootId);
        bridge.highlightNode(*rootIt->second, true, traversal.traversal.traversalId);
        dispatcher.pushNote(*rootIt->second, orphanedInstanceId, context, 0);
    }
}

void EventManager::processEvents(int numSamples, const DispatchContext& context)
{
    handleOrphanNotes(context);

    auto& activeNotes = scheduler.activeNotes;

    while (true)
    {
        int    expiringIndex = -1;
        double expiringTime  = static_cast<double>(numSamples);

        for (int i = 0; i < static_cast<int>(activeNotes.size()); ++i)
        {
            if (activeNotes[i].remainingSamples < expiringTime) {
                expiringTime  = activeNotes[i].remainingSamples;
                expiringIndex = i;
            }
        }

        if (dispatcher.flagScheduler.startNextDue(expiringTime, context)) {
            continue;
        }

        if (expiringIndex == -1) {
            break;
        }

        const double expiryTime   = juce::jmax(0.0, expiringTime);
        const int    expirySample = static_cast<int>(expiryTime);

        auto& expiringNote = activeNotes[expiringIndex];

        scheduler.sendNoteOff(expiringNote, context.midiMessages, expirySample);

        if (expiringNote.instanceId == -1) {
            if (NoteScheduler::isNodeAudible(expiringNote.nodeType)) {
                bridge.highlightNode(expiringNote.nodeId, false);
            }
            scheduler.removeNote(expiringIndex);
            continue;
        }

        const NoteScheduler::ActiveNote expiredNote = expiringNote;
        scheduler.removeNote(expiringIndex);

        dispatcher.handleExpiredNote(expiredNote, expiryTime, context);
    }

    dispatcher.flagScheduler.advance(numSamples);

    for (auto& note : activeNotes) {
        note.remainingSamples -= numSamples;
    }
}
