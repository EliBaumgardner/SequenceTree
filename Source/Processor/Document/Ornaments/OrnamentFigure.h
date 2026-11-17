#pragma once

#include "Core/Model/RTData.h"

#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>

#include <vector>

enum class OrnamentStatus { Draft, Published, ChangesPending };

class OrnamentFigure
{
public:

    static void setNodePosition(juce::ValueTree ornament, int nodeId, juce::Point<int> position, juce::UndoManager* undoManager);
    static void removeNode     (juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager);
    static void toggleHost     (juce::ValueTree ornament, int nodeId, juce::UndoManager* undoManager);

    static void addDanglingArrow   (juce::ValueTree ornament, int nodeId, juce::Point<int> tipOffset, juce::UndoManager* undoManager);
    static void removeDanglingArrow(juce::ValueTree ornament, juce::UndoManager* undoManager);
    static void setLastNodeLength  (juce::ValueTree ornament, juce::Point<int> tipOffset, juce::UndoManager* undoManager);

    static void publish(juce::ValueTree ornament, juce::UndoManager* undoManager);

    static juce::ValueTree addNode(juce::ValueTree ornament, int parentNodeId, juce::Point<int> position, juce::UndoManager* undoManager);

    static bool                      isPublishable(const juce::ValueTree& ornament);
    static std::vector<OrnamentStep> steps        (const juce::ValueTree& figure);

    static OrnamentStatus            statusOf     (const juce::ValueTree& ornament);

    static constexpr int maximumOffset          = 48;
    static constexpr int defaultVelocityPercent = 100;
    static constexpr int maximumVelocityPercent = 200;
    static constexpr int noHostId               = -1;

private:

    static void walkChain(juce::ValueTree node, const juce::ValueTree& figure, int hostId, std::vector<OrnamentStep>& steps);
};
