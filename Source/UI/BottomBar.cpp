#include "BottomBar.h"
#include "Theme/CustomLookAndFeel.h"

BottomBar::BottomBar(ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    paintTool = std::make_unique<PaintTool>(applicationContext);
    addAndMakeVisible(*paintTool);

    arrowTool = std::make_unique<ArrowTool>(applicationContext);
    addAndMakeVisible(*arrowTool);
}

void BottomBar::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient gradient(theme.barColour.darker(0.04f),   0, bounds.getY(),
                                  theme.barColour.brighter(0.06f), 0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int) bounds.getY(), bounds.getX(), bounds.getRight());

    g.setColour(theme.barColour.brighter(0.12f));
    g.drawHorizontalLine((int) bounds.getBottom() - 1, bounds.getX(), bounds.getRight());
}

void BottomBar::resized()
{
    auto bounds= getLocalBounds().reduced(4);
    int  height               = bounds.getHeight();

    paintTool->setBounds(bounds.removeFromRight(height));
    bounds.removeFromRight(cellGap);
    arrowTool->setBounds(bounds.removeFromRight(height));
}
