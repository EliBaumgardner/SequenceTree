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
        juce::ValueTree rootNode = ValueTreeState::addRootNode(undoManager);
        ValueTreeState::setNodePosition(rootNode,nodePosition,undoManager);
    }

    static void createRootNode(const Node& parentNode, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        int parentNodeId = parentNode.nodeID;

        juce::ValueTree parentNodeValueTree = ValueTreeState::getNode(parentNodeId);
        juce::ValueTree rootNodeValueTree = ValueTreeState::addRootNode(parentNodeValueTree,undoManager);

        ValueTreeState::setNodePosition(parentNodeValueTree,nodePosition,undoManager);
    }

    static void createNode(const Node& parentNode, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        int parentNodeId = parentNode.nodeID;

        juce::ValueTree parentNodeValueTree = ValueTreeState::getNode(parentNodeId);
        juce::ValueTree childNodeValueTree = ValueTreeState::addNode(parentNodeValueTree,undoManager);
        ValueTreeState::setNodePosition(childNodeValueTree,nodePosition,undoManager);
    }

    static void destroyNode(const Node& node, juce::UndoManager* undoManager)
    {
        juce::ValueTree nodeValueTree = ValueTreeState::getNode(node.nodeID);
        ValueTreeState::removeNode(nodeValueTree,undoManager);
    }

};

#endif //SEQUENCETREE_NODEFACTORY_H