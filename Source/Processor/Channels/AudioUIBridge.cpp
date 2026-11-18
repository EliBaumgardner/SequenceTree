#include "AudioUIBridge.h"

void AudioUIBridge::beginRecording()
{
    delivery                 = CommandDelivery::Record;
    recordClockMs            = 0.0;
    recordedHighlights.count = 0;
    recordedArrows.count     = 0;
    recordedCounts.count     = 0;
}

void AudioUIBridge::deliverRecording()
{
    delivery = CommandDelivery::Deliver;

    clearAllHighlights();

    pushArrowReset(BridgeCommands::allTrails);

    for (int i = 0; i < recordedHighlights.count; ++i) {
        highlights.push(recordedHighlights.entries[static_cast<size_t>(i)]);
    }

    for (int i = 0; i < recordedArrows.count; ++i) {
        const RecordedArrow& recorded = recordedArrows.entries[static_cast<size_t>(i)];

        BridgeCommands::ArrowCommand command = recorded.command;

        command.elapsedMs    = static_cast<int>(recordClockMs - recorded.recordedAtMs);

        const bool connectionFinished = command.kind == BridgeCommands::ArrowKind::Connection
                                     && command.elapsedMs >= command.durationMs;

        if (connectionFinished) {
            continue;
        }

        arrows.push(command);
    }

    for (int i = 0; i < recordedCounts.count; ++i) {
        counts.push(recordedCounts.entries[static_cast<size_t>(i)]);
    }
}

void AudioUIBridge::send(const BridgeCommands::HighlightCommand& command)
{
    if (delivery == CommandDelivery::Deliver) {
        highlights.push(command);

        return;
    }

    if (command.kind == BridgeCommands::HighlightKind::ClearEveryNode) {
        recordedHighlights.count = 0;

        return;
    }

    const bool hidesEveryRun = command.kind == BridgeCommands::HighlightKind::Hide && command.runId == -1;

    for (int i = recordedHighlights.count - 1; i >= 0; --i) {
        const BridgeCommands::HighlightCommand& recorded = recordedHighlights.entries[static_cast<size_t>(i)];

        const bool sameRun = hidesEveryRun || recorded.runId == command.runId;

        if (recorded.nodeId == command.nodeId && sameRun) {
            --recordedHighlights.count;
            recordedHighlights.entries[static_cast<size_t>(i)] = recordedHighlights.entries[static_cast<size_t>(recordedHighlights.count)];
        }
    }

    if (command.kind == BridgeCommands::HighlightKind::Show && recordedHighlights.count < recordedHighlightCapacity) {
        recordedHighlights.entries[static_cast<size_t>(recordedHighlights.count)] = command;
        ++recordedHighlights.count;
    }
}

void AudioUIBridge::send(const BridgeCommands::ArrowCommand& command)
{
    if (delivery == CommandDelivery::Deliver) {
        arrows.push(command);

        return;
    }

    const bool resetsTrail = command.kind == BridgeCommands::ArrowKind::TrailReset;

    for (int i = recordedArrows.count - 1; i >= 0; --i) {
        const BridgeCommands::ArrowCommand& recorded = recordedArrows.entries[static_cast<size_t>(i)].command;

        bool replaced = recorded.trailId == command.trailId
                     && recorded.parentNodeId == command.parentNodeId
                     && recorded.childNodeId == command.childNodeId;

        if (resetsTrail) {
            replaced = command.trailId == BridgeCommands::allTrails || recorded.trailId == command.trailId;
        }

        if (replaced) {
            --recordedArrows.count;
            recordedArrows.entries[static_cast<size_t>(i)] = recordedArrows.entries[static_cast<size_t>(recordedArrows.count)];
        }
    }

    if (!resetsTrail && recordedArrows.count < recordedArrowCapacity) {
        recordedArrows.entries[static_cast<size_t>(recordedArrows.count)] = { command, recordClockMs };
        ++recordedArrows.count;
    }
}

void AudioUIBridge::send(const BridgeCommands::CountCommand& command)
{
    if (delivery == CommandDelivery::Deliver) {
        counts.push(command);

        return;
    }

    for (int i = 0; i < recordedCounts.count; ++i) {
        if (recordedCounts.entries[static_cast<size_t>(i)].nodeId == command.nodeId) {
            recordedCounts.entries[static_cast<size_t>(i)] = command;

            return;
        }
    }

    if (recordedCounts.count < recordedCountCapacity) {
        recordedCounts.entries[static_cast<size_t>(recordedCounts.count)] = command;
        ++recordedCounts.count;
    }
}

void AudioUIBridge::highlightNode(int nodeId, BridgeCommands::HighlightKind kind, int runId, int traversalId)
{
    send(BridgeCommands::HighlightCommand { kind, nodeId, runId, traversalId });
}

void AudioUIBridge::highlightNode(const RTNode& node, BridgeCommands::HighlightKind kind, int runId, int traversalId)
{
    highlightNode(node.nodeID, kind, runId, traversalId);
}

void AudioUIBridge::clearAllHighlights()
{
    send(BridgeCommands::HighlightCommand { BridgeCommands::HighlightKind::ClearEveryNode });
}

void AudioUIBridge::pushArrowReset(int trailId)
{
    send(BridgeCommands::ArrowCommand { BridgeCommands::ArrowKind::TrailReset, 0, 0, 0, trailId, -1 });
}

void AudioUIBridge::pushCount(int nodeId, int currentCount, int countLimit)
{
    send(BridgeCommands::CountCommand { nodeId, currentCount, countLimit });
}
