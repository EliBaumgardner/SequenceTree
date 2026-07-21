//
// Created by Eli Baumgardner on 3/21/26.
//

#pragma once

#include "../Util/NodeInfo.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ValueTreeState {

public:

    ValueTreeState();

    juce::ValueTree addNodeTree     (juce::UndoManager* undoManager);

    void setNodeCountProperties(juce::UndoManager *undoManager, juce::ValueTree node);

    juce::ValueTree createTraversalData    (int traversalId, juce::UndoManager* undoManager);

    juce::ValueTree addRootNode         (juce::UndoManager* undoManager);

    juce::ValueTree addNode             (int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addAlternativeNode  (int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addTraversalFlagNode(int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addModulatorRoot    (int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addModulator        (int parentNodeId, juce::UndoManager* undoManager);

    void connectNodes   (int parentNodeId, int childNodeId, juce::UndoManager* undoManager);
    void disconnectNodes(int parentNodeId, int childNodeId, juce::UndoManager* undoManager);
    void removeRootNode (int rootNodeId, juce::UndoManager* undoManager);
    void removeNode     (int nodeId, juce::UndoManager* undoManager);
    void removeNodeTree (int treeId, juce::UndoManager* undoManager);

    void setNodePosition (juce::ValueTree node, NodePosition nodePosition, juce::UndoManager* undoManager);
    void setMidiValue    (int nodeId, NodeNote note, juce::UndoManager* undoManager);

    NodePosition    getNodePosition (int nodeId);
    juce::ValueTree getRootNode     (int nodeId);
    juce::ValueTree getNode         (int nodeId);
    juce::ValueTree getNodeParent   (int nodeId);
    juce::ValueTree getMidiNotes    (int nodeId);
    juce::ValueTree getNodeTree     (int treeId);

    int  getNodeIdIncrement() const  { return nodeIdIncrement; }
    void setNodeIdIncrement(int value) { nodeIdIncrement = value; }

    juce::ValueTree canvasData;
    juce::ValueTree nodeTreeIds;
    juce::ValueTree nodeMap;
    juce::ValueTree nodeTreeMap;
    juce::ValueTree traversalMap;

    static constexpr int defaultSwitchCount       {1};
    static constexpr int defaultNodeCount         {1};
    static constexpr int defaultSwitchCountLimit  {1};
    static constexpr int defaultNodeCountLimit    {1};
    static constexpr int defaultTriggerLimit      {0};
    static constexpr int defaultSubLoopCountLimit {1};
    static constexpr int defaultRootLoopLimit     {0};
    static constexpr int defaultRepeatValue       {1};
    static constexpr int defaultModAmount         {1};
    static constexpr int defaultMidiChannel       {1};
    static constexpr int defaultTempoMult         {1};

private:

    int nodeIdIncrement = 0;
};
