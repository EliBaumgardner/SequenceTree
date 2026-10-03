#pragma once

#include "Node.h"
#include "../Buttons/RootRectangle.h"

class RootNode : public Node
{
public:

    static constexpr int loopLimitRectangleWidth = 10;
    static constexpr int rectangleOverlap        = 8;

    RootRectangle rootRectangle;

    explicit RootNode(const ApplicationContext& context);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void equipTraversals();
    void bindToTree() override;
    juce::Point<int> getNodeCentre() const override;
    float            getBodyExtent(juce::Point<float> approachDirection) const override;
};
