/*
  ==============================================================================

    RTData.h
    Created: 12 Jul 2025 1:05:00pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>  // for std::hash
#include <utility>     // for std::pair
#include <string>      // if you use std::string

struct RTNote {

    float pitch       = 0;
    float velocity    = 0;
    float duration    = 0;
    int   midiChannel = 1;
};

struct RTtraversal {

    int traversalId = 0;
    double tempoMultiplier = 1;
    int channel = 1;
    int transpose = 0;
    double velocityMultiplier = 1.0;
};

struct RTNode {

    int alternativeRootId = -1;

    int nodeID       = 0;
    int parentId     = 0;
    int countLimit   = 0;
    int triggerLimit = 0;
    int repeatValue  = 1;

    int switchCountLimit  = 0;
    int subLoopCountLimit = 0;

    int pitchOffset = 0;

    bool isAlternativeNode = false;

    int flagTargetId = -1;

    bool flagRemovesTraversal = false;


    enum class NodeType {RootNode, Node, Alternative, Modulator, ModulatorRoot, TraversalFlagData};

    NodeType nodeType = NodeType::Node;

    std::vector<RTtraversal> traversals;
    std::vector<RTNote> notes;
    std::vector<int> children;
    std::unordered_map<int, int> durationMap;

    std::unordered_map<int, std::unordered_set<int>> disabledTraversalsByChild;

    std::unordered_set<int> treeJumpChildren;

    RTtraversal flagTraversal;

    int graphID = 0;

    RTNode() = default;

    RTNode(RTNode&&) noexcept            = default;
    RTNode& operator=(RTNode&&) noexcept = default;

    RTNode(const RTNode&)            = delete;
    RTNode& operator=(const RTNode&) = delete;

    RTNode clone() const
    {
        RTNode copy;

        copy.alternativeRootId     = alternativeRootId;
        copy.nodeID                = nodeID;
        copy.parentId              = parentId;
        copy.countLimit            = countLimit;
        copy.triggerLimit          = triggerLimit;
        copy.repeatValue           = repeatValue;
        copy.switchCountLimit      = switchCountLimit;
        copy.subLoopCountLimit     = subLoopCountLimit;
        copy.pitchOffset           = pitchOffset;
        copy.isAlternativeNode     = isAlternativeNode;
        copy.flagTargetId          = flagTargetId;
        copy.flagRemovesTraversal  = flagRemovesTraversal;
        copy.nodeType              = nodeType;
        copy.traversals            = traversals;
        copy.notes                 = notes;
        copy.children              = children;
        copy.durationMap           = durationMap;
        copy.disabledTraversalsByChild = disabledTraversalsByChild;
        copy.treeJumpChildren      = treeJumpChildren;
        copy.flagTraversal         = flagTraversal;
        copy.graphID               = graphID;

        return copy;
    }
};


inline bool isChildDisabledForTraversal(const RTNode& parent, int childId, int traversalId)
{
    const auto disabledIt = parent.disabledTraversalsByChild.find(childId);

    return disabledIt != parent.disabledTraversalsByChild.end()
        && disabledIt->second.count(traversalId) > 0;
}

using NodeMap = std::unordered_map<int, std::shared_ptr<const RTNode>>;

using NodeBuildMap = std::unordered_map<int, RTNode>;

inline NodeMap freezeNodes(NodeBuildMap& source)
{
    NodeMap frozen;
    frozen.reserve(source.size());

    for (auto& [nodeId, node] : source) {
        frozen.emplace(nodeId, std::make_shared<const RTNode>(std::move(node)));
    }

    return frozen;
}

struct RTGraph {


    NodeMap nodeMap;

    int rootID    = 0;
    int graphID   = 0;
    int loopLimit = 0;

    RTGraph() = default;

    RTGraph(RTGraph&& other) noexcept
        : nodeMap(std::move(other.nodeMap)),
          rootID(other.rootID),
          graphID(other.graphID),
          loopLimit(other.loopLimit)
    {}

    RTGraph& operator=(RTGraph&& other) noexcept {
        if(this != &other) {
            nodeMap   = std::move(other.nodeMap);
            rootID    = other.rootID;
            graphID   = other.graphID;
            loopLimit = other.loopLimit;
        }
        return *this;
    }
    
    RTGraph(const RTGraph&) = delete;
    RTGraph& operator=(const RTGraph&) = delete;
};

using RTGraphs = std::unordered_map<int, std::shared_ptr<RTGraph>>;

