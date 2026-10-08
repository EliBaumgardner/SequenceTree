#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/Theme.h"

class NodeCanvas;

class Bar : public juce::Component
{
public:

    enum class Orientation { Horizontal, Vertical };
    enum class Surface     { Solid, Frosted };

    Bar(Orientation orientation, float contentInsetRatio);
    Bar(NodeCanvas& nodeCanvas, Orientation orientation, float contentInsetRatio);

    void paint(juce::Graphics& graphics) final;

protected:

    virtual void paintOverBar(juce::Graphics& graphics)
    {
    }

    juce::Rectangle<int> getContentBounds() const;

    static constexpr int contentSpacing = 12;

private:

    Orientation orientation;
    float       contentInsetRatio;
    Surface     surface  = Surface::Solid;
    NodeCanvas* nodeCanvas = nullptr;
};
