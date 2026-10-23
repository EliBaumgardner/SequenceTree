#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentBuildPanel.h"
#include "OrnamentSelectionPanel.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

/*
    The window for editing ornaments (see OrnamentLibraries): an OrnamentBuildPanel for the active
    ornament on the left and an OrnamentSelectionPanel listing the libraries on the right, with
    the selection panel's width set by dragging its edge and never below what its content needs.
    narrowestSize is the smallest size that fits both.

    It takes the keys for the whole window. Escape leaves host-picking or arrow mode in the build
    panel, and Command+Z and Command+Shift+Z (or Command+Y) undo and redo on the UndoManager it
    is given, which BottomBar passes as the ornament undo manager, apart from the one for the
    graph.

    BottomBar opens it in a pop-up window, so it takes the CustomLookAndFeel in its constructor
    and sets it on itself.
*/

class OrnamentLibraries;

class OrnamentWindow : public juce::Component
{
public:

    OrnamentWindow(ArsNovaUI::CustomLookAndFeel& lookAndFeel, OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager);
    ~OrnamentWindow() override;

    void paint  (juce::Graphics& graphics) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;

    juce::Point<int> narrowestSize() const;

    static constexpr juce::Point<float> openingScale { 1.0f, 2.0f };

private:

    juce::UndoManager& undoManager;

    int selectionPanelWidth = 0;

    OrnamentBuildPanel     buildPanel;
    OrnamentSelectionPanel selectionPanel;
};
