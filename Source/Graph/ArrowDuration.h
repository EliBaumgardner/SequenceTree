#pragma once

#include <cmath>

namespace ArrowDuration
{
    inline constexpr float millisecondsPerPixel = 5.0f;

    inline int fromDelta(int deltaX, int deltaY, bool sourceIsAlternative)
    {
        const int span = sourceIsAlternative ? std::abs(deltaY) : std::abs(deltaX);
        return static_cast<int>(static_cast<float>(span) * millisecondsPerPixel);
    }
}
