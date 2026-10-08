#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Editors/ValueEditor.h"

class TempoDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:

    ValueEditor editor;

    explicit TempoDisplay(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;
};
