/*
  ==============================================================================

    RTGraphBuilder.h
    Created: 30 Apr 2026
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "RTData.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class SequenceTreeAudioProcessor;
class GraphState;

using NodeBuildMap = std::unordered_map<int, RTNode>;

class RTGraphBuilder : public juce::ValueTree::Listener,
                       public juce::AsyncUpdater
{
public:
    RTGraphBuilder(SequenceTreeAudioProcessor& processor, GraphState& graphState);
    ~RTGraphBuilder() override;

    void makeRTGraph(const juce::ValueTree& nodeValueTree);
    void rebuildAllGraphs();
    void updateDurationMaps(const std::vector<int>& nodeIds);
    void discardGraph(int graphId);

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& propertyIdentifier) override;
    void handleAsyncUpdate() override;

private:
    struct PendingChanges
    {
        std::unordered_set<int> addedNodeIds;
        std::unordered_set<int> rebuildNodeIds;
        std::unordered_set<int> rebuildRootIds;
        std::unordered_set<int> reshapedNodeIds;
        std::unordered_set<int> movedNodeIds;
        std::unordered_set<int> traversalIds;
        std::vector<int>        durationRefreshNodeIds;
    };

    void rememberStructureChange(const juce::ValueTree& parent, const juce::ValueTree& child);

    void rememberOwners(juce::ValueTree graphOwner, const juce::ValueTree& reshapedNode);

    void createRTNodes(juce::ValueTree rootNodeValueTree,
                       NodeBuildMap& builtNodes,
                       std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    void createRTNodeConnections(NodeBuildMap& builtNodes,
                                 std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    static RTNode::NodeType rtNodeTypeFor(const juce::Identifier& valueTreeType);

    void fillDurationMap(const juce::ValueTree& nodeValueTree, RTNode& rtNode);

    void fillEncapsulation(const juce::ValueTree& nodeValueTree, RTNode& rtNode);

    static void collectDisabledTraversals(const juce::ValueTree& owner, std::vector<TraversalKey>& disabledKeys);

    static RTConnection& connectionFor(RTNode& node, int childId);

    static void classifyRootConnection(const juce::ValueTree& parentValueTree,
                                       const juce::ValueTree& childIdTree,
                                       const juce::ValueTree& childValueTree,
                                       RTConnection& connection);

    static NodeMap freezeNodes(NodeBuildMap& source);

    RTtraversal buildRTtraversal(TraversalKey key);

    void rebuildGraphsForTraversal(int traversalId);

    SequenceTreeAudioProcessor& processor;
    GraphState&             graphState;

    std::unordered_set<int>     builtGraphIds;

    std::vector<int>            durationRefreshScratch;

    PendingChanges              pending;
};
