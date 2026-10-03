#pragma once

#include "Node.h"

class TraversalFlagNode : public Node
{
public:

    explicit TraversalFlagNode(const ApplicationContext& context);

    void paint(juce::Graphics& graphics) override;
    void resized() override;
    bool hitTest(int x, int y) override;
    void bindToTree() override;

private:

    juce::Path buildTrianglePath() const;

    juce::Colour outlineColour = juce::Colours::black;
    ValueEditor  traversalNumEditor;
};
