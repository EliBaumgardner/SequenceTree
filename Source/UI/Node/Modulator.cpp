//
// Created by Eli Baumgardner on 10/6/25.
//

#include "Modulator.h"

Modulator::Modulator(ApplicationContext& context) : Node(context)
{
}

void Modulator::resized() {
    auto editorArea = getLocalBounds().reduced(10.0f);

    upButton.setBounds(editorArea.removeFromTop(4.0f));
    downButton.setBounds(editorArea.removeFromBottom(4.0f));

    nodeTextEditor.get()->setBounds(editorArea);
    nodeTextEditor.get()->setJustification(juce::Justification::centred);
}

void Modulator::paint(juce::Graphics& g) {

    auto bounds = getLocalBounds().toFloat();
    auto squareBorder = bounds.reduced(2.5f);
    auto squareSelect = bounds.reduced(0.5f);
    auto squareHover = bounds.reduced(4.5f);
    auto squareFill = bounds.reduced(5.5f);

    g.setColour(juce::Colours::black);
    g.drawRect(squareBorder, 1.0f);

    float pulseExpansion = 4.0f * std::sin(pulsePhase * juce::MathConstants<float>::pi);
    auto pulsedFill = isHighlighted ? squareFill.expanded(pulseExpansion) : squareFill;

    g.setColour(isHighlighted ? nodeColour.darker() : nodeColour);
    g.fillRect(pulsedFill);

    if (isHovered)
        g.drawRect(squareHover, 2.0f);

    if (isSelected)
    {
        juce::Path dottedPath;
        dottedPath.addRectangle(squareSelect);

        juce::PathStrokeType stroke(0.325f);

        float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
}

