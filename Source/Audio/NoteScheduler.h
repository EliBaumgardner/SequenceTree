#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../Graph/RTData.h"
#include "DispatchContext.h"
#include <cstddef>
#include <span>
#include <vector>

/*
    The list of notes that are sounding, and the sender of their MIDI. Each entry in activeNotes
    records a note's pitch, velocity and channel, the node and traversal it belongs to, and the
    samples it has left. A note-on is sent when a note is added and a note-off when its time runs
    out, which can be several blocks later.

    scheduleNote starts a note, which a NoteVoicing can transpose, rescale, override or silence.
    nextToExpire finds the note that ends soonest, and expireNext ends it. A note with an ornament
    plays the figure's steps in turn within its length. Modulator roots, traversal flags, jumps to
    another tree and silent notes take up time but send no MIDI.

    TraversalDispatcher starts notes and EventManager ends them, on the audio thread. It holds at
    most maxExpectedActiveNotes, reserved up front, and drops a note past that.
*/

class NoteScheduler
{
public:

    enum class NoteRole;
    enum class NoteTrigger;
    struct ActiveNote;
    struct NoteTiming;
    struct NoteVoicing;

    NoteScheduler ();

    void scheduleNote (const RTNode& node, NoteRole role, NoteTrigger trigger, const NoteTiming& timing, const DispatchContext& context,
                       const NoteVoicing& voicing);

    void continueOrnament (ActiveNote note, const RTNode* node, juce::MidiBuffer& midiMessages, double sample);
    void sendNoteOff      (const ActiveNote& note, juce::MidiBuffer& midiMessages, int sample);

    void removeNote(int index);
    void clear ();

    const ActiveNote* nextToExpire();

    ActiveNote expireNext(juce::MidiBuffer& midiMessages, int sample);

    static bool isNoteSounding(const ActiveNote& note);
    static bool isNodeAudible (NodeInfo::Kind nodeType);

    struct MidiEvent
    {
        int pitch       = 0;
        int velocity    = 0;
        int duration    = 0;
        int midiChannel = 1;
    };

    enum class NoteRole    { Stepping, ChordVoice };
    enum class NoteTrigger { Step, Connection };
    enum class Sound       { Audible, Silent };

    struct NoteTiming
    {
        int runId    = -1;
        int duration = 0;

        double sample          = 0.0;
        double tempoMultiplier = 1.0;
    };

    struct OrnamentProgress
    {
        int    step             = 0;
        int    basePitch        = 0;
        int    baseVelocity     = 0;

        double remainingSamples = 0.0;
    };

    struct ActiveNote
    {
        MidiEvent event;

        NoteRole  role          = NoteRole::Stepping;
        NodeInfo::Kind nodeType = NodeInfo::Kind::Node;

        int runId  = 0;
        int nodeId = 0;

        double remainingSamples = 0.0;

        bool  isConnectionTrigger = false;

        Sound sound  = Sound::Audible;

        OrnamentProgress ornament;
    };

    struct NoteVoicing
    {
        int channel          = -1;
        int transpose        =  0;
        int pitchOverride    = -1;
        int velocityOverride = -1;

        double velocityMultiplier = 1.0;

        Sound  sound = Sound::Audible;
    };

    static constexpr int maxExpectedActiveNotes = 1024;

    std::vector<ActiveNote> activeNotes;

private:

    static void applyVoicing     (ActiveNote& note, const RTNode& node, const NoteVoicing& voicing);
    static void startOrnamentStep(ActiveNote& note, std::span<const OrnamentStep> figure, double sample);

    static constexpr double velocityPercentScale = 100.0;

    std::size_t orderedNotes = 0;
};
