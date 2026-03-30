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

enum class NodeType { Node, Connector, Root};

#endif //SEQUENCETREE_NODEPOSITION_H