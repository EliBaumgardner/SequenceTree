#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentTile.h"
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

#include <functional>
#include <memory>
#include <vector>

/*
    The part of the ornament list that shows one library: a header with the library's editable
    name (its LibraryName property) and a button that folds the section, and below it a grid of
    OrnamentTiles, one for each ornament in the library, ending with a card that adds a new
    ornament to the library and makes it the active one. Right-clicking the header opens a menu
    to reveal the library's folder in the file manager or to remove the library, which asks first
    and moves its folder to the Trash.

    syncTiles makes the tiles match the library's ornaments, and heightForWidth gives the height
    the section needs at a width, which depends on how many tiles fit in a row and whether it is
    folded. onFoldChanged tells the OrnamentSelectionPanel to lay its sections out again.

    OrnamentSelectionPanel owns one for each library.
*/

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
