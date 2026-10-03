#include "ArrowAnimation.h"

void ArrowAnimation::startTrail(int trailId, int durationMs, int elapsedMs, juce::Colour colour, bool oneShot,
                                TrailSource source)
{
    Trail& trail = trails[trailId];

    double originMs = juce::Time::getMillisecondCounterHiRes();

    if (trailsPaused && source == TrailSource::Live) {
        originMs = pausedAtMs;
    }

    trail.durationMs = juce::jmax(1, durationMs);
    trail.startMs    = originMs - elapsedMs;
    trail.progress   = static_cast<float>(juce::jlimit(0.0, 1.0, elapsedMs / static_cast<double>(trail.durationMs)));
    trail.colour     = colour;
    trail.active     = durationMs > 0;
    trail.oneShot    = oneShot;
    trail.source     = source;
}

void ArrowAnimation::resumeTrails()
{
    if (! trailsPaused) {
        return;
    }

    const double pausedForMs = juce::Time::getMillisecondCounterHiRes() - pausedAtMs;

    for (auto& [trailId, trail] : trails) {
        if (trail.source == TrailSource::Live) {
            trail.startMs += pausedForMs;
        }
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

    snapVelocity += (1.0f - snapProgress) * snapSpringStiffness * springTicks;
    snapVelocity *= std::pow(snapSpringDamping, springTicks);
    snapProgress += snapVelocity * springTicks;

    if (snapSettled()) {
        snapProgress = 1.0f;
        snapVelocity = 0.0f;
        return false;
    }

    return true;
}

bool ArrowAnimation::snapSettled()
{
    return std::abs(snapProgress - 1.0f) < snapSettledEpsilon
        && std::abs(snapVelocity) < snapSettledEpsilon;
}

bool ArrowAnimation::advanceTrails()
{
    const double nowMs     = juce::Time::getMillisecondCounterHiRes();
    bool         anyActive = false;

    for (auto entry = trails.begin(); entry != trails.end(); ) {
        Trail& trail = entry->second;

        if (! trail.active || (trailsPaused && trail.source == TrailSource::Live)) {
            ++entry;
            continue;
        }

        const double normalised = (nowMs - trail.startMs) / static_cast<double>(trail.durationMs);

        if (normalised >= 1.0) {
            if (trail.oneShot) {
                entry = trails.erase(entry);
                continue;
            }

            trail.progress = 1.0f;
            trail.active   = false;
        }
        else {
            trail.progress = static_cast<float>(juce::jlimit(0.0, 1.0, normalised));
            anyActive      = true;
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
