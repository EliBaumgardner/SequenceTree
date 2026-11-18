#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Editor/Ornaments/Builder/OrnamentBuildPanel.h"
#include "Editor/Ornaments/Library/OrnamentSelectionPanel.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

/*
    The window for editing ornaments (see OrnamentLibraries): an OrnamentBuildPanel for the active
    ornament on the left and an OrnamentSelectionPanel listing the libraries on the right, with
    the selection panel's width set by dragging its edge and never below what its content needs.
    narrowestSize is the smallest size that fits both.

    Escape, handled in keyPressed, leaves host-picking or arrow mode in the build panel. Undo and
    redo are commands of the editor's EditorCommandManager, whose key mappings the window listens
    to on itself: while focus is inside it the manager sends those two commands here, and perform
    runs them on the UndoManager it is given, which the editor passes as the ornament undo
    manager, apart from the one for the graph.

    BottomBar opens it in a pop-up window, so it takes the CustomLookAndFeel in its constructor
    and sets it on itself.
*/

class OrnamentLibraries;

class OrnamentWindow : public juce::Component, public juce::ApplicationCommandTarget
{
public:

    OrnamentWindow(ArsNovaUI::CustomLookAndFeel& lookAndFeel, OrnamentLibraries& ornamentLibraries, juce::ValueTree canvasSettings, juce::UndoManager& undoManager,
                   juce::ApplicationCommandManager& commandManager);
    ~OrnamentWindow() override;

    void paint  (juce::Graphics& graphics) override;
    void resized() override;

    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& info) override;

    bool keyPressed(const juce::KeyPress& key) override;
    bool perform   (const InvocationInfo& info) override;

    juce::Point<int> narrowestSize() const;

    juce::ApplicationCommandTarget* getNextCommandTarget() override;

    static constexpr juce::Point<float> openingScale { 1.0f, 2.0f };

private:

    juce::UndoManager&               undoManager;
    juce::ApplicationCommandManager& commandManager;

    int selectionPanelWidth = 0;

    OrnamentBuildPanel     buildPanel;
    OrnamentSelectionPanel selectionPanel;
};
