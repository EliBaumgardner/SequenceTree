#include "AudioUIBridge.h"

void AudioUIBridge::beginRecording()
{
    delivery              = CommandDelivery::Record;
    recordClockMs         = 0.0;
    numRecordedHighlights = 0;
    numRecordedArrows     = 0;
    numRecordedCounts     = 0;
}

void AudioUIBridge::deliverRecording()
{
    delivery = CommandDelivery::Deliver;

    clearAllHighlights();
    pushArrowReset(BridgeCommands::allTrails);

    for (int i = 0; i < numRecordedHighlights; ++i) {
        highlights.push(recordedHighlights[static_cast<size_t>(i)]);
    }

    for (int i = 0; i < numRecordedArrows; ++i) {
        const RecordedArrow& recorded = recordedArrows[static_cast<size_t>(i)];

        BridgeCommands::ArrowCommand command = recorded.command;
        command.elapsedMs    = static_cast<int>(recordClockMs - recorded.recordedAtMs);

        const bool connectionFinished = command.kind == BridgeCommands::ArrowKind::Connection
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

void AudioUIBridge::send(const BridgeCommands::HighlightCommand& command)
{
    if (delivery == CommandDelivery::Deliver) {
        highlights.push(command);
        return;
    }

    if (command.kind == BridgeCommands::HighlightKind::ClearEveryNode) {
        numRecordedHighlights = 0;
        return;
    }

    const bool hidesEveryRun = command.kind == BridgeCommands::HighlightKind::Hide && command.runId == -1;

    for (int i = numRecordedHighlights - 1; i >= 0; --i) {
        const BridgeCommands::HighlightCommand& recorded = recordedHighlights[static_cast<size_t>(i)];

        const bool sameRun = hidesEveryRun || recorded.runId == command.runId;

        if (recorded.nodeId == command.nodeId && sameRun) {
            --numRecordedHighlights;
            recordedHighlights[static_cast<size_t>(i)] = recordedHighlights[static_cast<size_t>(numRecordedHighlights)];
        }
    }

    if (command.kind == BridgeCommands::HighlightKind::Show && numRecordedHighlights < recordedHighlightCapacity) {
        recordedHighlights[static_cast<size_t>(numRecordedHighlights)] = command;
        ++numRecordedHighlights;
    }
}

void AudioUIBridge::send(const BridgeCommands::ArrowCommand& command)
{
    if (delivery == CommandDelivery::Deliver) {
        arrows.push(command);
        return;
    }

    const bool resetsTrail = command.kind == BridgeCommands::ArrowKind::TrailReset;

    for (int i = numRecordedArrows - 1; i >= 0; --i) {
        const BridgeCommands::ArrowCommand& recorded = recordedArrows[static_cast<size_t>(i)].command;

        bool replaced = recorded.trailId == command.trailId
                     && recorded.parentNodeId == command.parentNodeId
                     && recorded.childNodeId == command.childNodeId;

        if (resetsTrail) {
            replaced = command.trailId == BridgeCommands::allTrails || recorded.trailId == command.trailId;
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

void AudioUIBridge::send(const BridgeCommands::CountCommand& command)
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

void AudioUIBridge::pushProgress(int parentNodeId, int childNodeId, int durationMs, int trailId, int traversalId,
                                 BridgeCommands::TrailSource source, BridgeCommands::ArrowKind kind)
{
    send(BridgeCommands::ArrowCommand { kind, parentNodeId, childNodeId, durationMs, trailId, traversalId, 0, source });
}

void AudioUIBridge::pushArrowReset(int trailId)
{
    send(BridgeCommands::ArrowCommand { BridgeCommands::ArrowKind::TrailReset, 0, 0, 0, trailId, -1 });
}

void AudioUIBridge::pushCount(int nodeId, int currentCount, int countLimit)
{
    send(BridgeCommands::CountCommand { nodeId, currentCount, countLimit });
}
