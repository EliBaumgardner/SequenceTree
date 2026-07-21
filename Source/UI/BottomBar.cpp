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
    CustomLookAndFeel::get(*this).drawBottomBar(g, *this);
}

void BottomBar::resized()
{
    auto bounds= getLocalBounds().reduced(4);
    int  height               = bounds.getHeight();

    paintTool->setBounds(bounds.removeFromRight(height));
    bounds.removeFromRight(cellGap);
    arrowTool->setBounds(bounds.removeFromRight(height));
}
