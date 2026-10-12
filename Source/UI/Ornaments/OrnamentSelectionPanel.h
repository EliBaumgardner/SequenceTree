#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentLibrarySection.h"
#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/PanelResizer.h>

#include <memory>
#include <vector>

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

    class PanelTitlebar : public Bar
    {
    public:

        PanelTitlebar();

        IconButton newLibraryButton;
        IconButton addLibraryButton;

    private:

        void resized() override;
    };

    static constexpr int   titlebarHeight     = 28;
    static constexpr float titlebarInsetRatio = 0.143f;

    PanelResizer resizer;

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
