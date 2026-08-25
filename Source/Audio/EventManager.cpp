#include "EventManager.h"
#include "../Plugin/PluginProcessor.h"

EventManager::EventManager(SequenceTreeAudioProcessor* p)
    : dispatcher(*p, scheduler, bridge)
{
    jassert(p != nullptr);
}

void EventManager::handleOrphanNotes(juce::MidiBuffer& midiMessages, const NodeMap& nodes, TraversalPool& traversalMap)
{
    auto& activeNotes = scheduler.activeNotes;

    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        auto& activeNote = activeNotes[i];

        if (nodes.find(activeNote.nodeId) != nodes.end()) {
            continue;
        }

        scheduler.handleOrphanNoteOff(activeNote, midiMessages);

        int orphanedInstanceId = activeNote.instanceId;
        scheduler.removeNote(i);

        if (orphanedInstanceId == -1) {
            continue;
        }

        auto traversalIt = traversalMap.find(orphanedInstanceId);

        if (traversalIt == traversalMap.end()) {
            continue;
        }

        TraversalLogic& traversal = traversalIt->second.logic;
        auto rootIt = nodes.find(traversal.rootId);

        if (rootIt == nodes.end()) {
            continue;
        }

        traversal.primary.target = traversal.rootId;
        traversal.state          = TraversalLogic::TraversalState::Active;
        traversal.advanceAlternative(nodes, traversal.rootId);
        bridge.highlightNode(rootIt->second, true, traversal.traversal.traversalId);
        dispatcher.pushNote(rootIt->second, orphanedInstanceId, { nodes, traversalMap, midiMessages }, 0);
    }
}

void EventManager::processEvents(int numSamples, juce::MidiBuffer& midiMessages,
                                   const NodeMap& nodes, TraversalPool& traversalMap)
{
    handleOrphanNotes(midiMessages, nodes, traversalMap);

    const DispatchContext context { nodes, traversalMap, midiMessages };

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

        if (dispatcher.startNextDueFlag(expiringTime, context)) {
            continue;
        }

        if (expiringIndex == -1) {
            break;
        }

        const double expiryTime   = juce::jmax(0.0, expiringTime);
        const int    expirySample = static_cast<int>(expiryTime);

        auto& expiringNote = activeNotes[expiringIndex];

        scheduler.sendNoteOff(expiringNote, midiMessages, expirySample);

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

    dispatcher.advancePendingFlags(numSamples);

    for (auto& note : activeNotes) {
        note.remainingSamples -= numSamples;
    }
}
