#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentCanvas.h"
#include "OrnamentFigureController.h"
#include "OrnamentSymbolEditor.h"
#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Canvas/DynamicPort.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

class OrnamentLibraries;

class OrnamentBuildPanel : public juce::Component, private juce::ValueTree::Listener
{
public:

    OrnamentBuildPanel(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager);
    ~OrnamentBuildPanel() override;

    void paint  (juce::Graphics& graphics) override;
    void resized() override;

    int minimumWidth() const;

    class BuildTitlebar : public ArsNovaUI::Bar
    {
    public:

        BuildTitlebar();

        ArsNovaUI::IconButton hostButton;
        ArsNovaUI::IconButton arrowButton;
        ArsNovaUI::ButtonPane undoRedoPane;
        ArsNovaUI::IconButton publishButton;
        ArsNovaUI::IconButton symbolButton;

    private:

        void resized() override;
    };

    enum class BuildView { Figure, Symbol };

    static constexpr int   titlebarHeight     = 28;
    static constexpr float titlebarInsetRatio = 0.143f;
    static constexpr float nameWidthRatio     = 0.22f;
    static constexpr float buttonWidthRatio   = 3.2f;
    static constexpr int   titlebarButtons    = 4;
    static constexpr int   titlebarGaps       = 4;
    static constexpr int   nameHeight         = 20;
    static constexpr int   nameInset          = 6;

    OrnamentCanvas           canvas;
    ArsNovaUI::DynamicPort   port;
    OrnamentFigureController controller;

    juce::ValueTree ornament;
    BuildView       view = BuildView::Figure;

private:

    void applyView         ();
    void showActiveOrnament();
    void refreshFigure     ();

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded     (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved   (juce::ValueTree& parent, juce::ValueTree& child, int index) override;

    OrnamentLibraries& ornamentLibraries;

    BuildTitlebar          titlebar;
    OrnamentSymbolEditor   symbolEditor;
    ArsNovaUI::ValueEditor nameEditor;
};
