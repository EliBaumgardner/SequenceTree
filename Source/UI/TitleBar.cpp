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
    addAndMakeVisible(selectionBar);
    addAndMakeVisible(displaySelector);

    playButton.onClick = [=](){ toggled(); };
}

void TitleBar::paint(juce::Graphics& g)
{
    if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawTitleBar(g,*this); }
}

void TitleBar::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    int spacing = 6;

    int buttonSize = std::min(bounds.getHeight(), bounds.getWidth() / 25);
    int editorWidth = bounds.getWidth() / 8;
    int selectionBarWidth = bounds.getWidth() / 5;
    int displaySelectorWidth = bounds.getWidth() / 10;

    auto area = bounds;

    playButton.setBounds(area.removeFromLeft(buttonSize));
    area.removeFromLeft(spacing);
    editor.setBounds(area.removeFromLeft(editorWidth));
    area.removeFromLeft(spacing);
    syncButton.setBounds(area.removeFromLeft(buttonSize));

    area.removeFromRight(spacing);
    displaySelector.setBounds(area.removeFromRight(displaySelectorWidth));
    area.removeFromRight(spacing);
    selectionBar.setBounds(area.removeFromRight(selectionBarWidth));

    editor.refit();
}
