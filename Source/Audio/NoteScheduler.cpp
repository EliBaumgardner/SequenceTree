#include "NoteScheduler.h"
#include "AudioUIBridge.h"

NoteScheduler::NoteScheduler(AudioUIBridge& b)
    : bridge(b)
{
    activeNotes.reserve(maxExpectedActiveNotes);
}

bool NoteScheduler::isNodeAudible(RTNode::NodeType nodeType)
{
    return nodeType != RTNode::NodeType::ModulatorRoot
        && nodeType != RTNode::NodeType::TraversalFlagData;
}

void NoteScheduler::scheduleNote(const RTNode& node, int instanceId, int sample,
                                 juce::MidiBuffer& midiMessages,
                                 double sampleRate, double tempoMultiplier,
                                 int duration, bool isConnectionTrigger, int channel, int transpose,
                                 double velocityMultiplier,
                                 int pitchOverride, int velocityOverride)
{
    ActiveNote newNote;
    newNote.instanceId          = instanceId;
    newNote.event.pitch         = 63;
    newNote.event.velocity      = 63;
    newNote.event.duration      = duration;
    newNote.remainingSamples    = static_cast<int>((duration / 1000.0) * sampleRate / tempoMultiplier);
    newNote.nodeId              = node.nodeID;
    newNote.nodeType            = node.nodeType;
    newNote.isConnectionTrigger = isConnectionTrigger;

    if (!isConnectionTrigger && !node.notes.empty()) {
        const RTNote& noteData       = node.notes[0];
        newNote.event.pitch          = static_cast<int>(noteData.pitch);
        newNote.event.velocity       = static_cast<int>(noteData.velocity);
        newNote.event.midiChannel    = juce::jlimit(1, 16, noteData.midiChannel);

        if (pitchOverride >= 0) {
            newNote.event.pitch = pitchOverride;
        }
        if (velocityOverride >= 0) {
            newNote.event.velocity = velocityOverride;
        }

        if (newNote.event.velocity <= 0) {
            newNote.event.velocity = 63;
        }
    }

    if (channel >= 1) {
        newNote.event.midiChannel = juce::jlimit(1, 16, channel);
    }

    if (transpose != 0) {
        newNote.event.pitch = juce::jlimit(0, 127, newNote.event.pitch + transpose);
    }

    if (velocityMultiplier != 1.0) {
        newNote.event.velocity = juce::jlimit(0, 127, juce::roundToInt(newNote.event.velocity * velocityMultiplier));
    }

    activeNotes.push_back(newNote);

    if (!isConnectionTrigger && isNodeAudible(node.nodeType)) {
        midiMessages.addEvent(juce::MidiMessage::noteOn(newNote.event.midiChannel, newNote.event.pitch,
                              static_cast<juce::uint8>(newNote.event.velocity)), sample);
    }
}

void NoteScheduler::sendNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages, int sample)
{
    if (isNodeAudible(note.nodeType) && !note.isConnectionTrigger) {
        midiMessages.addEvent(juce::MidiMessage::noteOff(note.event.midiChannel, note.event.pitch), sample);
    }
}

void NoteScheduler::removeNote(int index)
{
    activeNotes[index] = std::move(activeNotes.back());
    activeNotes.pop_back();
}

void NoteScheduler::clearTraversalNotes(int instanceId)
{
    for (auto& note : activeNotes)
    {
        if (note.instanceId == instanceId) {
            note.instanceId = -1;
        }
    }
}

void NoteScheduler::handleOrphanNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages)
{
    if (isNodeAudible(note.nodeType) && !note.isConnectionTrigger) {
        midiMessages.addEvent(juce::MidiMessage::noteOff(note.event.midiChannel, note.event.pitch), 0);
    }
}
