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

inline constexpr int minimumMidiVelocity = 1;
inline constexpr int maximumMidiVelocity = 127;

inline constexpr int minimumMidiChannel = 1;
inline constexpr int maximumMidiChannel = 16;

inline constexpr int minimumProbability = 0;
inline constexpr int maximumProbability = 100;

inline constexpr int minimumCountLimit = 1;
inline constexpr int maximumCountLimit = 9999;

inline constexpr int minimumSubLoopLimit = 0;
inline constexpr int maximumSubLoopLimit = 9999;

inline constexpr int minimumRepeatValue = 1;
inline constexpr int maximumRepeatValue = 9999;

inline constexpr double minimumTraversalMultiplier  = 0.1;
inline constexpr int    traversalMultiplierDecimals = 3;

inline constexpr int minimumTraversalTranspose = -24;
inline constexpr int maximumTraversalTranspose =  24;

enum class NodeType        { Node, Root, Modulator, TraversalFlag, Encapsulator};
enum class NodeDisplayMode {Pitch, Velocity, Duration, CountLimit, Channel, RepeatValue, Probability};

#endif //SEQUENCETREE_NODEPOSITION_H
