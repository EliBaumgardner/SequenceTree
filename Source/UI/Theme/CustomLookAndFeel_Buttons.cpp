//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "../Buttons/PlayButton.h"
#include "../Buttons/SyncButton.h"
#include "../Buttons/NodeButton.h"
#include "../Buttons/ModulatorButton.h"
#include "../Buttons/TraversalFlagButton.h"
#include "../Buttons/UndoButton.h"
#include "../Buttons/RedoButton.h"
#include "../Buttons/UndoRedoPane.h"
#include "../Buttons/ResetButton.h"
#include "Buttons/ButtonConstants.h"
#include "../Buttons/PaintTool.h"
#include "../Buttons/PaintToolSettings.h"
#include "../Buttons/ArrowTool.h"
#include "../Buttons/IconButton.h"

void CustomLookAndFeel::drawPlayButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown, const PlayButton &button)
{
    auto area = button.getLocalBounds().reduced(5);

    if (isMouseOver) {
        g.setColour(buttonColour.brighter());

    }
    else if (isButtonDown) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }

    if (button.isOn) {
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

void CustomLookAndFeel::drawSyncButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown, const SyncButton &button)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);
    if (isMouseOver) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }
    g.drawEllipse(bounds, 1.0f);
}

void CustomLookAndFeel::drawNodeButton(juce::Graphics &g, const NodeButton& nodeButton)
{
    auto bounds = nodeButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (nodeButton.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }
    g.fillEllipse(bounds);
}

void CustomLookAndFeel::drawModulatorButton(juce::Graphics &g, const ModulatorButton &modulatorButton) {
    juce::Rectangle<float> bounds = modulatorButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (modulatorButton.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else{
        g.setColour(buttonColour);
    }

    g.fillRect(bounds);
    g.drawRect(bounds, 1.0f);
}

void CustomLookAndFeel::drawTraversalFlagButton(juce::Graphics &g, const TraversalFlagButton &traversalFlagButton)
{
    juce::Rectangle<float> bounds = traversalFlagButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (traversalFlagButton.isSelected) {
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

void CustomLookAndFeel::drawUndoButton(juce::Graphics &g, const UndoButton &undoButton, bool isButtonDown)
{
    auto area = undoButton.getLocalBounds().reduced(outerButtonBoundsReduction);

    if (isButtonDown) {
        g.setColour(buttonColour.darker());
    }
    else if (undoButton.isHovered) {
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

void CustomLookAndFeel::drawRedoButton(juce::Graphics &g, const RedoButton &redoButton, bool isButtonDown)
{
    auto area = redoButton.getLocalBounds().reduced(outerButtonBoundsReduction);

    if (isButtonDown) {
        g.setColour(buttonColour.darker());
    }
    else if (redoButton.isHovered) {
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

void CustomLookAndFeel::drawUndoRedoPane(juce::Graphics &g,const UndoRedoPane& undoRedoPane)
{
    auto bounds = undoRedoPane.getLocalBounds().reduced(outerButtonBoundsReduction).toFloat();
    g.setColour(buttonBarColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawResetButton(juce::Graphics &g, const ResetButton &resetButton, bool isButtonDown)
{
    auto area = resetButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (isButtonDown) {
        g.setColour(buttonColour.darker());
    }
    else if (resetButton.isHovered) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }

    g.fillRect(area);
}

void CustomLookAndFeel::drawPaintTool(juce::Graphics &g, const PaintTool &paintTool) {

    auto area = paintTool.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);
    g.setColour(buttonColour);
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

void CustomLookAndFeel::drawArrowTool(juce::Graphics &g, const ArrowTool &arrowTool)
{
    auto area = arrowTool.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);
    g.setColour(arrowTool.isSelected ? buttonColour.brighter(0.3f) : buttonColour);
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

void CustomLookAndFeel::drawNodeIcon(juce::Graphics &g, const IconButton &iconButton) {
    auto bounds = iconButton.getLocalBounds().toFloat();
    g.setColour(buttonColour);
    g.fillEllipse(bounds);

    auto glyphBounds = bounds.reduced(bounds.getWidth() * 0.32f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(glyphBounds);
}

void CustomLookAndFeel::drawTreeIcon(juce::Graphics &g, const IconButton &iconButton) {
    auto bounds = iconButton.getLocalBounds().toFloat();
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

void CustomLookAndFeel::drawTraversalIcon(juce::Graphics &g, const IconButton &iconButton) {
    auto bounds = iconButton.getLocalBounds().toFloat();
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