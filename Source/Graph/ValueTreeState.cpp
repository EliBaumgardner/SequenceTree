//
// Created by Eli Baumgardner on 3/21/26.
//

#include "ValueTreeState.h"
#include "ValueTreeIdentifiers.h"
#include "../Script/ScriptCompiler.h"

#include <algorithm>

ValueTreeState::ValueTreeState() {

    canvasData   = juce::ValueTree(ValueTreeIdentifiers::CanvasData);
    nodeTreeIds  = juce::ValueTree(ValueTreeIdentifiers::NodeTreeIds);

    nodeMap      = juce::ValueTree(ValueTreeIdentifiers::NodeMap);
    nodeTreeMap  = juce::ValueTree(ValueTreeIdentifiers::NodeTreeMap);
    traversalMap = juce::ValueTree(ValueTreeIdentifiers::TraversalMap);

    traversalRules = juce::ValueTree(ValueTreeIdentifiers::TraversalRules);

    canvasData.addChild(nodeTreeIds, -1, nullptr);
}

namespace {

bool isNoteBearingNode(const juce::ValueTree& node)
{
    return node.getType() == ValueTreeIdentifiers::NodeData
        || node.getType() == ValueTreeIdentifiers::AlternativeNodeData
        || node.getType() == ValueTreeIdentifiers::RootNodeData;
}

}

void ValueTreeState::replaceState(const juce::ValueTree& restoredTree)
{
    juce::ValueTree restoredNodeMap;
    juce::ValueTree restoredTraversalMap;
    juce::ValueTree restoredTraversalRules;

    if (restoredTree.getType() == ValueTreeIdentifiers::PluginState) {
        restoredNodeMap        = restoredTree.getChildWithName(ValueTreeIdentifiers::NodeMap);
        restoredTraversalMap   = restoredTree.getChildWithName(ValueTreeIdentifiers::TraversalMap);
        restoredTraversalRules = restoredTree.getChildWithName(ValueTreeIdentifiers::TraversalRules);
    }
    else {
        restoredNodeMap = restoredTree;
    }

    traversalRules.removeAllChildren(nullptr);
    traversalRules.removeAllProperties(nullptr);

    for (int i = 0; i < restoredTraversalRules.getNumChildren(); ++i) {
        traversalRules.addChild(restoredTraversalRules.getChild(i).createCopy(), -1, nullptr);
    }

    if (restoredTraversalRules.hasProperty(ValueTreeIdentifiers::ActiveRuleId)) {
        traversalRules.setProperty(ValueTreeIdentifiers::ActiveRuleId,
                                   restoredTraversalRules.getProperty(ValueTreeIdentifiers::ActiveRuleId),
                                   nullptr);
    }

    ruleIdIncrement = 0;

    for (int i = 0; i < traversalRules.getNumChildren(); ++i) {
        const int id = traversalRules.getChild(i).getProperty(ValueTreeIdentifiers::Id);

        if (id > ruleIdIncrement) {
            ruleIdIncrement = id;
        }
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

    juce::ValueTree node            {nodeType};
    juce::ValueTree nodeChildrenIds {ValueTreeIdentifiers::NodeChildrenIds};
    juce::ValueTree midiNotesData   {ValueTreeIdentifiers::MidiNotesData};

    node.addChild(nodeChildrenIds, -1, undoManager);
    node.addChild(midiNotesData, -1, undoManager);

    setNodeCountProperties(undoManager, node);

    node.setProperty(ValueTreeIdentifiers::RootNodeId, rootId, undoManager);
    node.setProperty(ValueTreeIdentifiers::Id, nodeIdIncrement, undoManager);

    connectNodes(parentNode.getProperty(ValueTreeIdentifiers::Id), nodeIdIncrement, undoManager);
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

    connectNodes(parentNode.getProperty(ValueTreeIdentifiers::Id), newNodeId, undoManager);

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

    juce::ValueTree childId {ValueTreeIdentifiers::NodeId};
    childId.setProperty(ValueTreeIdentifiers::Id, childNodeId, undoManager);

    ArrowInfo arrowInfo;

    if (parentNode.getType() == ValueTreeIdentifiers::AlternativeNodeData) {
        arrowInfo.xBinding = ArrowBinding::NoBind;
        arrowInfo.yBinding = ArrowBinding::DurationBind;
    }

    setArrowInfo(childId, arrowInfo, undoManager);

    parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).addChild(childId, -1, undoManager);
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

void ValueTreeState::setArrowInfo(juce::ValueTree arrowTree, const ArrowInfo& arrowInfo,
                                  juce::UndoManager* undoManager)
{
    if (! arrowTree.isValid()) {
        return;
    }

    arrowTree.setProperty(ValueTreeIdentifiers::ArrowType,        static_cast<int>(arrowInfo.type),     undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowXBinding,    static_cast<int>(arrowInfo.xBinding), undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowYBinding,    static_cast<int>(arrowInfo.yBinding), undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowXMultiplier, arrowInfo.xMultiplier,                undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowYMultiplier, arrowInfo.yMultiplier,                undoManager);
}

ArrowInfo ValueTreeState::getArrowInfo(const juce::ValueTree& arrowTree)
{
    ArrowInfo arrowInfo;

    if (! arrowTree.isValid()) {
        return arrowInfo;
    }

    arrowInfo.type = static_cast<ArrowType>((int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowType,
                                                                       static_cast<int>(arrowInfo.type)));

    arrowInfo.xBinding = static_cast<ArrowBinding>((int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowXBinding,
                                                                              static_cast<int>(arrowInfo.xBinding)));
    arrowInfo.yBinding = static_cast<ArrowBinding>((int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowYBinding,
                                                                              static_cast<int>(arrowInfo.yBinding)));

    arrowInfo.xMultiplier = arrowTree.getProperty(ValueTreeIdentifiers::ArrowXMultiplier, arrowInfo.xMultiplier);
    arrowInfo.yMultiplier = arrowTree.getProperty(ValueTreeIdentifiers::ArrowYMultiplier, arrowInfo.yMultiplier);

    return arrowInfo;
}

bool ValueTreeState::applyArrowPitchOffset(juce::ValueTree arrowTree, int targetNodeId,
                                           int deltaX, int deltaY, juce::UndoManager* undoManager)
{
    const ArrowInfo arrowInfo = getArrowInfo(arrowTree);

    if (! ArrowInfo::bindsTo(arrowInfo, ArrowBinding::PitchBind)) {
        return false;
    }

    const int offset = ArrowInfo::pitchOffsetFromDelta(arrowInfo, deltaX, deltaY);

    if (! arrowTree.hasProperty(ValueTreeIdentifiers::ArrowPitchOffset)) {
        arrowTree.setProperty(ValueTreeIdentifiers::ArrowPitchOffset, offset, undoManager);
        return false;
    }

    const int appliedPitchOffset = arrowTree.getProperty(ValueTreeIdentifiers::ArrowPitchOffset, 0);

    if (offset == appliedPitchOffset) {
        return false;
    }

    juce::ValueTree note = getMidiNotes(targetNodeId).getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    if (! note.isValid()) {
        return false;
    }

    const int currentPitch = note.getProperty(ValueTreeIdentifiers::MidiPitch, defaultMidiPitch);
    const int wantedPitch  = currentPitch + offset - appliedPitchOffset;
    const int newPitch     = std::clamp(wantedPitch, minimumMidiPitch, maximumMidiPitch);

    arrowTree.setProperty(ValueTreeIdentifiers::ArrowPitchOffset, offset - (wantedPitch - newPitch), undoManager);

    if (newPitch == currentPitch) {
        return false;
    }

    note.setProperty(ValueTreeIdentifiers::MidiPitch, newPitch, undoManager);

    return true;
}

std::vector<int> ValueTreeState::syncPitchBindings(int nodeId, juce::UndoManager* undoManager)
{
    std::vector<int> repitchedNodeIds;

    juce::ValueTree node = getNode(nodeId);

    if (! node.isValid()) {
        return repitchedNodeIds;
    }

    const int centreX = node.getProperty(ValueTreeIdentifiers::XPosition);
    const int centreY = node.getProperty(ValueTreeIdentifiers::YPosition);

    const juce::ValueTree childIds = node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
        juce::ValueTree other = nodeMap.getChild(i);

        const int otherId = other.getProperty(ValueTreeIdentifiers::Id);

        if (otherId == nodeId) {
            continue;
        }

        const int otherX = other.getProperty(ValueTreeIdentifiers::XPosition);
        const int otherY = other.getProperty(ValueTreeIdentifiers::YPosition);

        juce::ValueTree incoming = other.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds)
                                        .getChildWithProperty(ValueTreeIdentifiers::Id, nodeId);

        if (applyArrowPitchOffset(incoming, nodeId,
                                  centreX - otherX, centreY - otherY, undoManager)) {
            repitchedNodeIds.push_back(nodeId);
        }

        juce::ValueTree outgoing = childIds.getChildWithProperty(ValueTreeIdentifiers::Id, otherId);

        if (applyArrowPitchOffset(outgoing, otherId,
                                  otherX - centreX, otherY - centreY, undoManager)) {
            repitchedNodeIds.push_back(otherId);
        }
    }

    const juce::ValueTree danglingArrows = node.getChildWithName(ValueTreeIdentifiers::DanglingArrows);

    for (int i = 0; i < danglingArrows.getNumChildren(); ++i) {
        juce::ValueTree arrowTree = danglingArrows.getChild(i);

        if (applyArrowPitchOffset(arrowTree, nodeId,
                                  (int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipX),
                                  (int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipY),
                                  undoManager)) {
            repitchedNodeIds.push_back(nodeId);
        }
    }

    return repitchedNodeIds;
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

    traversalData.setProperty(ValueTreeIdentifiers::TraversalId, traversalId, undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TempoMultiplier,defaultTempoMult,undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TraversalColour, juce::Colours::white.toString(), undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TraversalChannel, 1, undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TraversalTranspose, 0, undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TraversalVelocity, 1.0, undoManager);

    traversalMap.addChild(traversalData, -1, undoManager);
    return traversalData;
}
juce::ValueTree ValueTreeState::addTraversalRule(juce::UndoManager* undoManager)
{
    ++ruleIdIncrement;

    juce::ValueTree rule {ValueTreeIdentifiers::TraversalRuleData};

    rule.setProperty(ValueTreeIdentifiers::Id, ruleIdIncrement, undoManager);
    rule.setProperty(ValueTreeIdentifiers::RuleName, "rule " + juce::String(ruleIdIncrement), undoManager);
    rule.setProperty(ValueTreeIdentifiers::RuleSource, defaultTraversalScriptSource(), undoManager);

    traversalRules.addChild(rule, -1, undoManager);

    return rule;
}

void ValueTreeState::removeTraversalRule(int ruleId, juce::UndoManager* undoManager)
{
    juce::ValueTree rule = getTraversalRule(ruleId);

    if (!rule.isValid()) {
        return;
    }

    traversalRules.removeChild(rule, undoManager);

    if (getActiveTraversalRuleId() != ruleId) {
        return;
    }

    const juce::ValueTree replacement = traversalRules.getChild(0);

    setActiveTraversalRuleId(replacement.isValid()
                             ? (int) replacement.getProperty(ValueTreeIdentifiers::Id)
                             : -1,
                             undoManager);

}

juce::ValueTree ValueTreeState::getTraversalRule(int ruleId) const
{
    for (int i = 0; i < traversalRules.getNumChildren(); ++i) {
        const juce::ValueTree rule = traversalRules.getChild(i);

        if ((int) rule.getProperty(ValueTreeIdentifiers::Id) == ruleId) {
            return rule;
        }
    }

    return {};
}

void ValueTreeState::setTraversalRuleName(int ruleId, const juce::String& name,
                                          juce::UndoManager* undoManager)
{
    juce::ValueTree rule = getTraversalRule(ruleId);

    if (rule.isValid()) {
        rule.setProperty(ValueTreeIdentifiers::RuleName, name, undoManager);
    }
}

void ValueTreeState::setTraversalRuleSource(int ruleId, const juce::String& source,
                                            juce::UndoManager* undoManager)
{
    juce::ValueTree rule = getTraversalRule(ruleId);

    if (rule.isValid()) {
        rule.setProperty(ValueTreeIdentifiers::RuleSource, source, undoManager);
    }
}

void ValueTreeState::setActiveTraversalRuleId(int ruleId, juce::UndoManager* undoManager)
{
    traversalRules.setProperty(ValueTreeIdentifiers::ActiveRuleId, ruleId, undoManager);
}

int ValueTreeState::getActiveTraversalRuleId() const
{
    return traversalRules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1);
}

juce::String ValueTreeState::getActiveTraversalRuleSource() const
{
    const juce::ValueTree rule = getTraversalRule(getActiveTraversalRuleId());

    if (!rule.isValid()) {
        return {};
    }

    return rule.getProperty(ValueTreeIdentifiers::RuleSource).toString();
}

void ValueTreeState::ensureDefaultTraversalRule()
{
    if (traversalRules.getNumChildren() == 0) {
        addTraversalRule(nullptr);
    }

    if (!getTraversalRule(getActiveTraversalRuleId()).isValid()) {
        setActiveTraversalRuleId(traversalRules.getChild(0).getProperty(ValueTreeIdentifiers::Id), nullptr);
    }
}
