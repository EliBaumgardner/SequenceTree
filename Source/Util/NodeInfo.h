#ifndef SEQUENCETREE_NODEPOSITION_H
#define SEQUENCETREE_NODEPOSITION_H

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

inline constexpr int defaultMidiPitch = 60;
inline constexpr int minimumMidiPitch = 0;
inline constexpr int maximumMidiPitch = 127;

enum class NodeType { Node, Root, Modulator, TraversalFlag};

enum class NodeDisplayMode {Pitch, Velocity, Duration, CountLimit, Channel, RepeatValue};

#endif //SEQUENCETREE_NODEPOSITION_H
