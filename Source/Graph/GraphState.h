#pragma once

#include "../Util/NodeInfo.h"
#include "../Util/ArrowInfo.h"

#include <juce_data_structures/juce_data_structures.h>

#include <unordered_map>
#include <vector>

class GraphState : public juce::ValueTree::Listener {

public:

    GraphState();
    ~GraphState() override;

    juce::ValueTree addRootNode         (juce::UndoManager* undoManager);
    juce::ValueTree addChildNode        (int parentNodeId, const juce::Identifier& nodeType,
                                         juce::UndoManager* undoManager);
    juce::ValueTree addTraversalFlagNode(int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addModulatorRoot    (int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addModulator        (int parentNodeId, juce::UndoManager* undoManager);

    juce::ValueTree addTraversalData(int traversalId, juce::UndoManager* undoManager);

    void addMidiNote(int nodeId, NodeNote note, juce::UndoManager* undoManager);

    void replaceState(const juce::ValueTree& restoredNodeMap,
                      const juce::ValueTree& restoredTraversalMap);

    void connectNodes   (int parentNodeId, int childNodeId, juce::UndoManager* undoManager);
    void disconnectNodes(int parentNodeId, int childNodeId, juce::UndoManager* undoManager);

    static void      setArrowInfo(juce::ValueTree arrowTree, const ArrowInfo& arrowInfo,
                                  juce::UndoManager* undoManager);
    static ArrowInfo getArrowInfo(const juce::ValueTree& arrowTree);

    std::vector<int> syncPitchBindings(int nodeId, juce::UndoManager* undoManager);

    void removeNode(int nodeId, juce::UndoManager* undoManager);

    static void  setNodePosition(juce::ValueTree node, NodePosition nodePosition,
                                 juce::UndoManager* undoManager);
    NodePosition getNodePosition(int nodeId) const;

    juce::ValueTree getNode      (int nodeId) const;
    juce::ValueTree getNodeParent(int nodeId) const;
    juce::ValueTree getMidiNotes (int nodeId) const;
    juce::ValueTree getConnection(int parentNodeId, int childNodeId) const;

    juce::ValueTree nodeMap;
    juce::ValueTree traversalMap;

    std::unordered_map<int, std::vector<int>> parentIdsOf;

    int nodeIdIncrement = 0;

    static constexpr int defaultTraversalId       {1};

    static constexpr int defaultSwitchCountLimit  {1};
    static constexpr int defaultNodeCountLimit    {1};
    static constexpr int defaultTriggerLimit      {0};
    static constexpr int defaultSubLoopCountLimit {1};
    static constexpr int defaultRootLoopLimit     {0};
    static constexpr int defaultRepeatValue       {1};
    static constexpr int defaultProbability       {100};
    static constexpr int defaultModAmount         {0};
    static constexpr int defaultMidiChannel       {1};
    static constexpr int defaultTempoMult         {1};

private:

    void valueTreeChildAdded  (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;

    void indexNode  (const juce::ValueTree& node);
    void unindexNode(const juce::ValueTree& node);

    void linkParent  (int parentNodeId, int childNodeId);
    void unlinkParent(int parentNodeId, int childNodeId);

    void setNodeLimitProperties(juce::ValueTree node, juce::UndoManager* undoManager);

    juce::ValueTree addModulatorNode(juce::ValueTree parentNode, const juce::Identifier& nodeType,
                                     int newNodeId, juce::UndoManager* undoManager);

    bool applyArrowPitchOffset(juce::ValueTree arrowTree, int targetNodeId,
                               int deltaX, int deltaY, juce::UndoManager* undoManager);

    std::unordered_map<int, juce::ValueTree> nodeIndex;
};
