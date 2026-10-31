#pragma once

#include <unordered_set>

#include "CanvasDocument.h"
#include "../Graph/NodeFactory.h"
#include "../Util/ArrowInfo.h"

/*
    The CanvasDocument for the graph: each edit the canvas makes, carried out on GraphState's
    nodeMap through GraphState and NodeFactory. It also holds the two settings that shape what
    the user's edits create: creationMode, the kind of node a drag grows out of a node (set by
    Titlebar), and currentArrowInfo, the ArrowInfo::Settings every new arrow and dangling arrow
    starts with (set by BottomBar).

    moveNode moves a node, every node below it that is not also above it, and a group whose
    first member moves, by the same offset. growFrom grows a collapsed group's new children from
    its last member. disconnect removes a traversal flag the arrow leads to rather than leaving
    it unattached. canConnect refuses a second arrow between the same two nodes and an arrow from
    a modulator into a root. ownershipOf answers which end of an arrow lists the other among its
    children, since an alternative child's arrow is drawn from the child to its parent.

    PluginEditor owns it; the canvas, the gesture classes, ArrowMenu and the bars hold it by
    reference. Every edit runs on the message thread.
*/

class GraphState;

class GraphDocument : public CanvasDocument
{
public:

    struct ArrowOwnership;

    GraphDocument(GraphState& graphState, juce::UndoManager& undoManager);

    juce::ValueTree findNode    (int nodeId) const override;
    juce::ValueTree addChildNode(int parentNodeId, juce::Point<int> position, const juce::ModifierKeys& mods) override;

    int growFrom(int nodeId) const override;

    void addRootNode(juce::Point<int> position) override;
    void moveNode   (juce::ValueTree nodeTree, juce::Point<int> position) override;
    void removeNode (int nodeId) override;

    void addDanglingArrow   (juce::ValueTree nodeTree, juce::Point<int> tipOffset) override;
    void setDanglingArrowTip(juce::ValueTree arrowTree, juce::Point<int> tipOffset) override;
    void removeDanglingArrow(juce::ValueTree arrowTree) override;

    void connect   (int parentNodeId, int childNodeId, ConnectionGesture gesture) override;
    void disconnect(int startNodeId, int endNodeId) override;

    bool canConnect(const Node& startNode, const Node& endNode) const override;

    ArrowOwnership ownershipOf(int startNodeId, int endNodeId) const;

    struct ArrowOwnership
    {
        int ownerNodeId;
        int childNodeId;
    };

    NodeCreationMode    creationMode = NodeCreationMode::Node;
    ArrowInfo::Settings currentArrowInfo;

private:

    void moveDescendants                (juce::ValueTree nodeTree, int deltaX, int deltaY, std::unordered_set<int>& visited, int draggedNodeId);
    void moveEncapsulatorWithEntryMember(int nodeId, int draggedNodeId, int deltaX, int deltaY);

    GraphState& graphState;
};
