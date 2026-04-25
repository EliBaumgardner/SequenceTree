//
// Created by Eli Baumgardner on 3/21/26.
//



#include "ValueTreeState.h"
#include "ValueTreeIdentifiers.h"

//
// Created by Eli Baumgardner on 3/21/26.
//

juce::ValueTree ValueTreeState::canvasData;
juce::ValueTree ValueTreeState::nodeTreeIds;
juce::ValueTree ValueTreeState::nodeMap;
juce::ValueTree ValueTreeState::nodeTreeMap;



ValueTreeState::ValueTreeState() {

    canvasData   = juce::ValueTree(ValueTreeIdentifiers::CanvasData);
    nodeTreeIds  = juce::ValueTree(ValueTreeIdentifiers::NodeTreeIds);
    nodeMap      = juce::ValueTree(ValueTreeIdentifiers::NodeMap);
    nodeTreeMap  = juce::ValueTree(ValueTreeIdentifiers::NodeTreeMap);
    canvasData.addChild(nodeTreeIds, -1, nullptr);
}

juce::ValueTree ValueTreeState::addNodeTree(juce::UndoManager* undoManager)
{
    juce::ValueTree nodeTreeId        {ValueTreeIdentifiers::NodeTreeId};
    juce::ValueTree nodeTree          {ValueTreeIdentifiers::NodeTreeData};

    nodeIdIncrement = nodeIdIncrement + 1;

    nodeTree.setProperty(ValueTreeIdentifiers::ValueTreeIdentifiers::Id, nodeIdIncrement,undoManager);
    nodeTreeId.setProperty(ValueTreeIdentifiers::ValueTreeIdentifiers::Id,nodeIdIncrement,undoManager);

    nodeTreeIds.addChild(nodeTreeId, -1, undoManager);
    nodeTreeMap.addChild(nodeTree,-1, undoManager);

    return nodeTree;
}

juce::ValueTree ValueTreeState::addRootNode(juce::UndoManager* undoManager)
{
    juce::ValueTree newTree = addNodeTree(undoManager);
    int newTreeId = newTree.getProperty(ValueTreeIdentifiers::Id);

    juce::ValueTree rootNodeId        {ValueTreeIdentifiers::NodeId};
    juce::ValueTree rootNode          {ValueTreeIdentifiers::RootNodeData};
    juce::ValueTree nodeChildrenIds   {ValueTreeIdentifiers::NodeChildrenIds};
    juce::ValueTree midiNotesData     {ValueTreeIdentifiers::MidiNotesData};

    rootNode.addChild     (midiNotesData, -1, undoManager);
    rootNode.addChild     (nodeChildrenIds, -1, undoManager);
    newTree.addChild      (rootNodeId, -1, undoManager);

    rootNode.setProperty(ValueTreeIdentifiers::Id, newTreeId,undoManager);
    rootNode.setProperty(ValueTreeIdentifiers::Count,defaultNodeCount,undoManager);
    rootNode.setProperty(ValueTreeIdentifiers::CountLimit,defaultNodeCountLimit,undoManager);
    rootNode.setProperty(ValueTreeIdentifiers::LoopLimit,defaultRootLoopLimit,undoManager);
    rootNode.setProperty(ValueTreeIdentifiers::RootNodeId,newTreeId,undoManager);

    rootNodeId.setProperty(ValueTreeIdentifiers::Id,newTreeId,undoManager);
    newTree.setProperty   (ValueTreeIdentifiers::RootNodeId,newTreeId,undoManager);

    nodeMap.addChild      (rootNode,-1, undoManager);
    return rootNode;
}

juce::ValueTree ValueTreeState::addRootNode(int parentNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(parentNode.getType() == ValueTreeIdentifiers::ConnectorData );
    jassert(parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getType() == ValueTreeIdentifiers::NodeChildrenIds);

    juce::ValueTree rootNode = addRootNode(undoManager);
    int rootId = rootNode.getProperty(ValueTreeIdentifiers::Id);

    juce::ValueTree connectorChildId {ValueTreeIdentifiers::NodeId};

    connectorChildId.setProperty(ValueTreeIdentifiers::Id,rootId,undoManager);

    parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).addChild(connectorChildId, -1, undoManager);

    return rootNode;
}

juce::ValueTree ValueTreeState::addNode(int parentNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(parentNode.getType() == ValueTreeIdentifiers::NodeData || parentNode.getType() == ValueTreeIdentifiers::RootNodeData || parentNode.getType() == ValueTreeIdentifiers::ConnectorData);
    jassert(parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getType() == ValueTreeIdentifiers::NodeChildrenIds);

    if (parentNode.getType() == ValueTreeIdentifiers::ConnectorData) {
        return addRootNode(parentNodeId, undoManager);
    }

    int rootId = parentNode.getProperty(ValueTreeIdentifiers::RootNodeId);
    nodeIdIncrement = nodeIdIncrement + 1;

    juce::ValueTree nodeId            {ValueTreeIdentifiers::NodeId};
    juce::ValueTree node              {ValueTreeIdentifiers::NodeData};
    juce::ValueTree nodeChildrenIds      {ValueTreeIdentifiers::NodeChildrenIds};
    juce::ValueTree midiNotesData     {ValueTreeIdentifiers::MidiNotesData};

    node.addChild(nodeChildrenIds, -1, undoManager);
    node.addChild(midiNotesData, -1, undoManager);

    node.setProperty(ValueTreeIdentifiers::RootNodeId,rootId,undoManager);
    node.setProperty(ValueTreeIdentifiers::Id,nodeIdIncrement,undoManager);
    node.setProperty(ValueTreeIdentifiers::Count,defaultNodeCount,undoManager);
    node.setProperty(ValueTreeIdentifiers::CountLimit,defaultNodeCountLimit,undoManager);

    nodeId.setProperty(ValueTreeIdentifiers::Id,nodeIdIncrement,undoManager);

    parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).addChild(nodeId, -1, undoManager);
    nodeMap.addChild(node,-1,undoManager);

    return node;
}

juce::ValueTree ValueTreeState::addConnector(int parentNodeId,juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(parentNode.getType() == ValueTreeIdentifiers::NodeData || parentNode.getType() == ValueTreeIdentifiers::RootNodeData || parentNode.getType() == ValueTreeIdentifiers::ConnectorData);

    juce::ValueTree connectorId {ValueTreeIdentifiers::NodeId};
    juce::ValueTree connector   {ValueTreeIdentifiers::ConnectorData};
    juce::ValueTree connectorChildrenIds {ValueTreeIdentifiers::NodeChildrenIds};

    nodeIdIncrement = nodeIdIncrement + 1;

    connector.setProperty(ValueTreeIdentifiers::Id,nodeIdIncrement,undoManager);
    connector.setProperty(ValueTreeIdentifiers::RootNodeId,parentNode.getProperty(ValueTreeIdentifiers::RootNodeId),undoManager);
    connector.setProperty(ValueTreeIdentifiers::Count,defaultNodeCount,undoManager);
    connector.setProperty(ValueTreeIdentifiers::CountLimit,defaultNodeCountLimit,undoManager);

    connectorId.setProperty(ValueTreeIdentifiers::Id,nodeIdIncrement,undoManager);

    connector.addChild(connectorChildrenIds,-1,undoManager);
    parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).addChild(connectorId, -1, undoManager);
    nodeMap.addChild(connector,-1, undoManager);

    return connector;
}

juce::ValueTree ValueTreeState::addModulatorRoot(int parentNodeId, juce::UndoManager *undoManager) {

    juce::ValueTree parentNode = getNode(parentNodeId);
    juce::ValueTree nodeTree = addNodeTree(undoManager);
    int nodeTreeId = nodeTree.getProperty(ValueTreeIdentifiers::Id);

    jassert(parentNode.isValid());
    jassert(parentNode.getType() == ValueTreeIdentifiers::NodeData || parentNode.getType() == ValueTreeIdentifiers::RootNodeData);

    int rootId = nodeTreeId;

    juce::ValueTree modulatorNode        {ValueTreeIdentifiers::ModulatorRootData};
    juce::ValueTree modulatorNodeId      {ValueTreeIdentifiers::NodeId};
    juce::ValueTree modulatorChildrenIds {ValueTreeIdentifiers::NodeChildrenIds};
    juce::ValueTree modulatorType        {ValueTreeIdentifiers::ModulationType};
    juce::ValueTree defaultModulatorType {ValueTreeIdentifiers::DurationMod};

    modulatorNode.setProperty(ValueTreeIdentifiers::RootNodeId,rootId,undoManager);
    modulatorNode.setProperty(ValueTreeIdentifiers::Id,rootId,undoManager);
    modulatorNodeId.setProperty(ValueTreeIdentifiers::Id,rootId,undoManager);

    defaultModulatorType.setProperty(ValueTreeIdentifiers::ModAmount,defaultModAmount,undoManager);

    modulatorType.addChild(defaultModulatorType, -1, undoManager);
    modulatorNode.addChild(modulatorType,-1,undoManager);
    modulatorNode.addChild(modulatorChildrenIds,-1,undoManager);

    parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).addChild(modulatorNodeId, -1, undoManager);
    nodeTree.addChild(modulatorNode,-1,undoManager);

    nodeMap.addChild(modulatorNode,-1,undoManager);

    return modulatorNode;
}

juce::ValueTree ValueTreeState::addModulator(int parentNodeId, juce::UndoManager *undoManager) {
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(parentNode.getType() == ValueTreeIdentifiers::NodeData
         || parentNode.getType() == ValueTreeIdentifiers::RootNodeData
         || parentNode.getType() == ValueTreeIdentifiers::ModulatorRootData);

    nodeIdIncrement = nodeIdIncrement + 1;
    int nodeId = nodeIdIncrement;
    int rootId = (parentNode.getType() == ValueTreeIdentifiers::ModulatorRootData)
                     ? (int)parentNode.getProperty(ValueTreeIdentifiers::Id)
                     : (int)parentNode.getProperty(ValueTreeIdentifiers::RootNodeId);

    juce::ValueTree modulatorNode        {ValueTreeIdentifiers::ModulatorData};
    juce::ValueTree modulatorNodeId      {ValueTreeIdentifiers::NodeId};
    juce::ValueTree modulatorType        {ValueTreeIdentifiers::ModulationType};
    juce::ValueTree defaultModulatorType {ValueTreeIdentifiers::DurationMod};

    modulatorNode.setProperty(ValueTreeIdentifiers::RootNodeId,rootId,undoManager);
    modulatorNode.setProperty(ValueTreeIdentifiers::Id,nodeId,undoManager);
    modulatorNodeId.setProperty(ValueTreeIdentifiers::Id,nodeId,undoManager);
    defaultModulatorType.setProperty(ValueTreeIdentifiers::ModAmount,defaultModAmount,undoManager);

    modulatorType.addChild(defaultModulatorType, -1, undoManager);
    modulatorNode.addChild(modulatorType,-1,undoManager);

    parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).addChild(modulatorNodeId, -1, undoManager);

    nodeMap.addChild(modulatorNode,-1,undoManager);

    return modulatorNode;
}
void ValueTreeState::connectNodes(int parentNodeId, int childNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    juce::ValueTree nodeChildrenIds = parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    juce::ValueTree childId {ValueTreeIdentifiers::NodeId};
    childId.setProperty(ValueTreeIdentifiers::Id, childNodeId, undoManager);
    nodeChildrenIds.addChild(childId, -1, undoManager);
}

void ValueTreeState::removeNodeTree(int treeId, juce::UndoManager* undoManager)
{
    juce::ValueTree nodeTree = nodeTreeMap.getChildWithProperty(ValueTreeIdentifiers::NodeTreeId,treeId);
    juce::ValueTree nodeTreeIdToErase = nodeTreeIds.getChildWithProperty(ValueTreeIdentifiers::Id,treeId);

    jassert(nodeTree.isValid() || nodeTreeIdToErase.isValid());

    nodeTreeIds.removeChild(nodeTreeIdToErase,undoManager);
    nodeTreeMap.removeChild(nodeTree,undoManager);
}

void ValueTreeState::removeRootNode(int rootNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree rootNode = getNode(rootNodeId);
    juce::ValueTree rootTree = nodeTreeMap.getChildWithProperty(ValueTreeIdentifiers::NodeTreeId,rootNodeId);

    jassert(rootNode.isValid() || rootTree.isValid());

    nodeTreeMap.removeChild(rootTree,undoManager);
    nodeMap.removeChild(rootNode,undoManager);
}

void ValueTreeState::removeNode(int nodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree node = getNode(nodeId);
    jassert(node.isValid());

    nodeMap.removeChild(node, undoManager);

    for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
        juce::ValueTree mapNode = nodeMap.getChild(i);
        juce::ValueTree mapNodeChildrenIds = mapNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        juce::ValueTree mapNodeChildId = mapNodeChildrenIds.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId);

        if (mapNodeChildId.isValid()) {
            mapNodeChildrenIds.removeChild(mapNodeChildId, undoManager);
            break;
        }
    }
}

void ValueTreeState::setNodePosition(juce::ValueTree node, NodePosition nodePosition, juce::UndoManager* undoManager)
{
    jassert(node.isValid());

    int xPosition = nodePosition.xPosition;
    int yPosition = nodePosition.yPosition;
    int radius    = nodePosition.radius;

    node.setProperty(ValueTreeIdentifiers::XPosition, xPosition, undoManager);
    node.setProperty(ValueTreeIdentifiers::YPosition, yPosition, undoManager);
    node.setProperty(ValueTreeIdentifiers::Radius, radius, undoManager);
}

void ValueTreeState::setMidiValue(int nodeId,NodeNote note,juce::UndoManager* undoManager)
{
    juce::ValueTree node = getNode(nodeId);

    jassert(node.isValid());
    jassert(node.getType() == ValueTreeIdentifiers::NodeData || node.getType() == ValueTreeIdentifiers::ConnectorData || node.getType() == ValueTreeIdentifiers::ValueTreeIdentifiers::RootNodeData);

    int pitch = note.pitch;
    int velocity = note.velocity;
    int duration = note.duration;

    juce::ValueTree midiNote {ValueTreeIdentifiers::MidiNoteData};

    midiNote.setProperty(ValueTreeIdentifiers::MidiPitch,pitch,undoManager);
    midiNote.setProperty(ValueTreeIdentifiers::MidiVelocity,velocity,undoManager);
    midiNote.setProperty(ValueTreeIdentifiers::MidiDuration,duration,undoManager);

    node.getChildWithName(ValueTreeIdentifiers::MidiNotesData).addChild(midiNote, -1, undoManager);
}

NodePosition ValueTreeState::getNodePosition(int nodeId)
{
    juce::ValueTree node = getNode(nodeId);
    jassert(node.isValid());

    int nodeXPosition = node.getProperty(ValueTreeIdentifiers::XPosition);
    int nodeYPosition = node.getProperty(ValueTreeIdentifiers::YPosition);
    int nodeRadius    = node.getProperty(ValueTreeIdentifiers::Radius);

    NodePosition nodePosition;
    nodePosition.xPosition = nodeXPosition;
    nodePosition.yPosition = nodeYPosition;
    nodePosition.radius    = nodeRadius;

    return nodePosition;
}

juce::ValueTree ValueTreeState::getNodeParent(int nodeId) {
    for(int i = 0; i < nodeMap.getNumChildren(); ++i) {
        juce::ValueTree node = nodeMap.getChild(i);
        juce::ValueTree nodeChildrenIds = node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        juce::ValueTree nodeChildId = nodeChildrenIds.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId);
        if (nodeChildId.isValid()) {
            return node;
        }
    }
    return juce::ValueTree();
}

juce::ValueTree ValueTreeState::getNodeTree(int nodeTreeId)
{
    return nodeTreeMap.getChildWithProperty(ValueTreeIdentifiers::Id, nodeTreeId);
}

juce::ValueTree ValueTreeState::getNode(int nodeId)
{
    juce::ValueTree node = nodeMap.getChildWithProperty(ValueTreeIdentifiers::Id,nodeId);
    jassert(node.isValid());

    return node;
}

juce::ValueTree ValueTreeState::getRootNode(int nodeId)
{
    juce::ValueTree node = getNode(nodeId);
    int rootId = node.getProperty(ValueTreeIdentifiers::RootNodeId);

    juce::ValueTree rootNode = nodeMap.getChildWithProperty(ValueTreeIdentifiers::Id, rootId);
    jassert(rootNode.isValid());

    return rootNode;
}

juce::ValueTree ValueTreeState::getMidiNotes(int nodeId) {
    juce::ValueTree node = getNode(nodeId);
    juce::ValueTree midiNotes = node.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    if (!node.isValid() || !midiNotes.isValid()){
        return juce::ValueTree();
    }

    return midiNotes;
}