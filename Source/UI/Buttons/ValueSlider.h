#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class ValueSlider : public juce::Component, public juce::SettableTooltipClient
{
public:

    ValueSlider();

    void paint(juce::Graphics& graphics) override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    std::function<void()> onValueChange;

    juce::Value                     boundValue;
    juce::NormalisableRange<double> range { 0.0, 1.0 };
    juce::String                    label;
    juce::String                    suffix;
};
