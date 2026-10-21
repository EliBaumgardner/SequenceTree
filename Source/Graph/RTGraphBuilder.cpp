/*
  ==============================================================================

    RTGraphBuilder.cpp
    Created: 30 Apr 2026
    Author:  Eli Baumgardner

  ==============================================================================
*/

#include "RTGraphBuilder.h"

#include <algorithm>
#include <cstdlib>
#include <unordered_set>
#include <utility>

#include "../Plugin/PluginProcessor.h"
#include "GraphState.h"
#include "ValueTreeIdentifiers.h"

RTGraphBuilder::RTGraphBuilder(SequenceTreeAudioProcessor& processorRef, GraphState& valueTreeStateRef)
    : processor(processorRef), graphState(valueTreeStateRef)
{
    graphState.nodeMap.addListener(this);
    graphState.traversals.map.addListener(this);
    processor.ornamentLibraries.libraries.addListener(this);
}

RTGraphBuilder::~RTGraphBuilder()
{
    graphState.nodeMap.removeListener(this);
    graphState.traversals.map.removeListener(this);
    processor.ornamentLibraries.libraries.removeListener(this);
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

    if (nodeValueTree.getType() == ValueTreeIdentifiers::DanglingArrow) {
        makeRTGraph(nodeValueTree.getParent().getParent());
        return;
    }

    int rootNodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    if (rootNodeId == 0) {
        return;
    }

    juce::ValueTree rootNodeValueTree = graphState.getNode(rootNodeId);

    if (!rootNodeValueTree.isValid()) {
        discardGraph(rootNodeId);
        return;
    }

    std::unordered_map<int,juce::ValueTree> tempNodeMap;

    NodeBuildMap builtNodes;

    createRTNodes(rootNodeValueTree, builtNodes, tempNodeMap);
    createRTNodeConnections(builtNodes, tempNodeMap);

    builtGraphIds.insert(rootNodeId);
    processor.snapshots.publishGraph(rootNodeId, freezeNodes(builtNodes));
}

void RTGraphBuilder::discardGraph(int graphId)
{
    builtGraphIds.erase(graphId);

    processor.snapshots.publishGraph(graphId, NodeMap{});
}

void RTGraphBuilder::rebuildAllGraphs()
{
    std::vector<int> rootlessGraphIds;

    for (const int graphId : builtGraphIds) {
        if (!graphState.getNode(graphId).isValid()) {
            rootlessGraphIds.push_back(graphId);
        }
    }

    for (const int graphId : rootlessGraphIds) {
        discardGraph(graphId);
    }

    for (int i = 0; i < graphState.nodeMap.getNumChildren(); ++i) {
        juce::ValueTree node = graphState.nodeMap.getChild(i);

        if (node.getType() == ValueTreeIdentifiers::RootNodeData) {
            makeRTGraph(node);
        }
    }
}

void RTGraphBuilder::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap) {
        pending.addedNodeIds.insert(static_cast<int>(child.getProperty(ValueTreeIdentifiers::Id)));
        triggerAsyncUpdate();
        return;
    }

    rememberStructureChange(parent, child);
}

void RTGraphBuilder::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap) {
        const int rootNodeId = child.getProperty(ValueTreeIdentifiers::RootNodeId);

        if (rootNodeId != 0) {
            pending.rebuildRootIds.insert(rootNodeId);
            triggerAsyncUpdate();
        }

        return;
    }

    rememberStructureChange(parent, child);
}

void RTGraphBuilder::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& propertyIdentifier)
{
    juce::ValueTree graphOwner;
    juce::ValueTree reshapedNode;

    if (tree.getRoot() == processor.ornamentLibraries.libraries) {
        return;
    }

    if (propertyIdentifier == ValueTreeIdentifiers::XPosition
        || propertyIdentifier == ValueTreeIdentifiers::YPosition
        || propertyIdentifier == ValueTreeIdentifiers::Radius) {
        reshapedNode = tree;
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiDuration) {
        graphOwner = tree.getParent().getParent();
        pending.durationRefreshNodeIds.push_back(graphOwner.getProperty(ValueTreeIdentifiers::Id));
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiPitch
        || propertyIdentifier == ValueTreeIdentifiers::MidiVelocity) {
        graphOwner = tree.getParent().getParent();
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiChannel
        || propertyIdentifier == ValueTreeIdentifiers::CountLimit
        || propertyIdentifier == ValueTreeIdentifiers::TriggerLimit
        || propertyIdentifier == ValueTreeIdentifiers::LoopLimit
        || propertyIdentifier == ValueTreeIdentifiers::SwitchCountLimit
        || propertyIdentifier == ValueTreeIdentifiers::SubLoopCountLimit
        || propertyIdentifier == ValueTreeIdentifiers::RepeatValue
        || propertyIdentifier == ValueTreeIdentifiers::Probability
        || propertyIdentifier == ValueTreeIdentifiers::ModAmount
        || propertyIdentifier == ValueTreeIdentifiers::TraversalFlagValue
        || propertyIdentifier == ValueTreeIdentifiers::EncapsulatorId
        || propertyIdentifier == ValueTreeIdentifiers::OrnamentUid) {
        graphOwner = tree;
    }
    else if (tree.getType() == ValueTreeIdentifiers::TraversalData
        && (propertyIdentifier == ValueTreeIdentifiers::TempoMultiplier
         || propertyIdentifier == ValueTreeIdentifiers::TraversalChannel
         || propertyIdentifier == ValueTreeIdentifiers::TraversalTranspose
         || propertyIdentifier == ValueTreeIdentifiers::TraversalVelocity)) {
        pending.traversalIds.insert(static_cast<int>(tree.getProperty(ValueTreeIdentifiers::TraversalId)));
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowTipX
        || propertyIdentifier == ValueTreeIdentifiers::ArrowTipY) {
        reshapedNode = tree.getParent().getParent();
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowDuration) {
        pending.durationRefreshNodeIds.push_back(tree.getParent().getParent().getProperty(ValueTreeIdentifiers::Id));
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowType
        || propertyIdentifier == ValueTreeIdentifiers::ArrowSync
        || propertyIdentifier == ValueTreeIdentifiers::ArrowXBinding
        || propertyIdentifier == ValueTreeIdentifiers::ArrowYBinding
        || propertyIdentifier == ValueTreeIdentifiers::ArrowXMultiplier
        || propertyIdentifier == ValueTreeIdentifiers::ArrowYMultiplier) {
        graphOwner = tree.getParent().getParent();
    }

    rememberOwners(graphOwner, reshapedNode);
}

void RTGraphBuilder::updateDurationMaps(std::span<const int> nodeIds)
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
        if (std::ranges::find(durationRefreshScratch, nodeId) == durationRefreshScratch.end()) {
            durationRefreshScratch.push_back(nodeId);
        }

        const auto parents = graphState.parentIdsOf.find(nodeId);

        if (parents == graphState.parentIdsOf.end()) {
            continue;
        }

        for (const int parentId : parents->second) {
            if (std::ranges::find(durationRefreshScratch, parentId) == durationRefreshScratch.end()) {
                durationRefreshScratch.push_back(parentId);
            }
        }
    }

    bool refreshedAny = false;

    for (int targetId : durationRefreshScratch) {
        const juce::ValueTree targetTree = graphState.getNode(targetId);
        if (!targetTree.isValid()) {
            continue;
        }

        auto& globalNodes    = edit->globalNodes->sortedById;
        const auto globalNode = std::ranges::lower_bound(globalNodes, targetId, {}, &RTNode::nodeID);

        if (globalNode == globalNodes.end() || globalNode->nodeID != targetId) {
            continue;
        }

        fillDurationMap(targetTree, *globalNode);

        refreshedAny = true;
    }

    if (!refreshedAny) {
        return;
    }

    processor.snapshots.publish(std::move(edit));
}

void RTGraphBuilder::handleAsyncUpdate()
{
    PendingChanges changes;
    std::swap(changes, pending);

    for (int nodeId : changes.addedNodeIds) {
        if (! graphState.getNode(nodeId).isValid()) {
            continue;
        }

        changes.rebuildNodeIds.insert(nodeId);

        auto parentsIt = graphState.parentIdsOf.find(nodeId);

        if (parentsIt != graphState.parentIdsOf.end()) {
            changes.rebuildNodeIds.insert(parentsIt->second.begin(), parentsIt->second.end());
        }
    }

    for (int nodeId : changes.rebuildNodeIds) {
        const int rootNodeId = graphState.getNode(nodeId).getProperty(ValueTreeIdentifiers::RootNodeId);

        if (rootNodeId != 0) {
            changes.rebuildRootIds.insert(rootNodeId);
        }
    }

    for (int rootNodeId : changes.rebuildRootIds) {
        const juce::ValueTree rootNodeTree = graphState.getNode(rootNodeId);

        if (rootNodeTree.isValid()) {
            makeRTGraph(rootNodeTree);
        }
        else {
            discardGraph(rootNodeId);
        }
    }

    for (int traversalId : changes.traversalIds) {
        makeRTGraph(graphState.traversals.map.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId));
    }

    updateDurationMaps(changes.durationRefreshNodeIds);

    if (changes.ornamentsChanged) {
        rebuildAllGraphs();
    }
}

RTtraversal RTGraphBuilder::buildRTtraversal(TraversalKey key)
{
    RTtraversal rtTraversal;
    rtTraversal.key = key;

    juce::ValueTree traversalData = graphState.traversals.map.getChildWithProperty(ValueTreeIdentifiers::TraversalId, key.typeId);
    if (traversalData.isValid()) {
        const double storedTempoMultiplier = traversalData.getProperty(ValueTreeIdentifiers::TempoMultiplier);

        if (storedTempoMultiplier > 0.0) {
            rtTraversal.tempoMultiplier = juce::jlimit(RTtraversal::minimumTempoMultiplier, RTtraversal::maximumTempoMultiplier, storedTempoMultiplier);
        }

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

void RTGraphBuilder::rebuildGraphsForTraversal(int traversalId)
{
    juce::ValueTree nodeMap = graphState.nodeMap;

    std::unordered_set<int> rootsToRebuild;

    for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
        juce::ValueTree node = nodeMap.getChild(i);

        if (node.getType() == ValueTreeIdentifiers::RootNodeData) {
            juce::ValueTree traversalChildrenIds = node.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

            if (traversalChildrenIds.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId).isValid()) {
                rootsToRebuild.insert(static_cast<int>(node.getProperty(ValueTreeIdentifiers::RootNodeId)));
            }
        }
        else if (node.getType() == ValueTreeIdentifiers::TraversalFlagData) {
            const int flagValue = node.getProperty(ValueTreeIdentifiers::TraversalFlagValue, 0);

            if (std::abs(flagValue) == traversalId) {
                rootsToRebuild.insert(static_cast<int>(node.getProperty(ValueTreeIdentifiers::RootNodeId)));
            }
        }
    }

    for (int rootId : rootsToRebuild) {
        makeRTGraph(graphState.getNode(rootId));
    }
}

void RTGraphBuilder::createRTNodes(juce::ValueTree rootNodeValueTree, NodeBuildMap& builtNodes, std::unordered_map<int, juce::ValueTree>& tempNodeMap) {
    std::vector<juce::ValueTree> stack = {rootNodeValueTree};

    while(!stack.empty()) {

        juce::ValueTree currentValueTree = stack.back();

        int nodeId = currentValueTree.getProperty(ValueTreeIdentifiers::Id);

        stack.pop_back();

        if (tempNodeMap.count(nodeId) != 0) {
            continue;
        }

        juce::ValueTree nodeValueTreeChildren   = currentValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        juce::ValueTree nodeValueTreeTraversals = currentValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);
        juce::ValueTree nodeMidiNotes           = currentValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
        juce::ValueTree publishedFigure         = processor.ornamentLibraries.findOrnament(currentValueTree.getProperty(ValueTreeIdentifiers::OrnamentUid).toString())
                                                                             .getChildWithName(ValueTreeIdentifiers::PublishedFigure);

        int loopLimit = currentValueTree.getProperty(ValueTreeIdentifiers::LoopLimit, GraphState::defaultRootLoopLimit);

        RTNode rtNode;

        tempNodeMap[nodeId] = currentValueTree;

        rtNode.graphID  = currentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
        rtNode.nodeID   = nodeId;
        rtNode.nodeType = rtNodeTypeFor(currentValueTree.getType());

        rtNode.countLimit        = currentValueTree.getProperty(ValueTreeIdentifiers::CountLimit);
        rtNode.triggerLimit      = currentValueTree.getProperty(ValueTreeIdentifiers::TriggerLimit, GraphState::defaultTriggerLimit);
        rtNode.switchCountLimit  = currentValueTree.getProperty(ValueTreeIdentifiers::SwitchCountLimit);
        rtNode.subLoopCountLimit = currentValueTree.getProperty(ValueTreeIdentifiers::SubLoopCountLimit);
        rtNode.repeatValue       = currentValueTree.getProperty(ValueTreeIdentifiers::RepeatValue, GraphState::defaultRepeatValue);
        rtNode.probability       = currentValueTree.getProperty(ValueTreeIdentifiers::Probability, GraphState::defaultProbability);

        rtNode.isAlternativeNode = rtNode.nodeType == RTNode::NodeType::Alternative || rtNode.nodeType == RTNode::NodeType::AlternativeModulator;

        if (rtNode.nodeType == RTNode::NodeType::RootNode) {
            rtNode.subLoopCountLimit = loopLimit;
            rtNode.graphLoopLimit    = loopLimit;
        }

        if (rtNode.isAlternativeNode) {
            rtNode.subLoopCountLimit = GraphState::defaultSubLoopCountLimit;
        }

        if (rtNode.nodeType == RTNode::NodeType::ModulatorRoot || rtNode.nodeType == RTNode::NodeType::Modulator
            || rtNode.nodeType == RTNode::NodeType::AlternativeModulator) {
            rtNode.pitchOffset = currentValueTree.getProperty(ValueTreeIdentifiers::ModAmount, GraphState::defaultModAmount);
        }

        for (int i = 0; i < nodeValueTreeTraversals.getNumChildren(); i++) {
            juce::ValueTree traversalIdTree = nodeValueTreeTraversals.getChild(i);

            const TraversalKey key { static_cast<int>(traversalIdTree.getProperty(ValueTreeIdentifiers::TraversalId)),
                                     static_cast<int>(traversalIdTree.getProperty(ValueTreeIdentifiers::TraversalInstance, 0)) };

            rtNode.traversals.push_back(buildRTtraversal(key));
        }

        juce::ValueTree parentValueTree = graphState.getNodeParent(rtNode.nodeID);

        if (parentValueTree.isValid()) {
            const int parentId = parentValueTree.getProperty(ValueTreeIdentifiers::Id);

            auto parentIt = builtNodes.find(parentId);

            if (parentIt != builtNodes.end()) {
                RTNode& parentNode = parentIt->second;

                rtNode.parentId = parentId;

                if (rtNode.isAlternativeNode && !parentNode.isAlternativeNode) {
                    parentNode.alternativeRootId = rtNode.nodeID;
                    rtNode.alternativeRootId     = rtNode.nodeID;
                }
                else if (rtNode.isAlternativeNode) {
                    rtNode.alternativeRootId = parentNode.alternativeRootId;
                }
            }
        }

        fillDurationMap(currentValueTree, rtNode);
        fillEncapsulation(currentValueTree, rtNode);

        if (rtNode.nodeType == RTNode::NodeType::TraversalFlagData) {
            fillTraversalFlag(currentValueTree, rtNode);
        }
        else {
            for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
                juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
                int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);
                juce::ValueTree childDataTree = graphState.getNode(childId);

                jassert(childDataTree.isValid());
                stack.push_back(childDataTree);
            }
        }

        for (int i = 0; i < nodeMidiNotes.getNumChildren(); i++) {
            juce::ValueTree note = nodeMidiNotes.getChild(i);
            RTNote rtNote;

            rtNote.pitch       = note.getProperty(ValueTreeIdentifiers::MidiPitch);
            rtNote.velocity    = note.getProperty(ValueTreeIdentifiers::MidiVelocity);
            rtNote.duration    = note.getProperty(ValueTreeIdentifiers::MidiDuration);
            rtNote.midiChannel = juce::jlimit(1, 16, static_cast<int>(note.getProperty(ValueTreeIdentifiers::MidiChannel, GraphState::defaultMidiChannel)));
            rtNode.notes.push_back(std::move(rtNote));
        }

        rtNode.ornament = processor.ornamentLibraries.figureSteps(publishedFigure);

        builtNodes[nodeId] = std::move(rtNode);
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

            juce::ValueTree childDataTree = graphState.getNode(childId);

            if (!childDataTree.isValid()) {
                continue;
            }

            RTConnection& connection = connectionFor(builtNodes[id], childId);

            classifyRootConnection(nodeValueTree, childIdTree, childDataTree, connection);

            connection.isSynced = ArrowBindingOps::getArrowInfo(childIdTree).isSynced;

            collectDisabledTraversals(childIdTree, connection.disabledTraversals);
        }
    }
}

void RTGraphBuilder::fillDurationMap(const juce::ValueTree& nodeValueTree, RTNode& rtNode)
{
    for (RTConnection& connection : rtNode.connections) {
        connection.duration = -1;
    }

    rtNode.alternativeArrowDuration = -1;
    rtNode.danglingArrows.clear();

    const bool isAlternative = (nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeNodeData
                             || nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeModulatorData);

    const int centreX = nodeValueTree.getProperty(ValueTreeIdentifiers::XPosition);
    const int centreY = nodeValueTree.getProperty(ValueTreeIdentifiers::YPosition);

    auto durationTo = [&](const juce::ValueTree& connection, const juce::ValueTree& other) {
        const int deltaX = static_cast<int>(other.getProperty(ValueTreeIdentifiers::XPosition)) - centreX;
        const int deltaY = static_cast<int>(other.getProperty(ValueTreeIdentifiers::YPosition)) - centreY;

        return ArrowInfo::durationFromDelta(ArrowBindingOps::getArrowInfo(connection), deltaX, deltaY);
    };

    if (isAlternative) {
        juce::ValueTree parent = graphState.getNodeParent(rtNode.nodeID);

        if (parent.isValid()) {
            const int parentId = parent.getProperty(ValueTreeIdentifiers::Id);

            rtNode.alternativeArrowDuration = durationTo(graphState.getConnection(parentId, rtNode.nodeID), parent);
        }
    }

    if (nodeValueTree.getType() != ValueTreeIdentifiers::TraversalFlagData) {
        juce::ValueTree childIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

        for (int i = 0; i < childIds.getNumChildren(); i++) {
            juce::ValueTree childIdTree = childIds.getChild(i);

            const int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);
            juce::ValueTree childTree = graphState.getNode(childId);

            if (!childTree.isValid()) {
                continue;
            }

            RTConnection& connection = connectionFor(rtNode, childId);

            if (childTree.getType() != ValueTreeIdentifiers::AlternativeNodeData
                && childTree.getType() != ValueTreeIdentifiers::AlternativeModulatorData) {
                connection.duration = durationTo(childIdTree, childTree);
            }
        }
    }

    juce::ValueTree danglingArrows = nodeValueTree.getChildWithName(ValueTreeIdentifiers::DanglingArrows);

    for (int i = 0; i < danglingArrows.getNumChildren(); i++) {
        juce::ValueTree arrowTree = danglingArrows.getChild(i);

        const int tipX = arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipX);
        const int tipY = arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipY);

        RTNode::DanglingArrow dangling;

        dangling.duration   = ArrowInfo::durationFromDelta(ArrowBindingOps::getArrowInfo(arrowTree), tipX, tipY);
        dangling.countLimit = arrowTree.getProperty(ValueTreeIdentifiers::CountLimit, GraphState::defaultNodeCountLimit);

        collectDisabledTraversals(arrowTree, dangling.disabledTraversals);

        rtNode.danglingArrows.push_back(std::move(dangling));
    }
}

void RTGraphBuilder::fillEncapsulation(const juce::ValueTree& nodeValueTree, RTNode& rtNode)
{
    const int encapsulatorId = nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    const juce::ValueTree encapsulator = graphState.getNode(encapsulatorId);

    if (! encapsulator.isValid()) {
        return;
    }

    const std::vector<int> memberNodeIds = graphState.encapsulation.memberIds(encapsulatorId);

    if (memberNodeIds.empty()) {
        return;
    }

    rtNode.encapsulationEntryId = memberNodeIds.front();
}

void RTGraphBuilder::fillTraversalFlag(const juce::ValueTree& nodeValueTree, RTNode& rtNode)
{
    const int flagValue    = nodeValueTree.getProperty(ValueTreeIdentifiers::TraversalFlagValue, 0);
    const int flagInstance = nodeValueTree.getProperty(ValueTreeIdentifiers::TraversalInstance, 0);

    juce::ValueTree flagChildren = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    if (flagValue == 0) {
        return;
    }

    rtNode.flagTraversal        = buildRTtraversal({ std::abs(flagValue), flagInstance });
    rtNode.flagRemovesTraversal = flagValue < 0;

    if (flagChildren.getNumChildren() > 0) {
        rtNode.flagTargetId = flagChildren.getChild(0).getProperty(ValueTreeIdentifiers::Id);
    }
    else if (!rtNode.flagRemovesTraversal) {
        rtNode.flagTargetId = rtNode.parentId;
    }
}

void RTGraphBuilder::rememberStructureChange(const juce::ValueTree& parent, const juce::ValueTree& child)
{
    juce::ValueTree graphOwner;
    juce::ValueTree reshapedNode;

    if (parent.getRoot() == processor.ornamentLibraries.libraries) {
        if (child.getType() == ValueTreeIdentifiers::OrnamentLibrary
            || child.getType() == ValueTreeIdentifiers::OrnamentData
            || child.getType() == ValueTreeIdentifiers::PublishedFigure) {
            pending.ornamentsChanged = true;

            triggerAsyncUpdate();
        }

        return;
    }

    if (parent.getType() == ValueTreeIdentifiers::NodeChildrenIds) {
        graphOwner = parent.getParent();
    }
    else if (parent.getType() == ValueTreeIdentifiers::TraversalChildrenIds
          || parent.getType() == ValueTreeIdentifiers::DisabledTraversalIds) {
        graphOwner = parent;
    }
    else if (child.getType() == ValueTreeIdentifiers::DanglingArrows) {
        reshapedNode = parent;
    }
    else if (child.getType() == ValueTreeIdentifiers::DanglingArrow) {
        reshapedNode = parent.getParent();
    }

    rememberOwners(graphOwner, reshapedNode);
}

void RTGraphBuilder::rememberOwners(juce::ValueTree graphOwner, const juce::ValueTree& reshapedNode)
{
    while (graphOwner.isValid() && ! graphOwner.hasProperty(ValueTreeIdentifiers::RootNodeId)) {
        graphOwner = graphOwner.getParent();
    }

    if (graphOwner.isValid()) {
        pending.rebuildNodeIds.insert(static_cast<int>(graphOwner.getProperty(ValueTreeIdentifiers::Id)));
    }

    if (reshapedNode.isValid()) {
        pending.durationRefreshNodeIds.push_back(reshapedNode.getProperty(ValueTreeIdentifiers::Id));
    }

    triggerAsyncUpdate();
}

void RTGraphBuilder::collectDisabledTraversals(const juce::ValueTree& owner, std::vector<TraversalKey>& disabledKeys)
{
    juce::ValueTree disabledTraversals = owner.getChildWithName(ValueTreeIdentifiers::DisabledTraversalIds);

    if (!disabledTraversals.isValid()) {
        return;
    }

    for (int i = 0; i < disabledTraversals.getNumChildren(); i++) {
        const juce::ValueTree entry = disabledTraversals.getChild(i);

        disabledKeys.push_back({ static_cast<int>(entry.getProperty(ValueTreeIdentifiers::TraversalId)),
                                 static_cast<int>(entry.getProperty(ValueTreeIdentifiers::TraversalInstance, 0)) });
    }
}

void RTGraphBuilder::classifyRootConnection(const juce::ValueTree& parentValueTree,
                                            const juce::ValueTree& childIdTree,
                                            const juce::ValueTree& childValueTree,
                                            RTConnection& connection)
{
    connection.isTreeJump  = false;
    connection.isCrossRoot = false;

    if (childValueTree.getType() != ValueTreeIdentifiers::RootNodeData) {
        return;
    }

    const int childId = childValueTree.getProperty(ValueTreeIdentifiers::Id);

    if (static_cast<int>(parentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId)) == childId) {
        return;
    }

    const int arrowType = childIdTree.getProperty(ValueTreeIdentifiers::ArrowType, static_cast<int>(ArrowType::Node));

    connection.isTreeJump  = arrowType == static_cast<int>(ArrowType::Traversal);
    connection.isCrossRoot = arrowType == static_cast<int>(ArrowType::CrossRootTree)
                          || arrowType == static_cast<int>(ArrowType::Node);
}

RTConnection& RTGraphBuilder::connectionFor(RTNode& node, int childId)
{
    for (RTConnection& connection : node.connections) {
        if (connection.childId == childId) {
            return connection;
        }
    }

    node.connections.emplace_back();
    node.connections.back().childId = childId;

    return node.connections.back();
}

RTNode::NodeType RTGraphBuilder::rtNodeTypeFor(const juce::Identifier& valueTreeType)
{
    if (valueTreeType == ValueTreeIdentifiers::AlternativeNodeData) {
        return RTNode::NodeType::Alternative;
    }
    if (valueTreeType == ValueTreeIdentifiers::RootNodeData) {
        return RTNode::NodeType::RootNode;
    }
    if (valueTreeType == ValueTreeIdentifiers::ModulatorRootData) {
        return RTNode::NodeType::ModulatorRoot;
    }
    if (valueTreeType == ValueTreeIdentifiers::ModulatorData) {
        return RTNode::NodeType::Modulator;
    }
    if (valueTreeType == ValueTreeIdentifiers::AlternativeModulatorData) {
        return RTNode::NodeType::AlternativeModulator;
    }
    if (valueTreeType == ValueTreeIdentifiers::TraversalFlagData) {
        return RTNode::NodeType::TraversalFlagData;
    }

    return RTNode::NodeType::Node;
}

NodeMap RTGraphBuilder::freezeNodes(NodeBuildMap& source)
{
    NodeMap frozen;
    frozen.sortedById.reserve(source.size());

    for (auto& [nodeId, node] : source) {
        frozen.sortedById.push_back(std::move(node));
    }

    std::ranges::sort(frozen.sortedById, {}, &RTNode::nodeID);

    return frozen;
}
