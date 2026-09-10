#include "GraphState.h"
#include "ValueTreeIdentifiers.h"

#include <algorithm>
#include <unordered_set>

GraphState::GraphState()
{
    nodeMap = juce::ValueTree(ValueTreeIdentifiers::NodeMap);

    nodeMap.addListener(this);
}

GraphState::~GraphState()
{
    nodeMap.removeListener(this);
}

void GraphState::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap) {
        indexNode(child);
    }
    else if (parent.getType() == ValueTreeIdentifiers::NodeChildrenIds) {
        linkParent(parent.getParent().getProperty(ValueTreeIdentifiers::Id),
                   child.getProperty(ValueTreeIdentifiers::Id));
    }
}

void GraphState::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex)
{
    juce::ignoreUnused(childIndex);

    if (parent.getType() == ValueTreeIdentifiers::NodeMap) {
        unindexNode(child);
    }
    else if (parent.getType() == ValueTreeIdentifiers::NodeChildrenIds) {
        unlinkParent(parent.getParent().getProperty(ValueTreeIdentifiers::Id),
                     child.getProperty(ValueTreeIdentifiers::Id));
    }
}

void GraphState::indexNode(const juce::ValueTree& node)
{
    const int nodeId = node.getProperty(ValueTreeIdentifiers::Id);

    nodeIndex[nodeId] = node;

    const juce::ValueTree childIds = node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < childIds.getNumChildren(); ++i) {
        linkParent(nodeId, childIds.getChild(i).getProperty(ValueTreeIdentifiers::Id));
    }
}

void GraphState::unindexNode(const juce::ValueTree& node)
{
    const int nodeId = node.getProperty(ValueTreeIdentifiers::Id);

    nodeIndex.erase(nodeId);

    const juce::ValueTree childIds = node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < childIds.getNumChildren(); ++i) {
        unlinkParent(nodeId, childIds.getChild(i).getProperty(ValueTreeIdentifiers::Id));
    }
}

void GraphState::linkParent(int parentNodeId, int childNodeId)
{
    std::vector<int>& parents = parentIdsOf[childNodeId];

    if (std::find(parents.begin(), parents.end(), parentNodeId) == parents.end()) {
        parents.push_back(parentNodeId);
    }
}

void GraphState::unlinkParent(int parentNodeId, int childNodeId)
{
    const auto entry = parentIdsOf.find(childNodeId);

    if (entry == parentIdsOf.end()) {
        return;
    }

    std::vector<int>& parents = entry->second;

    parents.erase(std::remove(parents.begin(), parents.end(), parentNodeId), parents.end());

    if (parents.empty()) {
        parentIdsOf.erase(entry);
    }
}

void GraphState::replaceState(const juce::ValueTree& restoredNodeMap,
                              const juce::ValueTree& restoredTraversalMap)
{
    traversals.map.removeAllChildren(nullptr);
    nodeMap.removeAllChildren(nullptr);

    for (int i = 0; i < restoredTraversalMap.getNumChildren(); ++i) {
        traversals.map.addChild(restoredTraversalMap.getChild(i).createCopy(), -1, nullptr);
    }

    for (int i = 0; i < restoredNodeMap.getNumChildren(); ++i) {
        nodeMap.addChild(restoredNodeMap.getChild(i).createCopy(), -1, nullptr);
    }

    nodeIdIncrement = 0;

    for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
        const int id = nodeMap.getChild(i).getProperty(ValueTreeIdentifiers::Id);

        if (id > nodeIdIncrement) {
            nodeIdIncrement = id;
        }
    }
}

void GraphState::setNodeLimitProperties(juce::ValueTree node, juce::UndoManager* undoManager)
{
    node.setProperty(ValueTreeIdentifiers::CountLimit,        defaultNodeCountLimit,    undoManager);
    node.setProperty(ValueTreeIdentifiers::TriggerLimit,      defaultTriggerLimit,      undoManager);
    node.setProperty(ValueTreeIdentifiers::SwitchCountLimit,  defaultSwitchCountLimit,  undoManager);
    node.setProperty(ValueTreeIdentifiers::SubLoopCountLimit, defaultSubLoopCountLimit, undoManager);

    node.setProperty(ValueTreeIdentifiers::RepeatValue,       defaultRepeatValue,       undoManager);
    node.setProperty(ValueTreeIdentifiers::Probability,       defaultProbability,       undoManager);
}

juce::ValueTree GraphState::addRootNode(juce::UndoManager* undoManager)
{
    nodeIdIncrement = nodeIdIncrement + 1;

    const int rootId = nodeIdIncrement;

    juce::ValueTree rootNode             {ValueTreeIdentifiers::RootNodeData};
    juce::ValueTree nodeChildrenIds      {ValueTreeIdentifiers::NodeChildrenIds};
    juce::ValueTree midiNotesData        {ValueTreeIdentifiers::MidiNotesData};
    juce::ValueTree traversalChildrenIds {ValueTreeIdentifiers::TraversalChildrenIds};

    rootNode.addChild(midiNotesData,        -1, undoManager);
    rootNode.addChild(nodeChildrenIds,      -1, undoManager);
    rootNode.addChild(traversalChildrenIds, -1, undoManager);

    setNodeLimitProperties(rootNode, undoManager);

    rootNode.setProperty(ValueTreeIdentifiers::LoopLimit,  defaultRootLoopLimit, undoManager);
    rootNode.setProperty(ValueTreeIdentifiers::RootNodeId, rootId,               undoManager);
    rootNode.setProperty(ValueTreeIdentifiers::Id,         rootId,               undoManager);

    nodeMap.addChild(rootNode, -1, undoManager);

    return rootNode;
}

juce::ValueTree GraphState::addChildNode(int parentNodeId, const juce::Identifier& nodeType,
                                         juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).isValid());

    const int rootId = parentNode.getProperty(ValueTreeIdentifiers::RootNodeId);

    nodeIdIncrement = nodeIdIncrement + 1;

    juce::ValueTree node            {nodeType};
    juce::ValueTree nodeChildrenIds {ValueTreeIdentifiers::NodeChildrenIds};
    juce::ValueTree midiNotesData   {ValueTreeIdentifiers::MidiNotesData};

    node.addChild(nodeChildrenIds, -1, undoManager);
    node.addChild(midiNotesData,   -1, undoManager);

    setNodeLimitProperties(node, undoManager);

    node.setProperty(ValueTreeIdentifiers::RootNodeId, rootId,          undoManager);
    node.setProperty(ValueTreeIdentifiers::Id,         nodeIdIncrement, undoManager);

    connectNodes(parentNodeId, nodeIdIncrement, undoManager);
    nodeMap.addChild(node, -1, undoManager);

    return node;
}

juce::ValueTree GraphState::addTraversalFlagNode(int parentNodeId, juce::UndoManager* undoManager)
{
    jassert(getNode(parentNodeId).getType() == ValueTreeIdentifiers::NodeData
         || getNode(parentNodeId).getType() == ValueTreeIdentifiers::AlternativeNodeData
         || getNode(parentNodeId).getType() == ValueTreeIdentifiers::RootNodeData
         || getNode(parentNodeId).getType() == ValueTreeIdentifiers::TraversalFlagData);

    juce::ValueTree node = addChildNode(parentNodeId, ValueTreeIdentifiers::TraversalFlagData, undoManager);

    juce::ValueTree traversalChildrenIds {ValueTreeIdentifiers::TraversalChildrenIds};

    node.addChild(traversalChildrenIds, -1, undoManager);
    node.setProperty(ValueTreeIdentifiers::TraversalFlagValue, 0, undoManager);

    return node;
}

juce::ValueTree GraphState::addModulatorNode(juce::ValueTree parentNode, const juce::Identifier& nodeType,
                                             int newNodeId, juce::UndoManager* undoManager)
{
    int rootId = (int) parentNode.getProperty(ValueTreeIdentifiers::RootNodeId);

    if (nodeType == ValueTreeIdentifiers::ModulatorRootData) {
        rootId = newNodeId;
    }

    juce::ValueTree modulatorNode        {nodeType};
    juce::ValueTree modulatorChildrenIds {ValueTreeIdentifiers::NodeChildrenIds};

    modulatorNode.addChild(modulatorChildrenIds, -1, undoManager);

    setNodeLimitProperties(modulatorNode, undoManager);

    modulatorNode.setProperty(ValueTreeIdentifiers::RootNodeId, rootId,           undoManager);
    modulatorNode.setProperty(ValueTreeIdentifiers::ModAmount,  defaultModAmount, undoManager);
    modulatorNode.setProperty(ValueTreeIdentifiers::Id,         newNodeId,        undoManager);

    connectNodes(parentNode.getProperty(ValueTreeIdentifiers::Id), newNodeId, undoManager);

    return modulatorNode;
}

juce::ValueTree GraphState::addModulatorRoot(int parentNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(parentNode.getType() == ValueTreeIdentifiers::NodeData
         || parentNode.getType() == ValueTreeIdentifiers::AlternativeNodeData
         || parentNode.getType() == ValueTreeIdentifiers::RootNodeData);

    nodeIdIncrement = nodeIdIncrement + 1;

    juce::ValueTree modulatorNode = addModulatorNode(parentNode, ValueTreeIdentifiers::ModulatorRootData,
                                                     nodeIdIncrement, undoManager);

    nodeMap.addChild(modulatorNode, -1, undoManager);

    return modulatorNode;
}

juce::ValueTree GraphState::addModulator(int parentNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    jassert(parentNode.isValid());
    jassert(parentNode.getType() == ValueTreeIdentifiers::ModulatorData
         || parentNode.getType() == ValueTreeIdentifiers::ModulatorRootData);

    nodeIdIncrement = nodeIdIncrement + 1;

    juce::ValueTree modulatorNode = addModulatorNode(parentNode, ValueTreeIdentifiers::ModulatorData,
                                                     nodeIdIncrement, undoManager);

    nodeMap.addChild(modulatorNode, -1, undoManager);

    return modulatorNode;
}

void GraphState::connectNodes(int parentNodeId, int childNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree parentNode = getNode(parentNodeId);

    juce::ValueTree childId {ValueTreeIdentifiers::NodeId};
    childId.setProperty(ValueTreeIdentifiers::Id, childNodeId, undoManager);

    ArrowInfo arrowInfo;

    if (parentNode.getType() == ValueTreeIdentifiers::AlternativeNodeData) {
        arrowInfo.xBinding = ArrowBinding::NoBind;
        arrowInfo.yBinding = ArrowBinding::DurationBind;
    }

    ArrowBindingOps::setArrowInfo(childId, arrowInfo, undoManager);

    parentNode.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).addChild(childId, -1, undoManager);
}

void GraphState::disconnectNodes(int parentNodeId, int childNodeId, juce::UndoManager* undoManager)
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

void GraphState::removeNode(int nodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree node = getNode(nodeId);
    jassert(node.isValid());

    if (node.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        encapsulation.removeGroup(nodeId, undoManager);
        return;
    }

    std::vector<int> parentIds;

    const auto parents = parentIdsOf.find(nodeId);

    if (parents != parentIdsOf.end()) {
        parentIds = parents->second;
    }

    const int encapsulatorId = node.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    nodeMap.removeChild(node, undoManager);

    for (const int parentId : parentIds) {
        disconnectNodes(parentId, nodeId, undoManager);
    }

    juce::ValueTree encapsulatedIds = getNode(encapsulatorId).getChildWithName(ValueTreeIdentifiers::EncapsulatedIds);

    if (! encapsulatedIds.isValid()) {
        return;
    }

    encapsulatedIds.removeChild(encapsulatedIds.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId), undoManager);

    if (encapsulatedIds.getNumChildren() == 0) {
        encapsulation.dissolve(encapsulatorId, undoManager);
    }
}

void GraphState::setNodePosition(juce::ValueTree node, NodePosition nodePosition, juce::UndoManager* undoManager)
{
    jassert(node.isValid());

    node.setProperty(ValueTreeIdentifiers::XPosition, nodePosition.xPosition, undoManager);
    node.setProperty(ValueTreeIdentifiers::YPosition, nodePosition.yPosition, undoManager);
    node.setProperty(ValueTreeIdentifiers::Radius,    nodePosition.radius,    undoManager);
}

void GraphState::addMidiNote(int nodeId, NodeNote note, juce::UndoManager* undoManager)
{
    juce::ValueTree node = getNode(nodeId);

    jassert(node.isValid());
    jassert(node.getType() == ValueTreeIdentifiers::NodeData
         || node.getType() == ValueTreeIdentifiers::AlternativeNodeData
         || node.getType() == ValueTreeIdentifiers::RootNodeData);

    juce::ValueTree midiNote {ValueTreeIdentifiers::MidiNoteData};

    midiNote.setProperty(ValueTreeIdentifiers::MidiPitch,    note.pitch,       undoManager);
    midiNote.setProperty(ValueTreeIdentifiers::MidiVelocity, note.velocity,    undoManager);
    midiNote.setProperty(ValueTreeIdentifiers::MidiDuration, note.duration,    undoManager);
    midiNote.setProperty(ValueTreeIdentifiers::MidiChannel,  note.midiChannel, undoManager);

    node.getChildWithName(ValueTreeIdentifiers::MidiNotesData).addChild(midiNote, -1, undoManager);
}

NodePosition GraphState::getNodePosition(int nodeId) const
{
    juce::ValueTree node = getNode(nodeId);
    jassert(node.isValid());

    NodePosition nodePosition;

    nodePosition.xPosition = node.getProperty(ValueTreeIdentifiers::XPosition);
    nodePosition.yPosition = node.getProperty(ValueTreeIdentifiers::YPosition);
    nodePosition.radius    = node.getProperty(ValueTreeIdentifiers::Radius);

    return nodePosition;
}

juce::ValueTree GraphState::getNode(int nodeId) const
{
    const auto indexed = nodeIndex.find(nodeId);

    if (indexed == nodeIndex.end()) {
        return {};
    }

    return indexed->second;
}

std::vector<int> GraphState::nodeIdsBetween(int startNodeId, int endNodeId) const
{
    std::unordered_map<int, int> previousNodeId;
    std::vector<int>             frontier;
    std::vector<int>             neighbourIds;

    previousNodeId[startNodeId] = startNodeId;
    frontier.push_back(startNodeId);

    while (! frontier.empty() && previousNodeId.count(endNodeId) == 0) {
        std::vector<int> nextFrontier;

        for (const int nodeId : frontier) {
            neighbourIds.clear();

            const juce::ValueTree childIds = getNode(nodeId).getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

            for (int i = 0; i < childIds.getNumChildren(); ++i) {
                neighbourIds.push_back(childIds.getChild(i).getProperty(ValueTreeIdentifiers::Id));
            }

            const auto parents = parentIdsOf.find(nodeId);

            if (parents != parentIdsOf.end()) {
                neighbourIds.insert(neighbourIds.end(), parents->second.begin(), parents->second.end());
            }

            for (const int neighbourId : neighbourIds) {
                const juce::ValueTree neighbour = getNode(neighbourId);

                const juce::Identifier neighbourType = neighbour.getType();

                const bool isSequenceNode = neighbourType == ValueTreeIdentifiers::NodeData
                                         || neighbourType == ValueTreeIdentifiers::AlternativeNodeData
                                         || neighbourType == ValueTreeIdentifiers::RootNodeData;

                if (! isSequenceNode
                    || neighbour.hasProperty(ValueTreeIdentifiers::EncapsulatorId)
                    || previousNodeId.count(neighbourId) > 0) {
                    continue;
                }

                previousNodeId[neighbourId] = nodeId;
                nextFrontier.push_back(neighbourId);
            }
        }

        frontier = std::move(nextFrontier);
    }

    std::vector<int> spanNodeIds;

    if (previousNodeId.count(endNodeId) == 0) {
        return spanNodeIds;
    }

    for (int nodeId = endNodeId; nodeId != startNodeId; nodeId = previousNodeId.at(nodeId)) {
        spanNodeIds.push_back(nodeId);
    }

    spanNodeIds.push_back(startNodeId);

    std::reverse(spanNodeIds.begin(), spanNodeIds.end());

    std::unordered_set<int> spannedIds(spanNodeIds.begin(), spanNodeIds.end());

    std::vector<int> descendantFrontier(spanNodeIds.begin(), spanNodeIds.end() - 1);

    while (! descendantFrontier.empty()) {
        const int nodeId = descendantFrontier.back();
        descendantFrontier.pop_back();

        const juce::ValueTree childIds = getNode(nodeId).getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

        for (int i = 0; i < childIds.getNumChildren(); ++i) {
            const int childId = childIds.getChild(i).getProperty(ValueTreeIdentifiers::Id);

            const juce::ValueTree child = getNode(childId);

            const juce::Identifier childType = child.getType();

            const bool isSequenceNode = childType == ValueTreeIdentifiers::NodeData
                                     || childType == ValueTreeIdentifiers::AlternativeNodeData
                                     || childType == ValueTreeIdentifiers::RootNodeData;

            if (! isSequenceNode
                || child.hasProperty(ValueTreeIdentifiers::EncapsulatorId)
                || ! spannedIds.insert(childId).second) {
                continue;
            }

            spanNodeIds.push_back(childId);
            descendantFrontier.push_back(childId);
        }
    }

    return spanNodeIds;
}

juce::ValueTree GraphState::getNodeParent(int nodeId) const
{
    std::vector<int>        frontier {nodeId};
    std::unordered_set<int> visited  {nodeId};

    while (! frontier.empty()) {
        const int currentId = frontier.front();
        frontier.erase(frontier.begin());

        const auto parents = parentIdsOf.find(currentId);

        if (parents == parentIdsOf.end()) {
            continue;
        }

        for (const int parentId : parents->second) {
            const juce::ValueTree parent = getNode(parentId);

            if (! parent.isValid()) {
                continue;
            }

            if (parent.getType() != ValueTreeIdentifiers::TraversalFlagData) {
                return parent;
            }

            if (visited.insert(parentId).second) {
                frontier.push_back(parentId);
            }
        }
    }

    return {};
}

juce::ValueTree GraphState::getConnection(int parentNodeId, int childNodeId) const
{
    juce::ValueTree nodeChildrenIds = getNode(parentNodeId).getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    return nodeChildrenIds.getChildWithProperty(ValueTreeIdentifiers::Id, childNodeId);
}

juce::ValueTree GraphState::getMidiNotes(int nodeId) const
{
    juce::ValueTree node      = getNode(nodeId);
    juce::ValueTree midiNotes = node.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    if (! node.isValid() || ! midiNotes.isValid()) {
        return {};
    }

    return midiNotes;
}
