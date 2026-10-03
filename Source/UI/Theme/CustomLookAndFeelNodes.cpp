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

    juce::ColourGradient gradient(juce::Colours::black.withAlpha(Theme::nodeShadowAlpha), shadowCenter.x, shadowCenter.y,
                                  juce::Colours::black.withAlpha(0.0f), shadowCenter.x + outerR, shadowCenter.y, true);
    gradient.addColour(innerR / outerR, juce::Colours::black.withAlpha(Theme::nodeShadowAlpha * 0.6f));

    graphics.setGradientFill(gradient);
    graphics.fillEllipse(shadowBounds);
}

void CustomLookAndFeel::drawNode(juce::Graphics& graphics, const NodeVisual& visual)
{
    auto         circleBounds = getNodeCircleBounds(visual.bounds);
    auto         circleFill   = circleBounds.reduced(0.5f);
    auto         circleSelect = circleBounds.expanded(selectionRingGap + selectionRingWidth * 0.5f);
    juce::Colour fill         = visual.colour;
    juce::Colour outline      = borderStrongColour;

    if (visual.isHovered) {
        fill    = fill.brighter(hoverFillBrightness);
        outline = hoverRingColour;
    }

    if (! visual.highlights.empty()) {
        fill = fill.interpolatedWith(visual.highlights.begin()->second, highlightTintAmount);
    }

    paintNodeShadow(graphics, circleBounds);

    graphics.setColour(fill);
    graphics.fillEllipse(circleFill);

    graphics.setColour(outline);
    graphics.drawEllipse(circleFill, borderThickness);

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
    auto         squareBounds = visual.bounds;
    auto         squareFill   = squareBounds.reduced(0.5f);
    auto         squareSelect = squareBounds.expanded(selectionRingGap + selectionRingWidth * 0.5f);
    const float  cornerRadius = squareBounds.getWidth() * modulatorCornerRatio;
    juce::Colour fill         = visual.colour;
    juce::Colour outline      = borderStrongColour;

    if (visual.isHovered) {
        fill    = fill.brighter(hoverFillBrightness);
        outline = hoverRingColour;
    }

    if (! visual.highlights.empty()) {
        fill = fill.interpolatedWith(visual.highlights.begin()->second, highlightTintAmount);
    }

    paintNodeShadow(graphics, squareBounds);

    graphics.setColour(fill);
    graphics.fillRoundedRectangle(squareFill, cornerRadius);

    graphics.setColour(outline);
    graphics.drawRoundedRectangle(squareFill, cornerRadius, borderThickness);

    float ringInset = highlightRingWidth * 0.5f;

    for (const auto& highlight : visual.highlights) {
        graphics.setColour(highlight.second);
        graphics.drawRoundedRectangle(squareFill.reduced(ringInset), juce::jmax(0.0f, cornerRadius - ringInset), highlightRingWidth);

        ringInset += highlightRingSpacing;
    }

    if (visual.isSelected) {
        graphics.setColour(selectionRingColour);
        graphics.drawRoundedRectangle(squareSelect, cornerRadius + selectionRingGap, selectionRingWidth);
    }
}

void CustomLookAndFeel::drawRootRectangle(juce::Graphics& graphics, juce::Rectangle<float> bounds)
{
    const auto area = bounds.reduced(borderThickness * 0.5f);

    graphics.setColour(raisedColour);
    graphics.fillRoundedRectangle(area, paneCornerRadius);

    graphics.setColour(borderStrongColour);
    graphics.drawRoundedRectangle(area, paneCornerRadius, borderThickness);
}
