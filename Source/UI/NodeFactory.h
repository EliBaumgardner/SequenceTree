//
// Created by Eli Baumgardner on 3/27/26.
//

#ifndef SEQUENCETREE_NODEFACTORY_H
#define SEQUENCETREE_NODEFACTORY_H

#pragma once



#include "Node/Node.h"
#include "../Util/PluginContext.h"
#include "../Util/ValueTreeState.h"

class NodeFactory
{
public:

    struct NodePosition {
        int xPosition;
        int yPosition;
        int radius;
    };


    static void createNode(const Node& parentNode, NodePosition nodePosition, juce::UndoManager* undoManager) {

        int parentNodeId = parentNode.nodeID;

        int xPosition = nodePosition.xPosition;
        int yPosition = nodePosition.yPosition;
        int radius = nodePosition.radius;

        ValueTreeState& valueTreeState = *ComponentContext::valueTreeState;
        juce::ValueTree parentNodeValueTree = valueTreeState.getNode(parentNodeId);

        juce::ValueTree childNodeValueTree = ValueTreeState::addNode(parentNodeId,undoManager);
        ValueTreeState::setNodePosition(childNodeValueTree, );

        childNodeValueTree.setProperty(ValueTreeState::XPosition,xPosition, undoManager);
        childNodeValueTree.setProperty(ValueTreeState::YPosition, yPosition, undoManager);
        childNodeValueTree.setProperty(ValueTreeState::Radius, radius, undoManager);

    }
};

#endif //SEQUENCETREE_NODEFACTORY_H