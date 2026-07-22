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

class SequenceTreeAudioProcessor;
class ValueTreeState;

class RTGraphBuilder
{
public:
    RTGraphBuilder(SequenceTreeAudioProcessor& processor, ValueTreeState& valueTreeState);

    void makeRTGraph(const juce::ValueTree& nodeValueTree);
    void rebuildAllGraphs();
    void updateDurationMap(int nodeId);

    std::unordered_map<int, std::shared_ptr<RTGraph>> rtGraphs;

private:
    void createRTNodes(juce::ValueTree rootNodeValueTree,
                       std::shared_ptr<RTGraph> rtGraph,
                       std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    void createRTNodeConnections(std::shared_ptr<RTGraph> rtGraph,
                                 std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    void fillDurationMap(const juce::ValueTree& nodeValueTree, RTNode& rtNode);

    RTtraversal buildRTtraversal(int traversalId);

    void rebuildGraphsForTraversal(int traversalId);

    SequenceTreeAudioProcessor& processor;
    ValueTreeState&             valueTreeState;
};
