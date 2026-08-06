//
// Created by Eli Baumgardner on 3/21/26.
//

#include "ValueTreeState.h"
#include "ValueTreeIdentifiers.h"

ValueTreeState::ValueTreeState() {

    canvasData   = juce::ValueTree(ValueTreeIdentifiers::CanvasData);
    nodeTreeIds  = juce::ValueTree(ValueTreeIdentifiers::NodeTreeIds);
    nodeMap      = juce::ValueTree(ValueTreeIdentifiers::NodeMap);
    nodeTreeMap  = juce::ValueTree(ValueTreeIdentifiers::NodeTreeMap);

    traversalMap = juce::ValueTree(ValueTreeIdentifiers::TraversalMap);

    canvasData.addChild(nodeTreeIds, -1, nullptr);
}

static bool isNoteBearingNode(const juce::ValueTree& node)
{
    return node.getType() == ValueTreeIdentifiers::NodeData
        || node.getType() == ValueTreeIdentifiers::AlternativeNodeData
        || node.getType() == ValueTreeIdentifiers::RootNodeData;
}

void ValueTreeState::replaceState(const juce::ValueTree& restoredTree)
{
    juce::ValueTree restoredNodeMap;
    juce::ValueTree restoredTraversalMap;

    if (restoredTree.getType() == ValueTreeIdentifiers::PluginState) {
        restoredNodeMap      = restoredTree.getChildWithName(ValueTreeIdentifiers::NodeMap);
        restoredTraversalMap = restoredTree.getChildWithName(ValueTreeIdentifiers::TraversalMap);
    }
    else {
        restoredNodeMap = restoredTree;
    }

    traversalMap.removeAllChildren(nullptr);
    for (int i = 0; i < restoredTraversalMap.getNumChildren(); ++i) {
        traversalMap.addChild(restoredTraversalMap.getChild(i).createCopy(), -1, nullptr);
    }

    nodeMap.removeAllChildren(nullptr);
    for (int i = 0; i < restoredNodeMap.getNumChildren(); ++i) {
        nodeMap.addChild(restoredNodeMap.getChild(i).createCopy(), -1, nullptr);
    }

    int maxId = 0;
    for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
        int id = nodeMap.getChild(i).getProperty(ValueTreeIdentifiers::Id);
        if (id > maxId) {
            maxId = id;
        }
    }

    nodeIdIncrement = maxId;
}

juce::ValueTree ValueTreeState::addNodeTree(juce::UndoManager* undoManager)
{
    juce::ValueTree nodeTreeId         {ValueTreeIdentifiers::NodeTreeId};
    juce::ValueTree nodeTree           {ValueTreeIdentifiers::NodeTreeData};

    nodeIdIncrement = nodeIdIncrement + 1;

    nodeTree.setProperty  (ValueTreeIdentifiers::Id, nodeIdIncrement,undoManager);
    nodeTreeId.setProperty(ValueTreeIdentifiers::Id,nodeIdIncrement,undoManager);

    nodeTreeIds.addChild(nodeTreeId, -1, undoManager);
    nodeTreeMap.addChild(nodeTree,-1, undoManager);

    return nodeTree;
}

void ValueTreeState::setNodeCountProperties(juce::UndoManager *undoManager, juce::ValueTree node) {
    node.setProperty(ValueTreeIdentifiers::Count,       defaultNodeCount,      undoManager);
    node.setProperty(ValueTreeIdentifiers::SwitchCount,defaultSwitchCount,undoManager);

    node.setProperty(ValueTreeIdentifiers::CountLimit,  defaultNodeCountLimit, undoManager);
    node.setProperty(ValueTreeIdentifiers::TriggerLimit, defaultTriggerLimit, undoManager);
    node.setProperty(ValueTreeIdentifiers::SwitchCountLimit, defaultSwitchCountLimit, undoManager);
    node.setProperty(ValueTreeIdentifiers::SubLoopCountLimit,defaultSubLoopCountLimit, undoManager);

    node.setProperty(ValueTreeIdentifiers::RepeatValue, defaultRepeatValue,    undoManager);
}

juce::ValueTree ValueTreeState::addRootNode(juce::UndoManager* undoManager)
{
    juce::ValueTree newTree = addNodeTree(undoManager);

    int newTreeId = newTree.getProperty(ValueTreeIdentifiers::Id);

    juce::ValueTree rootNodeId        {ValueTreeIdentifiers::NodeId};

    juce::ValueTree rootNode          {ValueTreeIdentifiers::RootNodeData};
    juce::ValueTree nodeChildrenIds   {ValueTreeIdentifiers::NodeChildrenIds};
    juce::ValueTree midiNotesData     {ValueTreeIdentifiers::MidiNotesData};
    juce::ValueTree traversalChildrenIds {ValueTreeIdentifiers::TraversalChildrenIds};


    rootNode.addChild     (midiNotesData, -1, undoManager);
    rootNode.addChild     (nodeChildrenIds, -1, undoManager);
    rootNode.addChild     (traversalChildrenIds, -1, undoManager);
    newTree.addChild      (rootNodeId, -1, undoManager);

    setNodeCountProperties(undoManager, rootNode);

    rootNode.setProperty(ValueTreeIdentifiers::LoopLimit,   defaultRootLoopLimit,  undoManager);
    rootNode.setProperty(ValueTreeIdentifiers::RootNodeId,newTreeId,undoManager);

    rootNode.setProperty(ValueTreeIdentifiers::Id, newTreeId,undoManager);
    rootNodeId.setProperty(ValueTreeIdentifiers::Id,newTreeId,undoManager);
    newTree.setProperty   (ValueTreeIdentifiers::RootNodeId,newTreeId,undoManager);

    nodeMap.addChild      (rootNode,-1, undoManager);
    return rootNode;
}

juce::ValueTree ValueTreeState::addChildNode(juce::ValueTree parentNode, const juce::Identifier& nodeType,
                                             juce::UndoManager* undoManager)
{
    jassert(parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getType() == ValueTreeIdentifiers::NodeChildrenIds);

    int rootId = parentNode.getProperty(ValueTreeIdentifiers::RootNodeId);
    nodeIdIncrement = nodeIdIncrement + 1;

    juce::ValueTree nodeId          {ValueTreeIdentifiers::NodeId};
    juce::ValueTree node            {nodeType};
    juce::ValueTree nodeChildrenIds {ValueTreeIdentifiers::NodeChildrenIds};
    juce::ValueTree midiNotesData   {ValueTreeIdentifiers::MidiNotesData};

    node.addChild(nodeChildrenIds, -1, undoManager);
    node.addChild(midiNotesData, -1, undoManager);

    setNodeCountProperties(undoManager, node);

    node.setProperty(ValueTreeIdentifiers::RootNodeId, rootId, undoManager);
    node.setProperty(ValueTreeIdentifiers::Id, nodeIdIncrement, undoManager);
    nodeId.setProperty(ValueTreeIdentifiers::Id, nodeIdIncrement, undoManager);

    parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).addChild(nodeId, -1, undoManager);
    nodeMap.addChild(node, -1, undoManager);

    return node;
}

juce::ValueTree ValueTreeState::addNode(int parentNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(isNoteBearingNode(parentNode));

    return addChildNode(parentNode, ValueTreeIdentifiers::NodeData, undoManager);
}

juce::ValueTree ValueTreeState::addAlternativeNode(int parentNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(isNoteBearingNode(parentNode));

    return addChildNode(parentNode, ValueTreeIdentifiers::AlternativeNodeData, undoManager);
}

juce::ValueTree ValueTreeState::addTraversalFlagNode(int parentNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(isNoteBearingNode(parentNode)
         || parentNode.getType() == ValueTreeIdentifiers::TraversalFlagData);

    juce::ValueTree node = addChildNode(parentNode, ValueTreeIdentifiers::TraversalFlagData, undoManager);

    juce::ValueTree traversalChildrenIds {ValueTreeIdentifiers::TraversalChildrenIds};

    node.addChild(traversalChildrenIds, -1, undoManager);
    node.setProperty(ValueTreeIdentifiers::TraversalFlagValue, 0, undoManager);

    return node;
}

juce::ValueTree ValueTreeState::addModulatorNode(juce::ValueTree parentNode, const juce::Identifier& nodeType,
                                                 int newNodeId, juce::UndoManager* undoManager)
{
    int rootId = (int) parentNode.getProperty(ValueTreeIdentifiers::RootNodeId);

    if (nodeType == ValueTreeIdentifiers::ModulatorRootData) {
        rootId = newNodeId;
    }

    juce::ValueTree modulatorNode        {nodeType};
    juce::ValueTree modulatorNodeId      {ValueTreeIdentifiers::NodeId};
    juce::ValueTree modulatorChildrenIds {ValueTreeIdentifiers::NodeChildrenIds};
    juce::ValueTree modulatorType        {ValueTreeIdentifiers::ModulationType};
    juce::ValueTree defaultModulatorType {ValueTreeIdentifiers::DurationMod};

    modulatorType.addChild(defaultModulatorType, -1, undoManager);
    modulatorNode.addChild(modulatorType, -1, undoManager);
    modulatorNode.addChild(modulatorChildrenIds, -1, undoManager);

    setNodeCountProperties(undoManager, modulatorNode);

    modulatorNode.setProperty(ValueTreeIdentifiers::RootNodeId, rootId, undoManager);
    modulatorNode.setProperty(ValueTreeIdentifiers::ModAmount, defaultModAmount, undoManager);
    modulatorNode.setProperty(ValueTreeIdentifiers::Id, newNodeId, undoManager);
    modulatorNodeId.setProperty(ValueTreeIdentifiers::Id, newNodeId, undoManager);

    parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).addChild(modulatorNodeId, -1, undoManager);

    return modulatorNode;
}

juce::ValueTree ValueTreeState::addModulatorRoot(int parentNodeId, juce::UndoManager *undoManager) {

    juce::ValueTree parentNode = getNode(parentNodeId);
    juce::ValueTree nodeTree = addNodeTree(undoManager);
    int nodeTreeId = nodeTree.getProperty(ValueTreeIdentifiers::Id);

    jassert(parentNode.isValid());
    jassert(isNoteBearingNode(parentNode));

    juce::ValueTree modulatorNode = addModulatorNode(parentNode, ValueTreeIdentifiers::ModulatorRootData,
                                                     nodeTreeId, undoManager);

    nodeTree.setProperty(ValueTreeIdentifiers::RootNodeId, nodeTreeId, undoManager);

    nodeMap.addChild(modulatorNode, -1, undoManager);

    return modulatorNode;
}

juce::ValueTree ValueTreeState::addModulator(int parentNodeId, juce::UndoManager *undoManager) {
    juce::ValueTree parentNode = getNode(parentNodeId);
    juce::Identifier parentNodeType = parentNode.getType();

    jassert(parentNode.isValid());
    jassert(parentNodeType == ValueTreeIdentifiers::ModulatorData || parentNodeType == ValueTreeIdentifiers::ModulatorRootData);

    nodeIdIncrement = nodeIdIncrement + 1;

    juce::ValueTree modulatorNode = addModulatorNode(parentNode, ValueTreeIdentifiers::ModulatorData,
                                                     nodeIdIncrement, undoManager);

    nodeMap.addChild(modulatorNode, -1, undoManager);

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

void ValueTreeState::disconnectNodes(int parentNodeId, int childNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    juce::ValueTree nodeChildrenIds = parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
    if (! nodeChildrenIds.isValid()) {
        return;
    }

    juce::ValueTree childId = nodeChildrenIds.getChildWithProperty(ValueTreeIdentifiers::Id, childNodeId);
    if (childId.isValid()) {
        nodeChildrenIds.removeChild(childId, undoManager);
    }
}

void ValueTreeState::setArrowType(int parentNodeId, int childNodeId, ArrowType arrowType,
                                  juce::UndoManager* undoManager)
{
    juce::ValueTree connection = getConnection(parentNodeId, childNodeId);

    if (connection.isValid()) {
        connection.setProperty(ValueTreeIdentifiers::ArrowType, static_cast<int>(arrowType), undoManager);
    }
}

void ValueTreeState::removeNodeTree(int treeId, juce::UndoManager* undoManager)
{
    juce::ValueTree nodeTree = nodeTreeMap.getChildWithProperty(ValueTreeIdentifiers::Id,treeId);
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
    jassert(isNoteBearingNode(node));

    int pitch = note.pitch;
    int velocity = note.velocity;
    int duration = note.duration;

    juce::ValueTree midiNote {ValueTreeIdentifiers::MidiNoteData};

    midiNote.setProperty(ValueTreeIdentifiers::MidiPitch,    pitch,            undoManager);
    midiNote.setProperty(ValueTreeIdentifiers::MidiVelocity, velocity,         undoManager);
    midiNote.setProperty(ValueTreeIdentifiers::MidiDuration, duration,         undoManager);
    midiNote.setProperty(ValueTreeIdentifiers::MidiChannel,  note.midiChannel, undoManager);

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

        if (node.getType() == ValueTreeIdentifiers::TraversalFlagData) {
            continue;
        }

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

juce::ValueTree ValueTreeState::getConnection(int parentNodeId, int childNodeId)
{
    juce::ValueTree nodeChildrenIds = getNode(parentNodeId).getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    return nodeChildrenIds.getChildWithProperty(ValueTreeIdentifiers::Id, childNodeId);
}

juce::ValueTree ValueTreeState::getNode(int nodeId)
{
    return nodeMap.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId);
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

    if (!node.isValid() || !midiNotes.isValid()) {
        return juce::ValueTree();
    }

    return midiNotes;
}

juce::ValueTree ValueTreeState::createTraversalData(int traversalId, juce::UndoManager* undoManager)
{
    juce::ValueTree traversalData   {ValueTreeIdentifiers::TraversalData};

    traversalData.setProperty  (ValueTreeIdentifiers::TraversalId, traversalId, undoManager);

    traversalData.setProperty(ValueTreeIdentifiers::TempoMultiplier,defaultTempoMult,undoManager);

    traversalData.setProperty(ValueTreeIdentifiers::TraversalColour, juce::Colours::white.toString(), undoManager);

    traversalData.setProperty(ValueTreeIdentifiers::TraversalChannel, 1, undoManager);

    traversalData.setProperty(ValueTreeIdentifiers::TraversalTranspose, 0, undoManager);

    traversalData.setProperty(ValueTreeIdentifiers::TraversalVelocity, 1.0, undoManager);

    traversalMap.addChild(traversalData, -1, undoManager);
    return traversalData;
}