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

        scheduler.sendNoteOff(activeNote, context.midiMessages, 0);

        int orphanedRunId = activeNote.runId;
        scheduler.removeNote(i);

        if (orphanedRunId == -1) {
            continue;
        }

        TraversalPool::Instance* const orphanedInstance = context.traversalMap.find(orphanedRunId);

        if (orphanedInstance == nullptr) {
            continue;
        }

        TraversalLogic& traversal = orphanedInstance->logic;
        auto rootIt = context.nodes.find(traversal.rootId);

        if (rootIt == context.nodes.end()) {
            continue;
        }

        traversal.primary.target = traversal.rootId;
        traversal.state          = TraversalLogic::TraversalState::Active;
        traversal.advanceAlternative(context.nodes, traversal.rootId);
        bridge.highlightNode(*rootIt->second, true, orphanedRunId, traversal.traversal.key.typeId);
        dispatcher.pushNote(*rootIt->second, orphanedRunId, context, 0);
    }
}

void EventManager::processEvents(int numSamples, const DispatchContext& context)
{
    handleOrphanNotes(context);

    auto& activeNotes = scheduler.activeNotes;

    for (int eventsProcessed = 0; eventsProcessed < maxEventsPerBlock; ++eventsProcessed)
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

        if (expiringNote.runId == -1) {
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
