#include "CustomLookAndFeel.h"
#include "../Node/Node.h"

juce::Rectangle<float> CustomLookAndFeel::getNodeCircleBounds(juce::Rectangle<float> componentBounds)
{
    float diameter = componentBounds.getWidth() - nodeCirclePad - nodeShadowOffsetX - nodeShadowBlur;
    return juce::Rectangle<float>(diameter, diameter)
               .withPosition(componentBounds.getX() + nodeCirclePad, componentBounds.getY() + nodeCirclePad);
}

static void paintNodeShadow(juce::Graphics& graphics, juce::Rectangle<float> shapeBounds)
{
    const float innerR       = shapeBounds.getWidth() * 0.5f;
    const float outerR       = innerR + Theme::nodeShadowBlur;
    const auto  shadowCenter = shapeBounds.getCentre() + juce::Point<float>(Theme::nodeShadowOffsetX, Theme::nodeShadowOffsetY);
    const auto  shadowBounds = juce::Rectangle<float>(outerR * 2.0f, outerR * 2.0f).withCentre(shadowCenter);

    juce::ColourGradient gradient(juce::Colours::black.withAlpha(0.15f), shadowCenter.x, shadowCenter.y,
                                  juce::Colours::black.withAlpha(0.0f), shadowCenter.x + outerR, shadowCenter.y, true);
    gradient.addColour(innerR / outerR, juce::Colours::black.withAlpha(0.10f));

    graphics.setGradientFill(gradient);
    graphics.fillEllipse(shadowBounds);
}

void CustomLookAndFeel::drawNode(juce::Graphics& graphics, const NodeVisual& visual)
{
    auto circleBounds = getNodeCircleBounds(visual.bounds);
    auto circleFill   = circleBounds.reduced(0.5f);
    auto circleSelect = circleBounds.expanded(selectionRingGap + selectionRingWidth * 0.5f);
    auto circleHover  = circleBounds.reduced(0.5f);

    paintNodeShadow(graphics, circleBounds);

    graphics.setColour(visual.colour);
    graphics.fillEllipse(circleFill);

    if (visual.hasInnerRim) {
        graphics.setColour(visual.colour.brighter(0.35f));
        graphics.drawEllipse(circleFill.reduced(encapsulatorRimInset), encapsulatorRimWidth);
    }

    if (visual.isEncapsulationRinged) {
        graphics.setColour(visual.encapsulationRingColour);
        graphics.drawEllipse(circleFill.expanded(encapsulationRingWidth * 0.5f), encapsulationRingWidth);
    }

    float ringInset = highlightRingWidth * 0.5f;

    for (const auto& highlight : visual.highlights) {
        graphics.setColour(highlight.second);
        graphics.drawEllipse(circleFill.reduced(ringInset), highlightRingWidth);

        ringInset += highlightRingSpacing;
    }

    if (visual.isHovered) {
        graphics.setColour(hoverRingColour);
        graphics.drawEllipse(circleHover, hoverRingWidth);
    }

    if (visual.isSelected) {
        graphics.setColour(selectionRingColour);
        graphics.drawEllipse(circleSelect, selectionRingWidth);
    }

    if (visual.isOutlined) {
        graphics.setColour(spanOutlineColour);
        graphics.drawEllipse(circleBounds.expanded(spanOutlineGap + spanOutlineWidth * 0.5f), spanOutlineWidth);
    }
}

void CustomLookAndFeel::drawModulatorNode(juce::Graphics& graphics, const NodeVisual& visual)
{
    auto squareBounds = visual.bounds;
    auto squareFill   = squareBounds.reduced(0.5f);
    auto squareHover  = squareBounds.reduced(0.5f).expanded(hoverRingWidth * 0.5f);
    auto squareRim    = squareBounds.reduced(selectionRimWidth * 0.5f);

    paintNodeShadow(graphics, squareBounds);

    graphics.setColour(visual.colour);
    graphics.fillRect(squareFill);

    float ringInset = highlightRingWidth * 0.5f;

    for (const auto& highlight : visual.highlights) {
        graphics.setColour(highlight.second);
        graphics.drawRect(squareFill.reduced(ringInset), highlightRingWidth);

        ringInset += highlightRingSpacing;
    }

    if (visual.isHovered) {
        graphics.setColour(hoverRingColour);
        graphics.drawRect(squareHover, hoverRingWidth);
    }

    if (visual.isSelected) {
        graphics.setColour(selectionRingColour);
        graphics.drawRect(squareRim, selectionRimWidth);
    }
}

void CustomLookAndFeel::drawRootRectangle(juce::Graphics& graphics, juce::Rectangle<float> bounds)
{
    graphics.setColour(baseDarkColour2.darker());
    graphics.fillRect(bounds);
}
