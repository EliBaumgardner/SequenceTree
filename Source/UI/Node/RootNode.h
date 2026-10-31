#pragma once

#include "Node.h"
#include "../Buttons/RootRectangle.h"

/*
    The Node for the root of a tree, where a walk starts. It is drawn as the circle with a
    RootRectangle on its left edge, which holds the text listing the traversals the root starts,
    such as "1 2b, 3". Its sub-loop editor is the root's loop limit rather than a sub-loop limit.

    equipTraversals reads the keys from that text with TraversalFlagFormat::parseKeys and makes
    the root's TraversalChildrenIds list match: entries for keys no longer typed are removed,
    new keys are added, and each key's traversal gets its settings in GraphState's traversals.
    It runs whenever the text changes. bindToTree does the reverse, writing the root's current
    keys into the text. getNodeCentre is the centre of the circle rather than of the whole
    component, and getBodyExtent counts the rectangle as part of the body.

    NodeManager creates it for a RootNodeData tree and NodeCanvas calls equipTraversals on every
    root when playback starts.
*/

class GraphState;

class RootNode : public Node
{
public:

    RootNode(juce::UndoManager& undoManager, GraphState& graphState);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void equipTraversals();
    void bindToTree     () override;

    juce::Point<int> getNodeCentre() const override;

    juce::Rectangle<int> getBoundsAt(juce::Point<int> position) const override;

    float getBodyExtent(juce::Point<float> approachDirection) const override;

    static constexpr int loopLimitRectangleWidth = 10;
    static constexpr int rectangleOverlap        = 8;

    RootRectangle rootRectangle;

private:

    GraphState& graphState;
};
