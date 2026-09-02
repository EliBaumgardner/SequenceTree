#pragma once

#include "TraversalPool.h"

#include <array>

class AudioUIBridge;
class TraversalDispatcher;

class FlagScheduler
{
public:

    FlagScheduler(TraversalDispatcher& owner, AudioUIBridge& bridgeRef);

    void dispatchFlags(const RTNode& node, int hostInstanceId, int hostTypeId,
                       int parentCount, double sample, double tempoMultiplier,
                       const DispatchContext& context);

    bool startNextDue(double before, const DispatchContext& context);

    void advance(int numSamples);

    void clear();

private:

    struct PendingStart
    {
        int    flagNodeId       = -1;
        int    hostTypeId       = 0;
        double remainingSamples = 0.0;
        bool   active           = false;
    };

    void queueStart(const RTNode& flagNode, int hostTypeId, int delayMs, double sample,
                    double tempoMultiplier, const DispatchContext& context);

    void queueRemoval(const RTNode& flagNode, int hostInstanceId, int hostTypeId,
                      TraversalPool& traversalMap);

    void startFlagTraversal(const RTNode& flagNode, int hostTypeId, double sample,
                            const DispatchContext& context);

    static constexpr int maxPendingStarts = 64;

    TraversalDispatcher& dispatcher;
    AudioUIBridge&       bridge;

    std::array<PendingStart, maxPendingStarts> pendingStarts {};
};
