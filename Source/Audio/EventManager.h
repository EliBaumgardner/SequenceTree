#pragma once

#include "AudioUIBridge.h"
#include "NoteScheduler.h"
#include "TraversalDispatcher.h"

class EventManager
{
public:

    AudioUIBridge       bridge;
    NoteScheduler       scheduler;
    TraversalDispatcher dispatcher  { scheduler, bridge };

    void processEvents(int numSamples, const DispatchContext& context);

private:

    static constexpr int maxEventsPerBlock = 4096;

    void handleOrphanNotes(const DispatchContext& context);
};
