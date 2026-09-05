/*
  ==============================================================================

    RTData.h
    Created: 12 Jul 2025 1:05:00pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <algorithm>
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

struct RTNodeData {

    int  childId    = 0;
    int  duration   = -1;
    bool isTreeJump = false;

    std::vector<int> disabledTraversals;
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


    struct DanglingArrow
    {
        int duration   = 0;
        int countLimit = 1;

        std::vector<int> disabledTraversals;
    };

    enum class NodeType {RootNode, Node, Alternative, Modulator, ModulatorRoot, TraversalFlagData};

    NodeType nodeType = NodeType::Node;

    std::vector<RTtraversal> traversals;
    std::vector<RTNote> notes;
    std::vector<RTNodeData> nodeData;
    std::vector<DanglingArrow> danglingArrows;

    int parentDuration = -1;

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
        copy.nodeData              = nodeData;
        copy.danglingArrows        = danglingArrows;
        copy.parentDuration        = parentDuration;
        copy.flagTraversal         = flagTraversal;
        copy.graphID               = graphID;

        return copy;
    }
};


inline const RTNodeData* findNodeData(const RTNode& node, int childId)
{
    for (const RTNodeData& data : node.nodeData) {
        if (data.childId == childId) {
            return &data;
        }
    }

    return nullptr;
}

inline int childDuration(const RTNode& parent, int childId)
{
    const RTNodeData* const data = findNodeData(parent, childId);

    return data != nullptr ? data->duration : -1;
}

inline bool isTraversalDisabled(const std::vector<int>& disabledTraversals, int traversalId)
{
    return std::find(disabledTraversals.begin(), disabledTraversals.end(), traversalId)
        != disabledTraversals.end();
}

inline bool isChildDisabledForTraversal(const RTNode& parent, int childId, int traversalId)
{
    const RTNodeData* const data = findNodeData(parent, childId);

    return data != nullptr && isTraversalDisabled(data->disabledTraversals, traversalId);
}

inline int selectDanglingArrow(const RTNode& node, int count, int traversalId)
{
    int chosen   = -1;
    int maxLimit = 0;

    for (std::size_t index = 0; index < node.danglingArrows.size(); ++index) {
        const RTNode::DanglingArrow& dangling = node.danglingArrows[index];

        if (dangling.duration <= 0 || dangling.countLimit <= 0) {
            continue;
        }

        if (isTraversalDisabled(dangling.disabledTraversals, traversalId)) {
            continue;
        }

        if (count % dangling.countLimit == 0 && dangling.countLimit > maxLimit) {
            chosen   = static_cast<int>(index);
            maxLimit = dangling.countLimit;
        }
    }

    return chosen;
}

inline int danglingArrowDuration(const RTNode& node, int danglingIndex)
{
    if (danglingIndex < 0 || danglingIndex >= static_cast<int>(node.danglingArrows.size())) {
        return 0;
    }

    return node.danglingArrows[static_cast<std::size_t>(danglingIndex)].duration;
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

