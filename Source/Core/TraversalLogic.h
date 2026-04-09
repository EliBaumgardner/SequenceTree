#pragma once

#include "../Util/RTData.h"
#include <unordered_map>
#include <vector>

class SequenceTreeAudioProcessor;

class TraversalLogic {
public:


    struct traversalModulation {
        int pitchModulation;
        int velocityModulation;
        int durationModulation;
    };

    std::unordered_map<int, int> counts;
    std::vector<int> traversers;

    bool isFirstEvent = false;
    bool isLooping    = false;
    int  rootId       = 0;
    int  targetId     = 0;
    int  lastTargetId = 0;
    int  referenceTargetId = 0;

    enum class TraversalState { Start, Active, End, Reset };
    enum class EventType      { Node, Connector, Counter  };

    TraversalState state = TraversalState::Start;

    SequenceTreeAudioProcessor* audioProcessor;

    TraversalLogic(int root, SequenceTreeAudioProcessor* processor);

    void advance(NodeMap& nodes);
    RTNode* peekNextTarget(NodeMap& nodes);
    std::vector<int> peekTraversers(NodeMap& nodes);

    const RTNode& getTargetNode   (const NodeMap& nodes) const;
    const RTNode& getLastNode     (const NodeMap& nodes) const;
    const RTNode& getReferenceNode(const NodeMap& nodes) const;
    const RTNode& getRootNode     (const NodeMap& nodes) const;
    static const RTNode& getRelayNode(int relayNodeId, const NodeMap& nodes);

    bool shouldTraverse() const;
    void handleNodeEvent(NodeMap& nodes);
    void handleRelayNodeEvent(int relayNodeId, const NodeMap& nodes) const;
};