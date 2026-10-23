#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Graph/OrnamentLibraries.h"
#include <ArsNovaUI/Canvas/DynamicPort.h>

/*
    The mouse listener that turns clicks and drags on an OrnamentCanvas into edits of the draft
    figure of its ornament, each made through OrnamentLibraries in an undo transaction. Positions
    snap to the 50 pixel grid when within a few pixels of a line.

    In the normal ClickMode::Build, a Shift-click on empty canvas adds a figure node, and a drag on
    a node moves it. Shift-dragging from the last node of the chain grows the chain with a new node
    that follows the cursor; in arrow mode the same drag instead creates a dangling arrow from
    that node, or moves the one it has. Dragging an arrow's head moves the node it points at,
    and dragging a dangling tip changes the length of the last node's arrow. Clicking a duration
    label edits it, Ctrl-Shift dragging a node scrubs its value, and dragging empty canvas pans
    the port. A Shift right-click removes a node or the dangling arrow. In ClickMode::PickHost the
    next click on a node makes it the host, or unmarks it, and the mode returns to Build.

    OrnamentBuildPanel owns it, sets ornament when the active ornament changes, and shows its
    click mode and arrow mode on the title bar through onClickModeChanged and onArrowModeChanged.
*/

class OrnamentCanvas;
class OrnamentNode;

class OrnamentFigureController : public juce::MouseListener
{
public:

    enum class ClickMode;

    OrnamentFigureController(OrnamentCanvas& canvas, ArsNovaUI::DynamicPort& port, OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager);

    void mouseMove      (const juce::MouseEvent& event) override;
    void mouseExit      (const juce::MouseEvent& event) override;
    void mouseDown      (const juce::MouseEvent& event) override;
    void mouseDrag      (const juce::MouseEvent& event) override;
    void mouseUp        (const juce::MouseEvent& event) override;

    void switchClickMode(ClickMode mode);
    void setArrowMode   (bool enabled);

    enum class DragState { Idle, MovingNode, MovingArrowHead, GrowingChain, MovingDanglingTip, CreatingDanglingArrow, ScrubbingValue, Panning };
    enum class ClickMode { Build, PickHost };

    static constexpr float arrowHoverRadius     = 8.0f;
    static constexpr float arrowHeadGrabRadius  = 16.0f;
    static constexpr float arrowLabelGrabRadius = 10.0f;
    static constexpr int   dragThreshold        = 5;
    static constexpr int   pixelsPerScrubStep   = 3;
    static constexpr int   hostCursorSize       = 20;

    juce::ValueTree ornament;

    ClickMode clickMode = ClickMode::Build;
    bool      arrowMode = false;

    std::function<void()> onClickModeChanged;
    std::function<void()> onArrowModeChanged;

private:

    void handleNodeMouseDown  (const juce::MouseEvent& event, OrnamentNode& node);
    void handleCanvasMouseDown(const juce::MouseEvent& event, juce::Point<int> point, juce::Point<int> snapped);

    OrnamentCanvas&         canvas;
    ArsNovaUI::DynamicPort& port;
    OrnamentLibraries&      ornamentLibraries;
    juce::UndoManager&      undoManager;

    DragState dragState      = DragState::Idle;
    int       draggedNodeId  = -1;
    double    dragStartValue = 0.0;
};