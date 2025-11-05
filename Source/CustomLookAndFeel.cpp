//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "Node/Node.h"

void CustomLookAndFeel::drawNode(juce::Graphics& g,const Node& node) {

    auto bounds = node.getLocalBounds().toFloat();
    auto circleBorder = bounds.reduced(2.5f);
    auto circleSelect = bounds.reduced(0.5f);
    auto circleHover = bounds.reduced(4.5f);
    auto circleFill = bounds.reduced(5.5f);

    // g.setColour(juce::Colours::black);
    // g.drawEllipse(circleBorder, 1.0f);

    g.setColour(node.isHighlighted ? nodeColour.darker() : nodeColour);
    g.fillEllipse(circleFill);

    if (node.isHovered) { g.drawEllipse(circleHover, 2.0f); }

    if (node.isSelected)
    {
        juce::Path dottedPath;
        dottedPath.addEllipse(circleSelect);

        juce::PathStrokeType stroke(0.325f);

        float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
}