#pragma once

#include "AudioUIBridge.h"
#include "NoteScheduler.h"
#include "TraversalDispatcher.h"

class EventManager
{
public:

    AudioUIBridge       bridge;
    NoteScheduler       scheduler   { bridge };
    TraversalDispatcher dispatcher  { scheduler, bridge };

    void processEvents(int numSamples, const DispatchContext& context);

private:

    void handleOrphanNotes(const DispatchContext& context);
};
