#pragma once
#include "ProjectModules.h"

class DynamicPort : public juce::Viewport
{
public:
    DynamicPort(juce::Component* content);
    ~DynamicPort() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void setZoom(float newZoom);
    
    void centerInComponent();

private:
    juce::Component* component = nullptr;

    float zoom = 1.0f;

    juce::Point<int> lastMousePosition;
    
    juce::Point<int> zoomPoint;
};
