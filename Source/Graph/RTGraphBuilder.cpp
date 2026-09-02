/*
  ==============================================================================

    RTGraphBuilder.cpp
    Created: 30 Apr 2026
    Author:  Eli Baumgardner

  ==============================================================================
*/

#include "RTGraphBuilder.h"

#include <algorithm>
#include <unordered_set>

#include "../Plugin/PluginProcessor.h"
#include "ValueTreeState.h"
#include "ValueTreeIdentifiers.h"


RTGraphBuilder::RTGraphBuilder(SequenceTreeAudioProcessor& processorRef, ValueTreeState& valueTreeStateRef)
    : processor(processorRef), valueTreeState(valueTreeStateRef)
{
}

namespace {

void appendOnce(std::vector<int>& ids, int id)
{
    if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
        ids.push_back(id);
    }
}

juce::Point<int> nodeCentre(const juce::ValueTree& nodeValueTree)
{
    return { (int) nodeValueTree.getProperty(ValueTreeIdentifiers::XPosition),
             (int) nodeValueTree.getProperty(ValueTreeIdentifiers::YPosition) };
}

void collectDisabledTraversals(const juce::ValueTree& owner, std::unordered_set<int>& disabledSet)
{
    juce::ValueTree disabledTraversals = owner.getChildWithName(ValueTreeIdentifiers::DisabledTraversalIds);

    if (!disabledTraversals.isValid()) {
        return;
    }

    for (int i = 0; i < disabledTraversals.getNumChildren(); i++) {
        disabledSet.insert((int) disabledTraversals.getChild(i).getProperty(ValueTreeIdentifiers::TraversalId));
    }
}

bool isTreeJumpConnection(const juce::ValueTree& parentValueTree,
                          const juce::ValueTree& childIdTree,
                          const juce::ValueTree& childValueTree)
{
    const int arrowType = childIdTree.getProperty(ValueTreeIdentifiers::ArrowType,
                                                  static_cast<int>(ArrowType::Node));

    if (arrowType != static_cast<int>(ArrowType::Traversal)) {
        return false;
    }

    if (childValueTree.getType() != ValueTreeIdentifiers::RootNodeData) {
        return false;
    }

    const int childId = childValueTree.getProperty(ValueTreeIdentifiers::Id);

    return (int) parentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId) != childId;
}

}

void RTGraphBuilder::fillDurationMap(const juce::ValueTree& nodeValueTree, RTNode& rtNode)
{
    const bool isAlternative = (nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeNodeData);

    const juce::Point<int> centre = nodeCentre(nodeValueTree);

    auto durationTo = [&](const juce::ValueTree& connection, const juce::ValueTree& other) {
        const juce::Point<int> delta = nodeCentre(other) - centre;
        return arrowDurationFromDelta(ValueTreeState::readArrowInfo(connection, isAlternative), delta.x, delta.y);
    };

    if (isAlternative) {
        juce::ValueTree parent = valueTreeState.getNodeParent(rtNode.nodeID);

        if (parent.isValid()) {
            const int parentId = parent.getProperty(ValueTreeIdentifiers::Id);

            rtNode.durationMap[parentId] = durationTo(valueTreeState.getConnection(parentId, rtNode.nodeID),
                                                      parent);
        }
    }

    juce::ValueTree childIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < childIds.getNumChildren(); i++) {
        juce::ValueTree childIdTree = childIds.getChild(i);

        const int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);
        juce::ValueTree childTree = valueTreeState.getNode(childId);

        if (!childTree.isValid() || childTree.getType() == ValueTreeIdentifiers::AlternativeNodeData) {
            continue;
        }

        rtNode.durationMap[childId] = durationTo(childIdTree, childTree);
    }

    juce::ValueTree danglingArrows = nodeValueTree.getChildWithName(ValueTreeIdentifiers::DanglingArrows);

    for (int i = 0; i < danglingArrows.getNumChildren(); i++) {
        juce::ValueTree arrowTree = danglingArrows.getChild(i);

        const int tipX = arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipX);
        const int tipY = arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipY);

        rtNode.durationMap[rtNode.nodeID] =
            arrowDurationFromDelta(ValueTreeState::readArrowInfo(arrowTree, isAlternative), tipX, tipY);

        collectDisabledTraversals(arrowTree, rtNode.disabledTraversalsByChild[rtNode.nodeID]);
    }
}

void RTGraphBuilder::makeRTGraph(const juce::ValueTree& nodeValueTree)
{
    if (!nodeValueTree.isValid()) {
        return;
    }

    if (nodeValueTree.getType() == ValueTreeIdentifiers::TraversalData) {
        rebuildGraphsForTraversal(nodeValueTree.getProperty(ValueTreeIdentifiers::TraversalId));
        return;
    }

    int rootNodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    if (rootNodeId == 0) {
        return;
    }

    juce::ValueTree rootNodeValueTree = valueTreeState.getNode(rootNodeId);

    if (!rootNodeValueTree.isValid()) {
        rtGraphs.erase(rootNodeId);
        auto emptyGraph = std::make_shared<RTGraph>();
        emptyGraph->graphID = rootNodeId;
        processor.snapshots.publishGraph(emptyGraph);
        return;
    }

    std::shared_ptr<RTGraph> rtGraph = std::make_shared<RTGraph>();
    std::unordered_map<int,juce::ValueTree> tempNodeMap;

    rtGraph->graphID   = rootNodeId;
    rtGraph->loopLimit = rootNodeValueTree.getProperty(ValueTreeIdentifiers::LoopLimit, 0);

    NodeBuildMap builtNodes;

    createRTNodes(rootNodeValueTree, builtNodes, tempNodeMap);
    createRTNodeConnections(builtNodes, tempNodeMap);

    rtGraph->nodeMap = freezeNodes(builtNodes);

    rtGraphs[rtGraph->graphID] = rtGraph;
    processor.snapshots.publishGraph(rtGraph);
}

void RTGraphBuilder::rebuildGraphsForTraversal(int traversalId)
{
    juce::ValueTree nodeMap = valueTreeState.nodeMap;

    std::unordered_set<int> rootsToRebuild;

    for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
        juce::ValueTree node = nodeMap.getChild(i);

        if (node.getType() == ValueTreeIdentifiers::RootNodeData) {
            juce::ValueTree traversalChildrenIds = node.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

            if (traversalChildrenIds.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId).isValid()) {
                rootsToRebuild.insert((int) node.getProperty(ValueTreeIdentifiers::RootNodeId));
            }
        }
        else if (node.getType() == ValueTreeIdentifiers::TraversalFlagData) {
            int flagValue = node.getProperty(ValueTreeIdentifiers::TraversalFlagValue, 0);
            if (flagValue < 0) {
                flagValue = -flagValue;
            }

            if (flagValue == traversalId) {
                rootsToRebuild.insert((int) node.getProperty(ValueTreeIdentifiers::RootNodeId));
            }
        }
    }

    for (int rootId : rootsToRebuild) {
        makeRTGraph(valueTreeState.getNode(rootId));
    }
}

void RTGraphBuilder::createRTNodes(juce::ValueTree rootNodeValueTree, NodeBuildMap& builtNodes, std::unordered_map<int, juce::ValueTree>& tempNodeMap) {
    std::vector<juce::ValueTree> stack = {rootNodeValueTree};

    while(!stack.empty()) {

        juce::ValueTree currentValueTree = stack.back();
        juce::ValueTree nodeParentValueTree = valueTreeState.getNodeParent(currentValueTree.getProperty(ValueTreeIdentifiers::Id));

        juce::ValueTree nodeValueTreeChildren = currentValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        juce::ValueTree nodeValueTreeTraversals = currentValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

        juce::ValueTree nodeMidiNotes = currentValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

        juce::Identifier nodeType = currentValueTree.getType();

        stack.pop_back();

        int nodeId      = currentValueTree.getProperty(ValueTreeIdentifiers::Id);
        int graphId     = currentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);

        int countLimit       = currentValueTree.getProperty(ValueTreeIdentifiers::CountLimit);
        int triggerLimit     = currentValueTree.getProperty(ValueTreeIdentifiers::TriggerLimit, ValueTreeState::defaultTriggerLimit);
        int switchCountLimit = currentValueTree.getProperty(ValueTreeIdentifiers::SwitchCountLimit);
        int subLoopLimit     = currentValueTree.getProperty(ValueTreeIdentifiers::SubLoopCountLimit);

        int repeatValue = currentValueTree.getProperty(ValueTreeIdentifiers::RepeatValue, ValueTreeState::defaultRepeatValue);
        int modAmount   = currentValueTree.getProperty(ValueTreeIdentifiers::ModAmount, ValueTreeState::defaultModAmount);

        bool isAlternativeNode = (nodeType == ValueTreeIdentifiers::AlternativeNodeData);

        if(tempNodeMap.count(nodeId) == false) {

            tempNodeMap[nodeId] = currentValueTree;

            RTNode rtNode;
            RTNode* parentNode = nullptr;

            rtNode.graphID = graphId;
            rtNode.nodeID  = nodeId;

            rtNode.countLimit        =  countLimit;
            rtNode.triggerLimit      = triggerLimit;
            rtNode.subLoopCountLimit = subLoopLimit;
            rtNode.switchCountLimit  = switchCountLimit;
            rtNode.repeatValue       = repeatValue;

            rtNode.isAlternativeNode = isAlternativeNode;

            for (int i = 0; i < nodeValueTreeTraversals.getNumChildren(); i++) {
                juce::ValueTree traversalIdTree = nodeValueTreeTraversals.getChild(i);
                int traversalId = traversalIdTree.getProperty(ValueTreeIdentifiers::TraversalId);

                rtNode.traversals.push_back(buildRTtraversal(traversalId));
            }

            if (nodeParentValueTree.isValid()) {
                int candidateParentId = nodeParentValueTree.getProperty(ValueTreeIdentifiers::Id);
                auto parentIt = builtNodes.find(candidateParentId);

                if (parentIt != builtNodes.end()) {
                    rtNode.parentId = candidateParentId;
                    parentNode = &parentIt->second;

                    if (isAlternativeNode) {
                        if (parentNode->nodeType != RTNode::NodeType::Alternative) {
                            parentNode->alternativeRootId = nodeId;
                            rtNode.alternativeRootId      = nodeId;
                        }
                        else {
                            rtNode.alternativeRootId = parentNode->alternativeRootId;
                        }
                    }
                }
            }

            fillDurationMap(currentValueTree, rtNode);


            if (nodeType == ValueTreeIdentifiers::NodeData) {
                rtNode.nodeType = RTNode::NodeType::Node;
            }
            if (nodeType == ValueTreeIdentifiers::AlternativeNodeData) {
                rtNode.nodeType = RTNode::NodeType::Alternative;
            }
            if (nodeType == ValueTreeIdentifiers::RootNodeData) {
                rtNode.nodeType = RTNode::NodeType::RootNode;
            }
            if (nodeType == ValueTreeIdentifiers::ModulatorRootData) {
                rtNode.nodeType    = RTNode::NodeType::ModulatorRoot;
                rtNode.pitchOffset = modAmount;
            }
            if (nodeType == ValueTreeIdentifiers::ModulatorData) {
                rtNode.nodeType    = RTNode::NodeType::Modulator;
                rtNode.pitchOffset = modAmount;
            }
            if (nodeType == ValueTreeIdentifiers::TraversalFlagData) {
                rtNode.nodeType = RTNode::NodeType::TraversalFlagData;

                int flagValue = currentValueTree.getProperty(ValueTreeIdentifiers::TraversalFlagValue, 0);
                if (flagValue != 0) {
                    int traversalNumber = flagValue;

                    if (flagValue < 0) {
                        traversalNumber = -flagValue;
                    }


                    rtNode.flagTraversal        = buildRTtraversal(traversalNumber);
                    rtNode.flagRemovesTraversal = (flagValue < 0);

                    if (nodeValueTreeChildren.getNumChildren() > 0) {
                        rtNode.flagTargetId = nodeValueTreeChildren.getChild(0).getProperty(ValueTreeIdentifiers::Id);
                    }
                    else if (!rtNode.flagRemovesTraversal) {
                        rtNode.flagTargetId = rtNode.parentId;
                    }
                }
            }


            for (int i = 0; i < nodeMidiNotes.getNumChildren(); i++) {
                juce::ValueTree note = nodeMidiNotes.getChild(i);
                RTNote rtNote;

                int pitch       = note.getProperty(ValueTreeIdentifiers::MidiPitch);
                int velocity    = note.getProperty(ValueTreeIdentifiers::MidiVelocity);
                int duration    = note.getProperty(ValueTreeIdentifiers::MidiDuration);
                int midiChannel = note.getProperty(ValueTreeIdentifiers::MidiChannel, ValueTreeState::defaultMidiChannel);

                rtNote.pitch       = pitch;
                rtNote.velocity    = velocity;
                rtNote.duration    = duration;
                rtNote.midiChannel = juce::jlimit(1, 16, midiChannel);
                rtNode.notes.push_back(std::move(rtNote));
            }

            if (nodeType != ValueTreeIdentifiers::TraversalFlagData) {
                for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
                    juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
                    int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);
                    juce::ValueTree childDataTree = valueTreeState.getNode(childId);

                    jassert(childDataTree.isValid());
                    stack.push_back(childDataTree);
                }
            }

            builtNodes[nodeId] = std::move(rtNode);
        }
    }
}

void RTGraphBuilder::createRTNodeConnections(NodeBuildMap& builtNodes, std::unordered_map<int, juce::ValueTree>& tempNodeMap)
{
    for (auto& [id, nodeValueTree] : tempNodeMap) {
        if (nodeValueTree.getType() == ValueTreeIdentifiers::TraversalFlagData) {
            continue;
        }

        juce::ValueTree nodeChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

        for (int i = 0; i < nodeChildrenIds.getNumChildren(); i++) {
            juce::ValueTree childIdTree = nodeChildrenIds.getChild(i);
            int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);

            juce::ValueTree childDataTree = valueTreeState.getNode(childId);

            if (!childDataTree.isValid()) {
                continue;
            }

            builtNodes[id].children.push_back(childId);

            if (isTreeJumpConnection(nodeValueTree, childIdTree, childDataTree)) {
                builtNodes[id].treeJumpChildren.insert(childId);
            }

            if (childIdTree.getChildWithName(ValueTreeIdentifiers::DisabledTraversalIds).isValid()) {
                collectDisabledTraversals(childIdTree, builtNodes[id].disabledTraversalsByChild[childId]);
            }
        }
    }
}

RTtraversal RTGraphBuilder::buildRTtraversal(int traversalId)
{
    RTtraversal rtTraversal;
    rtTraversal.traversalId = traversalId;

    juce::ValueTree traversalData = valueTreeState.traversalMap.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);
    if (traversalData.isValid()) {
        rtTraversal.tempoMultiplier = traversalData.getProperty(ValueTreeIdentifiers::TempoMultiplier);

        if (traversalData.hasProperty(ValueTreeIdentifiers::TraversalChannel)) {
            rtTraversal.channel = traversalData.getProperty(ValueTreeIdentifiers::TraversalChannel);
        }
        if (traversalData.hasProperty(ValueTreeIdentifiers::TraversalTranspose)) {
            rtTraversal.transpose = traversalData.getProperty(ValueTreeIdentifiers::TraversalTranspose);
        }
        if (traversalData.hasProperty(ValueTreeIdentifiers::TraversalVelocity)) {
            rtTraversal.velocityMultiplier = traversalData.getProperty(ValueTreeIdentifiers::TraversalVelocity);
        }
    }

    return rtTraversal;
}

void RTGraphBuilder::updateDurationMaps(const std::vector<int>& nodeIds)
{
    if (nodeIds.empty()) {
        return;
    }

    auto edit = processor.snapshots.beginEdit();

    if (edit->globalNodes == nullptr) {
        return;
    }

    edit->globalNodes = std::make_shared<NodeMap>(*edit->globalNodes);

    durationRefreshScratch.clear();

    for (int nodeId : nodeIds) {
        appendOnce(durationRefreshScratch, nodeId);

        const juce::ValueTree parentValueTree = valueTreeState.getNodeParent(nodeId);

        if (parentValueTree.isValid()) {
            appendOnce(durationRefreshScratch,
                       (int) parentValueTree.getProperty(ValueTreeIdentifiers::Id));
        }
    }

    bool refreshedAny = false;

    for (int targetId : durationRefreshScratch) {
        const juce::ValueTree targetTree = valueTreeState.getNode(targetId);
        if (!targetTree.isValid()) {
            continue;
        }

        auto globalNodeIt = edit->globalNodes->find(targetId);
        if (globalNodeIt == edit->globalNodes->end()) {
            continue;
        }

        RTNode refreshed = globalNodeIt->second->clone();
        refreshed.durationMap.clear();
        fillDurationMap(targetTree, refreshed);

        globalNodeIt->second = std::make_shared<const RTNode>(std::move(refreshed));

        refreshedAny = true;
    }

    if (!refreshedAny) {
        return;
    }

    processor.snapshots.publish(std::move(edit));
}

void RTGraphBuilder::rebuildAllGraphs()
{
    for (int i = 0; i < valueTreeState.nodeMap.getNumChildren(); ++i) {
        juce::ValueTree node = valueTreeState.nodeMap.getChild(i);

        if (node.getType() == ValueTreeIdentifiers::RootNodeData) {
            makeRTGraph(node);
        }
    }
}
