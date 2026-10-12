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

    void connect               (int parentNodeId, int childNodeId, ArrowType rootConnectionType);
    void applySelectedArrowInfo(int parentNodeId, int childNodeId, ArrowType rootConnectionType);
    void disconnect            (const Arrow* arrow);

    void setArrowType(const Arrow* arrow, ArrowType arrowType);
    void setArrowSync(const Arrow* arrow, bool shouldSync);

    bool connectsToOtherTreeRoot(int parentNodeId, int childNodeId) const;
    bool canBeTraversalArrow    (const Arrow* arrow) const;
    bool connectsToModulatorRoot(const Arrow* arrow) const;

    juce::ValueTree connectionTreeFor(const Arrow* arrow) const;

private:

    struct ArrowOwnership;

    ArrowOwnership resolveOwnership(const Arrow* arrow) const;

    struct ArrowOwnership
    {
        int ownerNodeId;
        int childNodeId;
    };

    GraphState&        graphState;
    juce::UndoManager& undoManager;
    const ArrowInfo&   currentArrowInfo;
};
