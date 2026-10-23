#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

/*
    The raised rectangle drawn on a root node, holding a text editor for the root's starting
    traversals. The editor accepts digits, spaces, commas and the instance letters of
    TraversalFlagFormat, up to 24 characters, and shows the text unchanged, spaces included.

    paint draws the panel with the look-and-feel's drawRaisedPanel and resized makes the editor
    fill it. RootNode owns it.
*/

class RootRectangle : public juce::Component
{
public:

    explicit RootRectangle(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    ArsNovaUI::ValueEditor traversalEditor;
};
