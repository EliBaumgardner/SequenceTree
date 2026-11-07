#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Menus/ItemSelector.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Menus/ColourSelector.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

/*
    The panel that edits the settings of one traversal at a time (see TraversalState). A selector
    at the top lists every traversal id in the graph's traversal map, and below it are fields for
    the chosen traversal's tempo multiplier, MIDI channel, transpose and velocity multiplier, a
    colour selector, and a button that opens the TraversalRulesWindow. Each field is bound to a
    property of the traversal's TraversalData tree, so editing writes the graph through the
    UndoManager. Until a traversal exists the panel shows a hint instead.

    selectTraversal binds the fields to a traversal and also makes it the traversal that
    NotePreviewGestures::traversalId uses for note preview. It listens to the traversal map, adding
    or removing an item when a traversal is created or removed and moving the selection if the
    chosen traversal is the one removed.

    PluginEditor owns it, and MenuArea shows it when the traversal icon is selected.
*/

class NotePreviewGestures;
class GraphState;

class TraversalMenu : public juce::Component, private juce::ValueTree::Listener
{
public:

    TraversalMenu(NotePreviewGestures& notePreviewGestures, GraphState& graphState, juce::ValueTree colourPresets, juce::UndoManager& undoManager);

    ~TraversalMenu() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void selectTraversal(int traversalId);

    int narrowestHeight() const;

    ArsNovaUI::ItemSelector displayMenu;

    ArsNovaUI::LabeledEditor multiplierField;
    ArsNovaUI::LabeledEditor channelField;
    ArsNovaUI::LabeledEditor transposeField;
    ArsNovaUI::LabeledEditor velocityField;

    ArsNovaUI::CaptionLabel   colourLabel;
    ArsNovaUI::ColourSelector colourSelector;

    ArsNovaUI::IconButton editTraversalRulesButton;

private:

    void valueTreeChildAdded  (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;

    NotePreviewGestures& notePreviewGestures;
    GraphState&          graphState;
    juce::UndoManager&   undoManager;

    juce::ValueTree currentTraversalData;
};
