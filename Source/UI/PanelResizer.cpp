#include "PanelResizer.h"

#include "../Util/ApplicationContext.h"
#include "Theme/CustomLookAndFeel.h"

PanelResizer::PanelResizer(const ApplicationContext& context, Edge edge) : edge(edge)
{
    setLookAndFeel(context.lookAndFeel);
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

PanelResizer::~PanelResizer()
{
    setLookAndFeel(nullptr);
}

void PanelResizer::paint(juce::Graphics& graphics)
{
    const Theme& theme      = CustomLookAndFeel::get(*this);
    const auto   bounds     = getLocalBounds().toFloat();
    const auto   gripBounds = bounds.withSizeKeepingCentre(Theme::resizerGripWidth, bounds.getHeight() * Theme::resizerGripHeightRatio);
    juce::Colour fill       = theme.borderStrongColour;

    if (isDragging) {
        fill = theme.accentColour;
    }
    else if (isHovered) {
        fill = theme.mutedTextColour;
    }

    graphics.setColour(fill);
    graphics.fillRoundedRectangle(gripBounds, Theme::resizerGripWidth * 0.5f);
}

void PanelResizer::mouseDown(const juce::MouseEvent&)
{
    dragStartWidth = getParentWidth();
    isDragging     = true;

    repaint();
}

void PanelResizer::mouseDrag(const juce::MouseEvent& event)
{
    if (onWidthDragged == nullptr) {
        return;
    }

    int delta = event.getScreenPosition().getX() - event.getMouseDownScreenPosition().getX();

    if (edge == Edge::Left) {
        delta = -delta;
    }

    onWidthDragged(dragStartWidth + delta);
}

void PanelResizer::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;

    repaint();
}

void PanelResizer::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;

    repaint();
}

void PanelResizer::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;

    repaint();
}
