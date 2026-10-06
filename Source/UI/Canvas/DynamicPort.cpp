#include "DynamicPort.h"
#include "NodeCanvas.h"

DynamicPort::DynamicPort(NodeCanvas& content)
    : canvas(content)
{
    setOpaque(false);

    addAndMakeVisible(canvas);
}

void DynamicPort::resized()
{
    canvas.setBounds(getLocalBounds());

    if (getWidth() <= 0 || getHeight() <= 0) {
        return;
    }

    if (!centeredOnce) {
        centeredOnce = true;
        translateX   = static_cast<float>(getWidth())  * 0.5f - initialViewCentre * zoom;
        translateY   = static_cast<float>(getHeight()) * 0.5f - initialViewCentre * zoom;
    }

    applyTransform();
}

void DynamicPort::applyTransform()
{
    canvas.viewTransform       = juce::AffineTransform::scale(zoom).translated(translateX, translateY);
    canvas.modelTransform      = canvas.viewTransform.inverted();
    canvas.valueField.viewZoom = zoom;

    for (juce::Component* child : canvas.getChildren()) {
        child->setTransform(canvas.viewTransform);
    }

    canvas.valueField.updateBrushCursor();

    canvas.valueField.refresh();

    canvas.repaint();
}

void DynamicPort::mouseDown(const juce::MouseEvent& event)
{
    lastMousePosition = event.getPosition();
}

void DynamicPort::mouseDrag(const juce::MouseEvent& event)
{
    if (!event.mods.isLeftButtonDown()) {
        return;
    }

    const auto delta = event.getPosition() - lastMousePosition;

    lastMousePosition = event.getPosition();

    translateX += static_cast<float>(delta.x);
    translateY += static_cast<float>(delta.y);

    applyTransform();
}

void DynamicPort::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    float scrollX = wheel.deltaX;
    float scrollY = wheel.deltaY;

    if (wheel.isReversed) {
        scrollX = -wheel.deltaX;
        scrollY = -wheel.deltaY;
    }

    if (event.mods.isShiftDown()) {
        setZoom(std::clamp(zoom * (1.0f + scrollY * wheelZoomStep), minimumZoom, maximumZoom), event.getEventRelativeTo(this).getPosition().toFloat());

        return;
    }

    translateX -= scrollX * wheelScrollDistance;
    translateY -= scrollY * wheelScrollDistance;

    applyTransform();
}

void DynamicPort::setZoom(float newZoom, juce::Point<float> pivot)
{
    const float canvasPivotX = (pivot.x - translateX) / zoom;
    const float canvasPivotY = (pivot.y - translateY) / zoom;

    zoom       = newZoom;
    translateX = pivot.x - canvasPivotX * zoom;
    translateY = pivot.y - canvasPivotY * zoom;

    applyTransform();
}

void DynamicPort::mouseMagnify(const juce::MouseEvent& event, float scaleFactor)
{
    setZoom(std::clamp(zoom * scaleFactor, minimumZoom, maximumZoom), event.getEventRelativeTo(this).getPosition().toFloat());
}
