//
// Created by Eli Baumgardner on 10/6/25.
//

#include "Modulator.h"

Modulator::Modulator(ApplicationContext& context) : Node(context)
{
    nodeType = NodeType::Modulator;
}

void Modulator::resized() {
    auto editorArea = getLocalBounds().reduced(10.0f);

    upButton->setBounds(editorArea.removeFromTop(4.0f));
    downButton->setBounds(editorArea.removeFromBottom(4.0f));

    nodeValueEditor.setBounds(editorArea);
}

void Modulator::paint(juce::Graphics& g) {

    const auto bounds = getLocalBounds().toFloat();
    const auto squareBorder = bounds.reduced(2.5f);
    const auto squareSelect = bounds.reduced(0.5f);
    const auto squareHover = bounds.reduced(4.5f);
    const auto squareFill = bounds.reduced(5.5f);

    g.setColour(juce::Colours::black);
    g.drawRect(squareBorder, 1.0f);

    const float pulseExpansion = 4.0f * std::sin(pulsePhase * juce::MathConstants<float>::pi);
    auto pulsedFill = squareFill;

    if (isHighlighted) {
        pulsedFill = squareFill.expanded(pulseExpansion);
        g.setColour(nodeColour.darker());
    } else {
        g.setColour(nodeColour);
    }

    g.fillRect(pulsedFill);

    if (isHovered) {
        g.drawRect(squareHover, 2.0f);
    }

    if (isSelected) {
        juce::Path dottedPath;
        dottedPath.addRectangle(squareSelect);

        const juce::PathStrokeType stroke(0.325f);

        const float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
}

