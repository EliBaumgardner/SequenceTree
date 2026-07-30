//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "Glyphs.h"
#include "Buttons/ButtonConstants.h"
#include "../Buttons/PaintToolSettings.h"

namespace {
    constexpr float transportGlyphInsetRatio = 0.2f;

    void fillArrowGlyph(juce::Graphics& g, juce::Rectangle<float> glyphArea,
                        float shaftThicknessFactor, float headLengthFactor, float headWidthFactor)
    {
        juce::Point<float> tail(glyphArea.getX(),     glyphArea.getCentreY());
        juce::Point<float> head(glyphArea.getRight(), glyphArea.getCentreY());

        juce::Path shaft;
        shaft.addLineSegment(juce::Line<float>(tail, head), glyphArea.getHeight() * shaftThicknessFactor);
        g.fillPath(shaft);

        const float headLength = glyphArea.getWidth()  * headLengthFactor;
        const float headWidth  = glyphArea.getHeight() * headWidthFactor;

        fillTriangle(g, { head.x - headLength, head.y - headWidth, headLength, headWidth * 2.0f },
                     TriangleDirection::right);
    }

    void strokeChevronGlyph(juce::Graphics& g, juce::Point<float> tip,
                            float headLength, float headWidth, float thickness)
    {
        juce::Path chevron;
        chevron.startNewSubPath(tip.x - headLength, tip.y - headWidth);
        chevron.lineTo(tip.x, tip.y);
        chevron.lineTo(tip.x - headLength, tip.y + headWidth);

        g.strokePath(chevron, juce::PathStrokeType(thickness,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::butt));
    }

    void strokePolyphonicArrowGlyph(juce::Graphics& g, juce::Rectangle<float> glyphArea,
                                    float shaftThicknessFactor, float headLengthFactor, float headWidthFactor)
    {
        const float headLength = glyphArea.getWidth()  * headLengthFactor;
        const float headWidth  = glyphArea.getHeight() * headWidthFactor;
        const float thickness  = glyphArea.getHeight() * shaftThicknessFactor;

        juce::Point<float> frontTip(glyphArea.getRight(),               glyphArea.getCentreY());
        juce::Point<float> rearTip (frontTip.x - headLength * 0.85f,    glyphArea.getCentreY());

        juce::Path shaft;
        shaft.addLineSegment(juce::Line<float>({ glyphArea.getX(), glyphArea.getCentreY() }, rearTip), thickness);
        g.fillPath(shaft);

        strokeChevronGlyph(g, rearTip,  headLength, headWidth, thickness);
        strokeChevronGlyph(g, frontTip, headLength, headWidth, thickness);
    }

    juce::Rectangle<float> fillArrowIconTile(juce::Graphics& g, juce::Rectangle<float> area,
                                             juce::Colour tileColour, float cornerRadius)
    {
        g.setColour(tileColour);
        g.fillRoundedRectangle(area, cornerRadius);

        const auto glyphArea = area.reduced(area.getWidth() * 0.18f, area.getHeight() * 0.34f);

        const float nodeDiameter = glyphArea.getHeight();

        g.setColour(juce::Colours::black);
        g.fillEllipse(juce::Rectangle<float>(nodeDiameter, nodeDiameter)
                          .withCentre({ glyphArea.getX(), glyphArea.getCentreY() }));

        return glyphArea;
    }
}

juce::Colour CustomLookAndFeel::pressableButtonColour(const ButtonState& state) const
{
    if (state.isDown) {
        return buttonColour.darker();
    }

    if (state.isHovered) {
        return buttonColour.brighter();
    }

    return buttonColour;
}

juce::Colour CustomLookAndFeel::selectableButtonColour(const ButtonState& state) const
{
    if (state.isSelected) {
        return buttonColour.darker();
    }

    return buttonColour;
}

void CustomLookAndFeel::drawPlayIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const auto area = bounds.reduced(bounds.getWidth() * transportGlyphInsetRatio);

    g.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        fillTriangle(g, area, TriangleDirection::right);
        return;
    }

    const float barWidth = area.getWidth() / 5.0f;

    g.fillRect(area.withWidth(barWidth).withX(area.getX() + barWidth));
    g.fillRect(area.withWidth(barWidth).withX(area.getX() + barWidth * 3.0f));
}

void CustomLookAndFeel::drawSyncIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);
    if (state.isHovered) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }
    g.drawEllipse(area, 1.0f);
}

void CustomLookAndFeel::drawNodeModeIcon(juce::Graphics &g, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    auto bounds = boundsIn.reduced(outerButtonBoundsReduction);

    g.setColour(selectableButtonColour(state));
    g.fillEllipse(bounds);
}

void CustomLookAndFeel::drawModulatorIcon(juce::Graphics &g, juce::Rectangle<float> boundsIn, const ButtonState& state) {
    juce::Rectangle<float> bounds = boundsIn.reduced(outerButtonBoundsReduction);

    g.setColour(selectableButtonColour(state));

    g.fillRect(bounds);
    g.drawRect(bounds, 1.0f);
}

void CustomLookAndFeel::drawTraversalFlagIcon(juce::Graphics &g, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    juce::Rectangle<float> bounds = boundsIn.reduced(outerButtonBoundsReduction);

    g.setColour(selectableButtonColour(state));

    juce::Path triangle;
    triangle.startNewSubPath(bounds.getCentreX(), bounds.getY());
    triangle.lineTo(bounds.getRight(), bounds.getBottom());
    triangle.lineTo(bounds.getX(), bounds.getBottom());
    triangle.closeSubPath();

    g.fillPath(triangle);
    g.strokePath(triangle, juce::PathStrokeType(1.0f));
}

void CustomLookAndFeel::drawUndoIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    g.setColour(pressableButtonColour(state));

    fillTriangle(g, area, TriangleDirection::left);
}

void CustomLookAndFeel::drawRedoIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    g.setColour(pressableButtonColour(state));

    fillTriangle(g, area, TriangleDirection::right);
}

void CustomLookAndFeel::drawResetIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(bounds.getWidth() * transportGlyphInsetRatio);

    const float gap           = area.getWidth() * 0.1f;
    const float triangleWidth = (area.getWidth() - gap) * 0.5f;

    g.setColour(pressableButtonColour(state));

    fillTriangle(g, area.removeFromLeft (triangleWidth), TriangleDirection::left);
    fillTriangle(g, area.removeFromRight(triangleWidth), TriangleDirection::left);
}

void CustomLookAndFeel::drawDisplayArrowIcon(juce::Graphics &g, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    auto bounds = boundsIn.reduced(outerButtonBoundsReduction);

    g.setColour(selectableButtonColour(state));

    fillTriangle(g, bounds.withSizeKeepingCentre(bounds.getWidth(), bounds.getHeight() * 0.9f),
                 TriangleDirection::down);
}

void CustomLookAndFeel::drawIncrementIcon(juce::Graphics &g, juce::Rectangle<float> boundsIn, bool pointsUp)
{
    g.setColour(juce::Colours::black);

    fillTriangle(g, boundsIn.reduced(4.0f, 1.0f),
                 pointsUp ? TriangleDirection::up : TriangleDirection::down);
}

void CustomLookAndFeel::drawTextButton(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state,
                                       float fontHeight)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    g.setColour(state.isSelected ? selectableButtonColour(state) : pressableButtonColour(state));
    g.fillRoundedRectangle(area, paneCornerRadius);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(area, paneCornerRadius, 1.0f);

    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.setFont(juce::Font(juce::FontOptions(fontHeight)));
    g.drawText(state.text, bounds, juce::Justification::centred);
}

void CustomLookAndFeel::drawPaintToolIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state) {

    auto area = bounds.reduced(outerButtonBoundsReduction);
    if (state.isSelected) {
        g.setColour(buttonColour.brighter(0.3f));
    } else {
        g.setColour(buttonColour);
    }

    g.fillRect(area);

    int wandBoundsReduction = 2;
    auto wandArea = area.reduced(wandBoundsReduction);

    g.setColour(juce::Colours::black);

    const float handleThickness = wandArea.getHeight() * 0.16f;
    const float circleDiameter  = wandArea.getHeight() * 0.4f;
    const float circleRadius    = circleDiameter * 0.5f;

    // Inset both ends by the circle radius so the whole wand stays inside the bounds.
    juce::Point<float> tipCentre   (wandArea.getRight() - circleRadius, wandArea.getY() + circleRadius);
    juce::Point<float> handleStart (wandArea.getX() + circleRadius,     wandArea.getBottom() - circleRadius);

    juce::Line<float> handleLine(handleStart, tipCentre);
    juce::Point<float> handleEnd = handleLine.getPointAlongLineProportionally(1.0f - circleRadius / handleLine.getLength());

    juce::Path handle;
    handle.addLineSegment(juce::Line<float>(handleStart, handleEnd), handleThickness);
    g.fillPath(handle);


    auto tipBounds = juce::Rectangle<float>(circleDiameter, circleDiameter).withCentre(tipCentre);
    g.fillEllipse(tipBounds);
}

void CustomLookAndFeel::drawArrowToolIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    g.setColour(pressableButtonColour(state));
    g.fillRect(area);

    auto glyphArea = area.reduced(area.getWidth() * 0.22f, area.getHeight() * 0.34f);

    g.setColour(juce::Colours::black);
    fillArrowGlyph(g, glyphArea, 0.16f, 0.32f, 0.5f);
}

juce::Colour CustomLookAndFeel::arrowIconTileColour(const ButtonState& state) const
{
    juce::Colour tileColour = state.isSelected ? buttonColour.brighter(0.3f) : buttonColour;

    if (state.isHovered) {
        return tileColour.brighter(0.15f);
    }

    return tileColour;
}

void CustomLookAndFeel::drawNodeArrowIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const auto glyphArea = fillArrowIconTile(g, bounds.reduced(outerButtonBoundsReduction),
                                             arrowIconTileColour(state), paneCornerRadius);

    fillArrowGlyph(g, glyphArea, 0.16f, 0.32f, 0.5f);
}

void CustomLookAndFeel::drawPolyphonicArrowIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const auto glyphArea = fillArrowIconTile(g, bounds.reduced(outerButtonBoundsReduction),
                                             arrowIconTileColour(state), paneCornerRadius);

    strokePolyphonicArrowGlyph(g, glyphArea, 0.16f, 0.32f, 0.5f);
}

void CustomLookAndFeel::drawPaintToolSettings(juce::Graphics &g, const PaintToolSettings &paintToolSettings) {

    auto bounds = paintToolSettings.getLocalBounds().toFloat();

    juce::ColourGradient gradient(barColour.brighter(0.06f), 0, bounds.getY(),
                                  barColour.darker(0.04f),   0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(barColour.brighter(0.12f));
    g.drawRect(bounds, 1.0f);
}

void CustomLookAndFeel::drawNodeIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState&) {
    g.setColour(buttonColour);
    g.fillEllipse(bounds);

    auto glyphBounds = bounds.reduced(bounds.getWidth() * 0.32f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(glyphBounds);
}

void CustomLookAndFeel::drawTreeIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState&) {
    g.setColour(buttonColour);
    g.fillEllipse(bounds);

    juce::Point<float> centre = bounds.getCentre();
    const float outerRadius = bounds.getWidth() * 0.5f;

    const float lineThickness   = 2.0f;
    const float nodeRadius      = outerRadius * 0.24f;
    const float insetMargin     = outerRadius * 0.15f;
    const float placementRadius = outerRadius - nodeRadius - insetMargin;

    juce::Point<float> rootPos (centre.x, centre.y - placementRadius * 0.55f);
    juce::Point<float> leftPos (centre.x - placementRadius * 0.8f, centre.y + placementRadius * 0.55f);
    juce::Point<float> rightPos(centre.x + placementRadius * 0.8f, centre.y + placementRadius * 0.55f);

    g.setColour(juce::Colours::black);
    g.drawLine(juce::Line<float>(rootPos, leftPos), lineThickness);
    g.drawLine(juce::Line<float>(rootPos, rightPos), lineThickness);

    auto nodeCircle = [&](juce::Point<float> pos) {
        g.fillEllipse(juce::Rectangle<float>(nodeRadius * 2.0f, nodeRadius * 2.0f).withCentre(pos));
    };
    nodeCircle(rootPos);
    nodeCircle(leftPos);
    nodeCircle(rightPos);
}

void CustomLookAndFeel::drawTraversalIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState&) {
    g.setColour(buttonColour);
    g.fillEllipse(bounds);

    auto glyphArea = bounds.reduced(bounds.getWidth() * 0.26f, bounds.getHeight() * 0.38f);

    g.setColour(juce::Colours::black);
    fillArrowGlyph(g, glyphArea, 0.24f, 0.4f, 0.65f);
}
