#include "EventManager.h"

EventManager::EventManager(AudioUIBridge& bridgeRef) : bridge(bridgeRef)
{
}

void EventManager::followTempo(double tempoMultiplier)
{
    if (lastTempoMultiplier > 0.0 && tempoMultiplier != lastTempoMultiplier) {
        const double remainingScale = lastTempoMultiplier / tempoMultiplier;

        for (auto& note : scheduler.activeNotes) {
            note.remainingSamples          *= remainingScale;
            note.ornament.remainingSamples *= remainingScale;
        }

        for (auto& pending : dispatcher.flagScheduler.pendingStarts) {
            pending.remainingSamples *= remainingScale;
        }
    }

    lastTempoMultiplier = tempoMultiplier;
}

void EventManager::processEvents(int numSamples, const DispatchContext& context)
{
    handleOrphanNotes(context);

    for (int eventsProcessed = 0; eventsProcessed < maxEventsPerBlock; ++eventsProcessed)
    {
        const NoteScheduler::ActiveNote* const nextNote = scheduler.nextToExpire();
        double expiringTime = static_cast<double>(numSamples);

        if (nextNote != nullptr) {
            expiringTime = juce::jmin(expiringTime, nextNote->remainingSamples);
        }

        if (dispatcher.flagScheduler.startNextDue(expiringTime, context)) {
            continue;
        }

        if (expiringTime >= static_cast<double>(numSamples)) {
            break;
        }

        const double expiryTime   = juce::jmax(0.0, expiringTime);
        const int    expirySample = static_cast<int>(expiryTime);

        const NoteScheduler::ActiveNote expiredNote = scheduler.expireNext(context.midiMessages, expirySample);

        if (expiredNote.ornament.remainingSamples > 0.0) {
            scheduler.continueOrnament(expiredNote, context.nodes.find(expiredNote.nodeId), context.midiMessages, expiryTime);
            continue;
        }

        if (expiredNote.role == NoteScheduler::NoteRole::ChordVoice) {
            if (NoteScheduler::isNodeAudible(expiredNote.nodeType)) {
                bridge.highlightNode(expiredNote.nodeId, BridgeCommands::HighlightKind::Hide, expiredNote.runId);
            }
            continue;
        }

        dispatcher.handleExpiredNote(expiredNote, expiryTime, context);
    }

    dispatcher.flagScheduler.advance(numSamples);

    for (auto& note : scheduler.activeNotes) {
        note.remainingSamples -= numSamples;
    }
}

void EventManager::handleOrphanNotes(const DispatchContext& context)
{
    auto& activeNotes = scheduler.activeNotes;

    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        auto& activeNote = activeNotes[i];

        if (context.nodes.find(activeNote.nodeId) != nullptr) {
            continue;
        }

        scheduler.sendNoteOff(activeNote, context.midiMessages, 0);

        const int                     orphanedRunId = activeNote.runId;
        const NoteScheduler::NoteRole orphanedRole  = activeNote.role;

        scheduler.removeNote(i);

        if (orphanedRole == NoteScheduler::NoteRole::ChordVoice) {
            continue;
        }

        TraversalPool::Instance* const orphanedInstance = context.traversalMap.find(orphanedRunId);

        if (orphanedInstance == nullptr) {
            continue;
        }

        TraversalLogic& traversal = orphanedInstance->logic;
        const RTNode*   rootNode  = context.nodes.find(traversal.rootId);

        if (rootNode == nullptr) {
            continue;
        }

        traversal.primary.target = traversal.rootId;
        traversal.state          = TraversalLogic::TraversalState::Active;
        traversal.advanceAlternative(context.nodes, traversal.rootId);

        bridge    .highlightNode(*rootNode, BridgeCommands::HighlightKind::Show, orphanedRunId, traversal.traversal.key.typeId);
        dispatcher.pushNote(*rootNode, orphanedRunId, context, 0);
    }
}
