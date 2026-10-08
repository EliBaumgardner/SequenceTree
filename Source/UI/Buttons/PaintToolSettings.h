#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Menus/ColourSelector.h"
#include "../Editors/LabeledEditor.h"
#include "../Canvas/ValueField.h"
#include "IconButton.h"

class NodeCanvas;

class PaintToolSettings : public juce::Component
{
public:

    static constexpr float minBrushFlow = 0.002f;
    static constexpr float maxBrushFlow = 0.25f;

    static constexpr float minBrushRadius = 1.0f;
    static constexpr float maxBrushRadius = 200.0f;

    static constexpr float cellWidthRatio  = 0.09f;
    static constexpr float labelWidthRatio = 0.144f;

    NodeCanvas& nodeCanvas;

    juce::Colour pitchColour    = juce::Colours::red;
    juce::Colour velocityColour = juce::Colours::green;
    juce::Colour durationColour = juce::Colours::blue;

    ValueField::PaintLayer paintLayer = ValueField::PaintLayer::Pitch;

    IconButton     paintTool;
    ColourSelector colourSelector;
    LabeledEditor  sizeField;
    LabeledEditor  flowField;

    PaintToolSettings(NodeCanvas& nodeCanvas, juce::ValueTree colourPresets, juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void configureValueFields(ValueField &valueField, float brushFlow);
    void componentCallBack();
    void setPaintMode(ValueField::PaintLayer layer);
    juce::Colour& paintLayerColour();
};
