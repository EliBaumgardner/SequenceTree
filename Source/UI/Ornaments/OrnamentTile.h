#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Editors/ValueEditor.h>

/*
    The card for one ornament in a library: the ornament's symbol, an editable name under it (its
    OrnamentName property) and a note in the corner saying "draft" or "unpublished" when the
    ornament has no published figure, or has changes that are not published yet. The card of the
    active ornament is drawn highlighted.

    Clicking it sets ActiveOrnamentUid on the libraries tree, which makes the ornament the one
    OrnamentBuildPanel edits and the one the ornament tool applies. Right-clicking opens a menu to
    duplicate the ornament or to delete it, which asks first. Both are done as undo transactions.

    OrnamentLibrarySection owns one for each ornament in its library.
*/

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

    ArsNovaUI::ValueEditor nameTag;
};
