/*
  ==============================================================================

    TitleBar.cpp
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "TitleBar.h"

TitleBar::TitleBar()
{
    setLookAndFeel(ComponentContext::lookAndFeel);
    addAndMakeVisible(playButton);
    addAndMakeVisible(editor);
    addAndMakeVisible(syncButton);

    playButton.onClick = [=](){ toggled(); };
}

void TitleBar::paint(juce::Graphics& g)
{
    if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawTitleBar(g,*this); }
}

void TitleBar::resized()
{
    auto bounds = getLocalBounds().reduced(2);

    // Determine square button size based on smaller of desired width or TitleBar height
    int desiredWidth = bounds.getWidth() / 25;
    int buttonSize = std::min(desiredWidth, bounds.getHeight());
    int spacing = 3;

    playButton.setBounds(bounds.removeFromLeft(buttonSize));
    editor.setBounds(bounds.removeFromLeft(bounds.getWidth() / 12.5).withTrimmedTop(2).withTrimmedBottom(2).withTrimmedLeft(spacing));
    syncButton.setBounds(bounds.removeFromLeft(buttonSize).withTrimmedLeft(spacing)); // now width scales with TitleBar

    editor.refit();
}
