#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

class TempoDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:

    explicit TempoDisplay(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    ArsNovaUI::ValueEditor editor;
};
