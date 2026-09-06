#ifndef SEQUENCETREE_NODEFACTORY_H
#define SEQUENCETREE_NODEFACTORY_H

#pragma once

#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/GraphState.h"
#include "../../Util/ArrowInfo.h"

class NodeFactory
{
public:

    static void createRootNode(GraphState& state, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        juce::ValueTree rootNodeValueTree = state.addRootNode(undoManager);
        const int rootId = rootNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        GraphState::setNodePosition(rootNodeValueTree, nodePosition, undoManager);

        setDefaultNodeNote(state, rootId, undoManager);
        setDefaultTraversal(state, rootId, undoManager);
    }

    static juce::ValueTree createNode(GraphState& state, const int parentNodeId, const juce::Identifier& nodeType,
                                      const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        const juce::ValueTree childNodeValueTree = state.addChildNode(parentNodeId, nodeType, undoManager);
        const int nodeId = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        GraphState::setNodePosition(childNodeValueTree, nodePosition, undoManager);
        inheritFromParent(state, parentNodeId, nodeId, childNodeValueTree, undoManager);

        return childNodeValueTree;
    }

    static juce::ValueTree createTraversalFlagNode(GraphState& state, const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager)
    {
        const juce::ValueTree childNodeValueTree = state.addTraversalFlagNode(parentNodeId, undoManager);

        GraphState::setNodePosition(childNodeValueTree, nodePosition, undoManager);

        return childNodeValueTree;
    }

    static juce::ValueTree createModulator(GraphState& state, const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager) {

        const juce::ValueTree modulatorValueTree = state.addModulator(parentNodeId, undoManager);
        GraphState::setNodePosition(modulatorValueTree, nodePosition, undoManager);

        return modulatorValueTree;
    }

    static juce::ValueTree createModulatorRoot(GraphState& state, const int parentNodeId, const NodePosition& nodePosition, juce::UndoManager* undoManager) {
        const juce::ValueTree modulatorRootValueTree = state.addModulatorRoot(parentNodeId, undoManager);
        GraphState::setNodePosition(modulatorRootValueTree, nodePosition, undoManager);
        return modulatorRootValueTree;
    }

    static void createDanglingArrow(GraphState& state, juce::ValueTree nodeTree,
                                    const juce::Point<int>& tipOffset,
                                    const ArrowInfo& arrowInfo, juce::UndoManager* undoManager)
    {
        if (!nodeTree.isValid()) {
            return;
        }

        juce::ValueTree arrowList = nodeTree.getChildWithName(ValueTreeIdentifiers::DanglingArrows);

        if (!arrowList.isValid()) {
            arrowList = juce::ValueTree(ValueTreeIdentifiers::DanglingArrows);
            nodeTree.addChild(arrowList, -1, undoManager);
        }

        juce::ValueTree arrowTree(ValueTreeIdentifiers::DanglingArrow);
        arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipX, tipOffset.x, undoManager);
        arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipY, tipOffset.y, undoManager);
        arrowTree.setProperty(ValueTreeIdentifiers::CountLimit, GraphState::defaultNodeCountLimit, undoManager);

        GraphState::setArrowInfo(arrowTree, arrowInfo, undoManager);

        arrowList.addChild(arrowTree, -1, undoManager);
    }

    static void destroyDanglingArrow(juce::ValueTree arrowTree, juce::UndoManager* undoManager)
    {
        juce::ValueTree arrowList = arrowTree.getParent();

        if (arrowList.isValid()) {
            arrowList.removeChild(arrowTree, undoManager);
        }
    }

    static void setDanglingArrowTip(juce::ValueTree arrowTree, const juce::Point<int>& tipOffset,
                                    juce::UndoManager* undoManager)
    {
        if (!arrowTree.isValid()) {
            return;
        }

        arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipX, tipOffset.x, undoManager);
        arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipY, tipOffset.y, undoManager);
    }

private:

    static void setDefaultTraversal(GraphState& state, int nodeId, juce::UndoManager *undoManager)
    {
        const juce::ValueTree rootNodeValueTree = state.getNode(nodeId);

        state.addTraversalData(GraphState::defaultTraversalId, undoManager);

        juce::ValueTree traversalId = juce::ValueTree{ValueTreeIdentifiers::TraversalId};
        traversalId.setProperty(ValueTreeIdentifiers::TraversalId, GraphState::defaultTraversalId, undoManager);

        juce::ValueTree traversalChildrenIds = rootNodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);
        traversalChildrenIds.addChild(traversalId, -1, undoManager);
    }

    static void setDefaultNodeNote(GraphState& state, int nodeId, juce::UndoManager *undoManager)
    {
        NodeNote note;
        note.pitch       = 60;
        note.velocity    = 60;
        note.duration    = 1000;
        note.midiChannel = GraphState::defaultMidiChannel;

        state.addMidiNote(nodeId, note, undoManager);
    }

    static void inheritFromParent(GraphState& state, int parentNodeId, int newNodeId, juce::ValueTree newNode, juce::UndoManager* undoManager)
    {
        const juce::ValueTree parentNode = state.getNode(parentNodeId);

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
            ValueTreeIdentifiers::RepeatValue,
            ValueTreeIdentifiers::Probability
        };

        for (const auto& prop : propsToCopy) {
            if (parentNode.hasProperty(prop)) {
                newNode.setProperty(prop, parentNode.getProperty(prop), undoManager);
            }
        }

        const juce::ValueTree parentMidi = parentNode.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
        juce::ValueTree newMidi    = newNode.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

        if (!parentMidi.isValid() || parentMidi.getNumChildren() == 0)
        {
            setDefaultNodeNote(state, newNodeId, undoManager);
            return;
        }

        for (int i = 0; i < parentMidi.getNumChildren(); ++i)
        {
            const juce::ValueTree parentNote = parentMidi.getChild(i);
            const juce::ValueTree clonedNote = parentNote.createCopy();
            newMidi.addChild(clonedNote, -1, undoManager);
        }
    }
};

#endif //SEQUENCETREE_NODEFACTORY_H
