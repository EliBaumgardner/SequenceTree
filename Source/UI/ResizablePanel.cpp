//
// Created by Eli Baumgardner on 7/21/26.
//

#include "ResizablePanel.h"

#include "../Util/ApplicationContext.h"
#include "Theme/CustomLookAndFeel.h"

ResizablePanel::ResizablePanel(ApplicationContext& context, ResizeEdge edgeIn, int thickness, bool showResizer)
    : applicationContext(context),
      resizerThickness(thickness),
      resizerVisible(showResizer),
      edge(edgeIn)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    addChildComponent(resizer);
    resizer.setVisible(resizerVisible);
}

ResizablePanel::~ResizablePanel()
{
    setLookAndFeel(nullptr);
}

void ResizablePanel::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds().toFloat());
}

void ResizablePanel::drawTopBar(juce::Graphics& g, juce::Rectangle<float> barBounds)
{
    const Theme& theme = CustomLookAndFeel::get(*this);

    juce::ColourGradient gradient(theme.barColour.brighter(0.06f), 0, barBounds.getY(),
                                  theme.barColour.darker(0.04f),   0, barBounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(barBounds);

    g.setColour(theme.barColour.brighter(0.12f));
    g.drawHorizontalLine((int) barBounds.getY(), barBounds.getX(), barBounds.getRight());

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int) barBounds.getBottom() - 1, barBounds.getX(), barBounds.getRight());
}

int ResizablePanel::resizeStartWidth() const
{
    return getWidth();
}

int ResizablePanel::minimumWidth() const
{
    return resizerThickness;
}

void ResizablePanel::applyResizedWidth(int newWidth)
{
    const int clamped = juce::jmax(minimumWidth(), newWidth);

    if (onWidthDragged) {
        onWidthDragged(clamped);
        return;
    }

    const int newX = (edge == ResizeEdge::Right)
                   ? dragStartX
                   : dragStartX + (dragStartWidth - clamped);

    setBounds(newX, getY(), clamped, getHeight());
}

void ResizablePanel::beginResize()
{
    dragStartWidth = resizeStartWidth();
    dragStartX     = getX();
}

void ResizablePanel::dragResize(int deltaX)
{
    const int signedDelta = (edge == ResizeEdge::Right) ? deltaX : -deltaX;

    applyResizedWidth(dragStartWidth + signedDelta);
}

ResizablePanel::Resizer::Resizer(ResizablePanel& ownerRef) : owner(ownerRef)
{
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void ResizablePanel::Resizer::mouseDown(const juce::MouseEvent& e)
{
    owner.beginResize();
    isDragging = true;
    repaint();
}

void ResizablePanel::Resizer::mouseDrag(const juce::MouseEvent& e)
{
    const int deltaX = e.getScreenPosition().getX() - e.getMouseDownScreenPosition().getX();

    owner.dragResize(deltaX);
}

void ResizablePanel::Resizer::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
    repaint();
}

void ResizablePanel::Resizer::mouseEnter(const juce::MouseEvent& e)
{
    isHovered = true;
    repaint();
}

void ResizablePanel::Resizer::mouseExit(const juce::MouseEvent& e)
{
    isHovered = false;
    repaint();
}

void ResizablePanel::Resizer::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds();

    juce::Colour fill = theme.baseDarkColour1;

    if (isDragging) {
        fill = theme.baseLightColour2;
    }
    else if (isHovered) {
        fill = theme.baseDarkColour1.brighter(0.25f);
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
