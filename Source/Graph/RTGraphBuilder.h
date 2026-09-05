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
#include <vector>

class SequenceTreeAudioProcessor;
class ValueTreeState;

class RTGraphBuilder
{
public:
    RTGraphBuilder(SequenceTreeAudioProcessor& processor, ValueTreeState& valueTreeState);

    void makeRTGraph(const juce::ValueTree& nodeValueTree);
    void rebuildAllGraphs();
    void updateDurationMaps(const std::vector<int>& nodeIds);

    std::unordered_map<int, std::shared_ptr<RTGraph>> rtGraphs;

private:
    void createRTNodes(juce::ValueTree rootNodeValueTree,
                       NodeBuildMap& builtNodes,
                       std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    void createRTNodeConnections(NodeBuildMap& builtNodes,
                                 std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    void fillDurationMap(const juce::ValueTree& nodeValueTree, RTNode& rtNode);

    RTtraversal buildRTtraversal(int traversalId);

    void rebuildGraphsForTraversal(int traversalId);

    void discardGraph(int graphId);

    SequenceTreeAudioProcessor& processor;
    ValueTreeState&             valueTreeState;

    std::vector<int>            durationRefreshScratch;
};
