#pragma once

#include "Node.h"

class GraphState;

class TraversalFlagNode : public Node
{
public:

    TraversalFlagNode(juce::UndoManager& undoManager, GraphState& graphState);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    bool hitTest(int x, int y) override;
    void bindToTree() override;
    void respondToClick(juce::Point<int> localPoint) override;

private:

    juce::Path buildTrianglePath() const;

    ValueEditor traversalNumEditor;
    GraphState& graphState;
};
