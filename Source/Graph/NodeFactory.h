#pragma once

#include <juce_graphics/juce_graphics.h>
#include "GraphState.h"
#include "../Util/ArrowInfo.h"

#include <span>

/*
    The functions that create nodes and arrows with their starting values. GraphState adds a node
    or a connection; NodeFactory calls it and then sets everything a new one begins with, so every
    gesture, paste and test creates nodes and arrows the same way.

    A new root gets its position, a default note and the default traversal. A new child gets its
    position and copies its parent's count limits, repeat value and notes. create chooses the
    create function from the NodeCreationMode, the parent's node type and whether the new node is
    an alternative. applyArrowInfo sets a newly connected arrow's ArrowInfo::Settings from the toolbar, with
    its type chosen from the nodes it joins: a traversal arrow only between two trees. It and
    createDanglingArrow both copy the bindings of an arrow already leaving the same parent.
    setDanglingArrowTip moves a dangling arrow's tip, clears its duration override and reapplies
    pitch bindings, and destroyDanglingArrow removes one.

    NodeGestures, ArrowGestures, EncapsulationGestures, ArrowManager and Clipboard call it inside
    their own undo transactions, and the tests build graphs with it. It only writes GraphState; the
    canvas and RTGraphBuilder react to the changes.
*/

enum class NodeCreationMode { Node, Modulator, TraversalFlag };

class NodeFactory
{
public:

    static void createRootNode(GraphState& state, const NodeInfo::Position& nodePosition, juce::UndoManager* undoManager);

    static void createDanglingArrow (GraphState& state, juce::ValueTree nodeTree, const juce::Point<int>& tipOffset, ArrowInfo::Settings arrowInfo,
                                     juce::UndoManager* undoManager);
    static void setDanglingArrowTip (GraphState& state, juce::ValueTree arrowTree, const juce::Point<int>& tipOffset, juce::UndoManager* undoManager);
    static void destroyDanglingArrow(juce::ValueTree arrowTree, juce::UndoManager* undoManager);

    static void applyArrowInfo(GraphState& state, int parentNodeId, int childNodeId, ArrowInfo::Settings arrowInfo, ArrowInfo::Type rootConnectionType,
                               juce::UndoManager* undoManager);

    static juce::ValueTree create(NodeCreationMode mode, GraphState& state, int parentNodeId, const juce::Identifier& parentType, bool makeAlternative,
                                  const NodeInfo::Position& position, juce::UndoManager* undoManager);

    static juce::ValueTree createNode             (GraphState& state, int parentNodeId, const juce::Identifier& nodeType, const NodeInfo::Position& nodePosition,
                                                   juce::UndoManager* undoManager);
    static juce::ValueTree createTraversalFlagNode(GraphState& state, int parentNodeId, const NodeInfo::Position& nodePosition, juce::UndoManager* undoManager);
    static juce::ValueTree createEncapsulator     (GraphState& state, std::span<const int> memberNodeIds, juce::UndoManager* undoManager);

    static juce::ValueTree createModulatorRoot       (GraphState& state, int parentNodeId, const NodeInfo::Position& nodePosition, juce::UndoManager* undoManager);
    static juce::ValueTree createModulator           (GraphState& state, int parentNodeId, const NodeInfo::Position& nodePosition, juce::UndoManager* undoManager);
    static juce::ValueTree createAlternativeModulator(GraphState& state, int parentNodeId, const NodeInfo::Position& nodePosition, juce::UndoManager* undoManager);

private:

    static void setDefaultNodeNote (GraphState& state, int nodeId, juce::UndoManager* undoManager);
    static void setDefaultTraversal(GraphState& state, int nodeId, juce::UndoManager* undoManager);

    static void inheritFromParent(GraphState& state, int parentNodeId, int newNodeId, juce::ValueTree newNode, juce::UndoManager* undoManager);

    static void inheritArrowBindings(const GraphState& state, ArrowInfo::Settings& arrowInfo, int nodeId, const juce::ValueTree& newArrow);
};
