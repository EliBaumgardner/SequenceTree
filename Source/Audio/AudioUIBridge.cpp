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

void AudioUIBridge::send(const HighlightCommand& command)
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

void AudioUIBridge::send(const ArrowCommand& command)
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

void AudioUIBridge::send(const CountCommand& command)
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

void AudioUIBridge::highlightNode(int nodeId, HighlightKind kind, int runId, int traversalId)
{
    send(HighlightCommand { kind, nodeId, runId, traversalId });
}

void AudioUIBridge::highlightNode(const RTNode& node, HighlightKind kind, int runId, int traversalId)
{
    highlightNode(node.nodeID, kind, runId, traversalId);
}

void AudioUIBridge::clearAllHighlights()
{
    send(HighlightCommand { HighlightKind::ClearEveryNode });
}

void AudioUIBridge::pushProgress(int parentNodeId, int childNodeId, int durationMs, int trailId, int traversalId,
                                 TrailSource source, ArrowKind kind)
{
    send(ArrowCommand { kind, parentNodeId, childNodeId, durationMs, trailId, traversalId, 0, source });
}

void AudioUIBridge::pushArrowReset(int trailId)
{
    send(ArrowCommand { ArrowKind::TrailReset, 0, 0, 0, trailId, -1 });
}

void AudioUIBridge::pushCount(int nodeId, int currentCount, int countLimit)
{
    send(CountCommand { nodeId, currentCount, countLimit });
}
