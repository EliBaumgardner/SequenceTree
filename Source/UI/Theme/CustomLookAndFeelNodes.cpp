//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "../Node/Node.h"

juce::Rectangle<float> CustomLookAndFeel::getNodeCircleBounds(juce::Rectangle<float> componentBounds)
{
    float diameter = componentBounds.getWidth() - nodeCirclePad - nodeShadowOffsetX - nodeShadowBlur;
    return juce::Rectangle<float>(diameter, diameter)
               .withPosition(componentBounds.getX() + nodeCirclePad, componentBounds.getY() + nodeCirclePad);
}

static void paintNodeShadow(juce::Graphics& g, juce::Rectangle<float> shapeBounds)
{
    const float innerR       = shapeBounds.getWidth() * 0.5f;
    const float outerR       = innerR + Theme::nodeShadowBlur;
    const auto  shadowCenter = shapeBounds.getCentre() + juce::Point<float>(Theme::nodeShadowOffsetX,
                                                                           Theme::nodeShadowOffsetY);
    const auto  shadowBounds = juce::Rectangle<float>(outerR * 2.0f, outerR * 2.0f).withCentre(shadowCenter);

    juce::ColourGradient gradient(
        juce::Colours::black.withAlpha(0.15f), shadowCenter.x, shadowCenter.y,
        juce::Colours::black.withAlpha(0.0f),  shadowCenter.x + outerR, shadowCenter.y,
        true);
    gradient.addColour(innerR / outerR, juce::Colours::black.withAlpha(0.10f));

    g.setGradientFill(gradient);
    g.fillEllipse(shadowBounds);
}

void CustomLookAndFeel::drawNode(juce::Graphics& g, const NodeVisual& visual)
{
    auto circleBounds = getNodeCircleBounds(visual.bounds);
    auto circleFill   = circleBounds.reduced(0.5f);
    auto circleSelect = circleBounds.expanded(selectionRingGap + selectionRingWidth * 0.5f);
    auto circleHover  = circleBounds.reduced(0.5f);

    paintNodeShadow(g, circleBounds);

    g.setColour(visual.colour);
    g.fillEllipse(circleFill);

    if (visual.hasInnerRim) {
        g.setColour(visual.colour.brighter(0.35f));
        g.drawEllipse(circleFill.reduced(encapsulatorRimInset), encapsulatorRimWidth);
    }

    if (visual.isEncapsulationRinged) {
        g.setColour(visual.encapsulationRingColour);
        g.drawEllipse(circleFill.expanded(encapsulationRingWidth * 0.5f), encapsulationRingWidth);
    }

    float ringInset = highlightRingWidth * 0.5f;

    for (const auto& highlight : visual.highlights) {
        g.setColour(highlight.second);
        g.drawEllipse(circleFill.reduced(ringInset), highlightRingWidth);
        ringInset += highlightRingSpacing;
    }

    if (visual.isHovered) {
        g.setColour(hoverRingColour);
        g.drawEllipse(circleHover, hoverRingWidth);
    }

    if (visual.isSelected) {
        g.setColour(selectionRingColour);
        g.drawEllipse(circleSelect, selectionRingWidth);
    }

    if (visual.isOutlined) {
        g.setColour(spanOutlineColour);
        g.drawEllipse(circleBounds.expanded(spanOutlineGap + spanOutlineWidth * 0.5f), spanOutlineWidth);
    }
}

void CustomLookAndFeel::drawModulatorNode(juce::Graphics& g, const NodeVisual& visual)
{
    auto squareBounds = visual.bounds;
    auto squareFill   = squareBounds.reduced(0.5f);
    auto squareHover  = squareBounds.reduced(0.5f).expanded(hoverRingWidth * 0.5f);
    auto squareRim    = squareBounds.reduced(selectionRimWidth * 0.5f);

    paintNodeShadow(g, squareBounds);

    g.setColour(visual.colour);
    g.fillRect(squareFill);

    float ringInset = highlightRingWidth * 0.5f;

    for (const auto& highlight : visual.highlights) {
        g.setColour(highlight.second);
        g.drawRect(squareFill.reduced(ringInset), highlightRingWidth);
        ringInset += highlightRingSpacing;
    }

    if (visual.isHovered) {
        g.setColour(hoverRingColour);
        g.drawRect(squareHover, hoverRingWidth);
    }

    if (visual.isSelected) {
        g.setColour(selectionRingColour);
        g.drawRect(squareRim, selectionRimWidth);
    }
}

void CustomLookAndFeel::drawRootRectangle(juce::Graphics &g, juce::Rectangle<float> bounds)
{
    g.setColour(baseDarkColour2.darker());
    g.fillRect(bounds);
}
