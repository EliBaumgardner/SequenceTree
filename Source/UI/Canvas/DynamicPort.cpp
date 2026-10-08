#include "DynamicPort.h"
#include "NodeCanvas.h"

DynamicPort::DynamicPort(NodeCanvas& nodeCanvas)
    : nodeCanvas(nodeCanvas)
{
    setOpaque(false);

    addAndMakeVisible(nodeCanvas);
}

void DynamicPort::resized()
{
    nodeCanvas.setBounds(getLocalBounds());

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
    nodeCanvas.viewTransform       = juce::AffineTransform::scale(zoom).translated(translateX, translateY);
    nodeCanvas.modelTransform      = nodeCanvas.viewTransform.inverted();
    nodeCanvas.valueField.viewZoom = zoom;

    for (juce::Component* child : nodeCanvas.getChildren()) {
        child->setTransform(nodeCanvas.viewTransform);
    }

    nodeCanvas.valueField.updateBrushCursor();

    nodeCanvas.valueField.refresh();

    nodeCanvas.repaint();
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
