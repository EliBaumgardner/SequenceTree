//
// Created by Eli Baumgardner on 8/26/26.
//

#ifndef SEQUENCETREE_ARROWINFO_H
#define SEQUENCETREE_ARROWINFO_H

#include <cmath>

enum class ArrowType    { Node   = 0, Polyphonic = 1, Traversal    = 2 };
enum class ArrowBinding { NoBind = 0, PitchBind  = 1, DurationBind = 2 };

struct ArrowInfo {
    ArrowType    type        = ArrowType::Node;
    ArrowBinding xBinding    = ArrowBinding::DurationBind;
    ArrowBinding yBinding    = ArrowBinding::NoBind;
    double       xMultiplier = 1.0;
    double       yMultiplier = 1.0;

    static constexpr float millisecondsPerPixel = 5.0f;
    static constexpr float semitonesPerPixel    = 0.25f;

    static bool bindsTo(const ArrowInfo& info, ArrowBinding binding)
    {
        return info.xBinding == binding || info.yBinding == binding;
    }

    static int durationFromDelta(const ArrowInfo& info, int deltaX, int deltaY)
    {
        double span = 0.0;

        if (info.xBinding == ArrowBinding::DurationBind) {
            span += std::abs(static_cast<double>(deltaX)) * info.xMultiplier;
        }

        if (info.yBinding == ArrowBinding::DurationBind) {
            span += std::abs(static_cast<double>(deltaY)) * info.yMultiplier;
        }

        return static_cast<int>(span * millisecondsPerPixel);
    }

    static int pitchOffsetFromDelta(const ArrowInfo& info, int deltaX, int deltaY)
    {
        double span = 0.0;

        if (info.xBinding == ArrowBinding::PitchBind) {
            span += static_cast<double>(deltaX) * info.xMultiplier;
        }

        if (info.yBinding == ArrowBinding::PitchBind) {
            span -= static_cast<double>(deltaY) * info.yMultiplier;
        }

        return static_cast<int>(std::round(span * semitonesPerPixel));
    }
};

#endif //SEQUENCETREE_ARROWINFO_H
