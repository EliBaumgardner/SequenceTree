#include "Bar.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"

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
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);

    if (style.surface == Surface::Frosted) {
        lookAndFeel.drawFrostedGlass(graphics, *this, *applicationContext.canvas->getParentComponent(), getLocalBounds());
    }
    else {
        graphics.setColour(lookAndFeel.barColour);
        graphics.fillRect(getLocalBounds());
    }

    paintOverBar(graphics);
}

juce::Rectangle<int> Bar::getContentBounds() const
{
    if (style.orientation == Orientation::Horizontal) {
        return getLocalBounds().reduced(juce::roundToInt(getHeight() * style.contentInsetRatio));
    }

    return getLocalBounds().reduced(juce::roundToInt(getWidth() * style.contentInsetRatio));
}
