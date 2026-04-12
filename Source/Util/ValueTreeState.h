//
// Created by Eli Baumgardner on 3/21/26.
//

#ifndef SEQUENCETREE_TESTCLASS_H
#define SEQUENCETREE_TESTCLASS_H

//
// Created by Eli Baumgardner on 3/21/26.
//

#pragma once

#include "NodeInfo.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ValueTreeState {

public:

    ValueTreeState();

    static juce::ValueTree addNodeTree     (juce::UndoManager* undoManager);
    static juce::ValueTree addRootNode     (juce::UndoManager* undoManager);

    static juce::ValueTree addRootNode     (int parentNodeId, juce::UndoManager* undoManager);
    static juce::ValueTree addNode         (int parentNodeId, juce::UndoManager* undoManager);
    static juce::ValueTree addConnector    (int parentNodeId, juce::UndoManager* undoManager);
    static juce::ValueTree addModulatorRoot(int parentNodeId, juce::UndoManager* undoManager);
    static juce::ValueTree addModulator    (int parentNodeId, juce::UndoManager* undoManager);

    static void removeRootNode (int rootNodeId, juce::UndoManager* undoManager);
    static void removeNode     (int nodeId, juce::UndoManager* undoManager);
    static void removeNodeTree (int treeId, juce::UndoManager* undoManager);

    static void setNodePosition (juce::ValueTree node, NodePosition nodePosition,juce::UndoManager* undoManager);
    static void setMidiValue    (int nodeId, NodeNote note, juce::UndoManager* undoManager);

    static NodePosition    getNodePosition (int nodeId);
    static juce::ValueTree getRootNode     (int nodeId);
    static juce::ValueTree getNode         (int nocdId);
    static juce::ValueTree getNodeParent   (int nodeId);
    static juce::ValueTree getMidiNotes    (int nodeId);
    static juce::ValueTree getNodeTree     (int treeId);

    static juce::ValueTree canvasData;
    static juce::ValueTree nodeTreeIds;
    static juce::ValueTree nodeMap;
    static juce::ValueTree nodeTreeMap;
    static juce::ValueTree nodeArrows;

    static inline int nodeIdIncrement        {0};
    static inline int defaultNodeCount      {1};
    static inline int defaultNodeCountLimit {1};
    static inline int defaultRootLoopLimit  {0};  // 0 = loop infinitely
    static inline int defaultModAmount      {1};
};


#endif //SEQUENCETREE_VALUETREESTATE_H