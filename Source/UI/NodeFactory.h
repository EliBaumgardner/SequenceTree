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
        juce::ValueTree rootNodeValueTree = ValueTreeState::addRootNode(undoManager);
        int rootId = rootNodeValueTree.getProperty(ValueTreeIdentifiers::Id);


        ValueTreeState::setNodePosition(rootNodeValueTree,nodePosition,undoManager);

        setDefaultNodeNote(rootId, undoManager);
    }

    static juce::ValueTree createRootNode(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {

        juce::ValueTree rootNodeValueTree = ValueTreeState::addRootNode(parentNodeId,undoManager);
        int rootId = rootNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        ValueTreeState::setNodePosition(rootNodeValueTree,nodePosition,undoManager);

        setDefaultNodeNote(rootId, undoManager);

        return rootNodeValueTree;
    }

    static juce::ValueTree createNode(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree childNodeValueTree = ValueTreeState::addNode(parentNodeId, undoManager);
        int nodeId = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        ValueTreeState::setNodePosition(childNodeValueTree, nodePosition, undoManager);
        setDefaultNodeNote(nodeId, undoManager);

        return childNodeValueTree;
    }

    static juce::ValueTree createConnector(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree connectorValueTree = ValueTreeState::addConnector(parentNodeId, undoManager);
        ValueTreeState::setNodePosition(connectorValueTree, nodePosition, undoManager);

        return connectorValueTree;
    }

    static juce::ValueTree createModulator(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager) {

        juce::ValueTree modulatorValueTree = ValueTreeState::addModulator(parentNodeId, undoManager);
        ValueTreeState::setNodePosition(modulatorValueTree, nodePosition, undoManager);

        return modulatorValueTree;
    }

    static juce::ValueTree createModulatorRoot(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager) {
        juce::ValueTree modulatorRootValueTree = ValueTreeState::addModulatorRoot(parentNodeId, undoManager);
        ValueTreeState::setNodePosition(modulatorRootValueTree, nodePosition, undoManager);
        return modulatorRootValueTree;
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