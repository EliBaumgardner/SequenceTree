#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "RTData.h"

#include <memory>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/*
    The converter from GraphState's ValueTree to the RTNodes the audio thread plays, run again after
    every edit so playback follows the graph. A graph here is one tree: a root and every node
    reachable from it. For each graph it builds one RTNode per node, with its limits, notes,
    traversals, ornament steps and each arrow's duration computed from the arrow's length, and
    publishes the result with AudioSnapshotPublisher::publishGraph, which merges it into the
    published NodeMap.

    makeRTGraph rebuilds the graph a node belongs to, discardGraph publishes an empty one in its
    place, and rebuildAllGraphs rebuilds every graph and discards those whose root was deleted.
    updateDurationMaps recomputes only the arrow durations of the given nodes and their parents.
    It listens to nodeMap, the traversal map and the ornament libraries, records which graphs each
    change affects, and rebuilds them together in handleAsyncUpdate on the message thread.
    buildRTtraversal converts one traversal's settings into an RTtraversal.

    The processor owns it, so edits reach the audio thread with no editor open. It reads the
    document and never writes it. NodeController calls buildRTtraversal for note preview.
*/

class SequenceTreeAudioProcessor;
class GraphState;

using NodeBuildMap = std::unordered_map<int, RTNode>;

class RTGraphBuilder : public juce::ValueTree::Listener,
                       public juce::AsyncUpdater
{
public:

    RTGraphBuilder(SequenceTreeAudioProcessor& processor, GraphState& graphState);
    ~RTGraphBuilder() override;

    void makeRTGraph     (const juce::ValueTree& nodeValueTree);
    void discardGraph    (int graphId);
    void rebuildAllGraphs();

    void valueTreeChildAdded     (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved   (juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& propertyIdentifier) override;

    void updateDurationMaps(std::span<const int> nodeIds);
    void handleAsyncUpdate () override;

    RTtraversal buildRTtraversal(TraversalKey key);

private:

    void rebuildGraphsForTraversal(int traversalId);

    void createRTNodes          (juce::ValueTree rootNodeValueTree, NodeBuildMap& builtNodes, std::unordered_map<int, juce::ValueTree>& tempNodeMap);
    void createRTNodeConnections(NodeBuildMap& builtNodes, std::unordered_map<int, juce::ValueTree>& tempNodeMap);

    void fillDurationMap  (const juce::ValueTree& nodeValueTree, RTNode& rtNode);
    void fillEncapsulation(const juce::ValueTree& nodeValueTree, RTNode& rtNode);
    void fillTraversalFlag(const juce::ValueTree& nodeValueTree, RTNode& rtNode);

    void rememberStructureChange(const juce::ValueTree& parent, const juce::ValueTree& child);
    void rememberOwners         (juce::ValueTree graphOwner, const juce::ValueTree& reshapedNode);

    static void             collectDisabledTraversals(const juce::ValueTree& owner, std::vector<TraversalKey>& disabledKeys);
    static void             classifyRootConnection   (const juce::ValueTree& parentValueTree, const juce::ValueTree& childIdTree,
                                                      const juce::ValueTree& childValueTree, RTConnection& connection);
    static RTConnection&    connectionFor            (RTNode& node, int childId);
    static RTNode::NodeType rtNodeTypeFor            (const juce::Identifier& valueTreeType);
    static NodeMap          freezeNodes              (NodeBuildMap& source);

    struct PendingChanges
    {
        std::unordered_set<int> addedNodeIds;
        std::unordered_set<int> rebuildNodeIds;
        std::unordered_set<int> rebuildRootIds;
        std::unordered_set<int> traversalIds;
        std::vector<int>        durationRefreshNodeIds;
        bool                    ornamentsChanged = false;
    };

    SequenceTreeAudioProcessor& processor;
    GraphState&                 graphState;

    std::unordered_set<int> builtGraphIds;

    std::vector<int> durationRefreshScratch;

    PendingChanges pending;
};
