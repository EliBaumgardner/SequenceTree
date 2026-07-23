#pragma once

#include <cmath>

namespace ArrowDuration
{
    inline constexpr float millisecondsPerPixel = 5.0f;

    inline int fromDelta(int deltaX, int deltaY, bool sourceIsAlternative)
    {
        int span = std::abs(deltaX);

        if (sourceIsAlternative) {
            span = std::abs(deltaY);
        }

        return static_cast<int>(static_cast<float>(span) * millisecondsPerPixel);
    }
}
