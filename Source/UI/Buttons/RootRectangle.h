#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

class RootRectangle : public juce::Component
{
public:

    explicit RootRectangle(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    ArsNovaUI::ValueEditor traversalEditor;
};
