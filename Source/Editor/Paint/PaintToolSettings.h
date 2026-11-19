#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <ArsNovaUI/Menus/ColourSelector.h>
#include <ArsNovaUI/Editors/CaptionLabel.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include "ValueField.h"

/*
    The settings of the paint tool: a colour selector, a size field and a rate field for the brush,
    each in a labelled row of menuWidth by menuHeight altogether. It holds one brush colour for
    each of ValueField's paint layers (pitch, velocity, duration) and shows the colour of the layer
    that is current.

    The colour selector sets the colour of the current layer and passes it to
    NodeCanvas::valueField as brushColour. The two fields each hold a 0 to 1 value that is scaled
    into valueField's brushRadius or brushFlow. setPaintMode switches the layer and the colour
    shown with it; BottomBar calls it when the paint button turns paint mode on, and its
    applyDisplayMode calls it when the display mode changes to pitch or velocity.

    BottomBar owns it and shows it as the menu that the paint button's right-click opens.
*/

class NodeCanvas;

class PaintToolSettings : public juce::Component
{
public:

    PaintToolSettings(NodeCanvas& nodeCanvas, juce::ValueTree colourPresets, juce::UndoManager& undoManager);

    void resized() override;

    void configureValueFields(ValueField& valueField, float brushFlow);
    void setPaintMode        (ValueField::PaintLayer layer);
    void componentCallBack   ();

    juce::Colour& paintLayerColour();

    static constexpr float minBrushFlow = 0.002f;
    static constexpr float maxBrushFlow = 0.25f;

    static constexpr float minBrushRadius = 1.0f;
    static constexpr float maxBrushRadius = 200.0f;

    static constexpr int rowCount     = 3;
    static constexpr int menuWidth    = 150;
    static constexpr int menuHeight   = rowCount * ArsNovaUI::Theme::popupMenuItemHeight;
    static constexpr int editorWidth  = 57;
    static constexpr int editorHeight = 13;
    static constexpr int swatchWidth  = 30;

    NodeCanvas& nodeCanvas;

    juce::Colour pitchColour    = juce::Colours::red;
    juce::Colour velocityColour = juce::Colours::green;
    juce::Colour durationColour = juce::Colours::blue;

    ValueField::PaintLayer paintLayer = ValueField::PaintLayer::Pitch;

    ArsNovaUI::CaptionLabel   colourLabel;
    ArsNovaUI::ColourSelector colourSelector;
    ArsNovaUI::LabeledEditor  sizeField;
    ArsNovaUI::LabeledEditor  flowField;
};
