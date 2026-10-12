/*
  ==============================================================================

    RTData.h
    Created: 12 Jul 2025 1:05:00pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <compare>
#include <vector>

struct RTNote {

    static constexpr int fallbackValue = 63;

    int pitch       = 0;
    int velocity    = 0;
    int duration    = 0;
    int midiChannel = 1;
};

enum class OrnamentNodeKind { Note, Host };

struct OrnamentStep
{
    OrnamentNodeKind kind;
    int              offset;
    int              velocityPercent;
    int              durationWeight;
};

struct TraversalKey {

    static constexpr int maxInstances = 26;

    int typeId   = 0;
    int instance = 0;

    auto operator<=>(const TraversalKey&) const = default;
};

struct RTtraversal {

    static constexpr double minimumTempoMultiplier = 0.01;
    static constexpr double maximumTempoMultiplier = 100.0;

    TraversalKey key;
    double       tempoMultiplier    = 1;
    int          channel            = 1;
    int          transpose          = 0;
    double       velocityMultiplier = 1.0;
};

struct RTPreviewRequest {

    enum class Kind { Start, Stop, SetRepeat };

    Kind        kind       = Kind::Stop;
    int         nodeId     = -1;
    int         visitCount = 1;
    RTtraversal traversal;
    bool        repeat     = false;
};

struct RTConnection {

    int  childId     = 0;
    int  duration    = -1;
    bool isTreeJump  = false;
    bool isCrossRoot = false;
    bool isSynced    = true;

    std::vector<TraversalKey> disabledTraversals;
};

struct RTNode {

    const RTConnection* findConnection(int childId) const
    {
        for (const RTConnection& connection : connections) {
            if (connection.childId == childId) {
                return &connection;
            }
        }

        return nullptr;
    }

    struct DanglingArrow
    {
        int                       duration   = 0;
        int                       countLimit = 1;
        std::vector<TraversalKey> disabledTraversals;
    };

    enum class NodeType {RootNode, Node, Alternative, Modulator, ModulatorRoot, AlternativeModulator, TraversalFlagData};

    static constexpr int probabilityScale = 100;

    int      nodeID   = 0;
    int      parentId = 0;
    int      graphID  = 0;
    NodeType nodeType = NodeType::Node;

    int countLimit        = 0;
    int triggerLimit      = 0;
    int switchCountLimit  = 0;
    int subLoopCountLimit = 0;
    int graphLoopLimit    = 0;

    int repeatValue = 1;
    int probability = probabilityScale;
    int pitchOffset = 0;

    bool isAlternativeNode        = false;
    int  alternativeRootId        = -1;
    int  alternativeArrowDuration = -1;

    int encapsulationEntryId = -1;

    int         flagTargetId         = -1;
    bool        flagRemovesTraversal = false;
    RTtraversal flagTraversal;

    std::vector<RTtraversal>   traversals;
    std::vector<RTNote>        notes;
    std::vector<RTConnection>  connections;
    std::vector<DanglingArrow> danglingArrows;
    std::vector<OrnamentStep>  ornament;
};

struct NodeMap {

    std::vector<RTNode> sortedById;

    int firstUnlinkedRootId = -1;

    const RTNode* find(int nodeId) const
    {
        const auto position = std::ranges::lower_bound(sortedById, nodeId, {}, &RTNode::nodeID);

        if (position == sortedById.end() || position->nodeID != nodeId) {
            return nullptr;
        }

        return &*position;
    }
};
