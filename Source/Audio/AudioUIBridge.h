#pragma once

#include <juce_core/juce_core.h>
#include "../Graph/RTData.h"
#include <array>
#include <atomic>

template <typename Command, int Capacity = 512>
class CommandFifo
{
public:

    std::atomic<bool> overflowed { false };

    void push(const Command& command)
    {
        const auto scope = fifo.write(1);

        if (scope.blockSize1 > 0) {
            buffer[static_cast<size_t>(scope.startIndex1)] = command;
        }
        else if (scope.blockSize2 > 0) {
            buffer[static_cast<size_t>(scope.startIndex2)] = command;
        }
        else {
            overflowed.store(true);
        }
    }

    template <typename ApplyCommand>
    void drain(ApplyCommand&& apply)
    {
        const auto scope = fifo.read(fifo.getNumReady());

        for (int i = 0; i < scope.blockSize1; ++i) {
            apply(buffer[static_cast<size_t>(scope.startIndex1 + i)]);
        }

        for (int i = 0; i < scope.blockSize2; ++i) {
            apply(buffer[static_cast<size_t>(scope.startIndex2 + i)]);
        }
    }

    bool hasPending() const { return fifo.getNumReady() > 0; }

private:

    juce::AbstractFifo             fifo { Capacity };
    std::array<Command, Capacity>  buffer {};
};

class AudioUIBridge
{
public:

    struct HighlightCommand
    {
        int  nodeId          = 0;
        bool shouldHighlight = false;
        int  runId           = -1;
        int  typeId          = -1;
    };

    struct ArrowCommand
    {
        int  parentNodeId = 0;
        int  childNodeId  = 0;
        int  durationMs   = 0;
        int  trailId      = -1;
        int  typeId       = -1;
        bool isConnection = false;
        bool isReset      = false;
    };

    struct CountCommand
    {
        int nodeId       = 0;
        int currentCount = 0;
        int countLimit   = 1;
    };

    bool hasPendingCommands() const
    {
        return highlights.hasPending()
            || arrows.hasPending()
            || counts.hasPending();
    }

    static constexpr int allNodes  = -1;
    static constexpr int allTrails = -1;

    static int primaryTrail  (int runId) { return runId * 2; }
    static int modulatorTrail(int runId) { return runId * 2 + 1; }

    static int danglingArrowKey(int danglingIndex) { return -(danglingIndex + 1); }

private:

    friend class EventManager;
    friend class FlagScheduler;
    friend class TraversalDispatcher;
    friend class TraversalSession;
    friend class AudioCommandDrainer;

    static constexpr int arrowCommandCapacity = 1024;

    CommandFifo<HighlightCommand>                     highlights;
    CommandFifo<ArrowCommand, arrowCommandCapacity>   arrows;
    CommandFifo<CountCommand>                         counts;

    void highlightNode(int nodeId, bool shouldHighlight, int runId = -1, int typeId = -1)
    {
        highlights.push({ nodeId, shouldHighlight, runId, typeId });
    }

    void clearAllHighlights()
    {
        highlightNode(allNodes, false);
    }

    void highlightNode(const RTNode& node, bool shouldHighlight, int runId = -1, int typeId = -1)
    {
        highlightNode(node.nodeID, shouldHighlight, runId, typeId);
    }

    void pushProgress(int parentNodeId, int childNodeId, int durationMs, int trailId, int typeId, bool isConnection = false)
    {
        arrows.push({ .parentNodeId = parentNodeId, .childNodeId = childNodeId,
                      .durationMs   = durationMs,   .trailId     = trailId,
                      .typeId       = typeId,       .isConnection = isConnection });
    }

    void pushArrowReset(int trailId)
    {
        arrows.push({ .trailId = trailId, .isReset = true });
    }

    void pushCount(int nodeId, int currentCount, int countLimit)
    {
        counts.push({ nodeId, currentCount, countLimit });
    }
};
