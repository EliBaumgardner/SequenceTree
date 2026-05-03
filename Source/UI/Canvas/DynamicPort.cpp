#include "DynamicPort.h"

DynamicPort::DynamicPort(juce::Component* content)
    : component(content)
{
    setOpaque(false);
    component->setSize(3000, 3000);
    addAndMakeVisible(component);
}

DynamicPort::~DynamicPort() {}

void DynamicPort::resized()
{
    if (!centeredOnce && getWidth() > 0 && getHeight() > 0)
    {
        centeredOnce = true;
        centerOnCanvas();
    }
}

void DynamicPort::mouseDown(const juce::MouseEvent& e)
{
    lastMousePosition = e.getPosition();
}

void DynamicPort::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        auto delta = e.getPosition() - lastMousePosition;
        lastMousePosition = e.getPosition();
        translateX += static_cast<float>(delta.x);
        translateY += static_cast<float>(delta.y);
        applyTransform();
    }
}

void DynamicPort::mouseWheelMove(const juce::MouseEvent& e,
                                  const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isShiftDown())
    {
        auto pivot = e.getEventRelativeTo(this).getPosition().toFloat();
        float delta = wheel.isReversed ? -wheel.deltaY : wheel.deltaY;
        float newZoom = std::clamp(zoom * (1.0f + delta * 0.15f), 0.1f, 5.0f);
        setZoom(newZoom, pivot);
    }
    else
    {
        float dx = wheel.isReversed ? -wheel.deltaX : wheel.deltaX;
        float dy = wheel.isReversed ? -wheel.deltaY : wheel.deltaY;
        translateX += dx * 8.0f;
        translateY += dy * 8.0f;
        applyTransform();
    }
}

void DynamicPort::mouseMagnify(const juce::MouseEvent& e, float scaleFactor)
{
    auto pivot = e.getEventRelativeTo(this).getPosition().toFloat();
    setZoom(std::clamp(zoom * scaleFactor, 0.1f, 5.0f), pivot);
}

void DynamicPort::setZoom(float newZoom, juce::Point<float> pivot)
{
    if (component == nullptr) return;

    // Canvas-local point currently under pivot (in DynamicPort space).
    float cx = (pivot.x - translateX) / zoom;
    float cy = (pivot.y - translateY) / zoom;

    zoom = newZoom;

    // Reposition so (cx, cy) stays exactly under pivot.
    translateX = pivot.x - cx * zoom;
    translateY = pivot.y - cy * zoom;

    applyTransform();
}

void DynamicPort::centerOnCanvas()
{
    if (component == nullptr) return;
    translateX = (static_cast<float>(getWidth())  - static_cast<float>(component->getWidth())  * zoom) * 0.5f;
    translateY = (static_cast<float>(getHeight()) - static_cast<float>(component->getHeight()) * zoom) * 0.5f;
    applyTransform();
}

void DynamicPort::applyTransform()
{
    if (component == nullptr) return;
    component->setTransform(
        juce::AffineTransform::scale(zoom).translated(translateX, translateY));
}