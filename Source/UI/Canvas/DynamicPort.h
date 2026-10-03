#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class DynamicPort : public juce::Component
{
public:

    static constexpr int   canvasSize          = 3000;
    static constexpr float minimumZoom         = 0.1f;
    static constexpr float maximumZoom         = 5.0f;
    static constexpr float wheelZoomStep       = 0.15f;
    static constexpr float wheelScrollDistance = 100.0f;

    std::function<void(float)> onZoomChanged;

    explicit DynamicPort(juce::Component* content);

    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void setZoom(float newZoom, juce::Point<float> pivot);
    void mouseMagnify(const juce::MouseEvent& event, float scaleFactor) override;

private:

    void applyTransform();

    juce::Component* component    = nullptr;
    float            zoom         = 1.0f;
    float            translateX   = 0.0f;
    float            translateY   = 0.0f;
    bool             centeredOnce = false;

    juce::Point<int> lastMousePosition;
};
