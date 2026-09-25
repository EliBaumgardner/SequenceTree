#include "EventManager.h"
#include <algorithm>
#include <functional>

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
        bridge.highlightNode(*rootNode, AudioUIBridge::HighlightKind::Show, orphanedRunId,
                             traversal.traversal.key.typeId);
        dispatcher.pushNote(*rootNode, orphanedRunId, context, 0);
    }
}

void EventManager::followTempo(double tempoMultiplier)
{
    if (lastTempoMultiplier > 0.0 && tempoMultiplier != lastTempoMultiplier) {
        const double remainingScale = lastTempoMultiplier / tempoMultiplier;

        for (auto& note : scheduler.activeNotes) {
            note.remainingSamples *= remainingScale;
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

    auto&  activeNotes  = scheduler.activeNotes;
    size_t orderedNotes = 0;

    for (int eventsProcessed = 0; eventsProcessed < maxEventsPerBlock; ++eventsProcessed)
    {
        while (orderedNotes < activeNotes.size()) {
            ++orderedNotes;
            std::ranges::push_heap(activeNotes.begin(), activeNotes.begin() + static_cast<std::ptrdiff_t>(orderedNotes),
                                   std::ranges::greater {}, &NoteScheduler::ActiveNote::remainingSamples);
        }

        double expiringTime = static_cast<double>(numSamples);

        if (!activeNotes.empty()) {
            expiringTime = juce::jmin(expiringTime, activeNotes.front().remainingSamples);
        }

        if (dispatcher.flagScheduler.startNextDue(expiringTime, context)) {
            continue;
        }

        if (expiringTime >= static_cast<double>(numSamples)) {
            break;
        }

        const double expiryTime   = juce::jmax(0.0, expiringTime);
        const int    expirySample = static_cast<int>(expiryTime);

        scheduler.sendNoteOff(activeNotes.front(), context.midiMessages, expirySample);

        std::ranges::pop_heap(activeNotes, std::ranges::greater {}, &NoteScheduler::ActiveNote::remainingSamples);

        const NoteScheduler::ActiveNote expiredNote = activeNotes.back();
        activeNotes.pop_back();
        --orderedNotes;

        if (expiredNote.role == NoteScheduler::NoteRole::ChordVoice) {
            if (NoteScheduler::isNodeAudible(expiredNote.nodeType)) {
                bridge.highlightNode(expiredNote.nodeId, AudioUIBridge::HighlightKind::Hide, expiredNote.runId);
            }
            continue;
        }

        dispatcher.handleExpiredNote(expiredNote, expiryTime, context);
    }

    dispatcher.flagScheduler.advance(numSamples);

    for (auto& note : activeNotes) {
        note.remainingSamples -= numSamples;
    }
}
