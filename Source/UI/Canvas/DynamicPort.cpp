#include "DynamicPort.h"

DynamicPort::DynamicPort(juce::Component* content)
    : component(content)
{
    setOpaque(false);

    component->setSize(canvasSize, canvasSize);

    addAndMakeVisible(component);
}

void DynamicPort::resized()
{
    if (centeredOnce || getWidth() <= 0 || getHeight() <= 0 || component == nullptr) {
        return;
    }

    centeredOnce = true;

    translateX = (static_cast<float>(getWidth())  - static_cast<float>(component->getWidth())  * zoom) * 0.5f;
    translateY = (static_cast<float>(getHeight()) - static_cast<float>(component->getHeight()) * zoom) * 0.5f;

    applyTransform();
}

void DynamicPort::applyTransform()
{
    if (component != nullptr) {
        component->setTransform(juce::AffineTransform::scale(zoom).translated(translateX, translateY));
    }
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
    if (component == nullptr) {
        return;
    }

    const float canvasPivotX = (pivot.x - translateX) / zoom;
    const float canvasPivotY = (pivot.y - translateY) / zoom;

    zoom       = newZoom;
    translateX = pivot.x - canvasPivotX * zoom;
    translateY = pivot.y - canvasPivotY * zoom;

    applyTransform();

    if (onZoomChanged) {
        onZoomChanged(zoom);
    }
}

void DynamicPort::mouseMagnify(const juce::MouseEvent& event, float scaleFactor)
{
    setZoom(std::clamp(zoom * scaleFactor, minimumZoom, maximumZoom), event.getEventRelativeTo(this).getPosition().toFloat());
}
