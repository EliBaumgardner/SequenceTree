//
// Created by Eli Baumgardner on 3/27/26.
//

#ifndef SEQUENCETREE_NODEFACTORY_H
#define SEQUENCETREE_NODEFACTORY_H

#pragma once

#include "../Util/ValueTreeIdentifiers.h""
#include "../Util/PluginContext.h"
#include "../Util/ValueTreeState.h"

class NodeFactory
{
public:

    static void createRootNode(const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree rootNodeValueTree = ValueTreeState::addRootNode(nodePosition,undoManager);
        int rootId = rootNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        ValueTreeState::setNodePosition(rootNodeValueTree,nodePosition,undoManager);

        setDefaultNodeNote(rootId, undoManager);
    }

    static void createRootNode(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree rootNodeValueTree = ValueTreeState::addRootNode(parentNodeId,nodePosition,undoManager);
        int rootId = rootNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        ValueTreeState::setNodePosition(rootNodeValueTree,nodePosition,undoManager);

        setDefaultNodeNote(rootId, undoManager);
    }

    static juce::ValueTree createNode(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree childNodeValueTree = ValueTreeState::addNode(parentNodeId,nodePosition,undoManager);
        int nodeId = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        setDefaultNodeNote(nodeId, undoManager);

        return childNodeValueTree;
    }

    static void destroyNode(const int nodeId, juce::UndoManager* undoManager)
    {
        ValueTreeState::removeNode(nodeId,undoManager);
    }

private:

    static void setDefaultNodeNote(int nodeId, juce::UndoManager *undoManager)
    {
        NodeNote note;
        note.pitch = 60;
        note.velocity = 60;
        note.duration = 1000;

        ValueTreeState::setMidiValue(nodeId,note,undoManager);
    }
};

#endif //SEQUENCETREE_NODEFACTORY_H