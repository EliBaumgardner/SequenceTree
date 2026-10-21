#pragma once

/*
    The vocabulary of messages from the audio thread to the canvas: each type describes one change
    the canvas makes to show playback, which nodes are lit, how far each traversal has moved along
    its arrow, and each node's count.

    HighlightCommand turns a node's highlight on or off, or clears every node. ArrowCommand moves a
    traversal along an arrow, draws a jump to another tree (Connection), or erases a trail.
    CountCommand shows a node's count against its limit.

    Each run draws two trails, primaryTrail for its notes and modulatorTrail for its modulators, and
    allTrails means every trail. TrailSource says whether a trail comes from playback or from note
    preview, so Arrow can draw the two differently.

    It includes nothing, so ArrowAnimation and Arrow can use these types without including
    AudioUIBridge.
*/

namespace BridgeCommands
{

constexpr int primaryTrail(int runId)
{
    return runId * 2;
}

constexpr int modulatorTrail(int runId)
{
    return runId * 2 + 1;
}

constexpr int allTrails = -1;

enum class HighlightKind { Show, Hide, ClearEveryNode };
enum class ArrowKind     { Progress, Connection, TrailReset };
enum class TrailSource   { Live, Preview };

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

}