//
// Created by Eli Baumgardner on 7/21/26.
//

#include "PanelResizer.h"

#include "../Util/ApplicationContext.h"
#include "Theme/CustomLookAndFeel.h"

PanelResizer::PanelResizer(const ApplicationContext& context, Edge edgeIn) : edge(edgeIn)
{
    setLookAndFeel(context.lookAndFeel);
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

PanelResizer::~PanelResizer()
{
    setLookAndFeel(nullptr);
}

void PanelResizer::mouseDown(const juce::MouseEvent& e)
{
    dragStartWidth = getParentWidth();
    isDragging = true;
    repaint();
}

void PanelResizer::mouseDrag(const juce::MouseEvent& e)
{
    if (onWidthDragged == nullptr) {
        return;
    }

    int delta = e.getScreenPosition().getX() - e.getMouseDownScreenPosition().getX();

    if (edge == Edge::Left) {
        delta = -delta;
    }

    onWidthDragged(dragStartWidth + delta);
}

void PanelResizer::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
    repaint();
}

void PanelResizer::mouseEnter(const juce::MouseEvent& e)
{
    isHovered = true;
    repaint();
}

void PanelResizer::mouseExit(const juce::MouseEvent& e)
{
    isHovered = false;
    repaint();
}

void PanelResizer::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds();

    juce::Colour fill = theme.barColour.brighter(restingBrightness);

    if (isDragging) {
        fill = theme.baseLightColour2;
    }
    else if (isHovered) {
        fill = theme.barColour.brighter(hoveredBrightness);
    }

    g.setColour(fill);
    g.fillRect(bounds);

    auto gripBounds = bounds.toFloat().reduced(bounds.getWidth() * 0.3f, bounds.getHeight() * 0.35f);
    g.setColour(theme.baseLightColour1.withAlpha(0.5f));

    const float dotSpacing = 4.0f;
    for (float y = gripBounds.getY(); y < gripBounds.getBottom(); y += dotSpacing) {
        g.fillRect(gripBounds.getX(), y, gripBounds.getWidth(), 1.0f);
    }
}
