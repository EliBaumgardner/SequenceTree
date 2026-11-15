#include "NoteScheduler.h"
#include "../Util/ArrowInfo.h"
#include <algorithm>
#include <functional>

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

    const double durationSamples = duration / ArrowInfo::millisecondsPerSecond * sampleRate / tempoMultiplier;
    const double lengthInSamples = juce::jmax(1.0, durationSamples);

    ActiveNote newNote;

    newNote.role                = role;
    newNote.runId               = runId;
    newNote.event.pitch         = RTNote::fallbackValue;
    newNote.event.velocity      = RTNote::fallbackValue;
    newNote.event.duration      = duration;
    newNote.remainingSamples    = sample + lengthInSamples;
    newNote.nodeId              = node.nodeID;
    newNote.nodeType            = node.nodeType;
    newNote.isConnectionTrigger = isConnectionTrigger;
    newNote.sound               = voicing.sound;

    if (!isConnectionTrigger && !node.notes.empty()) {
        const RTNote& noteData = node.notes[0];

        newNote.event.pitch          = noteData.pitch;
        newNote.event.velocity       = noteData.velocity;
        newNote.event.midiChannel    = juce::jlimit(NodeInfo::minimumMidiChannel, NodeInfo::maximumMidiChannel, noteData.midiChannel);

        if (voicing.pitchOverride >= 0) {
            newNote.event.pitch = voicing.pitchOverride;
        }

        if (voicing.velocityOverride >= 0) {
            newNote.event.velocity = voicing.velocityOverride;
        }

        if (newNote.event.velocity <= 0) {
            newNote.event.velocity = RTNote::fallbackValue;
        }
    }

    if (voicing.channel >= 1) {
        newNote.event.midiChannel = juce::jlimit(NodeInfo::minimumMidiChannel, NodeInfo::maximumMidiChannel, voicing.channel);
    }

    if (voicing.transpose != 0) {
        const int transposedPitch = newNote.event.pitch + voicing.transpose;

        newNote.event.pitch = juce::jlimit(NodeInfo::minimumMidiPitch, NodeInfo::maximumMidiPitch, transposedPitch);
    }

    if (voicing.velocityMultiplier != 1.0) {
        const int scaledVelocity = juce::roundToInt(newNote.event.velocity * voicing.velocityMultiplier);

        newNote.event.velocity = juce::jlimit(NodeInfo::silentMidiVelocity, NodeInfo::maximumMidiVelocity, scaledVelocity);
    }

    if (!isConnectionTrigger && !node.ornament.empty()) {
        newNote.ornament = { 0, newNote.event.pitch, newNote.event.velocity, lengthInSamples };

        startOrnamentStep(newNote, node.ornament, sample);
    }

    activeNotes.push_back(newNote);

    if (isNoteSounding(newNote)) {
        const juce::uint8 noteOnVelocity = static_cast<juce::uint8>(newNote.event.velocity);

        const int noteOnSample = static_cast<int>(sample);

        const juce::MidiMessage noteOn = juce::MidiMessage::noteOn(newNote.event.midiChannel, newNote.event.pitch, noteOnVelocity);

        midiMessages.addEvent(noteOn, noteOnSample);
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
        const juce::uint8 noteOnVelocity = static_cast<juce::uint8>(note.event.velocity);

        const int noteOnSample = static_cast<int>(sample);

        const juce::MidiMessage noteOn = juce::MidiMessage::noteOn(note.event.midiChannel, note.event.pitch, noteOnVelocity);

        midiMessages.addEvent(noteOn, noteOnSample);
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

    orderedNotes = 0;
}

void NoteScheduler::clear()
{
    activeNotes.clear();

    orderedNotes = 0;
}

const NoteScheduler::ActiveNote* NoteScheduler::nextToExpire()
{
    while (orderedNotes < activeNotes.size()) {
        ++orderedNotes;

        const auto orderedEnd = activeNotes.begin() + static_cast<std::ptrdiff_t>(orderedNotes);

        std::ranges::push_heap(activeNotes.begin(), orderedEnd, std::ranges::greater {}, &ActiveNote::remainingSamples);
    }

    if (activeNotes.empty()) {
        return nullptr;
    }

    return &activeNotes.front();
}

NoteScheduler::ActiveNote NoteScheduler::expireNext(juce::MidiBuffer& midiMessages, int sample)
{
    sendNoteOff(activeNotes.front(), midiMessages, sample);

    std::ranges::pop_heap(activeNotes, std::ranges::greater {}, &ActiveNote::remainingSamples);

    const ActiveNote expiredNote = activeNotes.back();

    activeNotes.pop_back();

    --orderedNotes;

    return expiredNote;
}

bool NoteScheduler::isNoteSounding(const ActiveNote& note)
{
    return isNodeAudible(note.nodeType) && !note.isConnectionTrigger && note.sound == Sound::Audible;
}

bool NoteScheduler::isNodeAudible(NodeInfo::Kind nodeType)
{
    return nodeType != NodeInfo::Kind::ModulatorRoot
        && nodeType != NodeInfo::Kind::TraversalFlag;
}

void NoteScheduler::startOrnamentStep(ActiveNote& note, std::span<const OrnamentStep> figure, double sample)
{
    OrnamentProgress& progress = note.ornament;

    double stepSamples = progress.remainingSamples;

    int remainingWeight = 0;

    note.event.pitch    = progress.basePitch;
    note.event.velocity = progress.baseVelocity;

    if (progress.step < static_cast<int>(figure.size())) {
        const std::size_t stepIndex = static_cast<std::size_t>(progress.step);

        const OrnamentStep& step = figure[stepIndex];

        const int steppedPitch    = progress.basePitch + step.offset;
        const int steppedVelocity = juce::roundToInt(progress.baseVelocity * step.velocityPercent / velocityPercentScale);

        note.event.pitch    = juce::jlimit(NodeInfo::minimumMidiPitch, NodeInfo::maximumMidiPitch, steppedPitch);
        note.event.velocity = juce::jlimit(NodeInfo::silentMidiVelocity, NodeInfo::maximumMidiVelocity, steppedVelocity);

        for (const OrnamentStep& remainingStep : figure.subspan(stepIndex)) {
            remainingWeight += remainingStep.durationWeight;
        }

        if (progress.step + 1 < static_cast<int>(figure.size())) {
            stepSamples = progress.remainingSamples * step.durationWeight / remainingWeight;
        }
    }

    note.remainingSamples      = sample + stepSamples;
    progress.remainingSamples -= stepSamples;
}
