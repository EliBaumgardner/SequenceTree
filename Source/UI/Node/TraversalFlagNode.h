#pragma once

#include "Node.h"

/*
    The Node for a traversal flag, drawn as a triangle that points along the arrow arriving at it
    (incomingAngle). Its editor takes a traversal as TraversalFlagFormat text: "+2" spawns
    traversal 2, "-2" removes it, and a letter after the number picks the instance. The value is
    stored as TraversalFlagValue in the flag's tree, and typing a traversal id creates that
    traversal's settings in GraphState's traversals if they do not exist. It shows the count limit
    and switch count limit badges and no sub-loop badge.

    hitTest accepts the triangle and the visible badges. NodeManager creates it for a
    TraversalFlagData tree, and the arrows leaving it fade in only while it is hovered.
*/

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

    ArsNovaUI::ValueEditor traversalNumEditor;
    GraphState& graphState;
};
