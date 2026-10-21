#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Theme/Visuals.h>

class OrnamentMarkView : public juce::Component
{
public:

    void paint(juce::Graphics& graphics) override;

    ArsNovaUI::SymbolMark mark = ArsNovaUI::SymbolMark::None;
    juce::String          symbol;
    juce::ValueTree       nodeValueTree;
};
