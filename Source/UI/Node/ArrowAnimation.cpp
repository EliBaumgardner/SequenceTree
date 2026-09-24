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

void ArrowAnimation::resumeTrails()
{
    if (! trailsPaused) {
        return;
    }

    const double pausedForMs = juce::Time::getMillisecondCounterHiRes() - pausedAtMs;

    for (auto& [trailId, trail] : trails) {
        trail.startMs += pausedForMs;
    }

    trailsPaused = false;
}

bool ArrowAnimation::advance(double frameSec)
{
    float elapsedSec = 0.0f;

    if (lastFrameSec > 0.0) {
        elapsedSec = static_cast<float>(frameSec - lastFrameSec);
    }

    lastFrameSec = frameSec;

    const bool snapRunning   = advanceSnap(elapsedSec);
    const bool trailsRunning = advanceTrails();
    const bool hoverRunning  = advanceHover(elapsedSec);

    const bool stillRunning = snapRunning || trailsRunning || hoverRunning;

    if (! stillRunning) {
        lastFrameSec = 0.0;
    }

    return stillRunning;
}

bool ArrowAnimation::advanceSnap(float elapsedSec)
{
    if (snapSettled()) {
        return false;
    }

    const float springTicks = elapsedSec * snapSpringRateHz;

    snapVelocity += (1.0f - snapT) * snapSpringStiffness * springTicks;
    snapVelocity *= std::pow(snapSpringDamping, springTicks);
    snapT        += snapVelocity * springTicks;

    if (snapSettled()) {
        snapT        = 1.0f;
        snapVelocity = 0.0f;
        return false;
    }

    return true;
}

bool ArrowAnimation::snapSettled()
{
    return std::abs(snapT - 1.0f) < snapSettledEpsilon
        && std::abs(snapVelocity) < snapSettledEpsilon;
}

bool ArrowAnimation::advanceTrails()
{
    if (trailsPaused) {
        return false;
    }

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

bool ArrowAnimation::advanceHover(float elapsedSec)
{
    if (alphaTarget < alpha && ! snapSettled()) {
        return true;
    }

    if (std::abs(alpha - alphaTarget) < hoverFadeEpsilon) {
        alpha = alphaTarget;
        return false;
    }

    float step = -hoverFadePerSecond * elapsedSec;

    if (alpha < alphaTarget) {
        step = hoverFadePerSecond * elapsedSec;
    }

    alpha = juce::jlimit(0.0f, 1.0f, alpha + step);
    return true;
}
