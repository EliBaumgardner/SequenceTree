/*
  ==============================================================================

    RTData.h
    Created: 12 Jul 2025 1:05:00pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

struct RTNote {

    int pitch       = 0;
    int velocity    = 0;
    int duration    = 0;
    int midiChannel = 1;
};

struct RTtraversal {

    int traversalId = 0;
    double tempoMultiplier = 1;
    int channel = 1;
    int transpose = 0;
    double velocityMultiplier = 1.0;
};

struct RTConnection {

    int  childId    = 0;
    int  duration   = -1;
    bool isTreeJump = false;

    std::vector<int> disabledTraversals;
};

struct RTNode {

    int alternativeRootId = -1;

    int nodeID       = 0;
    int parentId     = 0;
    int countLimit   = 0;
    int triggerLimit = 0;
    int repeatValue  = 1;

    int switchCountLimit  = 0;
    int subLoopCountLimit = 0;

    int graphLoopLimit = 0;

    int pitchOffset = 0;

    bool isAlternativeNode = false;

    int flagTargetId = -1;

    bool flagRemovesTraversal = false;


    struct DanglingArrow
    {
        int duration   = 0;
        int countLimit = 1;
        std::vector<int> disabledTraversals;
    };

    enum class NodeType {RootNode, Node, Alternative, Modulator, ModulatorRoot, TraversalFlagData};

    NodeType nodeType = NodeType::Node;

    std::vector<RTtraversal>  traversals;
    std::vector<RTNote>       notes;
    std::vector<RTConnection> connections;
    std::vector<DanglingArrow> danglingArrows;

    int alternativeArrowDuration = -1;

    RTtraversal flagTraversal;

    int graphID = 0;

    const RTConnection* findConnection(int childId) const
    {
        for (const RTConnection& connection : connections) {
            if (connection.childId == childId) {
                return &connection;
            }
        }

        return nullptr;
    }
};

using NodeMap = std::unordered_map<int, std::shared_ptr<const RTNode>>;
