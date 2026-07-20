/*
  ==============================================================================

    RTData.h
    Created: 12 Jul 2025 1:05:00pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <vector>
#include <unordered_map>
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

    int alternativeRootId   = -1;

    int modulatorId = 0;

    int nodeID      = 0;
    int parentId    = 0;
    int countLimit  = 0;
    int triggerLimit = 0;
    int repeatValue = 1;

    int switchCount      = 0;
    int switchCountLimit = 0;

    int loopCount      = 0;
    int loopCountLimit = 0;

    int subLoopCount   = 0;
    int subLoopCountLimit   = 0;


    bool isConnectedToModulator = false;
    bool isAlternativeNode = false;
    bool isLeafNode = false;
    bool isAlternativeRoot = false;

    // Set on TraversalFlagData nodes: the traversal to spawn and the node it
    // starts on (reached via the flag's dashed arrow) when this flag's parent
    // is reached during traversal.
    int flagTargetId = -1;

    bool flagRemovesTraversal = false;


    enum class NodeType {RootNode, Node, Alternative, Modulator, ModulatorRoot, TraversalFlagData};

    NodeType nodeType = NodeType::Node;

    std::vector<RTtraversal> traversals;
    std::vector<RTNote> notes;
    std::vector<int> children;
    std::unordered_map<int, int> durationMap;

    // The traversal a TraversalFlagData node spawns; traversalId <= 0 means unset.
    RTtraversal flagTraversal;

    int graphID = 0;
};


using NodeMap = std::unordered_map<int, RTNode>;

struct NodeRuntimeState {

    int activeAlternativeId = -1;
    int lastAlternativeId   = -1;
    int lastNodeId          = -1;
};

using NodeStateMap = std::unordered_map<int, NodeRuntimeState>;

struct RTGraph {


    std::unordered_map<int, RTNode> nodeMap;
    std::atomic<bool> traversalRequested;

    int rootID    = 0;
    int graphID   = 0;
    int loopLimit = 0;

    RTGraph() = default;

    RTGraph(RTGraph&& other) noexcept : nodeMap(other.nodeMap)
    {}

    RTGraph& operator=(RTGraph&& other) noexcept {
        if(this != &other) {
            nodeMap = std::move(other.nodeMap);
        }
        return *this;
    }
    
    RTGraph(const RTGraph&) = delete;
    RTGraph& operator=(const RTGraph&) = delete;
};

