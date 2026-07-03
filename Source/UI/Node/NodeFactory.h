//
// Created by Eli Baumgardner on 3/27/26.
//

#ifndef SEQUENCETREE_NODEFACTORY_H
#define SEQUENCETREE_NODEFACTORY_H

#pragma once

#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/ValueTreeState.h"

static const int defaultTraversalId = 1;

class NodeFactory
{
public:

    static void createRootNode(const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree rootNodeValueTree = ValueTreeState::addRootNode(undoManager);
        int rootId = rootNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        ValueTreeState::setNodePosition(rootNodeValueTree,nodePosition,undoManager);

        setDefaultNodeNote(rootId, undoManager);
        setDefaultTraversal(rootId, undoManager);
    }

    static juce::ValueTree createNode(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree childNodeValueTree = ValueTreeState::addNode(parentNodeId, undoManager);
        int nodeId = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        ValueTreeState::setNodePosition(childNodeValueTree, nodePosition, undoManager);
        inheritFromParent(parentNodeId, nodeId, childNodeValueTree, undoManager);

        return childNodeValueTree;
    }


    static juce::ValueTree createAlternativeNode(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree childNodeValueTree = ValueTreeState::addAlternativeNode(parentNodeId, undoManager);
        int nodeId = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        ValueTreeState::setNodePosition(childNodeValueTree, nodePosition, undoManager);
        inheritFromParent(parentNodeId, nodeId, childNodeValueTree, undoManager);

        return childNodeValueTree;
    }

    static juce::ValueTree createTraversalFlagNode(const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree childNodeValueTree = ValueTreeState::addTraversalFlagNode(parentNodeId, undoManager);
        int nodeId = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        NodePosition traversalFlagNodePosition = nodePosition;

        ValueTreeState::setNodePosition(childNodeValueTree, traversalFlagNodePosition, undoManager);

        return childNodeValueTree;
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

    static void setDefaultTraversal(int nodeId, juce::UndoManager *undoManager)
    {
        juce::ValueTree rootNodeValueTree = ValueTreeState::getNode(nodeId);

        if (ValueTreeState::traversalMap.getNumChildren() == 0) {
            ValueTreeState::createTraversalData(defaultTraversalId, undoManager);
        }

        juce::ValueTree traversalId = juce::ValueTree{ValueTreeIdentifiers::TraversalId};
        traversalId.setProperty(ValueTreeIdentifiers::TraversalId, defaultTraversalId, undoManager);

        juce::ValueTree traversalChildrenIds = rootNodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);
        traversalChildrenIds.addChild(traversalId, -1, undoManager);
    }

    static void setDefaultNodeNote(int nodeId, juce::UndoManager *undoManager)
    {
        NodeNote note;
        note.pitch       = 60;
        note.velocity    = 60;
        note.duration    = 1000;
        note.midiChannel = ValueTreeState::defaultMidiChannel;

        ValueTreeState::setMidiValue(nodeId, note, undoManager);
    }

    static void inheritFromParent(int parentNodeId, int newNodeId, juce::ValueTree newNode, juce::UndoManager* undoManager)
    {
        juce::ValueTree parentNode = ValueTreeState::getNode(parentNodeId);

        if (!parentNode.isValid()
            || (parentNode.getType() != ValueTreeIdentifiers::NodeData
                && parentNode.getType() != ValueTreeIdentifiers::AlternativeNodeData
                && parentNode.getType() != ValueTreeIdentifiers::RootNodeData))
        {
            setDefaultNodeNote(newNodeId, undoManager);
            return;
        }

        const juce::Identifier propsToCopy[] = {
            ValueTreeIdentifiers::CountLimit,
            ValueTreeIdentifiers::SwitchCountLimit,
            ValueTreeIdentifiers::SubLoopCountLimit,
            ValueTreeIdentifiers::RepeatValue
        };

        for (const auto& prop : propsToCopy)
            if (parentNode.hasProperty(prop)) {
                newNode.setProperty(prop, parentNode.getProperty(prop), undoManager);
            }

        juce::ValueTree parentMidi = parentNode.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
        juce::ValueTree newMidi    = newNode.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

        if (!parentMidi.isValid() || parentMidi.getNumChildren() == 0)
        {
            setDefaultNodeNote(newNodeId, undoManager);
            return;
        }

        for (int i = 0; i < parentMidi.getNumChildren(); ++i)
        {
            juce::ValueTree parentNote = parentMidi.getChild(i);
            juce::ValueTree clonedNote = parentNote.createCopy();
            newMidi.addChild(clonedNote, -1, undoManager);
        }
    }
};

#endif //SEQUENCETREE_NODEFACTORY_H