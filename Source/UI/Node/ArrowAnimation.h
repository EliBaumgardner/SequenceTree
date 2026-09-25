#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>

class ArrowAnimation
{
public:

    struct Trail
    {
        float        t          = 0.0f;
        double       startMs    = 0.0;
        int          durationMs = 1;
        juce::Colour colour     { juce::Colours::white };
        bool         active     = false;
        bool         oneShot    = false;
    };

    void startTrail(int trailId, int durationMs, juce::Colour colour, bool oneShot);
    void resumeTrails();

    bool advance(double frameSec);

    static constexpr float snapSpringRateHz    {60.0f};
    static constexpr float snapSpringStiffness {0.20f};
    static constexpr float snapSpringDamping   {0.30f};
    static constexpr float snapSettledEpsilon  {0.001f};
    static constexpr float hoverFadePerSecond  {4.8f};
    static constexpr float hoverFadeEpsilon    {0.001f};

    float snapT        = 1.0f;
    float snapVelocity = 0.0f;
    float alpha        = 1.0f;
    float alphaTarget  = 1.0f;

    double lastFrameSec = 0.0;

    bool   trailsPaused = false;
    double pausedAtMs   = 0.0;

    std::map<int, Trail> trails;

private:


    bool advanceSnap(float elapsedSec);
    bool snapSettled();
    bool advanceTrails();
    bool advanceHover(float elapsedSec);
};
