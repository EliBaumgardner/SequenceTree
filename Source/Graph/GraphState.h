#pragma once

#include "../Util/NodeInfo.h"
#include "../Util/ArrowInfo.h"
#include "ArrowBindingOps.h"
#include "EncapsulationOps.h"
#include "RTData.h"
#include "TraversalState.h"

#include <juce_data_structures/juce_data_structures.h>

#include <unordered_map>
#include <vector>

/*
    The owner of the graph's ValueTree, which is the document the canvas, the audio model and the
    saved file are all built from. nodeMap holds one tree per node (root, node,
    alternative, modulator, traversal flag or encapsulation group) with its position, limits, MIDI
    notes and a NodeChildrenIds list whose entries are the arrows to its children. Every edit to
    the graph goes through this class or the parts it owns (encapsulation, arrows, traversals), so
    node ids, root ids and parent links stay consistent.

    The add functions create each kind of node with the next id. connectNodes and disconnectNodes
    add and remove an arrow, removeNode deletes a node and the arrows into and out of it, and
    setNodePosition moves a node and updates the arrow durations and pitch bindings the move
    affects. getNode,
    getNodeParent and parentIdsOf find nodes by id without searching nodeMap, using an index this
    class's listener on nodeMap keeps current, so undo, redo and paste update it too. replaceState
    loads a saved graph and traversal map.

    The processor owns it, and it is used on the message thread only. NodeFactory, Clipboard and
    the input gestures edit it; NodeCanvasTreeListener and RTGraphBuilder listen to nodeMap.
*/

class GraphState : public juce::ValueTree::Listener {

public:

    GraphState();
    ~GraphState() override;

    juce::ValueTree addRootNode         (juce::UndoManager* undoManager);
    juce::ValueTree addChildNode        (int parentNodeId, const juce::Identifier& nodeType, juce::UndoManager* undoManager);
    juce::ValueTree addTraversalFlagNode(int parentNodeId, juce::UndoManager* undoManager);

    juce::ValueTree addModulatorRoot       (int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addModulator           (int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addAlternativeModulator(int parentNodeId, juce::UndoManager* undoManager);

    juce::ValueTree getNode      (int nodeId) const;
    juce::ValueTree getNodeParent(int nodeId) const;
    juce::ValueTree getMidiNotes (int nodeId) const;
    juce::ValueTree getConnection(int parentNodeId, int childNodeId) const;

    void connectNodes   (int parentNodeId, int childNodeId, juce::UndoManager* undoManager);
    void disconnectNodes(int parentNodeId, int childNodeId, juce::UndoManager* undoManager);
    void removeNode     (int nodeId, juce::UndoManager* undoManager);

    void addMidiNote    (int nodeId, NodeNote note, juce::UndoManager* undoManager);
    void setNodeColour  (int nodeId, const juce::String& colourText, juce::UndoManager* undoManager);
    void setNodePosition(juce::ValueTree node, NodePosition nodePosition, juce::UndoManager* undoManager);

    void replaceState(const juce::ValueTree& restoredNodeMap, const juce::ValueTree& restoredTraversalMap);

    NodePosition getNodePosition(int nodeId) const;

    bool connectsToOtherTreeRoot(int parentNodeId, int childNodeId) const;

    std::vector<int> nodeIdsBetween(int startNodeId, int endNodeId) const;

    juce::ValueTree nodeMap;

    EncapsulationOps encapsulation {*this};
    ArrowBindingOps  arrows        {*this};
    TraversalState   traversals    {*this};

    std::unordered_map<int, std::vector<int>> parentIdsOf;

    int nodeIdIncrement = 0;

    static constexpr int defaultSwitchCountLimit  {1};
    static constexpr int defaultNodeCountLimit    {1};
    static constexpr int defaultTriggerLimit      {0};
    static constexpr int defaultSubLoopCountLimit {1};
    static constexpr int defaultRootLoopLimit     {0};
    static constexpr int defaultRepeatValue       {1};
    static constexpr int defaultProbability       {100};
    static constexpr int defaultModAmount         {0};
    static constexpr int defaultMidiChannel       {1};

private:

    void setNodeLimitProperties(juce::ValueTree node, juce::UndoManager* undoManager);

    void valueTreeChildAdded  (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;

    void indexNode  (const juce::ValueTree& node);
    void unindexNode(const juce::ValueTree& node);

    void linkParent  (int parentNodeId, int childNodeId);
    void unlinkParent(int parentNodeId, int childNodeId);

    juce::ValueTree addModulatorNode(juce::ValueTree parentNode, const juce::Identifier& nodeType, int newNodeId, juce::UndoManager* undoManager);

    std::unordered_map<int, juce::ValueTree> nodeIndex;
};
