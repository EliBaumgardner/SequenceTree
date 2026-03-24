//
// Created by Eli Baumgardner on 3/21/26.
//

#ifndef SEQUENCETREE_TESTCLASS_H
#define SEQUENCETREE_TESTCLASS_H

//
// Created by Eli Baumgardner on 3/21/26.
//

#pragma once

#include "../Util/PluginModules.h"
#include "../Util/PluginContext.h"

class ValueTreeState {

public:

    static const juce::Identifier CanvasData;
    static const juce::Identifier NodeTreesData;
    static const juce::Identifier NodeTreeData;

    static const juce::Identifier RootNodeData;
    static const juce::Identifier NodeData;
    static const juce::Identifier ConnectorData;

    static const juce::Identifier NodeChildrenData;
    static const juce::Identifier ConnectorChildrenData;

    static const juce::Identifier MidiNotesData;
    static const juce::Identifier MidiNoteData;

    static const juce::Identifier Id;
    static const juce::Identifier CountLimit;
    static const juce::Identifier Count;
    static const juce::Identifier XPosition;
    static const juce::Identifier YPosition;
    static const juce::Identifier Radius;
    static const juce::Identifier ColourId;


    juce::ValueTree canvasData    {CanvasData};
    juce::ValueTree nodeTrees     {NodeTreesData };


    ValueTreeState();
    juce::ValueTree addNodeTree (juce::UndoManager& undoManager);
    juce::ValueTree addRootNode (juce::UndoManager& undoManager);
    void addRootNode (juce::ValueTree parentNode, juce::UndoManager& undoManager);
    void addNode     (juce::ValueTree parentNode, juce::UndoManager& undoManager);
    void addConnector(juce::ValueTree parentNode,juce::UndoManager& undoManager);
    void addMidiNote (juce::ValueTree node, juce::ValueTree midiNote,juce::UndoManager& undoManager);

    juce::ValueTree getNode     (int nodeId);
    juce::ValueTree getConnector(int connectorId);
    juce::ValueTree getMidiNotes(int nodeId);
    juce::ValueTree getNodeTree (int nodeId);

    bool isRootNode  (int nodeId);
    bool isConnector (int nodeId);

    int nodeIdIncrement { 0 };
    int treeIdIncrement { 0 };
};


#endif //SEQUENCETREE_VALUETREESTATE_H