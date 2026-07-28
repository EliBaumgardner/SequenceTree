#include "Bar.h"
#include "Theme/CustomLookAndFeel.h"

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
    return getLocalBounds().reduced(style.contentInset);
}

void Bar::drawSeparator(juce::Graphics& g, int position)
{
    g.setColour(CustomLookAndFeel::get(*this).getTextColour().withAlpha(0.12f));

    if (style.orientation == Orientation::horizontal) {
        const float inset = getHeight() * separatorInsetRatio;
        g.drawVerticalLine(position, inset, getHeight() - inset);
        return;
    }

    const float inset = getWidth() * separatorInsetRatio;
    g.drawHorizontalLine(position, inset, getWidth() - inset);
}

void Bar::paintBackground(juce::Graphics& g)
{
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);
    const auto bounds = getLocalBounds().toFloat();

    if (style.background == Background::flat) {
        g.setColour(lookAndFeel.buttonBarColour);
        g.fillRect(bounds);
        return;
    }

    lookAndFeel.drawBar(g, bounds, style.background == Background::litFromTop);
}

void Bar::paint(juce::Graphics& g)
{
    paintBackground(g);

    paintOverBar(g);
}
