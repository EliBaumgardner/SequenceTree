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

    static void createRootNode(ValueTreeState& state, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree rootNodeValueTree = state.addRootNode(undoManager);
        int rootId = rootNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        state.setNodePosition(rootNodeValueTree,nodePosition,undoManager);

        setDefaultNodeNote(state, rootId, undoManager);
        setDefaultTraversal(state, rootId, undoManager);
    }

    static juce::ValueTree createNode(ValueTreeState& state, const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree childNodeValueTree = state.addNode(parentNodeId, undoManager);
        int nodeId = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        state.setNodePosition(childNodeValueTree, nodePosition, undoManager);
        inheritFromParent(state, parentNodeId, nodeId, childNodeValueTree, undoManager);

        return childNodeValueTree;
    }


    static juce::ValueTree createAlternativeNode(ValueTreeState& state, const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree childNodeValueTree = state.addAlternativeNode(parentNodeId, undoManager);
        int nodeId = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        state.setNodePosition(childNodeValueTree, nodePosition, undoManager);
        inheritFromParent(state, parentNodeId, nodeId, childNodeValueTree, undoManager);

        return childNodeValueTree;
    }

    static juce::ValueTree createTraversalFlagNode(ValueTreeState& state, const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree childNodeValueTree = state.addTraversalFlagNode(parentNodeId, undoManager);
        int nodeId = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        NodePosition traversalFlagNodePosition = nodePosition;

        state.setNodePosition(childNodeValueTree, traversalFlagNodePosition, undoManager);

        return childNodeValueTree;
    }

    static juce::ValueTree createModulator(ValueTreeState& state, const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager) {

        juce::ValueTree modulatorValueTree = state.addModulator(parentNodeId, undoManager);
        state.setNodePosition(modulatorValueTree, nodePosition, undoManager);

        return modulatorValueTree;
    }

    static juce::ValueTree createModulatorRoot(ValueTreeState& state, const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager) {
        juce::ValueTree modulatorRootValueTree = state.addModulatorRoot(parentNodeId, undoManager);
        state.setNodePosition(modulatorRootValueTree, nodePosition, undoManager);
        return modulatorRootValueTree;
    }

    static void destroyNode(ValueTreeState& state, const int nodeId, juce::UndoManager* undoManager)
    {
        state.removeNode(nodeId,undoManager);
    }

private:

    static void setDefaultTraversal(ValueTreeState& state, int nodeId, juce::UndoManager *undoManager)
    {
        juce::ValueTree rootNodeValueTree = state.getNode(nodeId);

        if (state.traversalMap.getNumChildren() == 0) {
            state.createTraversalData(defaultTraversalId, undoManager);
        }

        juce::ValueTree traversalId = juce::ValueTree{ValueTreeIdentifiers::TraversalId};
        traversalId.setProperty(ValueTreeIdentifiers::TraversalId, defaultTraversalId, undoManager);

        juce::ValueTree traversalChildrenIds = rootNodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);
        traversalChildrenIds.addChild(traversalId, -1, undoManager);
    }

    static void setDefaultNodeNote(ValueTreeState& state, int nodeId, juce::UndoManager *undoManager)
    {
        NodeNote note;
        note.pitch       = 60;
        note.velocity    = 60;
        note.duration    = 1000;
        note.midiChannel = ValueTreeState::defaultMidiChannel;

        state.setMidiValue(nodeId, note, undoManager);
    }

    static void inheritFromParent(ValueTreeState& state, int parentNodeId, int newNodeId, juce::ValueTree newNode, juce::UndoManager* undoManager)
    {
        juce::ValueTree parentNode = state.getNode(parentNodeId);

        if (!parentNode.isValid()
            || (parentNode.getType() != ValueTreeIdentifiers::NodeData
                && parentNode.getType() != ValueTreeIdentifiers::AlternativeNodeData
                && parentNode.getType() != ValueTreeIdentifiers::RootNodeData))
        {
            setDefaultNodeNote(state, newNodeId, undoManager);
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
            setDefaultNodeNote(state, newNodeId, undoManager);
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