#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Editors/ValueEditor.h>

class OrnamentLibraries;

class OrnamentTile : public juce::Component
{
public:

    OrnamentTile(OrnamentLibraries& ornamentLibraries, juce::ValueTree ornament, juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;

    void mouseDown(const juce::MouseEvent& event) override;

    static constexpr int tileWidth     = 72;
    static constexpr int tileHeight    = 84;
    static constexpr int nameTagHeight = 18;
    static constexpr int nameTagInset  = 4;
    static constexpr int markerHeight  = 14;

    juce::ValueTree ornament;

private:

    void showMenu     ();
    void confirmDelete();

    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& undoManager;

    ValueEditor nameTag;
};
