//
// Created by Eli Baumgardner on 3/21/26.
//

#include "ValueTreeState.h"

//
// Created by Eli Baumgardner on 3/21/26.
//
const juce::Identifier ValueTreeState::CanvasData           {"CanvasData"};
const juce::Identifier ValueTreeState::NodeTreesData        {"NodeTreesData"};
const juce::Identifier ValueTreeState::NodeTreeData         {"NodeTreeData"};

const juce::Identifier ValueTreeState::RootNodeData         {"RootNodeData"};
const juce::Identifier ValueTreeState::NodeData             {"NodeData"};
const juce::Identifier ValueTreeState::ConnectorData        {"ConnectorData"};

const juce::Identifier ValueTreeState::NodeChildrenData     {"NodeChildrenData"};
const juce::Identifier ValueTreeState::ConnectorChildrenData{"ConnectorChildrenData"};

const juce::Identifier ValueTreeState::MidiNotesData        {"MidiNotesData"};
const juce::Identifier ValueTreeState::MidiNoteData         {"MidiNoteData"};

const juce::Identifier ValueTreeState::Id                   {"Id"};
const juce::Identifier ValueTreeState::CountLimit           {"CountLimit"};
const juce::Identifier ValueTreeState::Count                {"Count"};
const juce::Identifier ValueTreeState::ColourId             {"ColourId"};

ValueTreeState::ValueTreeState() {

    canvasData.addChild(nodeTrees, -1, nullptr);
}

juce::ValueTree ValueTreeState::addNodeTree(juce::UndoManager& undoManager)
{
    juce::ValueTree nodeTree          {NodeTreeData};

    treeIdIncrement = treeIdIncrement + 1;
    nodeTree.setProperty(Id, treeIdIncrement,&undoManager);

    nodeTrees.addChild(nodeTree, -1, &undoManager);

    return nodeTree;
}

juce::ValueTree ValueTreeState::addRootNode(juce::UndoManager& undoManager)
{
    juce::ValueTree newTree = addNodeTree(undoManager);

    juce::ValueTree rootNode          {RootNodeData};
    juce::ValueTree nodeChildren      {NodeChildrenData};
    juce::ValueTree connectorChildren {ConnectorChildrenData};
    juce::ValueTree midiNotesData     {MidiNotesData};

    rootNode.addChild(nodeChildren, -1, &undoManager);
    rootNode.addChild(connectorChildren, -1, &undoManager);
    rootNode.addChild(midiNotesData, -1, &undoManager);

    nodeIdIncrement = nodeIdIncrement + 1;
    rootNode.setProperty(Id,nodeIdIncrement,&undoManager);

    newTree.addChild(rootNode,-1,&undoManager);

    return rootNode;
}

void ValueTreeState::addRootNode(juce::ValueTree parentNode, juce::UndoManager &undoManager)
{
    jassert(parentNode.isValid());
    jassert(parentNode.getType() == ConnectorData );
    jassert(parentNode.getChildWithName(ConnectorChildrenData).getType() == ConnectorChildrenData);

    juce::ValueTree rootNode = addRootNode(undoManager);

    parentNode.getChildWithName(ConnectorChildrenData).addChild(rootNode, -1, &undoManager);
}

void ValueTreeState::addNode(juce::ValueTree parentNode,juce::UndoManager& undoManager)
{
    jassert(parentNode.isValid());
    jassert(parentNode.getType() == NodeData || parentNode.getType() == RootNodeData);
    jassert(parentNode.getChildWithName(NodeChildrenData).getType() == NodeChildrenData);

    juce::ValueTree node              {NodeData};
    juce::ValueTree nodeChildren      {NodeChildrenData};
    juce::ValueTree connectorChildren {ConnectorChildrenData};
    juce::ValueTree midiNotesData     {MidiNotesData};

    node.addChild(nodeChildren, -1, &undoManager);
    node.addChild(connectorChildren, -1, &undoManager);
    node.addChild(midiNotesData, -1, &undoManager);

    nodeIdIncrement = nodeIdIncrement + 1;
    node.setProperty(Id,nodeIdIncrement,&undoManager);

    parentNode.getChildWithName(NodeChildrenData).addChild(node, -1, &undoManager);
}

void ValueTreeState::addConnector(juce::ValueTree parentNode,juce::UndoManager& undoManager)
{

    jassert(parentNode.isValid());
    jassert(parentNode.getType() == NodeData || parentNode.getType() == RootNodeData || parentNode.getType() == ConnectorData);

    juce::ValueTree connector {ConnectorData};

    nodeIdIncrement = nodeIdIncrement + 1;
    connector.setProperty(Id,nodeIdIncrement,&undoManager);

    parentNode.getChildWithName(ConnectorChildrenData).addChild(connector, -1, &undoManager);
}

void ValueTreeState::addMidiNote(juce::ValueTree node,juce::ValueTree midiNote,juce::UndoManager& undoManager)
{

    jassert(node.isValid());

    jassert(node.getType() == NodeData || node.getType() == ConnectorData || node.getType() == RootNodeData);
    jassert(node.getChildWithName(MidiNotesData).getType() == MidiNotesData);

    jassert(midiNote.isValid());
    jassert(midiNote.getType() == MidiNoteData);

    node.getChildWithName(MidiNotesData).addChild(midiNote, -1, &undoManager);
}

juce::ValueTree ValueTreeState::getNodeTree(int nodeTreeId)
{
    for (int i = 0; i < nodeTrees.getNumChildren(); i++) {
        juce::ValueTree nodeTree = nodeTrees.getChild(i);
        if ((int)nodeTree.getProperty(Id) == nodeTreeId){ return nodeTree; };
    }
}

juce::ValueTree ValueTreeState::getNode(int nodeId)
{
    for (int i = 0; i < nodeTrees.getNumChildren(); i++) {

        juce::ValueTree nodeTree = nodeTrees.getChild(i);
        juce::ValueTree rootNode = nodeTree.getChildWithName(RootNodeData);

        if ((int)rootNode.getProperty(Id) == nodeId){ return rootNode; };

        juce::Array<juce::ValueTree> nodesToVisit;
        nodesToVisit.add(rootNode.getChildWithName(NodeChildrenData));

        int index = 0;

        while (index < nodesToVisit.size()) {
            juce::ValueTree parentNode = nodesToVisit[index];
            juce::ValueTree nodeChildren = parentNode.getChildWithName(NodeChildrenData);
            index++;

            for (int j = 0; j < nodeChildren.getNumChildren(); j++) {
                juce::ValueTree childNode = nodeChildren.getChild(j);
                if ((int)childNode.getProperty(Id) == nodeId){ return childNode; };

                juce::ValueTree subChildren = childNode.getChildWithName(NodeChildrenData);
                nodesToVisit.add(subChildren);
            }
        }
    }

    return juce::ValueTree();
}

juce::ValueTree ValueTreeState::getMidiNotes(int nodeId)
{

}

