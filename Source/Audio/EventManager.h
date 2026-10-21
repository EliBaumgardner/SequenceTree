#pragma once

#include "ThreadMessaging/AudioUIBridge.h"
#include "NoteScheduler.h"
#include "TraversalDispatcher.h"

/*
    The part of playback that starts and ends notes. Once per block, processEvents works through
    everything that falls inside the block in time order: it ends each note whose length runs out
    (sending the note-off at its exact sample), then either moves the note's ornament to its next
    step, hides a chord voice, or has TraversalDispatcher play the node the traversal moves to
    next, and it starts the traversals that traversal flags delayed. It first handles orphan
    notes: a note whose node was deleted while it sounds gets a note-off, and its traversal
    restarts from its root.

    processEvents runs one block. followTempo rescales the time left on every waiting event when
    the tempo changes, so each still lands at the same point in the music.

    It owns the NoteScheduler and TraversalDispatcher it plays through. The processor owns two, one
    for playback and one for note preview, and TraversalSession drives both on the audio thread.
*/

class EventManager
{
public:

    explicit EventManager(AudioUIBridge& bridgeRef);

    void followTempo  (double tempoMultiplier);
    void processEvents(int numSamples, const DispatchContext& context);

    AudioUIBridge&      bridge;
    NoteScheduler       scheduler;
    TraversalDispatcher dispatcher { scheduler, bridge };

    double lastTempoMultiplier = 0.0;

private:

    static constexpr int maxEventsPerBlock = 4096;

    void handleOrphanNotes(const DispatchContext& context);
};
