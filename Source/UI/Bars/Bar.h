#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/Theme.h"

class Bar : public juce::Component
{
public:

    enum class Orientation;

    Bar(Orientation orientation, float contentInsetRatio);
    Bar(juce::Component& backdrop, Orientation orientation, float contentInsetRatio);

    void paint(juce::Graphics& graphics) final;

    enum class Orientation { Horizontal, Vertical };
    enum class Surface     { Solid, Frosted };

    static constexpr int contentSpacing = 12;

protected:

    virtual void paintOverBar(juce::Graphics& graphics) {}

    juce::Rectangle<int> getContentBounds() const;

private:

    Orientation      orientation;
    float            contentInsetRatio;
    Surface          surface  = Surface::Solid;
    juce::Component* backdrop = nullptr;
};
