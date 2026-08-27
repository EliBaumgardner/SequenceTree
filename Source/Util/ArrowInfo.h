//
// Created by Eli Baumgardner on 8/26/26.
//

#ifndef SEQUENCETREE_ARROWINFO_H
#define SEQUENCETREE_ARROWINFO_H

#include <algorithm>
#include <cmath>

enum class ArrowType    { Node = 0, Polyphonic = 1, Traversal = 2 };
enum class ArrowBinding { NoBind = 0, PitchBind = 1, DurationBind = 2 };

struct ArrowInfo {
    ArrowType    type        = ArrowType::Node;
    ArrowBinding xBinding    = ArrowBinding::DurationBind;
    ArrowBinding yBinding    = ArrowBinding::NoBind;
    double       xMultiplier = 1.0;
    double       yMultiplier = 1.0;
};

inline constexpr float arrowMillisecondsPerPixel = 5.0f;
inline constexpr float arrowSemitonesPerPixel    = 0.25f;

inline constexpr int arrowDefaultBasePitch = 60;
inline constexpr int arrowMinimumPitch = 0;
inline constexpr int arrowMaximumPitch = 127;

inline constexpr double arrowMinimumMultiplier = 0.1;
inline constexpr double arrowMaximumMultiplier = 100.0;

inline ArrowInfo defaultArrowInfo(bool sourceIsAlternative)
{
    if (sourceIsAlternative) {
        return { ArrowType::Node, ArrowBinding::NoBind, ArrowBinding::DurationBind, 1.0, 1.0 };
    }

    return {};
}

inline bool arrowHasCustomBindings(const ArrowInfo& info)
{
    return info.xBinding    != ArrowBinding::DurationBind
        || info.yBinding    != ArrowBinding::NoBind
        || info.xMultiplier != 1.0
        || info.yMultiplier != 1.0;
}

inline bool arrowBindsTo(const ArrowInfo& info, ArrowBinding binding)
{
    return info.xBinding == binding || info.yBinding == binding;
}

inline int arrowDurationFromDelta(const ArrowInfo& info, int deltaX, int deltaY)
{
    double span = 0.0;

    if (info.xBinding == ArrowBinding::DurationBind) {
        span += std::abs(static_cast<double>(deltaX)) * info.xMultiplier;
    }

    if (info.yBinding == ArrowBinding::DurationBind) {
        span += std::abs(static_cast<double>(deltaY)) * info.yMultiplier;
    }

    return static_cast<int>(span * arrowMillisecondsPerPixel);
}

inline int arrowPitchFromDelta(const ArrowInfo& info, int basePitch, int deltaX, int deltaY)
{
    double span = 0.0;

    if (info.xBinding == ArrowBinding::PitchBind) {
        span += static_cast<double>(deltaX) * info.xMultiplier;
    }

    if (info.yBinding == ArrowBinding::PitchBind) {
        span -= static_cast<double>(deltaY) * info.yMultiplier;
    }

    const int semitones = static_cast<int>(std::round(span * arrowSemitonesPerPixel));

    return std::clamp(basePitch + semitones, arrowMinimumPitch, arrowMaximumPitch);
}

#endif //SEQUENCETREE_ARROWINFO_H
