#include "ArrowAnimation.h"

void ArrowAnimation::startTrail(int trailId, int durationMs, juce::Colour colour, bool oneShot)
{
    Trail& trail = trails[trailId];

    trail.t          = 0.0f;
    trail.startMs    = juce::Time::getMillisecondCounterHiRes();
    trail.durationMs = juce::jmax(1, durationMs);
    trail.colour     = colour;
    trail.active     = durationMs > 0;
    trail.oneShot    = oneShot;
}

bool ArrowAnimation::advance()
{
    const bool snapRunning   = advanceSnap();
    const bool trailsRunning = advanceTrails();
    const bool hoverRunning  = advanceHover();

    return snapRunning || trailsRunning || hoverRunning;
}

bool ArrowAnimation::snapSettled() const
{
    return std::abs(snapT - 1.0f) < snapSettledEpsilon
        && std::abs(snapVelocity) < snapSettledEpsilon;
}

bool ArrowAnimation::advanceSnap()
{
    if (snapSettled()) {
        return false;
    }

    snapVelocity += (1.0f - snapT) * snapSpringStiffness;
    snapVelocity *= snapSpringDamping;
    snapT        += snapVelocity;

    if (snapSettled()) {
        snapT        = 1.0f;
        snapVelocity = 0.0f;
        return false;
    }

    return true;
}

bool ArrowAnimation::advanceTrails()
{
    bool anyActive = false;

    const double nowMs = juce::Time::getMillisecondCounterHiRes();

    for (auto entry = trails.begin(); entry != trails.end(); )
    {
        Trail& trail = entry->second;

        if (! trail.active) {
            ++entry;
            continue;
        }

        const double normalised = (nowMs - trail.startMs) / static_cast<double>(trail.durationMs);

        if (normalised >= 1.0) {
            if (trail.oneShot) {
                entry = trails.erase(entry);
                continue;
            }

            trail.t      = 1.0f;
            trail.active = false;
        }
        else {
            trail.t   = static_cast<float>(juce::jlimit(0.0, 1.0, normalised));
            anyActive = true;
        }

        ++entry;
    }

    return anyActive;
}

bool ArrowAnimation::advanceHover()
{
    if (alphaTarget < alpha && ! snapSettled()) {
        return true;
    }

    if (std::abs(alpha - alphaTarget) < hoverFadeEpsilon) {
        alpha = alphaTarget;
        return false;
    }

    float step = -hoverFadeStep;

    if (alpha < alphaTarget) {
        step = hoverFadeStep;
    }

    alpha = juce::jlimit(0.0f, 1.0f, alpha + step);
    return true;
}
