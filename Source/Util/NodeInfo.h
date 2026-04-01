//
// Created by Eli Baumgardner on 3/27/26.
//

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
};

enum class NodeType { Node, Connector, Root};

enum class NodeDisplayMode {Pitch, Velocity,Duration,CountLimit};

#endif //SEQUENCETREE_NODEPOSITION_H