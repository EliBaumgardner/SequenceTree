#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/ThreadMessaging/BridgeCommands.h"
#include <map>

/*
    The animation state of one Arrow, held as numbers and advanced by time with nothing drawn here.
    It has three parts. The snap is a spring that moves snapProgress from 0 to 1 when an arrow is
    connected, and a ruler that fades out as it settles. The fade moves alpha toward alphaTarget,
    which is how an arrow appears and disappears. The trails are the marks that show a traversal
    walking along the arrow: each Trail is kept under its trail id and goes from 0 to 1 over
    durationMs, and a oneShot trail is removed when it finishes. A trail's source is Live (from
    the playing audio) or Preview (from note preview). While trailsPaused is set the Live trails
    stand still, and resumeTrails moves their start times forward by the time spent paused.

    startTrail begins or restarts a trail, with elapsedMs for one that is already partway along.
    advance takes the time of the display frame and moves all three parts, and returns whether
    any is still moving. Arrow calls it from a callback it attaches to the display frames and
    removes once advance returns false, then reads the state to build its ArrowVisual.
*/

class ArrowAnimation
{
public:

    void startTrail  (int trailId, int durationMs, int elapsedMs, juce::Colour colour, bool oneShot, BridgeCommands::TrailSource source);
    void resumeTrails();

    bool advance(double frameSec);

    struct Trail
    {
        float        progress   = 0.0f;
        double       startMs    = 0.0;
        int          durationMs = 1;
        juce::Colour colour     { juce::Colours::white };
        bool         active     = false;
        bool         oneShot    = false;
        BridgeCommands::TrailSource  source     = BridgeCommands::TrailSource::Live;
    };

    static constexpr float snapSpringRateHz    = 60.0f;
    static constexpr float snapSpringStiffness = 0.20f;
    static constexpr float snapSpringDamping   = 0.30f;
    static constexpr float snapSettledEpsilon  = 0.001f;
    static constexpr float hoverFadePerSecond  = 4.8f;
    static constexpr float hoverFadeEpsilon    = 0.001f;
    static constexpr float rulerFadePerSecond  = 8.0f;
    static constexpr float rulerFadeStartGap   = 0.02f;

    float snapProgress = 1.0f;
    float snapVelocity = 0.0f;

    float alpha       = 1.0f;
    float alphaTarget = 1.0f;

    float rulerFade = 0.0f;

    double lastFrameSec = 0.0;

    bool   trailsPaused = false;
    double pausedAtMs   = 0.0;

    std::map<int, Trail> trails;

private:

    bool advanceSnap  (float elapsedSec);
    bool advanceHover (float elapsedSec);
    bool advanceTrails();

    bool snapSettled();
};
