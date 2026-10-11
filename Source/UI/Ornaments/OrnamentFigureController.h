#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Graph/OrnamentLibraries.h"

class DynamicPort;
class OrnamentCanvas;
class OrnamentNode;

class OrnamentFigureController : public juce::MouseListener
{
public:

    enum class DragState { Idle, MovingNode, MovingArrowHead, GrowingChain, MovingDanglingTip, CreatingDanglingArrow, ScrubbingValue, Panning };
    enum class ClickMode { Build, PickHost };

    OrnamentFigureController(OrnamentCanvas& canvas, DynamicPort& port, OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager);

    void mouseMove      (const juce::MouseEvent& event) override;
    void mouseExit      (const juce::MouseEvent& event) override;
    void mouseDown      (const juce::MouseEvent& event) override;
    void mouseDrag      (const juce::MouseEvent& event) override;
    void mouseUp        (const juce::MouseEvent& event) override;
    void switchClickMode(ClickMode mode);
    void setArrowMode   (bool enabled);

    static constexpr float arrowHoverRadius     = 8.0f;
    static constexpr float arrowHeadGrabRadius  = 16.0f;
    static constexpr float arrowLabelGrabRadius = 10.0f;
    static constexpr int   dragThreshold        = 5;
    static constexpr int   pixelsPerScrubStep   = 3;
    static constexpr int   hostCursorSize       = 20;

    juce::ValueTree       ornament;
    ClickMode             clickMode = ClickMode::Build;
    bool                  arrowMode = false;
    std::function<void()> onClickModeChanged;
    std::function<void()> onArrowModeChanged;

private:

    void handleNodeMouseDown  (const juce::MouseEvent& event, OrnamentNode& node);
    void handleCanvasMouseDown(const juce::MouseEvent& event, juce::Point<int> point, juce::Point<int> snapped);

    OrnamentCanvas&    canvas;
    DynamicPort&       port;
    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& undoManager;

    DragState dragState      = DragState::Idle;
    int       draggedNodeId  = -1;
    double    dragStartValue = 0.0;
};