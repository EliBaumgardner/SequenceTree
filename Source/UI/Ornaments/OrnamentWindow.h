#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentBuildPanel.h"
#include "OrnamentSelectionPanel.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

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
