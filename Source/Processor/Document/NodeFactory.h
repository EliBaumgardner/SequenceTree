#pragma once

#include <juce_graphics/juce_graphics.h>
#include "GraphState.h"
#include "Core/Model/ArrowInfo.h"

#include <span>

/*
    The functions that create nodes and arrows with their starting values. GraphState adds a node
    or a connection; NodeFactory calls it and then sets everything a new one begins with, so every
    gesture, paste and test creates nodes and arrows the same way.

    A new root gets its position, a default note and the default traversal. A new child gets its
    position and copies its parent's count limits, repeat value and notes. It is built from the
    GraphState it writes and the UndoManager the writes are recorded in (nullptr records nothing),
    so no call repeats them. create chooses the create function from the NodeCreationMode, the
    parent's node kind and whether the new node is an alternative; createModulatorNode makes a
    modulator root, a modulator or an alternative modulator from its NodeInfo::Kind.
    applyArrowInfo sets a newly connected arrow's ArrowInfo::Settings from the toolbar, with its
    type chosen from the nodes it joins: a traversal arrow only between two trees. It and
    createDanglingArrow both copy the bindings of an arrow already leaving the same parent.
    setDanglingArrowTip moves a dangling arrow's tip, clears its duration override and reapplies
    pitch bindings, and destroyDanglingArrow removes one.

    GraphDocument holds one and carries out the canvas's edits through it, EncapsulationGestures
    and Clipboard build one to create a group, each inside its caller's undo transaction, and the
    tests build graphs with it. It only writes GraphState; the canvas and RTGraphBuilder react to
    the changes.
*/

enum class NodeCreationMode { Node, Modulator, TraversalFlag };

class NodeFactory
{
public:

    NodeFactory(GraphState& state, juce::UndoManager* undoManager);

    void createRootNode(const NodeInfo::Position& nodePosition);

    void createDanglingArrow (juce::ValueTree nodeTree, const juce::Point<int>& tipOffset, ArrowInfo::Settings arrowInfo);
    void setDanglingArrowTip (juce::ValueTree arrowTree, const juce::Point<int>& tipOffset);
    void destroyDanglingArrow(juce::ValueTree arrowTree);

    void applyArrowInfo(int parentNodeId, int childNodeId, ArrowInfo::Settings arrowInfo, ArrowInfo::Type rootConnectionType);

    juce::ValueTree create(NodeCreationMode mode, int parentNodeId, bool makeAlternative, const NodeInfo::Position& position);

    juce::ValueTree createNode             (int parentNodeId, const juce::Identifier& nodeType, const NodeInfo::Position& nodePosition);
    juce::ValueTree createTraversalFlagNode(int parentNodeId, const NodeInfo::Position& nodePosition);
    juce::ValueTree createEncapsulator     (std::span<const int> memberNodeIds);
    juce::ValueTree createModulatorNode    (NodeInfo::Kind kind, int parentNodeId, const NodeInfo::Position& nodePosition);

private:

    void setDefaultNodeNote (int nodeId);
    void setDefaultTraversal(int nodeId);

    void inheritFromParent(int parentNodeId, int newNodeId, juce::ValueTree newNode);

    void inheritArrowBindings(ArrowInfo::Settings& arrowInfo, int nodeId, const juce::ValueTree& newArrow) const;

    GraphState& state;

    juce::UndoManager* undoManager;
};
