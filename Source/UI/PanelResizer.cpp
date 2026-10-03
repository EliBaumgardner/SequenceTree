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
    const auto   bounds     = getLocalBounds();
    const auto   gripBounds = bounds.toFloat().reduced(bounds.getWidth() * 0.3f, bounds.getHeight() * 0.35f);
    const float  dotSpacing = 4.0f;
    juce::Colour fill       = theme.barColour.brighter(restingBrightness);

    if (isDragging) {
        fill = theme.baseLightColour2;
    }
    else if (isHovered) {
        fill = theme.barColour.brighter(hoveredBrightness);
    }

    graphics.setColour(fill);
    graphics.fillRect(bounds);

    graphics.setColour(theme.baseLightColour1.withAlpha(0.5f));

    for (float gripY = gripBounds.getY(); gripY < gripBounds.getBottom(); gripY += dotSpacing) {
        graphics.fillRect(gripBounds.getX(), gripY, gripBounds.getWidth(), 1.0f);
    }
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
