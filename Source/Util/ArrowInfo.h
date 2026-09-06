//
// Created by Eli Baumgardner on 8/26/26.
//

#ifndef SEQUENCETREE_ARROWINFO_H
#define SEQUENCETREE_ARROWINFO_H

#include <algorithm>
#include <cmath>

enum class ArrowType    { Node   = 0, Polyphonic = 1, Traversal    = 2 };
enum class ArrowBinding { NoBind = 0, PitchBind  = 1, DurationBind = 2 };

struct ArrowInfo {
    ArrowType    type        = ArrowType::Node;
    ArrowBinding xBinding    = ArrowBinding::DurationBind;
    ArrowBinding yBinding    = ArrowBinding::PitchBind;
    double       xMultiplier = 1.0;
    double       yMultiplier = 1.0;

    static constexpr float  pixelsPerGridSpace       = 50.0f;
    static constexpr double millisecondsPerGridSpace = 250.0;
    static constexpr double semitonesPerGridSpace    = 1.0;
    static constexpr double maximumDurationMs        = 3600000.0;
    static constexpr double maximumSemitoneOffset    = 127.0;

    static bool bindsTo(const ArrowInfo& info, ArrowBinding binding)
    {
        return info.xBinding == binding || info.yBinding == binding;
    }

    static int durationFromDelta(const ArrowInfo& info, int deltaX, int deltaY)
    {
        double gridSpaces = 0.0;

        if (info.xBinding == ArrowBinding::DurationBind) {
            gridSpaces += std::abs(static_cast<double>(deltaX)) * info.xMultiplier / pixelsPerGridSpace;
        }

        if (info.yBinding == ArrowBinding::DurationBind) {
            gridSpaces += std::abs(static_cast<double>(deltaY)) * info.yMultiplier / pixelsPerGridSpace;
        }

        return static_cast<int>(std::min(gridSpaces * millisecondsPerGridSpace, maximumDurationMs));
    }

    static int pitchOffsetFromDelta(const ArrowInfo& info, int deltaX, int deltaY)
    {
        double gridSpaces = 0.0;

        if (info.xBinding == ArrowBinding::PitchBind) {
            gridSpaces += static_cast<double>(deltaX) * info.xMultiplier / pixelsPerGridSpace;
        }

        if (info.yBinding == ArrowBinding::PitchBind) {
            gridSpaces -= static_cast<double>(deltaY) * info.yMultiplier / pixelsPerGridSpace;
        }

        const double semitones = std::clamp(gridSpaces * semitonesPerGridSpace,
                                            -maximumSemitoneOffset, maximumSemitoneOffset);

        return static_cast<int>(std::round(semitones));
    }
};

#endif //SEQUENCETREE_ARROWINFO_H
