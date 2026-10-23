#ifndef SEQUENCETREE_NODEPOSITION_H
#define SEQUENCETREE_NODEPOSITION_H

/*
    The plain values that describe a node, shared by the graph, the input layer and the UI: where
    a node sits on the canvas (Position, with its radius), one MIDI note it plays (Note), the
    lowest and highest value each editable property may hold (pitch, velocity, channel,
    probability, count limit, sub-loop limit, repeat value and the two transposes), the kinds a
    node comes in (Type) and which of its properties a node shows as its value (DisplayMode).

    GraphState reads and writes Position and Note as it stores them in a node's tree. The
    editors in the UI and the menus use the limits to clamp what the user types, and Titlebar
    sets the DisplayMode through NodeManager::setDisplayMode.
*/

namespace NodeInfo
{

struct Position {
    int xPosition;
    int yPosition;
    int radius;
};

struct Note {
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

inline constexpr double minimumTraversalMultiplier = 0.1;

inline constexpr int minimumTraversalTranspose = -24;
inline constexpr int maximumTraversalTranspose =  24;

inline constexpr int minimumGlobalTranspose = -48;
inline constexpr int maximumGlobalTranspose =  48;

enum class Type        { Node, Root, Modulator, TraversalFlag, Encapsulator};
enum class DisplayMode {Pitch, Velocity, Duration, CountLimit, Channel, RepeatValue, Probability};

}

#endif //SEQUENCETREE_NODEPOSITION_H
