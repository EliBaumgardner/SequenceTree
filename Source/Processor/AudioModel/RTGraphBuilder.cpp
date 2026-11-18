#include "RTGraphBuilder.h"

#include <algorithm>
#include <cstdlib>
#include <unordered_set>
#include <utility>

#include "AudioSnapshotPublisher.h"
#include "Processor/Document/Graph/GraphState.h"
#include "Processor/Document/Ornaments/OrnamentLibraries.h"
#include "Processor/Document/Ornaments/OrnamentFigure.h"
#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

RTGraphBuilder::RTGraphBuilder(GraphState& graphState, OrnamentLibraries& ornamentLibraries, AudioSnapshotPublisher& snapshots)
    : graphState       (graphState),
      ornamentLibraries(ornamentLibraries),
      snapshots        (snapshots)
{
    graphState.nodeMap.addListener(this);
    graphState.traversals.map.addListener(this);
    ornamentLibraries.libraries.addListener(this);
}

RTGraphBuilder::~RTGraphBuilder()
{
    graphState.nodeMap.removeListener(this);
    graphState.traversals.map.removeListener(this);
    ornamentLibraries.libraries.removeListener(this);
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

    publishQueuedGraphs();
}

void RTGraphBuilder::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap) {
        const int addedId = child.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        pending.addedNodeIds.insert(addedId);

        triggerAsyncUpdate();
        return;
    }

    rememberStructureChange(parent, child);
}

void RTGraphBuilder::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap) {
        const NodeProperties removed(child);

        const int rootNodeId = removed.rootNodeId;

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

    if (tree.getRoot() == ornamentLibraries.libraries) {
        return;
    }

    if (propertyIdentifier == ArsNovaUI::CanvasIdentifiers::XPosition
        || propertyIdentifier == ArsNovaUI::CanvasIdentifiers::YPosition
        || propertyIdentifier == ValueTreeIdentifiers::Radius) {
        reshapedNode = tree;
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiDuration) {
        graphOwner = tree.getParent().getParent();
        pending.durationRefreshNodeIds.push_back(graphOwner.getProperty(ArsNovaUI::CanvasIdentifiers::Id));
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiPitch
        || propertyIdentifier == ValueTreeIdentifiers::MidiVelocity) {
        graphOwner = tree.getParent().getParent();
    }
    else if (NodePropertyTable::isAudioProperty(propertyIdentifier)) {
        graphOwner = tree;
    }
    else if (tree.getType() == ValueTreeIdentifiers::TraversalData
        && (propertyIdentifier == ValueTreeIdentifiers::TempoMultiplier
         || propertyIdentifier == ValueTreeIdentifiers::TraversalChannel
         || propertyIdentifier == ValueTreeIdentifiers::TraversalTranspose
         || propertyIdentifier == ValueTreeIdentifiers::TraversalVelocity)) {
        const TraversalProperties traversal(tree);

        const int changedTraversalId = traversal.traversalId;

        pending.traversalIds.insert(changedTraversalId);
    }
    else if (propertyIdentifier == ArsNovaUI::CanvasIdentifiers::ArrowTipX
        || propertyIdentifier == ArsNovaUI::CanvasIdentifiers::ArrowTipY) {
        reshapedNode = tree.getParent().getParent();
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowDuration) {
        pending.durationRefreshNodeIds.push_back(tree.getParent().getParent().getProperty(ArsNovaUI::CanvasIdentifiers::Id));
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

    auto edit = snapshots.beginEdit();

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

    snapshots.publish(std::move(edit));
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
        const NodeProperties node(graphState.getNode(nodeId));

        const int rootNodeId = node.rootNodeId;

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

    publishQueuedGraphs();
}

RTtraversal RTGraphBuilder::buildRTtraversal(TraversalKey key)
{
    RTtraversal rtTraversal;
    rtTraversal.key = key;

    juce::ValueTree traversalData = graphState.traversals.map.getChildWithProperty(ValueTreeIdentifiers::TraversalId, key.typeId);

    const TraversalProperties traversal(traversalData);

    if (traversalData.isValid()) {
        const double storedTempoMultiplier = traversal.tempoMultiplier;

        if (storedTempoMultiplier > 0.0) {
            rtTraversal.tempoMultiplier = juce::jlimit(RTtraversal::minimumTempoMultiplier, RTtraversal::maximumTempoMultiplier, storedTempoMultiplier);
        }

        if (! traversal.channel.isUsingDefault()) {
            rtTraversal.channel = traversal.channel;
        }

        if (! traversal.transpose.isUsingDefault()) {
            rtTraversal.transpose = traversal.transpose;
        }

        if (! traversal.velocity.isUsingDefault()) {
            rtTraversal.velocityMultiplier = traversal.velocity;
        }
    }

    return rtTraversal;
}

void RTGraphBuilder::makeRTGraph(const juce::ValueTree& nodeValueTree)
{
    if (!nodeValueTree.isValid()) {
        return;
    }

    if (nodeValueTree.getType() == ValueTreeIdentifiers::TraversalData) {
        const TraversalProperties traversal(nodeValueTree);

        rebuildGraphsForTraversal(traversal.traversalId);

        return;
    }

    if (nodeValueTree.getType() == ArsNovaUI::CanvasIdentifiers::DanglingArrow) {
        makeRTGraph(nodeValueTree.getParent().getParent());
        return;
    }

    const NodeProperties properties(nodeValueTree);

    int rootNodeId = properties.rootNodeId;

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
    queueGraph(rootNodeId, freezeNodes(builtNodes));
}

void RTGraphBuilder::discardGraph(int graphId)
{
    builtGraphIds.erase(graphId);

    queueGraph(graphId, NodeMap{});
}

void RTGraphBuilder::queueGraph(int graphId, NodeMap graphNodes)
{
    std::erase_if(queuedGraphs, [graphId](const GraphUpdate& queued) { return queued.graphId == graphId; });

    queuedGraphs.push_back({ graphId, std::move(graphNodes) });
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
                const NodeProperties root(node);

                const int rootId = root.rootNodeId;

                rootsToRebuild.insert(rootId);
            }
        }
        else if (node.getType() == ValueTreeIdentifiers::TraversalFlagData) {
            const NodeProperties flag(node);

            const int flagValue = flag.traversalFlagValue;
            const int rootId    = flag.rootNodeId;

            if (std::abs(flagValue) == traversalId) {
                rootsToRebuild.insert(rootId);
            }
        }
    }

    for (int rootId : rootsToRebuild) {
        makeRTGraph(graphState.getNode(rootId));
    }
}

void RTGraphBuilder::publishQueuedGraphs()
{
    if (queuedGraphs.empty()) {
        return;
    }

    std::vector<GraphUpdate> updates;

    updates.swap(queuedGraphs);

    snapshots.publishGraphs(std::move(updates));
}

void RTGraphBuilder::createRTNodes(juce::ValueTree rootNodeValueTree, NodeBuildMap& builtNodes, std::unordered_map<int, juce::ValueTree>& tempNodeMap) {
    std::vector<juce::ValueTree> stack = {rootNodeValueTree};

    while(!stack.empty()) {

        juce::ValueTree currentValueTree = stack.back();

        int nodeId = currentValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        stack.pop_back();

        if (tempNodeMap.count(nodeId) != 0) {
            continue;
        }

        const NodeProperties properties(currentValueTree);

        const juce::String ornamentUid = properties.ornamentUid;

        juce::ValueTree nodeValueTreeChildren   = currentValueTree.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);
        juce::ValueTree nodeValueTreeTraversals = currentValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);
        juce::ValueTree nodeMidiNotes           = currentValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
        juce::ValueTree publishedFigure         = ornamentLibraries.findOrnament(ornamentUid).getChildWithName(ValueTreeIdentifiers::PublishedFigure);

        int loopLimit = properties.loopLimit;

        RTNode rtNode;

        tempNodeMap[nodeId]      = currentValueTree;
        rtNode.graphID           = properties.rootNodeId;
        rtNode.nodeID            = nodeId;
        rtNode.nodeType          = GraphState::kindOf(currentValueTree);
        rtNode.countLimit        = properties.countLimit;
        rtNode.triggerLimit      = properties.triggerLimit;
        rtNode.switchCountLimit  = properties.switchCountLimit;
        rtNode.subLoopCountLimit = properties.subLoopCountLimit;
        rtNode.repeatValue       = properties.repeatValue;
        rtNode.probability       = properties.probability;

        const bool isAlternative = NodeInfo::kindTraits[static_cast<int>(rtNode.nodeType)].isAlternative;

        if (rtNode.nodeType == NodeInfo::Kind::Root) {
            rtNode.subLoopCountLimit = loopLimit;
            rtNode.graphLoopLimit    = loopLimit;
        }

        if (isAlternative) {
            rtNode.subLoopCountLimit = GraphState::defaultSubLoopCountLimit;
        }

        if (NodeInfo::kindTraits[static_cast<int>(rtNode.nodeType)].isModulator) {
            rtNode.pitchOffset = properties.modAmount;
        }

        for (int i = 0; i < nodeValueTreeTraversals.getNumChildren(); i++) {
            const TraversalReferenceProperties reference(nodeValueTreeTraversals.getChild(i));

            const TraversalKey key { reference.traversalId, reference.traversalInstance };

            rtNode.traversals.push_back(buildRTtraversal(key));
        }

        juce::ValueTree parentValueTree = graphState.getNodeParent(rtNode.nodeID);

        if (parentValueTree.isValid()) {
            const int parentId = parentValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

            auto parentIt = builtNodes.find(parentId);

            if (parentIt != builtNodes.end()) {
                RTNode& parentNode = parentIt->second;

                rtNode.parentId = parentId;

                if (isAlternative && !NodeInfo::kindTraits[static_cast<int>(parentNode.nodeType)].isAlternative) {
                    parentNode.alternativeRootId = rtNode.nodeID;
                    rtNode.alternativeRootId     = rtNode.nodeID;
                }
                else if (isAlternative) {
                    rtNode.alternativeRootId = parentNode.alternativeRootId;
                }
            }
        }

        fillDurationMap(currentValueTree, rtNode);
        fillEncapsulation(currentValueTree, rtNode);

        if (rtNode.nodeType == NodeInfo::Kind::TraversalFlag) {
            fillTraversalFlag(currentValueTree, rtNode);
        }
        else {
            for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
                juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
                int childId = childIdTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
                juce::ValueTree childDataTree = graphState.getNode(childId);

                if (!childDataTree.isValid()) {
                    continue;
                }

                stack.push_back(childDataTree);
            }
        }

        for (int i = 0; i < nodeMidiNotes.getNumChildren(); i++) {
            const NoteProperties note(nodeMidiNotes.getChild(i));

            const int storedChannel = note.midiChannel;

            RTNote rtNote;

            rtNote.pitch       = note.midiPitch;
            rtNote.velocity    = note.midiVelocity;
            rtNote.duration    = note.midiDuration;
            rtNote.midiChannel = juce::jlimit(NodeInfo::minimumMidiChannel, NodeInfo::maximumMidiChannel, storedChannel);

            rtNode.notes.push_back(std::move(rtNote));
        }

        rtNode.ornament = OrnamentFigure::steps(publishedFigure);

        builtNodes[nodeId] = std::move(rtNode);
    }
}

void RTGraphBuilder::createRTNodeConnections(NodeBuildMap& builtNodes, std::unordered_map<int, juce::ValueTree>& tempNodeMap)
{
    for (auto& [id, nodeValueTree] : tempNodeMap) {
        if (nodeValueTree.getType() == ValueTreeIdentifiers::TraversalFlagData) {
            continue;
        }

        juce::ValueTree nodeChildrenIds = nodeValueTree.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

        for (int i = 0; i < nodeChildrenIds.getNumChildren(); i++) {
            juce::ValueTree childIdTree = nodeChildrenIds.getChild(i);
            int childId = childIdTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

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

    const int centreX = nodeValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
    const int centreY = nodeValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);

    auto durationTo = [&](const juce::ValueTree& connection, const juce::ValueTree& other) {
        const int deltaX = static_cast<int>(other.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition)) - centreX;
        const int deltaY = static_cast<int>(other.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition)) - centreY;

        return ArrowInfo::Settings::durationFromDelta(ArrowBindingOps::getArrowInfo(connection), deltaX, deltaY);
    };

    if (isAlternative) {
        juce::ValueTree parent = graphState.getNodeParent(rtNode.nodeID);

        if (parent.isValid()) {
            const int parentId = parent.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

            rtNode.alternativeArrowDuration = durationTo(graphState.getConnection(parentId, rtNode.nodeID), parent);
        }
    }

    if (nodeValueTree.getType() != ValueTreeIdentifiers::TraversalFlagData) {
        juce::ValueTree childIds = nodeValueTree.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

        for (int i = 0; i < childIds.getNumChildren(); i++) {
            juce::ValueTree childIdTree = childIds.getChild(i);

            const int childId = childIdTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
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

    juce::ValueTree danglingArrows = nodeValueTree.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

    for (int i = 0; i < danglingArrows.getNumChildren(); i++) {
        juce::ValueTree arrowTree = danglingArrows.getChild(i);

        const int tipX = arrowTree.getProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX);
        const int tipY = arrowTree.getProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY);

        const ArrowProperties arrow(arrowTree);

        RTNode::DanglingArrow dangling;

        dangling.duration   = ArrowInfo::Settings::durationFromDelta(ArrowBindingOps::getArrowInfo(arrowTree), tipX, tipY);
        dangling.countLimit = arrow.countLimit;

        collectDisabledTraversals(arrowTree, dangling.disabledTraversals);

        rtNode.danglingArrows.push_back(std::move(dangling));
    }
}

void RTGraphBuilder::fillEncapsulation(const juce::ValueTree& nodeValueTree, RTNode& rtNode)
{
    const NodeProperties properties(nodeValueTree);

    const int encapsulatorId = properties.encapsulatorId;

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
    const NodeProperties flag(nodeValueTree);

    const int flagValue    = flag.traversalFlagValue;
    const int flagInstance = flag.traversalInstance;

    juce::ValueTree flagChildren = nodeValueTree.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

    if (flagValue == 0) {
        return;
    }

    rtNode.flag.traversal        = buildRTtraversal({ std::abs(flagValue), flagInstance });
    rtNode.flag.removesTraversal = flagValue < 0;

    if (flagChildren.getNumChildren() > 0) {
        rtNode.flag.targetId = flagChildren.getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    }
    else if (!rtNode.flag.removesTraversal) {
        rtNode.flag.targetId = rtNode.parentId;
    }
}

void RTGraphBuilder::rememberStructureChange(const juce::ValueTree& parent, const juce::ValueTree& child)
{
    juce::ValueTree graphOwner;
    juce::ValueTree reshapedNode;

    if (parent.getRoot() == ornamentLibraries.libraries) {
        if (child.getType() == ValueTreeIdentifiers::OrnamentLibrary
            || child.getType() == ValueTreeIdentifiers::OrnamentData
            || child.getType() == ValueTreeIdentifiers::PublishedFigure) {
            pending.ornamentsChanged = true;

            triggerAsyncUpdate();
        }

        return;
    }

    if (parent.getType() == ArsNovaUI::CanvasIdentifiers::NodeChildrenIds) {
        graphOwner = parent.getParent();
    }
    else if (parent.getType() == ValueTreeIdentifiers::TraversalChildrenIds
          || parent.getType() == ValueTreeIdentifiers::DisabledTraversalIds) {
        graphOwner = parent;
    }
    else if (child.getType() == ArsNovaUI::CanvasIdentifiers::DanglingArrows) {
        reshapedNode = parent;
    }
    else if (child.getType() == ArsNovaUI::CanvasIdentifiers::DanglingArrow) {
        reshapedNode = parent.getParent();
    }

    rememberOwners(graphOwner, reshapedNode);
}

void RTGraphBuilder::rememberOwners(juce::ValueTree graphOwner, const juce::ValueTree& reshapedNode)
{
    while (graphOwner.isValid()) {
        const NodeProperties owner(graphOwner);

        if (! owner.rootNodeId.isUsingDefault()) {
            break;
        }

        graphOwner = graphOwner.getParent();
    }

    if (graphOwner.isValid()) {
        const int ownerId = graphOwner.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        pending.rebuildNodeIds.insert(ownerId);
    }

    if (reshapedNode.isValid()) {
        pending.durationRefreshNodeIds.push_back(reshapedNode.getProperty(ArsNovaUI::CanvasIdentifiers::Id));
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
        const TraversalReferenceProperties entry(disabledTraversals.getChild(i));

        disabledKeys.push_back({ entry.traversalId, entry.traversalInstance });
    }
}

void RTGraphBuilder::classifyRootConnection(const juce::ValueTree& parentValueTree,
                                            const juce::ValueTree& childIdTree,
                                            const juce::ValueTree& childValueTree,
                                            RTConnection& connection)
{
    const NodeProperties parentProperties(parentValueTree);

    const ArrowProperties arrow(childIdTree);

    const int childId   = childValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int arrowType = arrow.arrowType;

    const bool isRootChild          = childValueTree.getType() == ValueTreeIdentifiers::RootNodeData;
    const bool isOwnRoot            = parentProperties.rootNodeId == childId;
    const bool isOtherRoot          = isRootChild && ! isOwnRoot;
    const bool isTraversalArrow     = arrowType == static_cast<int>(ArrowInfo::Type::Traversal);
    const bool isCrossRootTreeArrow = arrowType == static_cast<int>(ArrowInfo::Type::CrossRootTree);
    const bool isNodeArrow          = arrowType == static_cast<int>(ArrowInfo::Type::Node);

    connection.isTreeJump  = isOtherRoot && isTraversalArrow;
    connection.isCrossRoot = isOtherRoot && (isCrossRootTreeArrow || isNodeArrow);
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
