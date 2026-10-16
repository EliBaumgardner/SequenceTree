#pragma once

#include <juce_core/juce_core.h>
#include "../Graph/RTData.h"
#include <array>
#include <atomic>
#include <concepts>

enum class CommandDelivery { Deliver, Record };
enum class TrailSource     { Live, Preview };

template <typename Command, int Capacity = 512>
class CommandFifo
{
public:

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

    std::atomic<bool> overflowed { false };

private:

    juce::AbstractFifo            fifo { Capacity };
    std::array<Command, Capacity> buffer {};
};

class AudioUIBridge
{
public:

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
        ArrowKind   kind         = ArrowKind::Progress;

        int         parentNodeId = 0;
        int         childNodeId  = 0;
        int         durationMs   = 0;
        int         trailId      = -1;
        int         traversalId  = -1;
        int         elapsedMs    = 0;

        TrailSource source       = TrailSource::Live;
    };

    struct CountCommand
    {
        int nodeId       = 0;
        int currentCount = 0;
        int countLimit   = 1;
    };

    static constexpr int allTrails = -1;

    CommandDelivery delivery = CommandDelivery::Deliver;

private:

    friend class EventManager;
    friend class FlagScheduler;
    friend class TraversalDispatcher;
    friend class TraversalSession;
    friend class AudioCommandDrainer;

    void beginRecording  ();
    void deliverRecording();

    void send(const HighlightCommand& command);
    void send(const ArrowCommand& command);
    void send(const CountCommand& command);

    void highlightNode     (int nodeId, HighlightKind kind, int runId = -1, int traversalId = -1);
    void highlightNode     (const RTNode& node, HighlightKind kind, int runId = -1, int traversalId = -1);
    void clearAllHighlights();

    void pushProgress  (int parentNodeId, int childNodeId, int durationMs, int trailId, int traversalId,
                        TrailSource source, ArrowKind kind = ArrowKind::Progress);
    void pushArrowReset(int trailId);
    void pushCount     (int nodeId, int currentCount, int countLimit);

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
};
