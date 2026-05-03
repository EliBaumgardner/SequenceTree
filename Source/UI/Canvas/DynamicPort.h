#pragma once
#include "../../Util/PluginModules.h"

class DynamicPort : public juce::Component
{
public:
    explicit DynamicPort(juce::Component* content);
    ~DynamicPort() override;

    void paint(juce::Graphics&) override {}
    void resized() override;

    void mouseDown    (const juce::MouseEvent& e) override;
    void mouseDrag    (const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify (const juce::MouseEvent& e, float scaleFactor) override;

    void setZoom(float newZoom, juce::Point<float> pivotInParent);
    void centerOnCanvas();

private:
    void applyTransform();

    juce::Component* component       = nullptr;
    float            zoom            = 1.0f;
    float            translateX      = 0.0f;
    float            translateY      = 0.0f;
    bool             centeredOnce    = false;

    juce::Point<int> lastMousePosition;
};