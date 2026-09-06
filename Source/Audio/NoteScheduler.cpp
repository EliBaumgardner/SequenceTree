#include "NoteScheduler.h"

NoteScheduler::NoteScheduler()
{
    activeNotes.reserve(maxExpectedActiveNotes);
}

bool NoteScheduler::isNodeAudible(RTNode::NodeType nodeType)
{
    return nodeType != RTNode::NodeType::ModulatorRoot
        && nodeType != RTNode::NodeType::TraversalFlagData;
}

void NoteScheduler::scheduleNote(const RTNode& node, int instanceId, double sample,
                                 juce::MidiBuffer& midiMessages,
                                 double sampleRate, double tempoMultiplier,
                                 int duration, bool isConnectionTrigger,
                                 const NoteVoicing& voicing)
{
    if (static_cast<int>(activeNotes.size()) >= maxExpectedActiveNotes) {
        return;
    }

    const double lengthInSamples = juce::jmax(1.0, (duration / 1000.0) * sampleRate / tempoMultiplier);

    ActiveNote newNote;
    newNote.instanceId          = instanceId;
    newNote.event.pitch         = 63;
    newNote.event.velocity      = 63;
    newNote.event.duration      = duration;
    newNote.remainingSamples    = sample + lengthInSamples;
    newNote.nodeId              = node.nodeID;
    newNote.nodeType            = node.nodeType;
    newNote.isConnectionTrigger = isConnectionTrigger;

    if (!isConnectionTrigger && !node.notes.empty()) {
        const RTNote& noteData       = node.notes[0];
        newNote.event.pitch          = noteData.pitch;
        newNote.event.velocity       = noteData.velocity;
        newNote.event.midiChannel    = juce::jlimit(1, 16, noteData.midiChannel);

        if (voicing.pitchOverride >= 0) {
            newNote.event.pitch = voicing.pitchOverride;
        }
        if (voicing.velocityOverride >= 0) {
            newNote.event.velocity = voicing.velocityOverride;
        }

        if (newNote.event.velocity <= 0) {
            newNote.event.velocity = 63;
        }
    }

    if (voicing.channel >= 1) {
        newNote.event.midiChannel = juce::jlimit(1, 16, voicing.channel);
    }

    if (voicing.transpose != 0) {
        newNote.event.pitch = juce::jlimit(0, 127, newNote.event.pitch + voicing.transpose);
    }

    if (voicing.velocityMultiplier != 1.0) {
        newNote.event.velocity = juce::jlimit(0, 127,
            juce::roundToInt(newNote.event.velocity * voicing.velocityMultiplier));
    }

    activeNotes.push_back(newNote);

    if (!isConnectionTrigger && isNodeAudible(node.nodeType)) {
        midiMessages.addEvent(juce::MidiMessage::noteOn(newNote.event.midiChannel, newNote.event.pitch,
                              static_cast<juce::uint8>(newNote.event.velocity)), static_cast<int>(sample));
    }
}

bool NoteScheduler::isNoteSounding(const ActiveNote& note)
{
    return isNodeAudible(note.nodeType) && !note.isConnectionTrigger;
}

void NoteScheduler::sendNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages, int sample)
{
    if (isNoteSounding(note)) {
        midiMessages.addEvent(juce::MidiMessage::noteOff(note.event.midiChannel, note.event.pitch), sample);
    }
}

void NoteScheduler::removeNote(int index)
{
    activeNotes[index] = std::move(activeNotes.back());
    activeNotes.pop_back();
}
