//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "Buttons/ButtonConstants.h"
#include "../Buttons/PaintToolSettings.h"

void CustomLookAndFeel::drawPlayIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.toNearestInt().reduced(5);

    if (state.isHovered) {
        g.setColour(buttonColour.brighter());

    }
    else if (state.isDown) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }

    if (state.isSelected) {
        juce::Path playButton;
        playButton.startNewSubPath((float)area.getX(), (float)area.getY());
        playButton.lineTo((float)area.getRight(), (float)area.getCentreY());
        playButton.lineTo((float)area.getX(), (float)area.getBottom());
        playButton.closeSubPath();
        g.fillPath(playButton);
    }
    else {
        int barWidth = area.getWidth() / 5;
        int gap = barWidth;
        int barHeight = area.getHeight();
        int x1 = area.getX() + barWidth;
        int x2 = x1 + barWidth + gap;

        juce::Rectangle<int> leftBar(x1, area.getY(), barWidth, barHeight);
        juce::Rectangle<int> rightBar(x2, area.getY(), barWidth, barHeight);

        g.fillRect(leftBar);
        g.fillRect(rightBar);
    }
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

    if (state.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }
    g.fillEllipse(bounds);
}

void CustomLookAndFeel::drawModulatorIcon(juce::Graphics &g, juce::Rectangle<float> boundsIn, const ButtonState& state) {
    juce::Rectangle<float> bounds = boundsIn.reduced(outerButtonBoundsReduction);

    if (state.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else{
        g.setColour(buttonColour);
    }

    g.fillRect(bounds);
    g.drawRect(bounds, 1.0f);
}

void CustomLookAndFeel::drawTraversalFlagIcon(juce::Graphics &g, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    juce::Rectangle<float> bounds = boundsIn.reduced(outerButtonBoundsReduction);

    if (state.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }

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

    if (state.isDown) {
        g.setColour(buttonColour.darker());
    }
    else if (state.isHovered) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }

    juce::Path path;
    path.startNewSubPath((float)area.getRight(), (float)area.getY());
    path.lineTo((float)area.getX(), (float)area.getCentreY());
    path.lineTo((float)area.getRight(), (float)area.getBottom());
    path.closeSubPath();
    g.fillPath(path);
}

void CustomLookAndFeel::drawRedoIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    if (state.isDown) {
        g.setColour(buttonColour.darker());
    }
    else if (state.isHovered) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }

    juce::Path path;
    path.startNewSubPath((float)area.getX(), (float)area.getY());
    path.lineTo((float)area.getRight(), (float)area.getCentreY());
    path.lineTo((float)area.getX(), (float)area.getBottom());
    path.closeSubPath();
    g.fillPath(path);
}

void CustomLookAndFeel::drawResetIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    if (state.isDown) {
        g.setColour(buttonColour.darker());
    }
    else if (state.isHovered) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }

    g.fillRect(area);
}

void CustomLookAndFeel::drawDisplayArrowIcon(juce::Graphics &g, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    auto bounds = boundsIn.reduced(outerButtonBoundsReduction);

    g.setColour(state.isSelected ? buttonColour.darker() : buttonColour);

    float triangleHeight = bounds.getHeight() * 0.9f;
    float centreY = bounds.getCentreY();
    float topY    = centreY - triangleHeight / 2.0f;
    float bottomY = centreY + triangleHeight / 2.0f;

    juce::Path vPath;
    vPath.startNewSubPath(bounds.getX(), topY);
    vPath.lineTo(bounds.getCentreX(), bottomY);
    vPath.lineTo(bounds.getRight(), topY);
    vPath.closeSubPath();

    g.fillPath(vPath);
}

void CustomLookAndFeel::drawIncrementIcon(juce::Graphics &g, juce::Rectangle<float> boundsIn, bool pointsUp)
{
    auto bounds = boundsIn.reduced(4.0f, 1.0f);
    auto w = bounds.getWidth();
    auto h = bounds.getHeight();
    auto x = bounds.getX();
    auto y = bounds.getY();

    juce::Path path;
    if (pointsUp) {
        path.startNewSubPath(x,              y + h);
        path.lineTo         (x + w * 0.5f,   y);
        path.lineTo         (x + w,          y + h);
    }
    else {
        path.startNewSubPath(x,              y);
        path.lineTo         (x + w * 0.5f,   y + h);
        path.lineTo         (x + w,          y);
    }
    path.closeSubPath();

    g.setColour(juce::Colours::black);
    g.fillPath(path);
}

void CustomLookAndFeel::drawTextButton(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    juce::Colour fill = buttonColour;

    if (state.isDown) {
        fill = buttonColour.darker();
    }
    else if (state.isHovered) {
        fill = buttonColour.brighter();
    }

    g.setColour(fill);
    g.fillRoundedRectangle(area, paneCornerRadius);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(area, paneCornerRadius, 1.0f);

    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.setFont(juce::Font(juce::FontOptions(labelFontHeight)));
    g.drawText(state.text, bounds, juce::Justification::centred);
}

void CustomLookAndFeel::drawPaintToolIcon(juce::Graphics &g, juce::Rectangle<float> bounds, const ButtonState& state) {

    auto area = bounds.reduced(outerButtonBoundsReduction);
    g.setColour(state.isSelected ? buttonColour.brighter(0.3f) : buttonColour);
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
    g.setColour(state.isSelected ? buttonColour.brighter(0.3f) : buttonColour);
    g.fillRect(area);

    auto glyphArea = area.reduced(area.getWidth() * 0.22f, area.getHeight() * 0.34f);

    juce::Point<float> tail(glyphArea.getX(), glyphArea.getCentreY());
    juce::Point<float> head(glyphArea.getRight(), glyphArea.getCentreY());

    g.setColour(juce::Colours::black);

    juce::Path shaft;
    shaft.addLineSegment(juce::Line<float>(tail, head), glyphArea.getHeight() * 0.16f);
    g.fillPath(shaft);

    const float headLength = glyphArea.getWidth() * 0.32f;
    const float headWidth  = glyphArea.getHeight() * 0.5f;

    juce::Path arrowHead;
    arrowHead.startNewSubPath(head.x, head.y);
    arrowHead.lineTo(head.x - headLength, head.y - headWidth);
    arrowHead.lineTo(head.x - headLength, head.y + headWidth);
    arrowHead.closeSubPath();
    g.fillPath(arrowHead);
}

void CustomLookAndFeel::drawPaintToolSettings(juce::Graphics &g, const PaintToolSettings &paintToolSettings) {

    auto bounds = paintToolSettings.getLocalBounds();

    g.setColour(buttonColour);
    g.fillRect(bounds);
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

    juce::Point<float> tail(glyphArea.getX(),     glyphArea.getCentreY());
    juce::Point<float> head(glyphArea.getRight(), glyphArea.getCentreY());

    g.setColour(juce::Colours::black);

    juce::Path shaft;
    shaft.addLineSegment(juce::Line<float>(tail, head), glyphArea.getHeight() * 0.24f);
    g.fillPath(shaft);

    const float headLength = glyphArea.getWidth() * 0.4f;
    const float headWidth  = glyphArea.getHeight() * 0.65f;

    juce::Path arrowHead;
    arrowHead.startNewSubPath(head.x, head.y);
    arrowHead.lineTo(head.x - headLength, head.y - headWidth);
    arrowHead.lineTo(head.x - headLength, head.y + headWidth);
    arrowHead.closeSubPath();
    g.fillPath(arrowHead);
}
