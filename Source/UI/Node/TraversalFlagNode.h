#pragma once

#include "Node.h"

class GraphState;

class TraversalFlagNode : public Node
{
public:

    TraversalFlagNode(juce::UndoManager& undoManager, GraphState& graphState);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void bindToTree    () override;
    void respondToClick(juce::Point<int> localPoint) override;

    bool hitTest(int x, int y) override;

private:

    juce::Path buildTrianglePath() const;

    ValueEditor traversalNumEditor;
    GraphState& graphState;
};
