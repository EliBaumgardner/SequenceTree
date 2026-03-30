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

    static void createRootNode(const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree rootNode = ValueTreeState::addRootNode(nodePosition,undoManager);
    }

    static void createRootNode(const Node& parentNode, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        int parentNodeId = parentNode.nodeID;

        juce::ValueTree rootNodeValueTree = ValueTreeState::addRootNode(parentNodeId,nodePosition,undoManager);
    }

    static void createNode(const Node& parentNode, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        int parentNodeId = parentNode.nodeID;

        juce::ValueTree childNodeValueTree = ValueTreeState::addNode(parentNodeId,nodePosition,undoManager);
    }

    static void destroyNode(const Node& node, juce::UndoManager* undoManager)
    {
        int nodeId = node.nodeID;

        ValueTreeState::removeNode(nodeId,undoManager);
    }

};

#endif //SEQUENCETREE_NODEFACTORY_H