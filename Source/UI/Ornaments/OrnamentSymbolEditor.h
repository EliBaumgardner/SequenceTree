#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

#include "../Buttons/ButtonPane.h"
#include "../Editors/LabeledEditor.h"
#include "../../Graph/OrnamentLibraries.h"

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

    ButtonPane    toolPane;
    IconButton    clearButton;
    LabeledEditor sizeField;

private:

    void eraseAlong(juce::Point<float> target);

    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& undoManager;

    juce::Rectangle<float> drawingArea;
    juce::Path             stroke;
    juce::Point<float>     lastPoint;

    std::optional<juce::Point<float>> cursorPosition;
};
