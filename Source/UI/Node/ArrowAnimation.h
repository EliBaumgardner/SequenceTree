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

    bool advance();

    static inline const int   tickRateHz          {60};
    static inline const float snapSpringStiffness {0.20f};
    static inline const float snapSpringDamping   {0.30f};
    static inline const float snapSettledEpsilon  {0.001f};
    static inline const float hoverFadeStep       {0.08f};
    static inline const float hoverFadeEpsilon    {0.001f};

    float snapT        = 1.0f;
    float snapVelocity = 0.0f;
    float alpha        = 1.0f;
    float alphaTarget  = 1.0f;

    std::map<int, Trail> trails;

private:

    bool snapSettled() const;

    bool advanceSnap();
    bool advanceTrails();
    bool advanceHover();
};
