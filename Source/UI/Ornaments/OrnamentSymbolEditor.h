#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include "../../Graph/OrnamentLibraries.h"

/*
    The drawing surface for an ornament's symbol, the small picture shown on the canvas where the
    ornament is applied. The symbol is stored on the ornament's tree as the OrnamentSymbol
    property, as a set of strokes in a square whose side counts as 1. The editor shows a square
    drawing area with the current symbol and has a pencil, an eraser, a size for both and a button
    that clears the symbol.

    With the pencil, a drag draws a stroke that is only shown on screen while the mouse is down;
    mouseUp scales it into the square's units and stores it with OrnamentLibraries::addSymbolStroke.
    With the eraser, each step of a drag calls OrnamentLibraries::eraseSymbol on the symbol under
    the eraser's circle. The size is the brush width as a percentage of the square, from 1 to 20.
    Each edit is an undo transaction.

    OrnamentBuildPanel owns it, shows it in place of the figure and sets ornament.
*/

class OrnamentSymbolEditor : public juce::Component
{
public:

    OrnamentSymbolEditor(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager);

    void paint  (juce::Graphics& graphics) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp  (const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    juce::MouseCursor getMouseCursor() override;

    enum class SymbolTool { Pencil, Eraser };

    static constexpr float pointSpacing      = 2.0f;
    static constexpr float drawingInsetRatio = 0.1f;
    static constexpr int   clearButtonSize   = 22;
    static constexpr int   clearButtonInset  = 6;
    static constexpr int   toolPaneHeight    = 26;
    static constexpr int   toolPaneInset     = 6;
    static constexpr float eraserStepRatio   = 0.5f;
    static constexpr float dashLength        = 2.0f;
    static constexpr float percent           = 100.0f;

    static constexpr double minBrushPercent    = 1.0;
    static constexpr double maxBrushPercent    = 20.0;
    static constexpr int    brushDecimalPlaces = 1;
    static constexpr int    sizeLabelWidth     = 30;
    static constexpr int    sizeEditorWidth    = 44;
    static constexpr int    sizeFieldGap       = 4;

    juce::ValueTree ornament;
    SymbolTool      tool       = SymbolTool::Pencil;
    float           brushWidth = OrnamentLibraries::defaultStrokeWidth;

    ArsNovaUI::ButtonPane    toolPane;
    ArsNovaUI::IconButton    clearButton;
    ArsNovaUI::LabeledEditor sizeField;

private:

    void eraseAlong(juce::Point<float> target);

    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& undoManager;

    juce::Rectangle<float> drawingArea;
    juce::Path             stroke;
    juce::Point<float>     lastPoint;

    std::optional<juce::Point<float>> cursorPosition;
};
