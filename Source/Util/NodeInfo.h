//
// Created by Eli Baumgardner on 3/27/26.
//

#ifndef SEQUENCETREE_NODEPOSITION_H
#define SEQUENCETREE_NODEPOSITION_H

#include <cmath>

struct NodePosition {
    int xPosition;
    int yPosition;
    int radius;
};

struct NodeNote {
    int pitch;
    int velocity;
    int duration;
    int midiChannel = 1;
};

enum class ModulationType {Pitch,Velocity,Duration};
enum class NodeType { Node, Root, Modulator, TraversalFlag};
enum class ArrowType { Node = 0, Polyphonic = 1, Traversal = 2 };

enum class NodeDisplayMode {Pitch, Velocity, Duration, CountLimit, Channel, RepeatValue};

inline constexpr float arrowMillisecondsPerPixel = 5.0f;

inline int arrowDurationFromDelta(int deltaX, int deltaY, bool sourceIsAlternative)
{
    const int span = std::abs(sourceIsAlternative ? deltaY : deltaX);

    return static_cast<int>(static_cast<float>(span) * arrowMillisecondsPerPixel);
}

#endif //SEQUENCETREE_NODEPOSITION_H