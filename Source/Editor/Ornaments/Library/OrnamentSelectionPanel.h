#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentLibrarySection.h"
#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/PanelResizer.h>

#include <memory>
#include <vector>

/*
    The scrolling list of ornament libraries in the OrnamentWindow: one OrnamentLibrarySection for
    each library in OrnamentLibraries, stacked in a viewport. Its title bar has two buttons: one
    adds a new empty library named "New Library", and the other opens a file chooser to add a
    library from disk, with a warning when the file is already added or is not an ornament library.
    A resizer on its left edge lets the window change its width.

    It listens to the libraries tree. When a library or an ornament is added or removed, it
    rebuilds its list of sections on the next message-loop turn, keeping the section of every
    library that is still there, and lays the sections out again when one is folded.

    OrnamentWindow owns it and sizes it from narrowestSize.
*/

class OrnamentLibraries;

class OrnamentSelectionPanel : public juce::Component,
                               private juce::ValueTree::Listener,
                               private juce::AsyncUpdater
{
public:

    OrnamentSelectionPanel(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager);
    ~OrnamentSelectionPanel() override;

    void paint  (juce::Graphics& graphics) override;
    void resized() override;

    juce::Point<int> narrowestSize() const;

    class PanelTitlebar : public ArsNovaUI::Bar
    {
    public:

        PanelTitlebar();

        ArsNovaUI::IconButton newLibraryButton;
        ArsNovaUI::IconButton addLibraryButton;

    private:

        void resized() override;
    };

    static constexpr int   titlebarHeight     = 28;
    static constexpr float titlebarInsetRatio = 0.143f;

    ArsNovaUI::PanelResizer resizer;

private:

    void chooseLibraryFile();
    void layoutSections   ();

    void handleAsyncUpdate() override;

    void valueTreeChildAdded     (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved   (juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& undoManager;

    PanelTitlebar   titlebar;
    juce::Viewport  viewport;
    juce::Component sectionList;

    std::vector<std::unique_ptr<OrnamentLibrarySection>> sections;

    std::unique_ptr<juce::FileChooser> fileChooser;
};
