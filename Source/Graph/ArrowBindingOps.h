#pragma once

#include "../Util/ArrowInfo.h"

#include <juce_data_structures/juce_data_structures.h>

class GraphState;

class ArrowBindingOps {

public:

    explicit ArrowBindingOps(GraphState& graphState) : graphState(graphState) {}

    static void      setArrowInfo(juce::ValueTree arrowTree, const ArrowInfo& arrowInfo,
                                  juce::UndoManager* undoManager);
    static ArrowInfo getArrowInfo(const juce::ValueTree& arrowTree);

    void syncPitchBindings  (int nodeId, juce::UndoManager* undoManager);
    void clearArrowDurations(int nodeId, juce::UndoManager* undoManager);

    void applyNodeBinding(ArrowInfo& arrowInfo, int nodeId,
                          const juce::ValueTree& newArrow = {}) const;

private:

    void applyArrowPitchOffset(juce::ValueTree arrowTree, int targetNodeId,
                               int deltaX, int deltaY, juce::UndoManager* undoManager);

    GraphState& graphState;
};
