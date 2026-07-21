//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "MenuBar.h"
#include "../Canvas/NodeCanvas.h"
#include "../Titlebar.h"
#include "../BottomBar.h"
#include "../TraversalMenu.h"
#include "../MenuArea.h"
#include "../NodeMenu.h"
#include "../TraversalDisplayMenu.h"
#include "../Buttons/ButtonPane.h"
#include "../Buttons/TempoDisplay.h"
#include "../Buttons/DisplayMenu.h"
#include "../Buttons/DisplayButton.h"
#include "Buttons/ButtonConstants.h"

void CustomLookAndFeel::drawTitleBar(juce::Graphics &g, const Titlebar &titleBar)
{
    auto bounds = titleBar.getLocalBounds().toFloat();

    juce::ColourGradient gradient(barColour.brighter(0.06f), 0, bounds.getY(),
                                   barColour.darker(0.04f),  0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(barColour.brighter(0.12f));
    g.drawHorizontalLine((int)bounds.getY(), bounds.getX(), bounds.getRight());

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int)bounds.getBottom() - 1, bounds.getX(), bounds.getRight());
}

void CustomLookAndFeel::drawBottomBar(juce::Graphics &g, const BottomBar &bottomBar)
{
    auto bounds = bottomBar.getLocalBounds().toFloat();

    juce::ColourGradient gradient(barColour.darker(0.04f),  0, bounds.getY(),
                                   barColour.brighter(0.06f), 0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int)bounds.getY(), bounds.getX(), bounds.getRight());

    g.setColour(barColour.brighter(0.12f));
    g.drawHorizontalLine((int)bounds.getBottom() - 1, bounds.getX(), bounds.getRight());
}

void CustomLookAndFeel::drawTraversalMenu(juce::Graphics &g, const TraversalMenu &traversalMenu)
{
    auto bounds = traversalMenu.getLocalBounds().toFloat();
    g.setColour(baseDarkColour2);
    g.fillRect(bounds);

    auto barHeight = std::floor(bounds.getHeight() * 0.05f);
    auto barBounds = bounds.withHeight(barHeight);
    if (traversalMenu.hasResizer()) {
        barBounds = barBounds.withTrimmedLeft((float) TraversalMenu::resizerWidth);
    }

    juce::ColourGradient gradient(barColour.brighter(0.06f), 0, barBounds.getY(),
                                   barColour.darker(0.04f),  0, barBounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(barBounds);

    g.setColour(barColour.brighter(0.12f));
    g.drawHorizontalLine((int)barBounds.getY(), barBounds.getX(), barBounds.getRight());

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int)barBounds.getBottom() - 1, barBounds.getX(), barBounds.getRight());
}

void CustomLookAndFeel::drawTraversalMenuResizer(juce::Graphics &g, juce::Rectangle<int> bounds, bool isMouseOver, bool isDragging)
{
    juce::Colour fill = baseDarkColour1;

    if (isDragging) {
        fill = baseLightColour2;
    }
    else if (isMouseOver) {
        fill = baseDarkColour1.brighter(0.25f);
    }

    g.setColour(fill);
    g.fillRect(bounds);

    auto gripBounds = bounds.toFloat().reduced(bounds.getWidth() * 0.3f, bounds.getHeight() * 0.35f);
    g.setColour(baseLightColour1.withAlpha(0.5f));

    const float dotSpacing = 4.0f;
    for (float y = gripBounds.getY(); y < gripBounds.getBottom(); y += dotSpacing) {
        g.fillRect(gripBounds.getX(), y, gripBounds.getWidth(), 1.0f);
    }
}

void CustomLookAndFeel::drawMenuArea(juce::Graphics &g, const MenuArea &menuArea)
{
    auto bounds = menuArea.getLocalBounds().toFloat();
    g.setColour(baseDarkColour2);
    g.fillRect(bounds);

    auto barHeight = std::floor(bounds.getHeight() * 0.05f);
    auto barBounds = bounds.withHeight(barHeight)
                           .withTrimmedRight((float) MenuArea::resizerWidth);

    juce::ColourGradient gradient(barColour.brighter(0.06f), 0, barBounds.getY(),
                                   barColour.darker(0.04f),  0, barBounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(barBounds);

    g.setColour(barColour.brighter(0.12f));
    g.drawHorizontalLine((int)barBounds.getY(), barBounds.getX(), barBounds.getRight());

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int)barBounds.getBottom() - 1, barBounds.getX(), barBounds.getRight());
}

void CustomLookAndFeel::drawMenuAreaResizer(juce::Graphics &g, juce::Rectangle<int> bounds, bool isMouseOver, bool isDragging)
{
    juce::Colour fill = baseDarkColour1;

    if (isDragging) {
        fill = baseLightColour2;
    }
    else if (isMouseOver) {
        fill = baseDarkColour1.brighter(0.25f);
    }

    g.setColour(fill);
    g.fillRect(bounds);

    auto gripBounds = bounds.toFloat().reduced(bounds.getWidth() * 0.3f, bounds.getHeight() * 0.35f);
    g.setColour(baseLightColour1.withAlpha(0.5f));

    const float dotSpacing = 4.0f;
    for (float y = gripBounds.getY(); y < gripBounds.getBottom(); y += dotSpacing) {
        g.fillRect(gripBounds.getX(), y, gripBounds.getWidth(), 1.0f);
    }
}

void CustomLookAndFeel::drawNodeMenu(juce::Graphics &g, const NodeMenu &nodeMenu)
{
    auto bounds = nodeMenu.getLocalBounds().toFloat();
    g.setColour(baseDarkColour2);
    g.fillRect(bounds);
}

void CustomLookAndFeel::drawButtonPane(juce::Graphics &g, const ButtonPane& selectionBar)
{
    auto bounds = selectionBar.getLocalBounds().reduced(outerButtonBoundsReduction).toFloat();
    g.setColour(buttonBarColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawTempoDisplay(juce::Graphics &g, const TempoDisplay &display) {
}

void CustomLookAndFeel::drawDisplayMenu(juce::Graphics &g, const DisplayMenu& displaySelector)
{
    auto bounds = displaySelector.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);
    g.setColour(buttonBarColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawTraversalDisplayMenu(juce::Graphics &g, const TraversalDisplayMenu& displaySelector)
{
    auto bounds = displaySelector.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);
    g.setColour(buttonBarColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawDisplayButton(juce::Graphics &g, const DisplayButton &displayButton)
{
    auto bounds = displayButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (displayButton.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }

    float triangleHeight = bounds.getHeight() * 0.9f;
    float centerY = bounds.getCentreY();
    float topY = centerY - triangleHeight / 2.0f;
    float bottomY = centerY + triangleHeight / 2.0f;

    juce::Path vPath;
    vPath.startNewSubPath(bounds.getX(), topY);
    vPath.lineTo(bounds.getCentreX(), bottomY);
    vPath.lineTo(bounds.getRight(), topY);
    vPath.closeSubPath();

    g.fillPath(vPath);
}

void CustomLookAndFeel::drawMenuBar(juce::Graphics &g, const MenuBar &menuBar) {
    auto bounds = menuBar.getLocalBounds().toFloat();
    g.setColour(buttonBarColour);
    g.fillRect(bounds);
}