#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class RulePathBar : public juce::Component
{
public:

    void paint(juce::Graphics& graphics) override;

    static constexpr int preferredHeight = 18;
    static constexpr int pathInset       = 12;
    static constexpr int separatorWidth  = 14;

    static constexpr float pathFontHeight = 10.0f;

    juce::StringArray segments;
};
