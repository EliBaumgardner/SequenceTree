/*
  ==============================================================================

    RTGraphBuilder.h
    Created: 30 Apr 2026
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include "../Util/PluginModules.h"
#include "RTData.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class SequenceTreeAudioProcessor;
class GraphState;

using NodeBuildMap = std::unordered_map<int, RTNode>;

class RTGraphBuilder
{
public:
    RTGraphBuilder(SequenceTreeAudioProcessor& processor, GraphState& graphState);

    void makeRTGraph(const juce::ValueTree& nodeValueTree);
    void rebuildAllGraphs();
    void updateDurationMaps(const std::vector<int>& nodeIds);
    void discardGraph(int graphId);

private:
    void createRTNodes(juce::ValueTree rootNodeValueTree,
                       NodeBuildMap& builtNodes,
                       std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    void createRTNodeConnections(NodeBuildMap& builtNodes,
                                 std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    static RTNode::NodeType rtNodeTypeFor(const juce::Identifier& valueTreeType);

    void fillDurationMap(const juce::ValueTree& nodeValueTree, RTNode& rtNode);

    static void collectDisabledTraversals(const juce::ValueTree& owner, std::vector<int>& disabledIds);

    static RTConnection& connectionFor(RTNode& node, int childId);

    static bool isTreeJumpConnection(const juce::ValueTree& parentValueTree,
                                     const juce::ValueTree& childIdTree,
                                     const juce::ValueTree& childValueTree);

    static NodeMap freezeNodes(NodeBuildMap& source);

    RTtraversal buildRTtraversal(int traversalId);

    void rebuildGraphsForTraversal(int traversalId);

    SequenceTreeAudioProcessor& processor;
    GraphState&             graphState;

    std::unordered_set<int>     builtGraphIds;

    std::vector<int>            durationRefreshScratch;
};
