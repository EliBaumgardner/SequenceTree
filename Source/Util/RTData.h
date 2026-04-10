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
    
    float pitch = 0;
    float velocity = 0;
    float duration = 0;
};

struct RTNode {
    
    int nodeID = 0;
    int parentId = 0;
    int countLimit = 0;

    enum class NodeType {RootNode, Node, Connector, Modulator, ModulatorRoot};

    NodeType nodeType = NodeType::Node;

    std::vector<RTNote> notes;
    std::vector<int> children;
    std::vector<int> connectors;
    std::unordered_map<int, int> durationMap;

    int graphID = 0;
};


using NodeMap = std::unordered_map<int, RTNode>;

struct RTGraph {
    
    std::unordered_map<int, RTNode> nodeMap;
    std::atomic<bool> traversalRequested;

    int rootID = 0;
    int graphID = 0;

    RTGraph() = default;
    
    RTGraph(RTGraph&& other) noexcept : nodeMap(other.nodeMap)
    {}
    
    RTGraph& operator=(RTGraph&& other) noexcept {
        if(this != &other){
            nodeMap = std::move(other.nodeMap);
        }
        return *this;
    }
    
    RTGraph(const RTGraph&) = delete;
    RTGraph& operator=(const RTGraph&) = delete;
};

