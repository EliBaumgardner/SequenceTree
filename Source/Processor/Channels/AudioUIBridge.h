#pragma once

#include "BridgeCommands.h"
#include "CommandFifo.h"
#include "Core/Model/RTData.h"
#include <array>

enum class CommandDelivery { Deliver, Record };

/*
    The one-way channel from the audio thread to the canvas, which is how playback is shown on
    screen. The audio thread may not change a component, lock or allocate, so it puts plain
    commands (light a node, move along an arrow, show a count) into three CommandFifos and the
    canvas empties them on the message thread. Each command is either delivered as it is sent or
    recorded to deliver later.

    While delivery is CommandDelivery::Deliver, highlightNode, clearAllHighlights, send,
    pushArrowReset and pushCount each put their command straight into its queue. beginRecording
    switches to CommandDelivery::Record: a command then replaces the earlier ones for the same node,
    arrow or trail, so only what the canvas should end up showing is kept, and each arrow notes when
    it started. deliverRecording switches back and sends a clear, a reset of every trail, and then
    that kept state, each arrow marked with how long it has already been running.

    It is the only way the audio thread reaches the UI. TraversalSession records while it replays
    up to a host relocation. The audio thread only writes the queues and never posts a message,
    locks or allocates. AudioCommandDrainer polls them on the message thread once per display
    frame, so with no editor open the queues fill and overflow and the canvas resyncs on its first
    drain.

    Everything but hasPendingCommands and delivery is private to the friend classes that send or
    drain commands.
*/

class AudioUIBridge
{
public:

    constexpr bool hasPendingCommands() const
    {
        return highlights.hasPending()
        || arrows.hasPending()
        || counts.hasPending();
    }

    CommandDelivery delivery = CommandDelivery::Deliver;

private:

    friend class EventManager;
    friend class FlagScheduler;
    friend class TraversalDispatcher;
    friend class TraversalSession;
    friend class AudioCommandDrainer;

    void beginRecording  ();
    void deliverRecording();

    void send(const BridgeCommands::HighlightCommand& command);
    void send(const BridgeCommands::ArrowCommand&     command);
    void send(const BridgeCommands::CountCommand&     command);

    void highlightNode(int nodeId,         BridgeCommands::HighlightKind kind, int runId = -1, int traversalId = -1);
    void highlightNode(const RTNode& node, BridgeCommands::HighlightKind kind, int runId = -1, int traversalId = -1);
    void clearAllHighlights ();

    void pushArrowReset(int trailId);
    void pushCount     (int nodeId, int currentCount, int countLimit);

    struct RecordedArrow
    {
        BridgeCommands::ArrowCommand command;

        double recordedAtMs = 0.0;
    };

    template <typename Entry, int capacity>
    struct Recording
    {
        std::array<Entry, capacity> entries {};

        int count = 0;
    };

    static constexpr int arrowCommandCapacity      = 1024;
    static constexpr int recordedHighlightCapacity = 256;
    static constexpr int recordedArrowCapacity     = 512;
    static constexpr int recordedCountCapacity     = 256;

    double recordClockMs = 0.0;

    CommandFifo<BridgeCommands::HighlightCommand> highlights;
    CommandFifo<BridgeCommands::ArrowCommand, arrowCommandCapacity> arrows;
    CommandFifo<BridgeCommands::CountCommand> counts;

    Recording<BridgeCommands::HighlightCommand, recordedHighlightCapacity> recordedHighlights;
    Recording<RecordedArrow, recordedArrowCapacity>                        recordedArrows;
    Recording<BridgeCommands::CountCommand, recordedCountCapacity>         recordedCounts;
};
