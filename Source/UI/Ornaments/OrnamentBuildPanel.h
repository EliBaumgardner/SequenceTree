#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentCanvas.h"
#include "../../Input/FigureDocument.h"
#include "OrnamentFigureController.h"
#include "OrnamentSymbolEditor.h"
#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Canvas/DynamicPort.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

/*
    The editing area of the OrnamentWindow, for the active ornament (the one named by
    ActiveOrnamentUid on the libraries tree). Under a title bar of buttons it shows one of two
    views, chosen with the symbol button: the figure view, an OrnamentCanvas inside a zoomable
    port with an OrnamentFigureController doing the editing and an editor for the ornament's name,
    or the symbol view, an OrnamentSymbolEditor. With no ornament active it shows a hint.

    The title bar has buttons to pick the host node, to turn on arrow mode, to undo and redo, to
    publish the ornament, and to switch views. Publish calls OrnamentLibraries::publish and is
    enabled only when isPublishable accepts the figure and it is not already published.

    It listens to the libraries tree: when the active ornament changes it hands the new ornament
    and its figure to the FigureDocument and the symbol editor, moves the canvas's tree listener
    onto the new figure and asks the canvas to rebuild, and when the figure changes it refreshes
    the canvas's host rim and the publish button. OrnamentWindow owns it and asks for minimumWidth.
*/

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

    FigureDocument           document;
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
