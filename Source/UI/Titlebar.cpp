/*
  ==============================================================================

    TitleBar.cpp
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baimgardner

  ==============================================================================
*/


#include "Node/NodeCanvas.h"
#include "CustomLookAndFeel.h"
#include "../Util/PluginContext.h"

#include "Titlebar.h"


Titlebar::Titlebar()
{
    setLookAndFeel(ComponentContext::lookAndFeel);
    addAndMakeVisible(playButton);
    addAndMakeVisible(tempoDisplay);
    addAndMakeVisible(buttonPane);
    addAndMakeVisible(displaySelector);
    addAndMakeVisible(undoRedoPane);

    playButton.onClick = [=]() {
        NodeCanvas& canvas = *ComponentContext::canvas;
        jassert(&canvas);

        canvas.start = !canvas.start;
        canvas.setProcessorPlayblack(canvas.start);
    };
}

void Titlebar::paint(juce::Graphics& g)
{
    if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawTitleBar(g,*this); }
}

void Titlebar::resized()
{
    auto bounds = getLocalBounds().reduced(4.0f);
    int spacing = 6;

    int buttonSize = std::min(bounds.getHeight(), bounds.getWidth() / 25);
    int tempoDisplayWidth = bounds.getWidth() / 8;
    int buttonPaneWidth = bounds.getWidth() / 8;
    int displaySelectorWidth = bounds.getWidth() / 8;
    int undoRedoPaneWidth = bounds.getWidth() / 8;

    auto area = bounds;

    playButton.setBounds(area.removeFromLeft(buttonSize));
    area.removeFromLeft(spacing);

    tempoDisplay.setBounds(area.removeFromLeft(tempoDisplayWidth));
    area.removeFromLeft(spacing);

    undoRedoPane.setBounds(area.removeFromLeft(undoRedoPaneWidth));
    area.removeFromLeft(spacing);

    displaySelector.setBounds(area.removeFromRight(displaySelectorWidth));
    area.removeFromRight(spacing);

    buttonPane.setBounds(area.removeFromRight(buttonPaneWidth));
}
