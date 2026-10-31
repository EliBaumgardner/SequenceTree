//
// Created by Eli Baumgardner on 8/26/26.
//

#ifndef SEQUENCETREE_ARROWINFO_H
#define SEQUENCETREE_ARROWINFO_H

#include <algorithm>
#include <cmath>

/*
    The settings of one arrow as a plain value (Settings): its Type, what its horizontal (x) and
    vertical (y) extent is bound to (a Binding: pitch, duration or nothing), a multiplier for each
    axis, an isSynced flag (TraversalDispatcher and TraversalLogic read it on the arrow into a
    modulator's root), and an optional fixed duration that replaces the one its length would give.

    The arrow's length is how its duration and pitch are chosen. durationFromDelta turns the
    pixel offset from parent to child into milliseconds: every axis bound to duration adds its
    distance in grid spaces times its multiplier, at 250 ms per grid space and capped at an hour,
    unless durationOverride is set. pitchOffsetFromDelta turns the same offset into a whole
    number of semitones from every axis bound to pitch, where moving up raises the pitch.
    bindsTo asks whether either axis uses a binding.

    ArrowBindingOps stores it in an arrow's ValueTree and reads it back. BottomBar edits the copy
    in GraphDocument::currentArrowInfo that new arrows start from, and Arrow, RTGraphBuilder and
    Clipboard call the two conversions.
*/

namespace ArrowInfo
{

enum class Type    { Node   = 0, Polyphonic = 1, Traversal    = 2, CrossRootTree = 3, StepIntoTree = 4 };
enum class Binding { NoBind = 0, PitchBind  = 1, DurationBind = 2 };

inline constexpr int    noDurationOverride             = -1;
inline constexpr float  pixelsPerGridSpace             = 50.0f;
inline constexpr double millisecondsPerGridSpace       = 250.0;
inline constexpr double semitonesPerGridSpace          = 1.0;
inline constexpr double maximumDurationMs              = 3600000.0;
inline constexpr int    millisecondsPerDurationPercent = 10;
inline constexpr int    maximumDurationPercent         = 100;
inline constexpr double maximumSemitoneOffset          = 127.0;

struct Settings {
    Type    type        = Type::Node;
    Binding xBinding    = Binding::DurationBind;
    Binding yBinding    = Binding::PitchBind;

    double  xMultiplier = 1.0;
    double  yMultiplier = 1.0;
    bool    isSynced    = true;

    int durationOverride = noDurationOverride;

    static bool bindsTo(const Settings& info, Binding binding)
    {
        return info.xBinding == binding || info.yBinding == binding;
    }

    static int durationFromDelta(const Settings& info, int deltaX, int deltaY)
    {
        if (info.durationOverride != noDurationOverride) {
            return info.durationOverride;
        }

        double gridSpaces = 0.0;

        if (info.xBinding == Binding::DurationBind) {
            gridSpaces += std::abs(static_cast<double>(deltaX)) * info.xMultiplier / pixelsPerGridSpace;
        }

        if (info.yBinding == Binding::DurationBind) {
            gridSpaces += std::abs(static_cast<double>(deltaY)) * info.yMultiplier / pixelsPerGridSpace;
        }

        return static_cast<int>(std::min(gridSpaces * millisecondsPerGridSpace, maximumDurationMs));
    }

    static int pitchOffsetFromDelta(const Settings& info, int deltaX, int deltaY)
    {
        double semitones = 0.0;

        if (info.xBinding == Binding::PitchBind) {
            const double wholeGridSpaces = std::round(static_cast<double>(deltaX) / pixelsPerGridSpace);

            semitones += wholeGridSpaces * semitonesPerGridSpace * info.xMultiplier;
        }

        if (info.yBinding == Binding::PitchBind) {
            const double wholeGridSpaces = std::round(static_cast<double>(deltaY) / pixelsPerGridSpace);

            semitones -= wholeGridSpaces * semitonesPerGridSpace * info.yMultiplier;
        }

        return static_cast<int>(std::round(std::clamp(semitones,
                                                     -maximumSemitoneOffset, maximumSemitoneOffset)));
    }
};

}

#endif //SEQUENCETREE_ARROWINFO_H
