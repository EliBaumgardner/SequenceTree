#include "BottomBar.h"
#include "Theme/CustomLookAndFeel.h"

BottomBar::BottomBar(ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    addAndMakeVisible(colourSelector);

    applicationContext.onNodeSelected = [this](Node* node, bool selected) {
        colourSelector.setNode(selected ? node : nullptr);
    };
}

void BottomBar::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawBottomBar(g, *this);
}

void BottomBar::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    colourSelector.setBounds(bounds.removeFromLeft(bounds.getHeight()));
}
