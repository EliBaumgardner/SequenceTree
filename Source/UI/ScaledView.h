#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class ScaledView : public juce::Component, private juce::ChangeListener
{
public:

    ScaledView(juce::Component& content, juce::PropertiesFile& interfaceSettings);
    ~ScaledView() override;

    void resized() override;

    bool zoomFits(float candidateZoom) const;

    static constexpr float minimumZoom = 1.0f;
    static constexpr float maximumZoom = 2.0f;

    std::function<void()> onZoomChanged;

    juce::Component&      content;
    juce::PropertiesFile& interfaceSettings;
    juce::Point<int>      minimumContentSize;

    float                 zoom         = 1.0f;
    float                 previousZoom = 1.0f;

private:

    void changeListenerCallback(juce::ChangeBroadcaster*) override;
};
