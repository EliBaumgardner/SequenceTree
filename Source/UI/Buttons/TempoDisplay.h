#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

/*
    A pane showing a metronome icon with an editor beside it for the tempo multiplier. The editor
    takes a number between RTtraversal::minimumTempoMultiplier and maximumTempoMultiplier and
    shows it with an "x" suffix.

    TempoDisplay only displays the value. Titlebar connects editor to the processor's tempo
    parameter through a ParameterAttachment and owns the pane.
*/

class TempoDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:

    explicit TempoDisplay(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    ArsNovaUI::ValueEditor editor;
};
