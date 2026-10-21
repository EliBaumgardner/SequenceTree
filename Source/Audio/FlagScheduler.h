#pragma once

#include "Traversal/TraversalPool.h"
#include "DispatchContext.h"

#include <array>

/*
    The handler for traversal flags. A traversal flag is a node that starts or stops another
    traversal when the one playing passes it. This decides whether each flag fires, and keeps the
    starts that wait on their arrow's duration in pendingStarts until their time comes, which can
    be in a later block.

    dispatchFlags fires the flags under a node whose count has reached a multiple of the flag's
    count limit. A starting flag starts its traversal at once or after its arrow's duration; a
    removing flag ends the playing traversal with its current note. startNextDue starts the
    earliest waiting traversal due before a given time, and advance counts the rest down.

    TraversalDispatcher owns it and fires flags for every note it plays, and EventManager starts
    the due ones between notes. At most maxPendingStarts starts can wait at once.
*/

class AudioUIBridge;
class TraversalDispatcher;

class FlagScheduler
{
public:

    struct PendingStart
    {
        int          flagNodeId       = -1;
        TraversalKey hostKey;
        double       remainingSamples = 0.0;
        bool         active           = false;
    };


    FlagScheduler(TraversalDispatcher& owner, AudioUIBridge& bridgeRef);

    void dispatchFlags(const RTNode& node, int hostRunId, const TraversalKey& hostKey, int parentCount,
                       double sample, double tempoMultiplier, const DispatchContext& context);

    void advance(int numSamples);
    void clear  ();

    bool startNextDue(double before, const DispatchContext& context);

    static constexpr int maxPendingStarts = 64;

    std::array<PendingStart, maxPendingStarts> pendingStarts {};

private:

    void queueRemoval(const RTNode& flagNode, int hostRunId, const TraversalKey& hostKey, TraversalPool& traversalMap);
    void queueStart  (const RTNode& flagNode, const TraversalKey& hostKey, int delayMs, double sample, double tempoMultiplier, const DispatchContext& context);

    void startFlagTraversal(const RTNode& flagNode, const TraversalKey& hostKey, double sample, const DispatchContext& context);

    TraversalDispatcher& dispatcher;
    AudioUIBridge&       bridge;
};
