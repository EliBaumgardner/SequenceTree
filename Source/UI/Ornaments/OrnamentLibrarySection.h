#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentTile.h"
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

#include <functional>
#include <memory>
#include <vector>

class OrnamentLibraries;

class OrnamentLibrarySection : public juce::Component
{
public:

    OrnamentLibrarySection(OrnamentLibraries& ornamentLibraries, juce::ValueTree library, juce::UndoManager& undoManager);

    void paint  (juce::Graphics& graphics) override;
    void resized() override;

    void syncTiles();

    void mouseDown(const juce::MouseEvent& event) override;

    int heightForWidth(int width) const;

    static constexpr int headerHeight = 26;
    static constexpr int headerInset  = 4;
    static constexpr int tileGap      = 8;
    static constexpr int addGlyphSide = 28;

    juce::ValueTree library;

    std::function<void()> onFoldChanged;

private:

    void toggleFold ();
    void addOrnament();

    void showMenu     ();
    void confirmRemove();

    enum class Fold { Open, Closed };

    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& undoManager;

    ArsNovaUI::ValueEditor nameEditor;
    ArsNovaUI::IconButton  foldButton;
    ArsNovaUI::IconButton  addCard;
    juce::Component        body;

    Fold fold = Fold::Open;

    std::vector<std::unique_ptr<OrnamentTile>> tiles;
};
