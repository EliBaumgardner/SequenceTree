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
#include "../Util/PluginModules.h"



class ValueTreeState {

public:

    //ValueTree Identifiers

    static const juce::Identifier CanvasData;
    static const juce::Identifier NodeMap;
    static const juce::Identifier NodeTreeMap;

    static const juce::Identifier NodeTreeIds;
    static const juce::Identifier NodeTreeData;

    static const juce::Identifier RootNodeData;
    static const juce::Identifier NodeData;
    static const juce::Identifier ConnectorData;

    static const juce::Identifier NodeChildrenIds;

    static const juce::Identifier MidiNotesData;
    static const juce::Identifier MidiNoteData;

    //Id Property Identifiers

    static const juce::Identifier Id;

    static const juce::Identifier RootNodeId;
    static const juce::Identifier NodeId;
    static const juce::Identifier NodeTreeId;

    //Node Property Identifiers

    static const juce::Identifier CountLimit;
    static const juce::Identifier Count;

    static const juce::Identifier XPosition;
    static const juce::Identifier YPosition;
    static const juce::Identifier Radius;
    static const juce::Identifier ColourId;

    static const juce::Identifier MidiPitch;
    static const juce::Identifier MidiVelocity;
    static const juce::Identifier MidiDuration;

    //Static ValueTrees

    static juce::ValueTree canvasData;

    static juce::ValueTree nodeTreeIds;
    static juce::ValueTree nodeMap;
    static juce::ValueTree nodeTreeMap;




    //Functions

    ValueTreeState();

    static juce::ValueTree addNodeTree (juce::UndoManager* undoManager);
    static juce::ValueTree addRootNode (NodePosition nodePosition,juce::UndoManager* undoManager);

    static juce::ValueTree addRootNode    (int parentNodeId, NodePosition nodePosition, juce::UndoManager* undoManager);
    static juce::ValueTree addNode        (int parentNodeId, NodePosition nodePosition, juce::UndoManager* undoManager);
    static void addConnector              (int parentNodeId,NodePosition nodePosition, juce::UndoManager* undoManager);
    static void setMidiValue              (int nodeId, NodeNote note, juce::UndoManager* undoManager);

    static void removeRootNode (int rootNodeId, juce::UndoManager* undoManager);
    static void removeNode     (int nodeId, juce::UndoManager* undoManager);
    static void removeNodeTree (int treeId, juce::UndoManager* undoManager);

    static void setNodePosition (juce::ValueTree node, NodePosition nodePosition,juce::UndoManager* undoManager);

    static NodePosition    getNodePosition (int nodeId);
    static juce::ValueTree getNode         (int nocdId);
    static juce::ValueTree getNodeParent   (int nodeId);
    static juce::ValueTree getMidiNotes    (int nodeId);
    static juce::ValueTree getNodeTree     (int treeId);

    static inline int nodeIdIncrement       {0};
    static inline int defaultNodeCount      {1};
    static inline int defaultNodeCountLimit {1};
};


#endif //SEQUENCETREE_VALUETREESTATE_H