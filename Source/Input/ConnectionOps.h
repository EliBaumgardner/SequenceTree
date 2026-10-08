#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "../Util/ArrowInfo.h"

class Arrow;
class GraphState;

class ConnectionOps
{
public:

    ConnectionOps(GraphState& graphState, juce::UndoManager& undoManager, const ArrowInfo& currentArrowInfo)
        : graphState(graphState), undoManager(undoManager), currentArrowInfo(currentArrowInfo) {}

    void            disconnect        (const Arrow* arrow);
    juce::ValueTree connectionTreeFor (const Arrow* arrow) const;
    void            connect           (int parentNodeId, int childNodeId, ArrowType rootConnectionType);
    void            applySelectedArrowInfo(int parentNodeId, int childNodeId, ArrowType rootConnectionType);

    bool connectsToOtherTreeRoot(int parentNodeId, int childNodeId) const;
    bool canBeTraversalArrow(const Arrow* arrow) const;

    void setArrowType       (const Arrow* arrow, ArrowType arrowType);
    bool connectsToModulatorRoot(const Arrow* arrow) const;
    void setArrowSync           (const Arrow* arrow, bool shouldSync);

private:

    struct ArrowOwnership
    {
        int ownerNodeId;
        int childNodeId;
    };

    ArrowOwnership resolveOwnership(const Arrow* arrow) const;

    GraphState&        graphState;
    juce::UndoManager& undoManager;
    const ArrowInfo&   currentArrowInfo;
};
