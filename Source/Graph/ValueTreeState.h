//
// Created by Eli Baumgardner on 3/21/26.
//

#pragma once

#include "../Util/NodeInfo.h"
#include "../Util/ArrowInfo.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class ValueTreeState {

public:

    ValueTreeState();

    juce::ValueTree addNodeTree     (juce::UndoManager* undoManager);

    void setNodeCountProperties(juce::UndoManager *undoManager, juce::ValueTree node);

    juce::ValueTree createTraversalData    (int traversalId, juce::UndoManager* undoManager);

    juce::ValueTree addRootNode         (juce::UndoManager* undoManager);

    juce::ValueTree addNode             (int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addAlternativeNode  (int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addTraversalFlagNode(int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addModulatorRoot    (int parentNodeId, juce::UndoManager* undoManager);
    juce::ValueTree addModulator        (int parentNodeId, juce::UndoManager* undoManager);

    juce::ValueTree addChildNode(juce::ValueTree parentNode, const juce::Identifier& nodeType,
                                 juce::UndoManager* undoManager);

    juce::ValueTree addModulatorNode(juce::ValueTree parentNode, const juce::Identifier& nodeType,
                                     int newNodeId, juce::UndoManager* undoManager);

    void replaceState(const juce::ValueTree& restoredTree);

    void connectNodes   (int parentNodeId, int childNodeId, juce::UndoManager* undoManager);
    void disconnectNodes(int parentNodeId, int childNodeId, juce::UndoManager* undoManager);
    void setArrowType   (int parentNodeId, int childNodeId, ArrowType arrowType, juce::UndoManager* undoManager);

    static ArrowInfo readArrowInfo (const juce::ValueTree& arrowTree, bool sourceIsAlternative);
    static void      writeArrowInfo(juce::ValueTree arrowTree, const ArrowInfo& arrowInfo,
                                    juce::UndoManager* undoManager);

    void setArrowInfo(int parentNodeId, int childNodeId, const ArrowInfo& arrowInfo,
                      juce::UndoManager* undoManager);

    std::vector<int> syncPitchBindings(int nodeId, juce::UndoManager* undoManager);

    void removeRootNode (int rootNodeId, juce::UndoManager* undoManager);
    void removeNode     (int nodeId, juce::UndoManager* undoManager);
    void removeNodeTree (int treeId, juce::UndoManager* undoManager);

    void setNodePosition (juce::ValueTree node, NodePosition nodePosition, juce::UndoManager* undoManager);
    void setMidiValue    (int nodeId, NodeNote note, juce::UndoManager* undoManager);

    NodePosition    getNodePosition (int nodeId);
    juce::ValueTree getRootNode     (int nodeId);
    juce::ValueTree getNode         (int nodeId);
    juce::ValueTree getNodeParent   (int nodeId);
    juce::ValueTree getMidiNotes    (int nodeId);
    juce::ValueTree getNodeTree     (int treeId);
    juce::ValueTree getConnection   (int parentNodeId, int childNodeId);

    int  getNodeIdIncrement() const  { return nodeIdIncrement; }
    void setNodeIdIncrement(int value) { nodeIdIncrement = value; }

    juce::ValueTree addTraversalRule   (juce::UndoManager* undoManager);
    void            removeTraversalRule(int ruleId, juce::UndoManager* undoManager);

    juce::ValueTree getTraversalRule(int ruleId) const;

    void setTraversalRuleName  (int ruleId, const juce::String& name,   juce::UndoManager* undoManager);
    void setTraversalRuleSource(int ruleId, const juce::String& source, juce::UndoManager* undoManager);

    void setActiveTraversalRuleId(int ruleId, juce::UndoManager* undoManager);
    int  getActiveTraversalRuleId() const;

    juce::String getActiveTraversalRuleSource() const;

    void ensureDefaultTraversalRule();

    juce::ValueTree canvasData;
    juce::ValueTree nodeTreeIds;
    juce::ValueTree nodeMap;
    juce::ValueTree nodeTreeMap;
    juce::ValueTree traversalMap;
    juce::ValueTree traversalRules;

    static constexpr int defaultSwitchCount       {1};
    static constexpr int defaultNodeCount         {1};
    static constexpr int defaultSwitchCountLimit  {1};
    static constexpr int defaultNodeCountLimit    {1};
    static constexpr int defaultTriggerLimit      {0};
    static constexpr int defaultSubLoopCountLimit {1};
    static constexpr int defaultRootLoopLimit     {0};
    static constexpr int defaultRepeatValue       {1};
    static constexpr int defaultModAmount         {0};
    static constexpr int defaultMidiChannel       {1};
    static constexpr int defaultTempoMult         {1};

private:

    bool applyArrowPitchOffset(juce::ValueTree arrowTree, int targetNodeId, bool sourceIsAlternative,
                               int deltaX, int deltaY, juce::UndoManager* undoManager);

    int nodeIdIncrement = 0;
    int ruleIdIncrement = 0;
};
