#pragma once

#include "../Graph/RTData.h"
#include <unordered_map>
#include <vector>

class SequenceTreeAudioProcessor;

class TraversalLogic {
public:


    std::unordered_map<int, int> counts;
    std::unordered_map<int, int> modulatorCounts;

    int activeModulatorRootId = -1;
    int targetModulatorId     = -1;
    int lastModulatorId       = -1;

    bool isFirstEvent = false;
    bool isLooping    = false;
    int  rootId            = 0;
    int  targetId          = 0;
    int  lastTargetId      = 0;
    int  referenceTargetId = 0;
    int  loopCount         = 0;  // incremented each time the traversal resets to root
    int  loopLimit         = 0;  // 0 = loop infinitely; N > 0 = stop after N full loops

    enum class TraversalState { Start, Active, End, Reset };
    enum class EventType      { Node, Connector, Modulator };

    TraversalState state = TraversalState::Start;

    SequenceTreeAudioProcessor* audioProcessor;

    TraversalLogic(int root, SequenceTreeAudioProcessor* processor);

    void advance(NodeMap& nodes);
    void advanceModulator(NodeMap& nodes);
    RTNode* peekNextTarget(NodeMap& nodes);
    std::vector<int> peekCrossTreeNode(NodeMap& nodes);
    RTNode* peekModulators(NodeMap& nodes);

    const RTNode& getTargetNode   (const NodeMap& nodes) const;
    const RTNode& getLastNode     (const NodeMap& nodes) const;
    const RTNode& getReferenceNode(const NodeMap& nodes) const;
    const RTNode& getRootNode     (const NodeMap& nodes) const;
    static const RTNode& getRelayNode(int relayNodeId, const NodeMap& nodes);

    RTNode* getModulatorNode(NodeMap& nodes, int nodeId);
    int     findActiveModulatorRoot(NodeMap& nodes, int regularNodeId);

    bool shouldTraverse() const;
    void handleNodeEvent(NodeMap& nodes);

    void resetCounts() { counts.clear(); }
};

using TraversalMap = std::unordered_map<int, TraversalLogic>;