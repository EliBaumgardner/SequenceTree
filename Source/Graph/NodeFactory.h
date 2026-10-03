#pragma once

#include <juce_graphics/juce_graphics.h>
#include "GraphState.h"
#include "../Util/ArrowInfo.h"

#include <span>

class NodeFactory
{
public:

    static void createRootNode(GraphState& state, const NodePosition& nodePosition, juce::UndoManager* undoManager);

    static juce::ValueTree createNode(GraphState& state, int parentNodeId, const juce::Identifier& nodeType,
                                      const NodePosition& nodePosition, juce::UndoManager* undoManager);

    static juce::ValueTree createTraversalFlagNode(GraphState& state, int parentNodeId, const NodePosition& nodePosition,
                                                   juce::UndoManager* undoManager);

    static juce::ValueTree createModulator(GraphState& state, int parentNodeId, const NodePosition& nodePosition,
                                           juce::UndoManager* undoManager);

    static juce::ValueTree createAlternativeModulator(GraphState& state, int parentNodeId, const NodePosition& nodePosition,
                                                      juce::UndoManager* undoManager);

    static juce::ValueTree createModulatorRoot(GraphState& state, int parentNodeId, const NodePosition& nodePosition,
                                               juce::UndoManager* undoManager);

    static juce::ValueTree createEncapsulator(GraphState& state, std::span<const int> memberNodeIds,
                                              int subLoopCountLimit, juce::UndoManager* undoManager);

    static void createDanglingArrow(GraphState& state, juce::ValueTree nodeTree, const juce::Point<int>& tipOffset,
                                    const ArrowInfo& arrowInfo, juce::UndoManager* undoManager);

    static void destroyDanglingArrow(juce::ValueTree arrowTree, juce::UndoManager* undoManager);

    static void setDanglingArrowTip(GraphState& state, juce::ValueTree arrowTree, const juce::Point<int>& tipOffset,
                                    juce::UndoManager* undoManager);

private:

    static void setDefaultNodeNote(GraphState& state, int nodeId, juce::UndoManager* undoManager);
    static void setDefaultTraversal(GraphState& state, int nodeId, juce::UndoManager* undoManager);

    static void inheritFromParent(GraphState& state, int parentNodeId, int newNodeId, juce::ValueTree newNode,
                                  juce::UndoManager* undoManager);
};
