//
// Created by Eli Baumgardner on 3/21/26.
//



#include "../Util/ValueTreeState.h"

//
// Created by Eli Baumgardner on 3/21/26.
//

                                //VALUE TREES//
const juce::Identifier ValueTreeState::CanvasData           {"CanvasData"};
const juce::Identifier ValueTreeState::NodeMap              {"NodeMap"};
const juce::Identifier ValueTreeState::NodeTreeMap          {"NodeTreeMap"};

const juce::Identifier ValueTreeState::NodeTreeIds          {"NodeTreeIds"};
const juce::Identifier ValueTreeState::NodeTreeData         {"NodeTreeData"};

const juce::Identifier ValueTreeState::RootNodeData         {"RootNodeData"};
const juce::Identifier ValueTreeState::NodeData             {"NodeData"};
const juce::Identifier ValueTreeState::ConnectorData        {"ConnectorData"};

const juce::Identifier ValueTreeState::NodeChildrenIds     {"NodeChildrenIds"};

const juce::Identifier ValueTreeState::MidiNotesData        {"MidiNotesData"};
const juce::Identifier ValueTreeState::MidiNoteData         {"MidiNoteData"};

                                //PROPERTIES//
const juce::Identifier ValueTreeState::Id                   {"Id"};
const juce::Identifier ValueTreeState::NodeRootId           {"NodeRootId"};

const juce::Identifier ValueTreeState::NodeTreeId           {"NodeTreeId"};
const juce::Identifier ValueTreeState::NodeId               {"NodeId"};

const juce::Identifier ValueTreeState::CountLimit           {"CountLimit"};
const juce::Identifier ValueTreeState::Count                {"Count"};

const juce::Identifier ValueTreeState::XPosition            {"XPosition"};
const juce::Identifier ValueTreeState::YPosition            {"YPosition"};
const juce::Identifier ValueTreeState::Radius               {"Radius"};
const juce::Identifier ValueTreeState::ColourId             {"ColourId"};

ValueTreeState::ValueTreeState() {

    canvasData.addChild(nodeTreeIds, -1, nullptr);
}

juce::ValueTree ValueTreeState::addNodeTree(juce::UndoManager* undoManager)
{
    juce::ValueTree nodeTreeId        {NodeTreeId};
    juce::ValueTree nodeTree          {NodeTreeData};

   nodeIdIncrement = nodeIdIncrement + 1;

    nodeTree.setProperty(Id, nodeIdIncrement,undoManager);
    nodeTreeId.setProperty(Id,nodeIdIncrement,undoManager);

    nodeTreeMap.addChild(nodeTree,-1, undoManager);
    nodeTreeIds.addChild(nodeTreeId, -1, undoManager);

    return nodeTree;
}

juce::ValueTree ValueTreeState::addRootNode(juce::UndoManager* undoManager)
{
    juce::ValueTree newTree = addNodeTree(undoManager);
    int newTreeId = newTree.getProperty(Id);

    juce::ValueTree rootNodeId        {NodeId};

    juce::ValueTree rootNode          {RootNodeData};
    juce::ValueTree nodeChildrenIds   {NodeChildrenIds};
    juce::ValueTree midiNotesData     {MidiNotesData};

    rootNode.addChild(midiNotesData, -1, undoManager);
    rootNode.addChild(nodeChildrenIds, -1, undoManager);

    rootNode.setProperty(NodeRootId,newTreeId , undoManager);
    rootNode.setProperty(Id, newTreeId,undoManager);
    rootNodeId.setProperty(Id,newTreeId,undoManager);


    nodeMap.addChild(rootNode,-1, undoManager);

    newTree.addChild(rootNodeId, -1, undoManager);

    return rootNode;
}

juce::ValueTree ValueTreeState::addRootNode(juce::ValueTree parentNode, juce::UndoManager* undoManager)
{

    jassert(parentNode.isValid());
    jassert(parentNode.getType() == ConnectorData );
    jassert(parentNode.getChildWithName(NodeChildrenIds).getType() == NodeChildrenIds);

    juce::ValueTree connectorChildId {NodeId};
    juce::ValueTree rootNode = addRootNode(undoManager);

    int rootId = rootNode.getProperty(Id);
    connectorChildId.setProperty(Id,rootId,undoManager);

    parentNode.getChildWithName(NodeChildrenIds).addChild(connectorChildId, -1, undoManager);

    return rootNode;
}

juce::ValueTree ValueTreeState::addNode(const juce::ValueTree& parentNode,juce::UndoManager* undoManager)
{
    jassert(parentNode.isValid());
    jassert(parentNode.getType() == NodeData || parentNode.getType() == RootNodeData || parentNode.getType() == ConnectorData);
    jassert(parentNode.getChildWithName(NodeChildrenIds).getType() == NodeChildrenIds);

    if (parentNode.getType() == ConnectorData) {
        return addRootNode(parentNode, undoManager);
    }

    juce::ValueTree nodeId            {NodeId};
    juce::ValueTree node              {NodeData};
    juce::ValueTree nodeChildrenIds      {NodeChildrenIds};
    juce::ValueTree midiNotesData     {MidiNotesData};

    node.addChild(nodeChildrenIds, -1, undoManager);
    node.addChild(midiNotesData, -1, undoManager);
    node.setProperty(NodeRootId,parentNode.getProperty(NodeRootId),undoManager);

    nodeIdIncrement = nodeIdIncrement + 1;

    nodeId.setProperty(Id,nodeIdIncrement,undoManager);
    node.setProperty(Id,nodeIdIncrement,undoManager);

    nodeMap.addChild(node,-1,undoManager);
    parentNode.getChildWithName(NodeChildrenIds).addChild(nodeId, -1, undoManager);

    return node;
}

void ValueTreeState::addConnector(int parentNodeId,juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);
    jassert(parentNode.isValid());
    jassert(parentNode.getType() == NodeData || parentNode.getType() == RootNodeData || parentNode.getType() == ConnectorData);

    juce::ValueTree connectorId {NodeId};
    juce::ValueTree connector   {ConnectorData};

    nodeIdIncrement = nodeIdIncrement + 1;

    connector.setProperty(Id,nodeIdIncrement,undoManager);
    connector.setProperty(NodeRootId,parentNode.getProperty(NodeRootId),undoManager);
    connectorId.setProperty(Id,nodeIdIncrement,undoManager);
    nodeMap.addChild(connector,-1, undoManager);

    parentNode.getChildWithName(NodeChildrenIds).addChild(connectorId, -1, undoManager);
}

void ValueTreeState::addMidiNote(juce::ValueTree node,juce::ValueTree midiNote,juce::UndoManager* undoManager)
{
    jassert(node.isValid());

    jassert(node.getType() == NodeData || node.getType() == ConnectorData || node.getType() == RootNodeData);
    jassert(node.getChildWithName(MidiNotesData).getType() == MidiNotesData);

    jassert(midiNote.isValid());
    jassert(midiNote.getType() == MidiNoteData);

    node.getChildWithName(MidiNotesData).addChild(midiNote, -1, undoManager);
}

void ValueTreeState::removeNodeTree(int treeId, juce::UndoManager* undoManager)
{
    juce::ValueTree nodeTree = nodeTreeMap.getChildWithProperty(NodeTreeId,treeId);
    juce::ValueTree nodeTreeIdToErase = nodeTreeIds.getChildWithProperty(Id,treeId);

    jassert(nodeTree.isValid() || nodeTreeIdToErase.isValid());

    nodeTreeMap.removeChild(nodeTree,undoManager);
    nodeTreeIds.removeChild(nodeTreeIdToErase,undoManager);
}

void ValueTreeState::removeRootNode(int rootNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree rootNode = getNode(rootNodeId);
    juce::ValueTree rootTree = nodeTreeMap.getChildWithProperty(NodeTreeId,rootNodeId);

    jassert(rootNode.isValid() || rootTree.isValid());

    nodeTreeMap.removeChild(rootTree,undoManager);
    nodeMap.removeChild(rootNode,undoManager);
}

void ValueTreeState::removeNode(juce::ValueTree& node, juce::UndoManager* undoManager)
{
    int nodeId = node.getProperty(Id);
    jassert(node.isValid());

    for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
        juce::ValueTree mapNode = nodeMap.getChild(i);
        juce::ValueTree mapNodeChildrenIds = mapNode.getChildWithName(NodeChildrenIds);
        juce::ValueTree mapNodeChildId = mapNodeChildrenIds.getChildWithProperty(Id, nodeId);

        if (mapNodeChildId.isValid()) {
            mapNodeChildrenIds.removeChild(mapNodeChildId, undoManager);
        }
    }
}

juce::ValueTree ValueTreeState::getNodeTree(int nodeTreeId)
{
    return nodeTreeMap.getChildWithProperty(Id, nodeTreeId);
}

juce::ValueTree ValueTreeState::getNode(int nodeId)
{
    juce::ValueTree node = nodeMap.getChildWithProperty(Id,nodeId);
    jassert(node.isValid());
    return node;
}

juce::ValueTree ValueTreeState::getMidiNotes(int nodeId)
{
        return juce::ValueTree();

}

void ValueTreeState::setNodePosition(juce::ValueTree node, NodePosition nodePosition, juce::UndoManager* undoManager)
{
    jassert(node.isValid());

    int xPosition = nodePosition.xPosition;
    int yPosition = nodePosition.yPosition;
    int radius    = nodePosition.radius;


    node.setProperty(XPosition, xPosition, undoManager);
    node.setProperty(YPosition, yPosition, undoManager);
    node.setProperty(Radius, radius, undoManager);
}

NodePosition ValueTreeState::getNodePosition(int nodeId)
{
    juce::ValueTree node = getNode(nodeId);
    jassert(node.isValid());

    int nodeXPosition = node.getProperty(XPosition);
    int nodeYPosition = node.getProperty(YPosition);
    int nodeRadius    = node.getProperty(Radius);

    NodePosition nodePosition;
    nodePosition.xPosition = nodeXPosition;
    nodePosition.yPosition = nodeYPosition;
    nodePosition.radius    = nodeRadius;

    return nodePosition;
}

bool ValueTreeState::isRootNode(int nodeId)
{
   juce::ValueTree node = getNode(nodeId);
    jassert(node.isValid());
    bool isRootNode;
    if (node.getType() == RootNodeData) {
        isRootNode = true;
    }
    else {
        isRootNode = false;
    }

    return isRootNode;
}