#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>

class ArrowProgress
{
public:

    struct Track
    {
        float        t          = 0.0f;
        double       startMs    = 0.0;
        int          durationMs = 1;
        juce::Colour colour     { juce::Colours::white };
        bool         active     = false;
    };

    void start(int traversalId, int durationMs, juce::Colour colour)
    {
        Track& track = tracks[traversalId];
        track.t          = 0.0f;
        track.startMs    = juce::Time::getMillisecondCounterHiRes();
        track.durationMs = juce::jmax(1, durationMs);
        track.colour     = colour;
        track.active     = durationMs > 0;
    }

    void reset()
    {
        tracks.clear();
    }

    void reset(int traversalId)
    {
        tracks.erase(traversalId);
    }

    bool advance()
    {
        bool anyActive = false;
        const double nowMs = juce::Time::getMillisecondCounterHiRes();

        for (auto& entry : tracks)
        {
            Track& track = entry.second;

            if (! track.active) {
                continue;
            }

            const double normalised = (nowMs - track.startMs) / static_cast<double>(track.durationMs);

            if (normalised >= 1.0) {
                track.t      = 1.0f;
                track.active = false;
            }
            else {
                track.t   = static_cast<float>(juce::jlimit(0.0, 1.0, normalised));
                anyActive = true;
            }
        }

        return anyActive;
    }

    bool hasTracks() const { return ! tracks.empty(); }

    std::map<int, Track> tracks;
};
