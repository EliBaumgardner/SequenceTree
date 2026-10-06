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
    void respondToClick(juce::Point<int> localPoint) override;

private:

    juce::Path buildTrianglePath() const;

    ValueEditor traversalNumEditor;
};
