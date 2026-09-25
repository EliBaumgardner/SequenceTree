#pragma once

#include <juce_core/juce_core.h>
#include "../Graph/RTData.h"
#include <array>
#include <atomic>
#include <concepts>

enum class CommandDelivery { Deliver, Record };

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

    template <std::invocable<const Command&> ApplyCommand>
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

    constexpr bool hasPending() const
    {
        return fifo.getNumReady() > 0;
    }

private:

    juce::AbstractFifo            fifo { Capacity };
    std::array<Command, Capacity> buffer {};
};

class AudioUIBridge
{
public:

    enum class HighlightKind { Show, Hide, ClearEveryNode };

    enum class ArrowKind { Progress, Connection, TrailReset };

    struct HighlightCommand
    {
        HighlightKind kind        = HighlightKind::Hide;
        int           nodeId      = 0;
        int           runId       = -1;
        int           traversalId = -1;
    };

    struct ArrowCommand
    {
        ArrowKind kind         = ArrowKind::Progress;
        int       parentNodeId = 0;
        int       childNodeId  = 0;
        int       durationMs   = 0;
        int       trailId      = -1;
        int       traversalId  = -1;
        int       elapsedMs    = 0;
    };

    struct CountCommand
    {
        int nodeId       = 0;
        int currentCount = 0;
        int countLimit   = 1;
    };

    static constexpr int allTrails = -1;

    CommandDelivery delivery = CommandDelivery::Deliver;

    static constexpr int primaryTrail(int runId)
    {
        return runId * 2;
    }
    static constexpr int modulatorTrail(int runId)
    {
        return runId * 2 + 1;
    }

    constexpr bool hasPendingCommands() const
    {
        return highlights.hasPending()
        || arrows.hasPending()
        || counts.hasPending();
    }

private:

    friend class EventManager;
    friend class FlagScheduler;
    friend class TraversalDispatcher;
    friend class TraversalSession;
    friend class AudioCommandDrainer;

    struct RecordedArrow
    {
        ArrowCommand command;
        double       recordedAtMs = 0.0;
    };

    static constexpr int arrowCommandCapacity = 1024;

    static constexpr int recordedHighlightCapacity = 256;
    static constexpr int recordedArrowCapacity     = 512;
    static constexpr int recordedCountCapacity     = 256;

    CommandFifo<HighlightCommand>                   highlights;
    CommandFifo<ArrowCommand, arrowCommandCapacity> arrows;
    CommandFifo<CountCommand>                       counts;

    double recordClockMs = 0.0;

    std::array<HighlightCommand, recordedHighlightCapacity> recordedHighlights {};
    std::array<RecordedArrow, recordedArrowCapacity>        recordedArrows {};
    std::array<CountCommand, recordedCountCapacity>         recordedCounts {};

    int numRecordedHighlights = 0;
    int numRecordedArrows     = 0;
    int numRecordedCounts     = 0;

    void beginRecording()
    {
        delivery              = CommandDelivery::Record;
        recordClockMs         = 0.0;
        numRecordedHighlights = 0;
        numRecordedArrows     = 0;
        numRecordedCounts     = 0;
    }

    void deliverRecording()
    {
        delivery = CommandDelivery::Deliver;

        clearAllHighlights();
        pushArrowReset(allTrails);

        for (int i = 0; i < numRecordedHighlights; ++i) {
            highlights.push(recordedHighlights[static_cast<size_t>(i)]);
        }

        for (int i = 0; i < numRecordedArrows; ++i) {
            const RecordedArrow& recorded = recordedArrows[static_cast<size_t>(i)];

            ArrowCommand command = recorded.command;
            command.elapsedMs    = static_cast<int>(recordClockMs - recorded.recordedAtMs);

            const bool connectionFinished = command.kind == ArrowKind::Connection
                                         && command.elapsedMs >= command.durationMs;

            if (connectionFinished) {
                continue;
            }

            arrows.push(command);
        }

        for (int i = 0; i < numRecordedCounts; ++i) {
            counts.push(recordedCounts[static_cast<size_t>(i)]);
        }
    }

    void send(const HighlightCommand& command)
    {
        if (delivery == CommandDelivery::Deliver) {
            highlights.push(command);
            return;
        }

        if (command.kind == HighlightKind::ClearEveryNode) {
            numRecordedHighlights = 0;
            return;
        }

        const bool hidesEveryRun = command.kind == HighlightKind::Hide && command.runId == -1;

        for (int i = numRecordedHighlights - 1; i >= 0; --i) {
            const HighlightCommand& recorded = recordedHighlights[static_cast<size_t>(i)];

            const bool sameRun = hidesEveryRun || recorded.runId == command.runId;

            if (recorded.nodeId == command.nodeId && sameRun) {
                --numRecordedHighlights;
                recordedHighlights[static_cast<size_t>(i)] = recordedHighlights[static_cast<size_t>(numRecordedHighlights)];
            }
        }

        if (command.kind == HighlightKind::Show && numRecordedHighlights < recordedHighlightCapacity) {
            recordedHighlights[static_cast<size_t>(numRecordedHighlights)] = command;
            ++numRecordedHighlights;
        }
    }

    void send(const ArrowCommand& command)
    {
        if (delivery == CommandDelivery::Deliver) {
            arrows.push(command);
            return;
        }

        const bool resetsTrail = command.kind == ArrowKind::TrailReset;

        for (int i = numRecordedArrows - 1; i >= 0; --i) {
            const ArrowCommand& recorded = recordedArrows[static_cast<size_t>(i)].command;

            bool replaced = recorded.trailId == command.trailId
                         && recorded.parentNodeId == command.parentNodeId
                         && recorded.childNodeId == command.childNodeId;

            if (resetsTrail) {
                replaced = command.trailId == allTrails || recorded.trailId == command.trailId;
            }

            if (replaced) {
                --numRecordedArrows;
                recordedArrows[static_cast<size_t>(i)] = recordedArrows[static_cast<size_t>(numRecordedArrows)];
            }
        }

        if (!resetsTrail && numRecordedArrows < recordedArrowCapacity) {
            recordedArrows[static_cast<size_t>(numRecordedArrows)] = { command, recordClockMs };
            ++numRecordedArrows;
        }
    }

    void send(const CountCommand& command)
    {
        if (delivery == CommandDelivery::Deliver) {
            counts.push(command);
            return;
        }

        for (int i = 0; i < numRecordedCounts; ++i) {
            if (recordedCounts[static_cast<size_t>(i)].nodeId == command.nodeId) {
                recordedCounts[static_cast<size_t>(i)] = command;
                return;
            }
        }

        if (numRecordedCounts < recordedCountCapacity) {
            recordedCounts[static_cast<size_t>(numRecordedCounts)] = command;
            ++numRecordedCounts;
        }
    }

    void highlightNode(int nodeId, HighlightKind kind, int runId = -1, int traversalId = -1)
    {
        send(HighlightCommand { kind, nodeId, runId, traversalId });
    }

    void highlightNode(const RTNode& node, HighlightKind kind, int runId = -1, int traversalId = -1)
    {
        highlightNode(node.nodeID, kind, runId, traversalId);
    }

    void clearAllHighlights()
    {
        send(HighlightCommand { HighlightKind::ClearEveryNode });
    }

    void pushProgress(int parentNodeId, int childNodeId, int durationMs, int trailId, int traversalId,
                      ArrowKind kind = ArrowKind::Progress)
    {
        send(ArrowCommand { kind, parentNodeId, childNodeId, durationMs, trailId, traversalId });
    }

    void pushArrowReset(int trailId)
    {
        send(ArrowCommand { ArrowKind::TrailReset, 0, 0, 0, trailId, -1 });
    }

    void pushCount(int nodeId, int currentCount, int countLimit)
    {
        send(CountCommand { nodeId, currentCount, countLimit });
    }
};
