#pragma once

#include "../Util/ArrowInfo.h"

#include <juce_data_structures/juce_data_structures.h>

class GraphState;

class ArrowBindingOps {

public:

    explicit ArrowBindingOps(GraphState& graphState) : graphState(graphState) {}

    void syncPitchBindings  (int nodeId, juce::UndoManager* undoManager);
    void clearArrowDurations(int nodeId, juce::UndoManager* undoManager);

    static void      setArrowInfo(juce::ValueTree arrowTree, const ArrowInfo& arrowInfo, juce::UndoManager* undoManager);
    static ArrowInfo getArrowInfo(const juce::ValueTree& arrowTree);

private:

    void applyArrowPitchOffset(juce::ValueTree arrowTree, int targetNodeId, int deltaX, int deltaY, juce::UndoManager* undoManager);

    GraphState& graphState;
};
