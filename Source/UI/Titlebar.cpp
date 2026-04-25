/*
  ==============================================================================

    TitleBar.cpp
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baimgardner

  ==============================================================================
*/


#include "Node/NodeCanvas.h"
#include "CustomLookAndFeel.h"
#include "../Core/PluginProcessor.h"

#include "Titlebar.h"


Titlebar::Titlebar(ApplicationContext& context)
    : applicationContext(context),
      buttonPane(context),
      displaySelector(context),
      tempoDisplay(context),
      colorIntensityControl(context),
      playButton(context),
      resetButton(context),
      undoRedoPane(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);
    addAndMakeVisible(playButton);
    addAndMakeVisible(resetButton);
    addAndMakeVisible(tempoDisplay);
    addAndMakeVisible(colorIntensityControl);
    addAndMakeVisible(buttonPane);
    addAndMakeVisible(displaySelector);
    addAndMakeVisible(undoRedoPane);

    playButton.onClick = [this]() {
        NodeCanvas& canvas = *applicationContext.canvas;
        jassert(&canvas);

        canvas.start = !canvas.start;
        canvas.setProcessorPlayblack(canvas.start);
    };

    resetButton.onClick = [this]() {
        applicationContext.processor->resetRequested.store(true);
        if (auto* canvas = applicationContext.canvas)
            canvas->resetAllArrowProgress();
    };

    auto applyMultiplier = [this]() {
        double value = tempoDisplay.editor.getText().getDoubleValue();
        if (value > 0.0)
            applicationContext.processor->tempoMultiplier.store(value);
        else
            tempoDisplay.editor.setText(juce::String(applicationContext.processor->tempoMultiplier.load()), false);
    };

    tempoDisplay.editor.onReturnKey = applyMultiplier;
    tempoDisplay.editor.onFocusLost = applyMultiplier;
}

void Titlebar::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawTitleBar(g, *this);

    auto& laf = CustomLookAndFeel::get(*this);
    g.setColour(laf.getTextColour().withAlpha(0.12f));
    float inset = getHeight() * 0.22f;

    auto drawSep = [&](int x) {
        g.drawVerticalLine(x, inset, getHeight() - inset);
    };

    drawSep((resetButton.getRight() + tempoDisplay.getX()) / 2);
    drawSep((tempoDisplay.getRight() + colorIntensityControl.getX()) / 2);
    drawSep((colorIntensityControl.getRight() + undoRedoPane.getX()) / 2);
    drawSep((buttonPane.getRight() + displaySelector.getX()) / 2);
}

void Titlebar::resized()
{
    auto bounds = getLocalBounds().reduced(4.0f);
    int spacing = 12;

    int buttonSize = std::min(bounds.getHeight(), bounds.getWidth() / 25);
    int tempoDisplayWidth = bounds.getWidth() / 8;
    int colorIntensityWidth = bounds.getWidth() / 25;
    int buttonPaneWidth = bounds.getWidth() / 8;
    int displaySelectorWidth = bounds.getWidth() / 8;
    int undoRedoPaneWidth = bounds.getWidth() / 8;

    auto area = bounds;

    playButton.setBounds(area.removeFromLeft(buttonSize));
    area.removeFromLeft(spacing);

    resetButton.setBounds(area.removeFromLeft(buttonSize));
    area.removeFromLeft(spacing);

    tempoDisplay.setBounds(area.removeFromLeft(tempoDisplayWidth));
    area.removeFromLeft(spacing);

    colorIntensityControl.setBounds(area.removeFromLeft(colorIntensityWidth));
    area.removeFromLeft(spacing);

    undoRedoPane.setBounds(area.removeFromLeft(undoRedoPaneWidth));
    area.removeFromLeft(spacing);

    displaySelector.setBounds(area.removeFromRight(displaySelectorWidth));
    area.removeFromRight(spacing);

    buttonPane.setBounds(area.removeFromRight(buttonPaneWidth));
}
