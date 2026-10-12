#include "Bar.h"
#include "../Theme/CustomLookAndFeel.h"

Bar::Bar(Orientation orientation, float contentInsetRatio)
    : orientation(orientation), contentInsetRatio(contentInsetRatio)
{
}

Bar::Bar(juce::Component& backdrop, Orientation orientation, float contentInsetRatio)
    : orientation(orientation), contentInsetRatio(contentInsetRatio), surface(Surface::Frosted), backdrop(&backdrop)
{
}

void Bar::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);

    if (surface == Surface::Frosted) {
        lookAndFeel.drawFrostedGlass(graphics, *this, *backdrop, getLocalBounds());
    }
    else {
        graphics.setColour(lookAndFeel.barColour);
        graphics.fillRect(getLocalBounds());
    }

    paintOverBar(graphics);
}

juce::Rectangle<int> Bar::getContentBounds() const
{
    if (orientation == Orientation::Horizontal) {
        return getLocalBounds().reduced(juce::roundToInt(getHeight() * contentInsetRatio));
    }

    return getLocalBounds().reduced(juce::roundToInt(getWidth() * contentInsetRatio));
}
