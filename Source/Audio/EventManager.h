#pragma once

#include "ThreadMessaging/AudioUIBridge.h"
#include "NoteScheduler.h"
#include "TraversalDispatcher.h"

/*
    The event loop of playback. It moves a set of traversals forward through one block of time and
    carries out every event that falls inside it, a note ending or a delayed traversal starting,
    in the order they happen and at the exact sample.

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
