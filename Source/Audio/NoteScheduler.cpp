#include "NoteScheduler.h"

NoteScheduler::NoteScheduler()
{
    activeNotes.reserve(maxExpectedActiveNotes);
}

void NoteScheduler::scheduleNote(const RTNode& node, NoteRole role, int runId, double sample,
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
    newNote.role                = role;
    newNote.runId               = runId;
    newNote.event.pitch         = 63;
    newNote.event.velocity      = 63;
    newNote.event.duration      = duration;
    newNote.remainingSamples    = sample + lengthInSamples;
    newNote.nodeId              = node.nodeID;
    newNote.nodeType            = node.nodeType;
    newNote.isConnectionTrigger = isConnectionTrigger;
    newNote.sound               = voicing.sound;

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
        newNote.event.velocity = juce::jlimit(0, 127, juce::roundToInt(newNote.event.velocity * voicing.velocityMultiplier));
    }

    if (!isConnectionTrigger && !node.ornament.empty()) {
        newNote.ornament = { 0, newNote.event.pitch, newNote.event.velocity, lengthInSamples };

        startOrnamentStep(newNote, node.ornament, sample);
    }

    activeNotes.push_back(newNote);

    if (isNoteSounding(newNote)) {
        midiMessages.addEvent(juce::MidiMessage::noteOn(newNote.event.midiChannel, newNote.event.pitch,
                              static_cast<juce::uint8>(newNote.event.velocity)), static_cast<int>(sample));
    }
}

void NoteScheduler::continueOrnament(ActiveNote note, const RTNode* node, juce::MidiBuffer& midiMessages, double sample)
{
    std::span<const OrnamentStep> figure;

    if (node != nullptr) {
        figure = node->ornament;
    }

    ++note.ornament.step;

    startOrnamentStep(note, figure, sample);

    activeNotes.push_back(note);

    if (isNoteSounding(note)) {
        midiMessages.addEvent(juce::MidiMessage::noteOn(note.event.midiChannel, note.event.pitch,
                              static_cast<juce::uint8>(note.event.velocity)), static_cast<int>(sample));
    }
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

bool NoteScheduler::isNoteSounding(const ActiveNote& note)
{
    return isNodeAudible(note.nodeType) && !note.isConnectionTrigger && note.sound == Sound::Audible;
}

bool NoteScheduler::isNodeAudible(RTNode::NodeType nodeType)
{
    return nodeType != RTNode::NodeType::ModulatorRoot
        && nodeType != RTNode::NodeType::TraversalFlagData;
}

void NoteScheduler::startOrnamentStep(ActiveNote& note, std::span<const OrnamentStep> figure, double sample)
{
    OrnamentProgress& progress        = note.ornament;
    double            stepSamples     = progress.remainingSamples;
    int               remainingWeight = 0;

    note.event.pitch    = progress.basePitch;
    note.event.velocity = progress.baseVelocity;

    if (progress.step < static_cast<int>(figure.size())) {
        const OrnamentStep& step = figure[static_cast<std::size_t>(progress.step)];

        note.event.pitch    = juce::jlimit(0, 127, progress.basePitch + step.offset);
        note.event.velocity = juce::jlimit(0, 127, juce::roundToInt(progress.baseVelocity * step.velocityPercent / 100.0));

        for (const OrnamentStep& remainingStep : figure.subspan(static_cast<std::size_t>(progress.step))) {
            remainingWeight += remainingStep.durationWeight;
        }

        if (progress.step + 1 < static_cast<int>(figure.size())) {
            stepSamples = progress.remainingSamples * step.durationWeight / remainingWeight;
        }
    }

    note.remainingSamples      = sample + stepSamples;
    progress.remainingSamples -= stepSamples;
}
