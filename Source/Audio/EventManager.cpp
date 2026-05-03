#include "EventManager.h"
#include "../Plugin/PluginProcessor.h"

EventManager::EventManager(SequenceTreeAudioProcessor* p)
    : dispatcher(*p, scheduler, bridge)
{
    jassert(p != nullptr);
}

void EventManager::handleOrphanNotes(juce::MidiBuffer& midiMessages, NodeMap& nodes, TraversalMap& traversalMap)
{
    auto& activeNotes = scheduler.activeNotes;

    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        auto& activeNote = activeNotes[i];

        if (nodes.find(activeNote.noteNode.nodeID) != nodes.end()) {
            continue;
        }

        scheduler.handleOrphanNoteOff(activeNote, midiMessages);

        int orphanedTraversalId = activeNote.traversalId;
        scheduler.removeNote(i);

        if (orphanedTraversalId == -1) {
            continue;
        }

        auto traversalIt = traversalMap.find(orphanedTraversalId);

        if (traversalIt == traversalMap.end()) {
            continue;
        }

        TraversalLogic& traversal = traversalIt->second;
        auto rootIt = nodes.find(traversal.rootId);

        if (rootIt == nodes.end()) {
            continue;
        }

        traversal.primary.target = traversal.rootId;
        traversal.state          = TraversalLogic::TraversalState::Active;
        bridge.highlightNode(rootIt->second, true);
        dispatcher.pushNote(rootIt->second, orphanedTraversalId, midiMessages, 0, nodes, traversalMap);
    }
}

void EventManager::processEvents(int numSamples, juce::MidiBuffer& midiMessages,
                                   NodeMap& nodes, TraversalMap& traversalMap)
{
    handleOrphanNotes(midiMessages, nodes, traversalMap);

    auto& activeNotes = scheduler.activeNotes;

    while (true)
    {
        int smallestNoteIndex  = -1;
        int smallestNoteSamples = numSamples + 1;

        for (int i = 0; i < static_cast<int>(activeNotes.size()); ++i)
        {
            if (activeNotes[i].remainingSamples < smallestNoteSamples)
            {
                smallestNoteSamples = activeNotes[i].remainingSamples;
                smallestNoteIndex   = i;
            }
        }

        if (smallestNoteIndex == -1 || smallestNoteSamples > numSamples) {
            break;
        }

        int priorityNoteDuration = smallestNoteSamples;
        auto& activeNote = activeNotes[smallestNoteIndex];

        scheduler.sendNoteOff(activeNote, midiMessages, priorityNoteDuration);

        if (activeNote.traversalId == -1)
        {
            scheduler.removeNote(smallestNoteIndex);
            continue;
        }

        NoteScheduler::ActiveNote expiredNote = activeNote;
        scheduler.removeNote(smallestNoteIndex);

        dispatcher.handleExpiredNote(expiredNote, priorityNoteDuration,
                                     midiMessages, nodes, traversalMap);
    }

    for (auto& note : activeNotes) {
        note.remainingSamples -= numSamples;
    }
}
