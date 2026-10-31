#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Input/CanvasController.h"

/*
    The mouse router of an OrnamentCanvas: a CanvasController over the figure's FigureDocument,
    so a figure is edited with the gestures every canvas shares (see CanvasController). Shift
    dragging from the last node grows the chain with a new node that follows the cursor or, in
    arrow mode, draws the dangling arrow; dragging a node moves it and the nodes after it;
    dragging an arrow's head moves the node it points at; dragging the dangling tip sets the last
    note's length; a Ctrl drag scrubs a node's value; and a Shift right-click removes a node or
    the dangling arrow. What it handles itself is a Shift-click on the empty canvas, which adds
    the first node snapped to the grid, and the click mode: in ClickMode::PickHost the next
    click on a node makes it the host, or unmarks it, through OrnamentLibraries::toggleHost, and
    the mode returns to Build.

    OrnamentBuildPanel owns it and shows its click mode and arrow mode on the title bar through
    onClickModeChanged and onArrowModeChanged; OrnamentWindow leaves either mode on Escape.
*/

class OrnamentCanvas;
class FigureDocument;
class OrnamentLibraries;

class OrnamentFigureController : public CanvasController
{
public:

    enum class ClickMode;

    OrnamentFigureController(OrnamentCanvas& ornamentCanvas, FigureDocument& figureDocument, OrnamentLibraries& ornamentLibraries,
                             juce::UndoManager& undoManager);

    void mouseDown(const juce::MouseEvent& event) override;

    void switchClickMode(ClickMode mode);
    void setArrowMode   (bool enabled);

    enum class ClickMode { Build, PickHost };

    static constexpr int hostCursorSize = 20;

    ClickMode clickMode = ClickMode::Build;

    std::function<void()> onClickModeChanged;
    std::function<void()> onArrowModeChanged;

private:

    OrnamentCanvas&    ornamentCanvas;
    FigureDocument&    figureDocument;
    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& undoManager;
};