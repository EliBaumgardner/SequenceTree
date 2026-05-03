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

class NodeCanvas;
struct ApplicationContext;

class RTGraphBuilder
{
public:
    RTGraphBuilder(ApplicationContext& context, NodeCanvas& canvas);

    void makeRTGraph(const juce::ValueTree& nodeValueTree);
    void updateDurationMap(int nodeId);
    void destroyRTGraph(class Node* root);

    std::unordered_map<int, std::shared_ptr<RTGraph>> rtGraphs;

private:
    void createRTNodes(juce::ValueTree rootNodeValueTree,
                       std::shared_ptr<RTGraph> rtGraph,
                       std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    void createRTNodeConnections(std::shared_ptr<RTGraph> rtGraph,
                                 std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    ApplicationContext& applicationContext;
    NodeCanvas&         canvas;
};
