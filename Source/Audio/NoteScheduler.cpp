#include "NoteScheduler.h"
#include "AudioUIBridge.h"

NoteScheduler::NoteScheduler(AudioUIBridge& b)
    : bridge(b)
{
}

bool NoteScheduler::isNodeAudible(RTNode::NodeType nodeType)
{
    return nodeType != RTNode::NodeType::Connector
        && nodeType != RTNode::NodeType::ModulatorRoot;
}

void NoteScheduler::scheduleNote(const RTNode& node, int traversalId, int sample,
                                 juce::MidiBuffer& midiMessages,
                                 double sampleRate, double tempoMultiplier,
                                 int duration, bool isConnectionTrigger)
{
    ActiveNote newNote;
    newNote.traversalId         = traversalId;
    newNote.event.pitch         = 63;
    newNote.event.velocity      = 63;
    newNote.event.duration      = duration;
    newNote.remainingSamples    = static_cast<int>((duration / 1000.0) * sampleRate / tempoMultiplier);
    newNote.noteNode            = node;
    newNote.isConnectionTrigger = isConnectionTrigger;

    if (!isConnectionTrigger && !node.notes.empty())
    {
        const RTNote& noteData = node.notes[0];
        newNote.event.pitch    = static_cast<int>(noteData.pitch);
        newNote.event.velocity = static_cast<int>(noteData.velocity);

        if (newNote.event.velocity <= 0) {
            newNote.event.velocity = 63;
        }
    }

    activeNotes.push_back(newNote);

    if (!isConnectionTrigger && isNodeAudible(node.nodeType)) {
        midiMessages.addEvent(juce::MidiMessage::noteOn(1, newNote.event.pitch,
                              static_cast<juce::uint8>(newNote.event.velocity)), sample);
    }
}

void NoteScheduler::sendNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages, int sample)
{
    if (isNodeAudible(note.noteNode.nodeType) && !note.isConnectionTrigger)
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), sample);
}

void NoteScheduler::removeNote(int index)
{
    activeNotes[index] = std::move(activeNotes.back());
    activeNotes.pop_back();
}

void NoteScheduler::clearTraversalNotes(int traversalId)
{
    for (auto& note : activeNotes)
    {
        if (note.traversalId == traversalId)
            note.traversalId = -1;
    }
}

void NoteScheduler::handleOrphanNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages)
{
    if (isNodeAudible(note.noteNode.nodeType) && !note.isConnectionTrigger)
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, note.event.pitch), 0);
}
