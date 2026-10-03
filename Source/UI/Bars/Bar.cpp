#include "Bar.h"
#include "../Theme/CustomLookAndFeel.h"

Bar::Bar(const ApplicationContext& context, Style style)
    : applicationContext(context), style(style)
{
    setLookAndFeel(applicationContext.lookAndFeel);
}

Bar::~Bar()
{
    setLookAndFeel(nullptr);
}

void Bar::paint(juce::Graphics& graphics)
{
    graphics.setColour(CustomLookAndFeel::get(*this).barColour);
    graphics.fillRect(getLocalBounds());

    paintOverBar(graphics);
}

juce::Rectangle<int> Bar::getContentBounds() const
{
    if (style.orientation == Orientation::Horizontal) {
        return getLocalBounds().reduced(juce::roundToInt(getHeight() * style.contentInsetRatio));
    }

    return getLocalBounds().reduced(juce::roundToInt(getWidth() * style.contentInsetRatio));
}
