#include "RuleTabBar.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

RuleTabBar::RuleTabBar()
    : ArsNovaUI::Bar(Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio)
{
    addAndMakeVisible(tabStrip);
    addAndMakeVisible(transportPane);
    addAndMakeVisible(undoRedoPane);
}

void RuleTabBar::paintOverBar(juce::Graphics& graphics)
{
    const int borderHeight = juce::roundToInt(ArsNovaUI::Theme::borderThickness);
    const int borderTop    = getHeight() - borderHeight;

    graphics.setColour(ArsNovaUI::CustomLookAndFeel::get(*this).borderColour);
    graphics.fillRect(0, borderTop, getWidth(), borderHeight);
}

void RuleTabBar::resized()
{
    auto bounds = getContentBounds();

    const int paneHeight     = bounds.getHeight();
    const int undoRedoWidth  = undoRedoPane.idealWidth(paneHeight);
    const int transportWidth = transportPane.idealWidth(paneHeight);

    undoRedoPane.setBounds(bounds.removeFromRight(undoRedoWidth));

    bounds.removeFromRight(ArsNovaUI::Theme::windowGutter);

    transportPane.setBounds(bounds.removeFromRight(transportWidth));

    const int tabStripRight = transportPane.getX() - ArsNovaUI::Theme::windowGutter;

    tabStrip.setBounds(getLocalBounds().withRight(tabStripRight));
}
