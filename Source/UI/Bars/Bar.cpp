#include "Bar.h"
#include "../Theme/CustomLookAndFeel.h"

Bar::Bar(ApplicationContext& context, Style style)
    : applicationContext(context), style(style)
{
    setLookAndFeel(applicationContext.lookAndFeel);
}

Bar::~Bar()
{
    setLookAndFeel(nullptr);
}

juce::Rectangle<int> Bar::getContentBounds() const
{
    if (style.orientation == Orientation::horizontal) {
        return getLocalBounds().reduced(juce::roundToInt(getHeight() * style.contentInsetRatio));
    }

    return getLocalBounds().reduced(juce::roundToInt(getWidth() * style.contentInsetRatio));
}

void Bar::drawSeparator(juce::Graphics& g, int position)
{
    g.setColour(CustomLookAndFeel::get(*this).textColour.withAlpha(0.12f));

    if (style.orientation == Orientation::horizontal) {
        const float inset = getHeight() * separatorInsetRatio;
        g.drawVerticalLine(position, inset, getHeight() - inset);
        return;
    }

    const float inset = getWidth() * separatorInsetRatio;
    g.drawHorizontalLine(position, inset, getWidth() - inset);
}

void Bar::paint(juce::Graphics& g)
{
    g.setColour(CustomLookAndFeel::get(*this).barColour);
    g.fillRect(getLocalBounds());

    paintOverBar(g);
}
