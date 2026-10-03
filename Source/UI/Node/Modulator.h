#pragma once

#include <juce_graphics/juce_graphics.h>
#include "Node.h"

class NodeCanvas;

class Modulator : public Node
{
    public:

    explicit Modulator(const ApplicationContext& context);

    void  paint(juce::Graphics& graphics) override;

    juce::Rectangle<float> getSquareBounds() const;
    bool  hitTest(int x, int y) override;
    void  bindValueEditorForMode() override;
    float getBodyExtent(juce::Point<float> approachDirection) const override;

    static constexpr int minimumPitchOffset = -48;
    static constexpr int maximumPitchOffset =  48;

    static constexpr float equalAreaSideFactor = 0.8862f;
};
